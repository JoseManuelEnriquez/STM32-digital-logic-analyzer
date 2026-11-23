#ifndef DMA_GPIO_H
#define DMA_GPIO_H
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"

#define DMA_GPIO_MAX_CAPTURE_SAMPLES 4096U
#define DMA_GPIO_BUFFER_SAMPLES (DMA_GPIO_MAX_CAPTURE_SAMPLES * 3U)

HAL_StatusTypeDef DMA_GPIO_Init(void);
HAL_StatusTypeDef DMA_GPIO_Start(void);
HAL_StatusTypeDef DMA_GPIO_Stop(void);
const uint8_t *DMA_GPIO_GetBuffer(void);
uint32_t DMA_GPIO_GetCompletedSamples(void);
uint8_t DMA_GPIO_HasError(void);

#endif // DMA_GPIO_H