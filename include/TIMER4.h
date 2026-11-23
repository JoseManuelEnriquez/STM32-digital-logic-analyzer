#ifndef TIMER4_H
#define TIMER4_H
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#define TIMER4_DEFAULT_DIVIDER 15U

HAL_StatusTypeDef TIMER4_Init(void);
HAL_StatusTypeDef TIMER4_SetDivider(uint32_t divider);
HAL_StatusTypeDef TIMER4_Start(void);
HAL_StatusTypeDef TIMER4_Stop(void);
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim);

#endif 