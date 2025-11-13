#include "bsp_uart.h"
#include "bsp_pins.h"

#ifndef UART_DEFAULT_BAUD
#define UART_DEFAULT_BAUD 115200u
#endif

#ifdef USE_UART1
UART_HandleTypeDef huart1;
static uint8_t rx1;
static void uart1_gpio_init(void)
{
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 默认使用 GPIOA，可根据宏覆盖
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = UART1_TX_PIN | UART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = UART1_GPIO_AF;
    HAL_GPIO_Init(UART1_GPIO_PORT, &GPIO_InitStruct);

    // NVIC
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}
static void uart1_open(uint32_t baud)
{
    uart1_gpio_init();
    huart1.Instance = USART1;
    huart1.Init.BaudRate = baud;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
    HAL_UART_Receive_IT(&huart1, &rx1, 1);
}
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}
#endif

#ifdef USE_UART2
UART_HandleTypeDef huart2;
static uint8_t rx2;
static void uart2_gpio_init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = UART2_TX_PIN | UART2_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = UART2_GPIO_AF;
    HAL_GPIO_Init(UART2_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}
static void uart2_open(uint32_t baud)
{
    uart2_gpio_init();
    huart2.Instance = USART2;
    huart2.Init.BaudRate = baud;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
    HAL_UART_Receive_IT(&huart2, &rx2, 1);
}
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}
#endif

#ifdef USE_UART3
UART_HandleTypeDef huart3;
static uint8_t rx3;
static void uart3_gpio_init(void)
{
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = UART3_TX_PIN | UART3_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = UART3_GPIO_AF;
    HAL_GPIO_Init(UART3_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}
static void uart3_open(uint32_t baud)
{
    uart3_gpio_init();
    huart3.Instance = USART3;
    huart3.Init.BaudRate = baud;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart3);
    HAL_UART_Receive_IT(&huart3, &rx3, 1);
}
void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart3);
}
#endif

#ifdef USE_UART4
UART_HandleTypeDef huart4;
static uint8_t rx4;
static void uart4_gpio_init(void)
{
    __HAL_RCC_UART4_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = UART4_TX_PIN | UART4_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = UART4_GPIO_AF;
    HAL_GPIO_Init(UART4_GPIO_PORT, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(UART4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
}
static void uart4_open(uint32_t baud)
{
    uart4_gpio_init();
    huart4.Instance = UART4;
    huart4.Init.BaudRate = baud;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart4);
    HAL_UART_Receive_IT(&huart4, &rx4, 1);
}
void UART4_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart4);
}
#endif

// 对外接口
void BSP_UART_Init(void)
{
#ifdef USE_UART1
    uart1_open(UART_DEFAULT_BAUD);
#endif
#ifdef USE_UART2
    uart2_open(UART_DEFAULT_BAUD);
#endif
#ifdef USE_UART3
    uart3_open(UART_DEFAULT_BAUD);
#endif
#ifdef USE_UART4
    uart4_open(UART_DEFAULT_BAUD);
#endif
}

void BSP_UART_Open(uint8_t uart_id, uint32_t baudrate)
{
    switch (uart_id) {
#ifdef USE_UART1
    case 1: uart1_open(baudrate); break;
#endif
#ifdef USE_UART2
    case 2: uart2_open(baudrate); break;
#endif
#ifdef USE_UART3
    case 3: uart3_open(baudrate); break;
#endif
#ifdef USE_UART4
    case 4: uart4_open(baudrate); break;
#endif
    default: break;
    }
}

int BSP_UART_Write(uint8_t uart_id, const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return 0;
    HAL_StatusTypeDef st = HAL_OK;
    switch (uart_id) {
#ifdef USE_UART1
    case 1: st = HAL_UART_Transmit(&huart1, (uint8_t*)data, len, HAL_MAX_DELAY); break;
#endif
#ifdef USE_UART2
    case 2: st = HAL_UART_Transmit(&huart2, (uint8_t*)data, len, HAL_MAX_DELAY); break;
#endif
#ifdef USE_UART3
    case 3: st = HAL_UART_Transmit(&huart3, (uint8_t*)data, len, HAL_MAX_DELAY); break;
#endif
#ifdef USE_UART4
    case 4: st = HAL_UART_Transmit(&huart4, (uint8_t*)data, len, HAL_MAX_DELAY); break;
#endif
    default: st = HAL_ERROR; break;
    }
    return (st == HAL_OK) ? (int)len : -1;
}

// 默认弱回调，可在应用层重载
__attribute__((weak)) void BSP_UART_RxByteCallback(uint8_t uart_id, uint8_t byte)
{
    (void)uart_id; (void)byte;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#ifdef USE_UART1
    if (huart->Instance == USART1) {
        BSP_UART_RxByteCallback(1, rx1);
        HAL_UART_Receive_IT(&huart1, &rx1, 1);
        return;
    }
#endif
#ifdef USE_UART2
    if (huart->Instance == USART2) {
        BSP_UART_RxByteCallback(2, rx2);
        HAL_UART_Receive_IT(&huart2, &rx2, 1);
        return;
    }
#endif
#ifdef USE_UART3
    if (huart->Instance == USART3) {
        BSP_UART_RxByteCallback(3, rx3);
        HAL_UART_Receive_IT(&huart3, &rx3, 1);
        return;
    }
#endif
#ifdef USE_UART4
    if (huart->Instance == UART4) {
        BSP_UART_RxByteCallback(4, rx4);
        HAL_UART_Receive_IT(&huart4, &rx4, 1);
        return;
    }
#endif
}

