#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_uart.h"

#define GPIO_PORT GPIOA
#define GPIO_PIN GPIO_PIN_5
#define TIMER4_OUTPUT_HZ 1000000U
#define TIMER4_PERIOD 1U
#define DATA_LENGTH 1U

// ----------------------------------------------
// Global variables
// ----------------------------------------------
static TIM_HandleTypeDef htim4 = {0};
static DMA_HandleTypeDef hdma_gpio = {0};
static UART_HandleTypeDef huart2 = {0};
uint32_t buffer[DATA_LENGTH] = {0}; // Dummy buffer for DMA

// ----------------------------------------------
// Function Prototypes
// ----------------------------------------------
HAL_StatusTypeDef UART2_Init(void);
HAL_StatusTypeDef Debug_Log(const char *message);
HAL_StatusTypeDef TIMER4_Init(void);
HAL_StatusTypeDef DMA_GPIO_Init(void);
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim); 

void Error_Handler(const char *message) {
    Debug_Log(message);
}

int main() {
    HAL_Init();
    UART2_Init();
    Debug_Log("Debug logger ready");
    if(DMA_GPIO_Init() != HAL_OK) {
        Error_Handler("Failed to initialize DMA for GPIO");
    }
    if(TIMER4_Init() != HAL_OK) {
        Error_Handler("Failed to initialize Timer 4");
    }
    while(1){
        Debug_Log("Buffer DMA GPIO: ");
        for(int i = 0; i < DATA_LENGTH; i++) {
            char msg[50];
            snprintf(msg, sizeof(msg), "buffer[%d] = %lu", i, buffer[i]);
            Debug_Log(msg);
        }
        HAL_Delay(1000);
    }
}

/**
 * @brief Initializes USART2 for the NUCLEO-F411RE ST-LINK virtual COM port.
 */
HAL_StatusTypeDef UART2_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    return HAL_UART_Init(&huart2);
}

/**
 * @brief Sends one debug message followed by a CRLF line ending.
 */
HAL_StatusTypeDef Debug_Log(const char *message) {
    if (message == NULL) {
        return HAL_ERROR;
    }

    size_t length = strlen(message);
    if (length > UINT16_MAX) {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(
        &huart2, (uint8_t *)message, (uint16_t)length, HAL_MAX_DELAY);
    if (status != HAL_OK) {
        return status;
    }

    uint8_t line_ending[] = "\r\n";
    return HAL_UART_Transmit(
        &huart2, line_ending, sizeof(line_ending) - 1U, HAL_MAX_DELAY);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART2_CLK_ENABLE();

        GPIO_InitTypeDef gpio = {0};
        gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

/** 
 * @brief  Initializes the TIM4 peripheral for output compare in DMA mode.
 * @param  htim: TIM handle pointer
 */
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        // PB6 -> TIM4_CH1
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

/** 
 * @brief  Initializes the TIM4 peripheral for output compare in DMA mode.
 * @param  htim: TIM handle pointer
 */
HAL_StatusTypeDef TIMER4_Init(void) {
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = (SystemCoreClock / TIMER4_OUTPUT_HZ) - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = TIMER4_PERIOD;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    
    if (HAL_TIM_OC_Init(&htim4) != HAL_OK) {
        return HAL_ERROR;
    }
    
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
    sConfigOC.Pulse = TIMER4_PERIOD / 2; // Toggle at half period
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        return HAL_ERROR;
    }

    // !! Start the output compare in DMA mode with a dummy buffer (NULL) and length 1
    if (HAL_TIM_OC_Start_DMA(&htim4, TIM_CHANNEL_1, buffer, DATA_LENGTH) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/** 
 * @brief  Initializes the GPIO for DMA.
 * @param  None
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef DMA_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hdma_gpio.Instance = DMA1_Stream0;
    hdma_gpio.Init.Channel = DMA_CHANNEL_2;
    hdma_gpio.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_gpio.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_gpio.Init.MemInc = DMA_MINC_ENABLE;
    hdma_gpio.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_gpio.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_gpio.Init.Mode = DMA_CIRCULAR;
    hdma_gpio.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_gpio.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    return HAL_DMA_Init(&hdma_gpio);
}
