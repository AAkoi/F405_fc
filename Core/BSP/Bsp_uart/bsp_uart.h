/**
 * @file    bsp_uart.h
 * @brief   UART BSP 封装（基于 HAL UART）
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// 可用串口句柄（按需启用）
#ifdef USE_UART1
extern UART_HandleTypeDef huart1;
#endif
#ifdef USE_UART2
extern UART_HandleTypeDef huart2;
#endif
#ifdef USE_UART3
extern UART_HandleTypeDef huart3;
#endif
#ifdef USE_UART4
extern UART_HandleTypeDef huart4;
#endif

// 初始化所有启用的 UART（使用缺省波特率 115200）
void BSP_UART_Init(void);

// 打开指定 UART 并设置波特率（若已初始化将更新波特率）
void BSP_UART_Open(uint8_t uart_id, uint32_t baudrate);

// 发送数据（轮询）
int  BSP_UART_Write(uint8_t uart_id, const uint8_t *data, uint16_t len);

// 接收 1 字节完成回调（弱符号，可在其他文件重载）
void BSP_UART_RxByteCallback(uint8_t uart_id, uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif // BSP_UART_H

