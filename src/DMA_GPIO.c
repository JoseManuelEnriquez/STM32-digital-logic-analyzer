#include "DMA_GPIO.h"

static DMA_HandleTypeDef hdma_gpio = {0};

/** 
 * @brief  Initializes the GPIO for DMA.
 * @param  None
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef DMA_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hdma_gpio.Instance = DMA1_Stream0;
    hdma_gpio.Init.Channel = DMA_CHANNEL_2;
    hdma_gpio.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_gpio.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_gpio.Init.MemInc = DMA_MINC_ENABLE;
    hdma_gpio.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_gpio.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_gpio.Init.Mode = DMA_CIRCULAR;
    hdma_gpio.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_gpio.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    return HAL_DMA_Init(&hdma_gpio);
}

HAL_StatusTypeDef DMA_Start(uint32_t src_address, uint32_t dst_address, uint32_t data_len){
    return HAL_DMA_Start(&hdma_gpio, src_address, dst_address, data_len);
}

HAL_StatusTypeDef DMA_Stop(){
    return HAL_DMA_Abort(&hdma_gpio);
}