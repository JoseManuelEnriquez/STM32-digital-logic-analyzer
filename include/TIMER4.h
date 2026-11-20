#ifndef TIMER4_H
#define TIMER4_H
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#define TIMER4_OUTPUT_HZ 1000000U
#define TIMER4_PERIOD 1U
#define DATA_LENGTH 1U

extern uint32_t buffer[DATA_LENGTH]; // Dummy buffer for DMA

HAL_StatusTypeDef TIMER4_Init(void);
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim); 

#endif 