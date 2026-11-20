#include "stm32f4xx.h"
#include "TIMER4.h"
#include "DMA_GPIO.h"
#include "LOGGER.h"

void Error_Handler(void) {}

int main() {
    HAL_Init();
    if (UART2_Init() != HAL_OK) {
        Error_Handler();
    }
    if (Debug_Log("Debug logger ready") != HAL_OK) {
        Error_Handler();
    }
    if(DMA_GPIO_Init() != HAL_OK) {
        // Handle error
    }
    if(TIMER4_Init() != HAL_OK) {
        // Handle error
    }
    while(1){
        // Main loop
    }
}