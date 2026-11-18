#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_tim.h"

#define GPIO_PORT GPIOA
#define GPIO_PIN GPIO_PIN_5
#define TIMER4_OUTPUT_HZ 1000000U
#define TIMER4_PERIOD 1U
#define DATA_LENGTH 1U

/* Timer handle for TIM4 */

static TIM_HandleTypeDef htim4 = {0};
uint32_t buffer[DATA_LENGTH] = {0}; // Dummy buffer for DMA

void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim); 
HAL_StatusTypeDef TIMER4_Init(void);
void Error_Handler(void) {}

/* DMA handle for GPIO */
static DMA_HandleTypeDef hdma_gpio;
HAL_StatusTypeDef configure_gpio_dma(void);

int main() {
    HAL_Init();
    if(configure_gpio_dma() != HAL_OK) {
        // Handle error
    }
    if(TIMER4_Init() != HAL_OK) {
        // Handle error
    }
    while(1){
        // Main loop
    }
}


/* TIM4 Msp Init */
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

/* Initialize TIM4 for output compare in DMA mode */
HAL_StatusTypeDef TIMER4_Init(void) {
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = (SystemCoreClock / TIMER4_OUTPUT_HZ) - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = TIMER4_PERIOD - 1;
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

/* Configure GPIO for DMA */
HAL_StatusTypeDef configure_gpio_dma(void)
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
