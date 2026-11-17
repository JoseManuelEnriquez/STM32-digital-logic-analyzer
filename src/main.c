#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#define GPIO_PORT GPIOA
#define GPIO_PIN GPIO_PIN_5

void configure_gpio(void);

int main() {
    HAL_Init();
    configure_gpio();
    while(1){
        HAL_GPIO_TogglePin(GPIO_PORT, GPIO_PIN);
        HAL_Delay(500);
    }
}

void configure_gpio(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PORT, &GPIO_InitStruct);
}

void HAL_MspInit(void){
    __HAL_RCC_GPIOA_CLK_ENABLE();
}