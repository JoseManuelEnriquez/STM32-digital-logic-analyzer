#include "SUMP.h"

#include "DMA_GPIO.h"
#include "TIMER4.h"

#define SUMP_CMD_RESET          0x00U   /**< Detiene la captura y restaura la configuración inicial. */
#define SUMP_CMD_RUN            0x01U   /**< Inicia una adquisición. */
#define SUMP_CMD_ID             0x02U   /**< Solicita el identificador de cuatro bytes. */
#define SUMP_CMD_METADATA       0x04U   /**< Solicita los metadatos del analizador. */
#define SUMP_CMD_XON            0x11U   /**< Reanuda el envío serie pausado. */
#define SUMP_CMD_XOFF           0x13U   /**< Pausa el envío serie de las muestras. */
#define SUMP_CMD_SET_DIVIDER    0x80U   /**< Configura el divisor de muestreo (4 bytes). */
#define SUMP_CMD_CAPTURE_SIZE   0x81U   /**< Configura tamaño y muestras posteriores al trigger (4 bytes). */
#define SUMP_CMD_SET_FLAGS      0x82U   /**< Recibe los flags de captura SUMP (4 bytes). */
#define SUMP_CMD_TRIGGER_BASE   0xC0U   /**< Inicio de los registros de trigger por etapas. */
#define SUMP_TRIGGER_STAGES     4U      /**< Número máximo de etapas de trigger admitidas. */
#define SUMP_TRIGGER_START      0x08U   /**< Bit del registro de configuración que marca la etapa final. */
#define SUMP_SAMPLE_GROUP_SIZE  4U      /**< El tamaño de captura SUMP se expresa en grupos de 4 muestras. */
#define SUMP_UART_CHUNK_SIZE    64U     /**< Máximo de muestras por bloque transmitido antes de revisar XON/XOFF. */
#define SUMP_METADATA_NAME      "STM32 Logic Analyzer" /**< Nombre reportado al host en los metadatos. */
#define SUMP_MAX_DIVIDER        0x00FFFFFFU /**< El protocolo reserva 24 bits para el divisor de muestreo. */

typedef enum {
    SUMP_STATE_IDLE,      /**< Espera comandos del host; no hay adquisición activa. */
    SUMP_STATE_CAPTURING, /**< El DMA llena el buffer mientras el firmware busca el fin de captura. */
    SUMP_STATE_SENDING    /**< La adquisición terminó y se transmiten sus muestras. */
} SUMP_State;

/** Puerto USART que transporta los comandos y las respuestas SUMP. */
static UART_HandleTypeDef *sump_uart;
/** Estado actual de la máquina de estados del protocolo. */
static SUMP_State sump_state = SUMP_STATE_IDLE;
/** Divisor solicitado por el host para fijar la frecuencia de muestreo de TIM4. */
static uint32_t sample_divider = TIMER4_DEFAULT_DIVIDER;
/** Número de muestras que se enviarán al host, limitado por la capacidad anunciada. */
static uint32_t requested_samples = DMA_GPIO_MAX_CAPTURE_SAMPLES;
/** Número de muestras a capturar después del trigger; las restantes son previas. */
static uint32_t post_trigger_samples = DMA_GPIO_MAX_CAPTURE_SAMPLES;
/** Máscara de canales que debe coincidir en cada etapa del trigger. */
static uint32_t trigger_masks[SUMP_TRIGGER_STAGES];
/** Valor esperado en cada etapa del trigger, aplicado junto con su máscara. */
static uint32_t trigger_values[SUMP_TRIGGER_STAGES];
/** Cantidad de etapas de trigger configuradas por el host. */
static uint8_t trigger_stage_count;
/** Etapa que provoca el trigger al coincidir. */
static uint8_t trigger_fire_stage;
/** Indica si el host marcó explícitamente una etapa final de trigger. */
static uint8_t trigger_fire_configured;
/** Etapa que se está comprobando en la secuencia de trigger actual. */
static uint8_t trigger_current_stage;
/** Indica si ya se encontró el patrón que dispara la captura. */
static uint8_t trigger_found;
/** Estado XON/XOFF aplicado al envío de muestras. */
static uint8_t flow_paused;
/** Opcode del comando cuyos argumentos se están recibiendo. */
static uint8_t command;
/** Argumentos de cuatro bytes de un comando largo SUMP. */
static uint8_t command_arguments[4];
/** Bytes de argumento recibidos más uno; cero significa que no hay comando pendiente. */
static uint8_t command_argument_count;
/** Cantidad acumulada de muestras DMA ya examinadas por el firmware. */
static uint32_t processed_samples;
/** Índice circular del primer sample que se enviará al host. */
static uint32_t capture_start_index;
/** Índice circular inmediatamente posterior al último sample que se enviará. */
static uint32_t capture_end_index;

/**
 * Convierte cuatro bytes recibidos en orden little-endian a un entero de 32 bits.
 */
static uint32_t SUMP_ReadUint32LE(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

/**
 * Convierte dos bytes recibidos en orden little-endian a un entero de 16 bits.
 */
static uint16_t SUMP_ReadUint16LE(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

/**
 * Envía bytes por USART2 sin añadir terminadores ni modificar el contenido.
 */
static HAL_StatusTypeDef SUMP_Send(const uint8_t *data, uint16_t length)
{
    return HAL_UART_Transmit(sump_uart, (uint8_t *)data, length, HAL_MAX_DELAY);
}

/**
 * Envía un entero de 32 bits en el orden big-endian usado por los metadatos SUMP.
 */
static HAL_StatusTypeDef SUMP_SendUint32BE(uint32_t value)
{
    uint8_t data[4] = {
        (uint8_t)(value >> 24U),
        (uint8_t)(value >> 16U),
        (uint8_t)(value >> 8U),
        (uint8_t)value
    };

    return SUMP_Send(data, sizeof(data));
}

/**
 * Responde a la consulta de metadatos con nombre, canales, memoria y frecuencia máxima.
 */
static HAL_StatusTypeDef SUMP_SendMetadata(void)
{
    static const uint8_t name_token = 0x01U;
    static const uint8_t channel_count_token = 0x20U;
    static const uint8_t sample_memory_token = 0x21U;
    static const uint8_t max_sample_rate_token = 0x23U;
    static const uint8_t end_token = 0x00U;
    static const uint8_t device_name[] = SUMP_METADATA_NAME;
    HAL_StatusTypeDef status;

    status = SUMP_Send(&name_token, sizeof(name_token));
    if (status != HAL_OK) {
        return status;
    }
    status = SUMP_Send(device_name, sizeof(device_name));
    if (status != HAL_OK) {
        return status;
    }

    status = SUMP_Send(&channel_count_token, sizeof(channel_count_token));
    if (status != HAL_OK) {
        return status;
    }
    status = SUMP_SendUint32BE(8U);
    if (status != HAL_OK) {
        return status;
    }

    status = SUMP_Send(&sample_memory_token, sizeof(sample_memory_token));
    if (status != HAL_OK) {
        return status;
    }
    status = SUMP_SendUint32BE(DMA_GPIO_MAX_CAPTURE_SAMPLES);
    if (status != HAL_OK) {
        return status;
    }

    status = SUMP_Send(&max_sample_rate_token, sizeof(max_sample_rate_token));
    if (status != HAL_OK) {
        return status;
    }
    status = SUMP_SendUint32BE(HAL_RCC_GetPCLK1Freq() *
                               (((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) ? 2U : 1U));
    if (status != HAL_OK) {
        return status;
    }

    return SUMP_Send(&end_token, sizeof(end_token));
}

/**
 * Detiene primero las peticiones de TIM4 y después el flujo circular del DMA.
 */
static HAL_StatusTypeDef SUMP_StopAcquisition(void)
{
    HAL_StatusTypeDef timer_status;
    HAL_StatusTypeDef dma_status;

    timer_status = TIMER4_Stop();
    dma_status = DMA_GPIO_Stop();
    sump_state = SUMP_STATE_IDLE;

    if (timer_status != HAL_OK) {
        return timer_status;
    }
    return dma_status;
}

static uint8_t SUMP_HasTrigger(void)
{
    uint8_t stage;

    /* No hay trigger si el host no marcó una etapa final válida. */
    if (trigger_fire_configured == 0U ||
        trigger_fire_stage >= trigger_stage_count) {
        return 0U;
    }

    /* Una máscara vacía en todas las etapas equivale a captura sin trigger. */
    for (stage = 0U; stage <= trigger_fire_stage; stage++) {
        if ((trigger_masks[stage] & 0xFFU) != 0U) {
            return 1U;
        }
    }

    return 0U;
}

/**
 * Restaura los valores predeterminados y cancela una captura que siga activa.
 */
static HAL_StatusTypeDef SUMP_Reset(void)
{
    uint8_t stage;

    if (sump_state == SUMP_STATE_CAPTURING) {
        HAL_StatusTypeDef status = SUMP_StopAcquisition();
        if (status != HAL_OK) {
            return status;
        }
    }

    sample_divider = TIMER4_DEFAULT_DIVIDER;
    requested_samples = DMA_GPIO_MAX_CAPTURE_SAMPLES;
    post_trigger_samples = DMA_GPIO_MAX_CAPTURE_SAMPLES;
    trigger_stage_count = 0U;
    trigger_fire_stage = 0U;
    trigger_fire_configured = 0U;
    trigger_current_stage = 0U;
    trigger_found = 0U;
    flow_paused = 0U;
    command_argument_count = 0U;

    for (stage = 0U; stage < SUMP_TRIGGER_STAGES; stage++) {
        trigger_masks[stage] = 0U;
        trigger_values[stage] = 0U;
    }

    return TIMER4_SetDivider(sample_divider);
}

/**
 * Arranca el DMA circular y luego habilita TIM4 como reloj de muestreo.
 */
static HAL_StatusTypeDef SUMP_StartAcquisition(void)
{
    HAL_StatusTypeDef status;

    if (sump_state == SUMP_STATE_CAPTURING) {
        status = SUMP_StopAcquisition();
        if (status != HAL_OK) {
            return status;
        }
    }

    trigger_found = 0U;
    trigger_current_stage = 0U;
    processed_samples = 0U;
    capture_start_index = 0U;
    capture_end_index = requested_samples;

    status = DMA_GPIO_Start();
    if (status != HAL_OK) {
        return status;
    }
    status = TIMER4_Start();
    if (status != HAL_OK) {
        (void)DMA_GPIO_Stop();
        return status;
    }

    sump_state = SUMP_STATE_CAPTURING;
    return HAL_OK;
}

/**
 * Compara una muestra con la etapa actual y calcula la ventana a enviar al coincidir.
 */
static void SUMP_CheckTrigger(uint32_t sample_index, uint8_t sample)
{
    uint32_t samples_before_trigger = requested_samples -
                                      post_trigger_samples;

    /* Espera a que exista suficiente historial previo al trigger solicitado. */
    if (trigger_found != 0U || SUMP_HasTrigger() == 0U ||
        sample_index < samples_before_trigger) {
        return;
    }

    /* Solo se comparan los bits habilitados por la máscara de la etapa actual. */
    if ((sample & (uint8_t)trigger_masks[trigger_current_stage]) !=
        ((uint8_t)trigger_values[trigger_current_stage] &
         (uint8_t)trigger_masks[trigger_current_stage])) {
        return;
    }

    /* Avanza por la secuencia; al coincidir la etapa final, fija la ventana de salida. */
    if (trigger_current_stage == trigger_fire_stage) {
        capture_start_index = sample_index - samples_before_trigger;
        capture_end_index = capture_start_index + requested_samples;
        trigger_found = 1U;
    } else {
        trigger_current_stage++;
    }
}

/**
 * Transmite la ventana final del buffer circular en orden cronológico.
 */
static HAL_StatusTypeDef SUMP_SendCapture(void)
{
    const uint8_t *buffer = DMA_GPIO_GetBuffer();
    uint32_t samples_sent = 0U;

    sump_state = SUMP_STATE_SENDING;
    while (samples_sent < requested_samples) {
        uint32_t buffer_index =
            (capture_start_index + samples_sent) % DMA_GPIO_BUFFER_SAMPLES;
        uint32_t contiguous_samples =
            DMA_GPIO_BUFFER_SAMPLES - buffer_index;
        uint32_t remaining_samples = requested_samples - samples_sent;
        uint32_t chunk_samples = contiguous_samples;

        if (chunk_samples > remaining_samples) {
            chunk_samples = remaining_samples;
        }

        while (flow_paused != 0U) {
            uint8_t received;
            HAL_StatusTypeDef status =
                HAL_UART_Receive(sump_uart, &received, 1U, 10U);

            if (status == HAL_OK && received == SUMP_CMD_XON) {
                flow_paused = 0U;
            } else if (status != HAL_OK && status != HAL_TIMEOUT) {
                sump_state = SUMP_STATE_IDLE;
                return status;
            }
        }

        while (chunk_samples > 0U) {
            uint16_t chunk_size = (chunk_samples > SUMP_UART_CHUNK_SIZE)
                                      ? SUMP_UART_CHUNK_SIZE
                                      : (uint16_t)chunk_samples;
            HAL_StatusTypeDef status =
                SUMP_Send(&buffer[buffer_index], chunk_size);
            if (status != HAL_OK) {
                sump_state = SUMP_STATE_IDLE;
                return status;
            }

            samples_sent += chunk_size;
            buffer_index += chunk_size;
            chunk_samples -= chunk_size;

            uint8_t received;
            status = HAL_UART_Receive(sump_uart, &received, 1U, 0U);
            if (status == HAL_OK && received == SUMP_CMD_XOFF) {
                flow_paused = 1U;
                break;
            }
            if (status != HAL_OK && status != HAL_TIMEOUT) {
                sump_state = SUMP_STATE_IDLE;
                return status;
            }
        }
    }

    sump_state = SUMP_STATE_IDLE;
    return HAL_OK;
}

/**
 * Detiene la adquisición y, si todo fue bien, envía las muestras capturadas.
 */
static HAL_StatusTypeDef SUMP_FinishAcquisition(void)
{
    HAL_StatusTypeDef status = SUMP_StopAcquisition();
    if (status != HAL_OK) {
        return status;
    }

    return SUMP_SendCapture();
}

/**
 * Examina las muestras DMA nuevas, detecta errores y decide cuándo finalizar.
 */
static HAL_StatusTypeDef SUMP_ProcessCapture(void)
{
    uint32_t completed;
    uint32_t pending_samples;
    const uint8_t *buffer = DMA_GPIO_GetBuffer();

    if (DMA_GPIO_HasError() != 0U) {
        (void)SUMP_StopAcquisition();
        return HAL_ERROR;
    }

    completed = DMA_GPIO_GetCompletedSamples();
    pending_samples = completed - processed_samples;
    if (pending_samples > DMA_GPIO_BUFFER_SAMPLES) {
        (void)SUMP_StopAcquisition();
        return HAL_ERROR;
    }

    while (pending_samples > 0U) {
        uint32_t sample_index = processed_samples;
        uint8_t sample = buffer[sample_index % DMA_GPIO_BUFFER_SAMPLES];

        SUMP_CheckTrigger(sample_index, sample);
        processed_samples++;
        pending_samples--;
    }

    if ((trigger_found != 0U &&
         (int32_t)(completed - capture_end_index) >= 0) ||
        (SUMP_HasTrigger() == 0U &&
         (int32_t)(completed - requested_samples) >= 0)) {
        return SUMP_FinishAcquisition();
    }

    return HAL_OK;
}

/**
 * Interpreta los cuatro bytes de un comando largo y actualiza la configuración.
 */
static HAL_StatusTypeDef SUMP_HandleLongCommand(void)
{
    uint32_t value;

    switch (command) {
    case SUMP_CMD_SET_DIVIDER:
        value = SUMP_ReadUint32LE(command_arguments) & SUMP_MAX_DIVIDER;
        sample_divider = value;
        return TIMER4_SetDivider(sample_divider);

    case SUMP_CMD_CAPTURE_SIZE: {
        uint32_t read_groups = (uint32_t)SUMP_ReadUint16LE(command_arguments) + 1U;
        uint32_t post_groups =
            (uint32_t)SUMP_ReadUint16LE(&command_arguments[2]) + 1U;

        requested_samples = read_groups * SUMP_SAMPLE_GROUP_SIZE;
        if (requested_samples > DMA_GPIO_MAX_CAPTURE_SAMPLES) {
            requested_samples = DMA_GPIO_MAX_CAPTURE_SAMPLES;
        }

        post_trigger_samples = post_groups * SUMP_SAMPLE_GROUP_SIZE;
        if (post_trigger_samples > requested_samples) {
            post_trigger_samples = requested_samples;
        }
        return HAL_OK;
    }

    case SUMP_CMD_SET_FLAGS:
        return HAL_OK;

    default:
        if (command >= SUMP_CMD_TRIGGER_BASE &&
            command < SUMP_CMD_TRIGGER_BASE + SUMP_TRIGGER_STAGES * 4U) {
            uint8_t trigger_register = command - SUMP_CMD_TRIGGER_BASE;
            uint8_t stage = trigger_register / 4U;
            uint8_t register_type = trigger_register % 4U;

            value = SUMP_ReadUint32LE(command_arguments);
            if (register_type == 0U) {
                trigger_masks[stage] = value;
            } else if (register_type == 1U) {
                trigger_values[stage] = value;
            } else if (register_type == 2U) {
                uint8_t configured_stage = command_arguments[2];
                if (configured_stage >= SUMP_TRIGGER_STAGES) {
                    return HAL_ERROR;
                }
                if (configured_stage + 1U > trigger_stage_count) {
                    trigger_stage_count = configured_stage + 1U;
                }
                if ((command_arguments[3] & SUMP_TRIGGER_START) != 0U) {
                    trigger_fire_stage = configured_stage;
                    trigger_fire_configured = 1U;
                }
            }
        }
        return HAL_OK;
    }
}

/**
 * Procesa un byte recibido: completa comandos largos o ejecuta comandos cortos.
 */
static HAL_StatusTypeDef SUMP_HandleCommand(uint8_t received)
{
    static const uint8_t device_id[] = {'1', 'A', 'L', 'S'};
    HAL_StatusTypeDef status;

    if (command_argument_count > 0U) {
        command_arguments[command_argument_count - 1U] = received;
        if (command_argument_count == sizeof(command_arguments)) {
            command_argument_count = 0U;
            if (sump_state != SUMP_STATE_CAPTURING) {
                return SUMP_HandleLongCommand();
            }
        } else {
            command_argument_count++;
        }
        return HAL_OK;
    }

    if (received == SUMP_CMD_XON) {
        flow_paused = 0U;
        return HAL_OK;
    }
    if (received == SUMP_CMD_XOFF) {
        flow_paused = 1U;
        return HAL_OK;
    }

    switch (received) {
    case SUMP_CMD_RESET:
        return SUMP_Reset();
    case SUMP_CMD_RUN:
        return SUMP_StartAcquisition();
    case SUMP_CMD_ID:
        return SUMP_Send(device_id, sizeof(device_id));
    case SUMP_CMD_METADATA:
        return SUMP_SendMetadata();
    default:
        if ((received >= SUMP_CMD_SET_DIVIDER &&
             received <= SUMP_CMD_SET_FLAGS) ||
            (received >= SUMP_CMD_TRIGGER_BASE &&
             received < SUMP_CMD_TRIGGER_BASE + SUMP_TRIGGER_STAGES * 4U)) {
            command = received;
            command_argument_count = 1U;
        }
        status = HAL_OK;
        return status;
    }
}

/**
 * Asocia el UART y restaura la configuración inicial del protocolo.
 */
HAL_StatusTypeDef SUMP_Init(UART_HandleTypeDef *uart)
{
    if (uart == NULL) {
        return HAL_ERROR;
    }

    sump_uart = uart;
    sump_state = SUMP_STATE_IDLE;
    return SUMP_Reset();
}

/**
 * Atiende como máximo un byte UART sin bloquear y avanza la captura si está activa.
 */
HAL_StatusTypeDef SUMP_Process(void)
{
    uint8_t received;
    HAL_StatusTypeDef status;

    status = HAL_UART_Receive(sump_uart, &received, 1U, 0U);
    if (status == HAL_OK) {
        status = SUMP_HandleCommand(received);
        if (status != HAL_OK) {
            return status;
        }
    } else if (status != HAL_TIMEOUT) {
        return status;
    }

    if (sump_state == SUMP_STATE_CAPTURING) {
        return SUMP_ProcessCapture();
    }

    return HAL_OK;
}
