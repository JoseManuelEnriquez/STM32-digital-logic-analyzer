#ifndef SUMP_H
#define SUMP_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"

/**
 * Inicializa el controlador SUMP y asocia el UART empleado por el protocolo.
 * @param uart Handle HAL del USART configurado para la comunicación con sigrok.
 * @return HAL_OK si queda inicializado; HAL_ERROR si uart es NULL o falla el reset.
 */
HAL_StatusTypeDef SUMP_Init(UART_HandleTypeDef *uart);

/**
 * Procesa un byte SUMP pendiente y actualiza la adquisición en curso.
 * Debe llamarse repetidamente desde el bucle principal.
 * @return HAL_OK si no hubo errores; otro estado HAL si falla UART, DMA o TIM4.
 */
HAL_StatusTypeDef SUMP_Process(void);

#endif
