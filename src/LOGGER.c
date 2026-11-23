#include "LOGGER.h"

static UART_HandleTypeDef huart2 = {0};

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

UART_HandleTypeDef *UART2_GetHandle(void) {
    return &huart2;
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