#include "DMA_GPIO.h"

static DMA_HandleTypeDef hdma_gpio = {0};
static uint8_t capture_buffer[DMA_GPIO_BUFFER_SAMPLES];
static volatile uint32_t completed_samples;
static volatile uint8_t dma_error;

static void DMA_GPIO_HalfComplete(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_gpio) {
        completed_samples += DMA_GPIO_BUFFER_SAMPLES / 2U;
    }
}

static void DMA_GPIO_Complete(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_gpio) {
        completed_samples += DMA_GPIO_BUFFER_SAMPLES / 2U;
    }
}

static void DMA_GPIO_Error(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_gpio) {
        dma_error = 1U;
    }
}

HAL_StatusTypeDef DMA_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
               GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hdma_gpio.Instance = DMA1_Stream6;
    hdma_gpio.Init.Channel = DMA_CHANNEL_2;
    hdma_gpio.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_gpio.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_gpio.Init.MemInc = DMA_MINC_ENABLE;
    hdma_gpio.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_gpio.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_gpio.Init.Mode = DMA_CIRCULAR;
    hdma_gpio.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_gpio.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_gpio) != HAL_OK) {
        return HAL_ERROR;
    }

    hdma_gpio.XferHalfCpltCallback = DMA_GPIO_HalfComplete;
    hdma_gpio.XferCpltCallback = DMA_GPIO_Complete;
    hdma_gpio.XferErrorCallback = DMA_GPIO_Error;

    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    return HAL_OK;
}

HAL_StatusTypeDef DMA_GPIO_Start(void)
{
    completed_samples = 0U;
    dma_error = 0U;

    return HAL_DMA_Start_IT(&hdma_gpio,
                            (uint32_t)(uintptr_t)&GPIOA->IDR,
                            (uint32_t)(uintptr_t)capture_buffer,
                            DMA_GPIO_BUFFER_SAMPLES);
}

HAL_StatusTypeDef DMA_GPIO_Stop(void)
{
    return HAL_DMA_Abort(&hdma_gpio);
}

const uint8_t *DMA_GPIO_GetBuffer(void)
{
    return capture_buffer;
}

uint32_t DMA_GPIO_GetCompletedSamples(void)
{
    return completed_samples;
}

uint8_t DMA_GPIO_HasError(void)
{
    return dma_error;
}

void DMA1_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_gpio);
}
