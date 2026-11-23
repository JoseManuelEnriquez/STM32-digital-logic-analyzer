#include "stm32f4xx.h"
#include "TIMER4.h"
#include "DMA_GPIO.h"
#include "LOGGER.h"
#include "SUMP.h"

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}

int main() {
    HAL_Init();
    if (UART2_Init() != HAL_OK) {
        Error_Handler();
    }
    if (DMA_GPIO_Init() != HAL_OK) {
        Error_Handler();
    }
    if (TIMER4_Init() != HAL_OK) {
        Error_Handler();
    }
    if (SUMP_Init(UART2_GetHandle()) != HAL_OK) {
        Error_Handler();
    }
    while (1) {
        if (SUMP_Process() != HAL_OK) {
            Error_Handler();
        }
    }
}