#include "TIMER4.h"

static TIM_HandleTypeDef htim4 = {0};
uint32_t buffer[DATA_LENGTH] = {0}; // Dummy buffer for DMA

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
