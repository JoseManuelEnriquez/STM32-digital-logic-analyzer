#include "TIMER4.h"
static TIM_HandleTypeDef htim4 = {0};
static uint32_t timer_clock_hz;

HAL_StatusTypeDef TIMER4_Init(void)
{
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 0U;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 0xFFFFU;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        return HAL_ERROR;
    }

    timer_clock_hz = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U) {
        timer_clock_hz *= 2U;
    }

    return TIMER4_SetDivider(TIMER4_DEFAULT_DIVIDER);
}

HAL_StatusTypeDef TIMER4_SetDivider(uint32_t divider)
{
    uint32_t divisor;
    uint32_t prescaler_divisor;
    uint32_t period_divisor;

    if (timer_clock_hz == 0U || (htim4.Instance->CR1 & TIM_CR1_CEN) != 0U) {
        return HAL_ERROR;
    }

    divisor = divider + 1U;
    prescaler_divisor = (divisor + 0xFFFFU) / 0x10000U;
    if (prescaler_divisor == 0U) {
        prescaler_divisor = 1U;
    }

    period_divisor = (divisor + (prescaler_divisor / 2U)) /
                     prescaler_divisor;
    if (period_divisor == 0U) {
        period_divisor = 1U;
    }

    __HAL_TIM_SET_PRESCALER(&htim4, prescaler_divisor - 1U);
    __HAL_TIM_SET_AUTORELOAD(&htim4, period_divisor - 1U);
    __HAL_TIM_SET_COUNTER(&htim4, 0U);
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);

    return HAL_OK;
}

HAL_StatusTypeDef TIMER4_Start(void)
{
    __HAL_TIM_SET_COUNTER(&htim4, 0U);
    __HAL_TIM_CLEAR_FLAG(&htim4, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_DMA(&htim4, TIM_DMA_UPDATE);

    if (HAL_TIM_Base_Start(&htim4) != HAL_OK) {
        __HAL_TIM_DISABLE_DMA(&htim4, TIM_DMA_UPDATE);
        return HAL_ERROR;
    }

    return HAL_OK;
}

HAL_StatusTypeDef TIMER4_Stop(void)
{
    __HAL_TIM_DISABLE_DMA(&htim4, TIM_DMA_UPDATE);
    return HAL_TIM_Base_Stop(&htim4);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
    }
}
