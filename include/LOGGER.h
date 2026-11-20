#ifndef LOGGER_H
#define LOGGER_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

HAL_StatusTypeDef UART2_Init(void);
void HAL_UART_MspInit(UART_HandleTypeDef *huart);
HAL_StatusTypeDef Debug_Log(const char *message);

#endif // LOGGER_H