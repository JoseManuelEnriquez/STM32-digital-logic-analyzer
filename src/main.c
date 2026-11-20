#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_uart.h"
#include "TIMER4.h"
#define GPIO_PORT GPIOA
#define GPIO_PIN GPIO_PIN_5

// ----------------------------------------------
// Global variables
// ----------------------------------------------
static DMA_HandleTypeDef hdma_gpio = {0};
static UART_HandleTypeDef huart2 = {0};

// ----------------------------------------------
// Function Prototypes
// ----------------------------------------------
HAL_StatusTypeDef UART2_Init(void);
HAL_StatusTypeDef Debug_Log(const char *message);

HAL_StatusTypeDef DMA_GPIO_Init(void);
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
void Error_Handler(void) {}

int main() {
    HAL_Init();
    if (UART2_Init() != HAL_OK) {
        Error_Handler();
    }
    if (Debug_Log("Debug logger ready") != HAL_OK) {
        Error_Handler();
    }
    if(DMA_GPIO_Init() != HAL_OK) {
        // Handle error
    }
    if(TIMER4_Init() != HAL_OK) {
        // Handle error
    }
    while(1){
        // Main loop
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
