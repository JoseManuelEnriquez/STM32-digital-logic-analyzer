#ifndef DMA_GPIO_H
#define DMA_GPIO_H
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"

HAL_StatusTypeDef DMA_GPIO_Init(void);
HAL_StatusTypeDef DMA_Start(uint32_t src_address, uint32_t dst_address, uint32_t data_len);

#endif // DMA_GPIO_H