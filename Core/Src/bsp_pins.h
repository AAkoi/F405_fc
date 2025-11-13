/**
 * @file    bsp_pins.h
 * @brief   全局引脚定义和GPIO宏
 * @note    所有外设引脚统一在此定义
 */

#ifndef BSP_PINS_H
#define BSP_PINS_H

#include "stm32f4xx_hal.h"

/* ============================================================================
 * GPIO 控制宏定义
 * ============================================================================ */

// GPIO置高电平
#define GPIO_PIN_SET_HIGH(port, pin)    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)

// GPIO置低电平
#define GPIO_PIN_SET_LOW(port, pin)     HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)

// GPIO电平翻转
#define GPIO_PIN_TOGGLE(port, pin)      HAL_GPIO_TogglePin(port, pin)

// GPIO读取电平
#define GPIO_PIN_READ(port, pin)        HAL_GPIO_ReadPin(port, pin)

/* ============================================================================
 * ICM42688P 引脚定义
 * ============================================================================ */

// SPI片选引脚 (CS)
#define ICM42688P_CS_GPIO_PORT          GPIOC
#define ICM42688P_CS_PIN                GPIO_PIN_2
#define ICM42688P_SPI1_GPIO_PORT        GPIOA
#define ICM42688P_SCK_PIN               GPIO_PIN_5 //PA5
#define ICM42688P_MISO_PIN              GPIO_PIN_6 //PA6
#define ICM42688P_MOSI_PIN              GPIO_PIN_7 //PA7

// 中断引脚 (INT1)
#define ICM42688P_INT_GPIO_PORT         GPIOC
#define ICM42688P_INT_PIN               GPIO_PIN_3

//IIC1
#define BMP280_IIC1_GPIO_PORT          GPIOB
#define BMP280_IIC1_SCL                GPIO_PIN_6
#define BMP280_IIC1_SDA                GPIO_PIN_7

/* ============================================================================
 * UART 引脚定义（使用 ifdef 选择性启用）
 * 建议在编译选项或全局宏中定义 USE_UARTx 以启用对应串口。
 * 缺省映射如下（STM32F405）：
 *  - USART1: TX=PA9  RX=PA10  AF7
 *  - USART2: TX=PA2  RX=PA3   AF7
 *  - USART3: TX=PC10 RX=PC11  AF7
 *  - UART4 : TX=PA0  RX=PA1   AF8
 * 可按需要在编译前覆盖 UARTx_* 宏实现换脚。
 * ============================================================================ */

#ifdef USE_UART1
#ifndef UART1_GPIO_PORT
#define UART1_GPIO_PORT  GPIOA
#define UART1_TX_PIN     GPIO_PIN_9
#define UART1_RX_PIN     GPIO_PIN_10
#define UART1_GPIO_AF    GPIO_AF7_USART1
#endif
#endif

#ifdef USE_UART2
#ifndef UART2_GPIO_PORT
#define UART2_GPIO_PORT  GPIOA
#define UART2_TX_PIN     GPIO_PIN_2
#define UART2_RX_PIN     GPIO_PIN_3
#define UART2_GPIO_AF    GPIO_AF7_USART2
#endif
#endif

#ifdef USE_UART3
#ifndef UART3_GPIO_PORT
#define UART3_GPIO_PORT  GPIOC
#define UART3_TX_PIN     GPIO_PIN_10
#define UART3_RX_PIN     GPIO_PIN_11
#define UART3_GPIO_AF    GPIO_AF7_USART3
#endif
#endif

#ifdef USE_UART4
#ifndef UART4_GPIO_PORT
#define UART4_GPIO_PORT  GPIOA
#define UART4_TX_PIN     GPIO_PIN_0
#define UART4_RX_PIN     GPIO_PIN_1
#define UART4_GPIO_AF    GPIO_AF8_UART4
#endif
#endif

/* ============================================================================
 * ICM42688P GPIO 控制宏
 * ============================================================================ */

// CS片选控制
#define ICM42688P_CS_LOW()              GPIO_PIN_SET_LOW(ICM42688P_CS_GPIO_PORT, ICM42688P_CS_PIN)
#define ICM42688P_CS_HIGH()             GPIO_PIN_SET_HIGH(ICM42688P_CS_GPIO_PORT, ICM42688P_CS_PIN)

// 中断引脚读取
#define ICM42688P_INT_READ()            GPIO_PIN_READ(ICM42688P_INT_GPIO_PORT, ICM42688P_INT_PIN)

/* ============================================================================
 * 其他外设引脚定义（可在此扩展）
 * ============================================================================ */

// 示例：LED引脚
// #define LED_GPIO_PORT                 GPIOC
// #define LED_PIN                       GPIO_PIN_13
// #define LED_ON()                      GPIO_PIN_SET_LOW(LED_GPIO_PORT, LED_PIN)
// #define LED_OFF()                     GPIO_PIN_SET_HIGH(LED_GPIO_PORT, LED_PIN)
// #define LED_TOGGLE()                  GPIO_PIN_TOGGLE(LED_GPIO_PORT, LED_PIN)

#endif // BSP_PINS_H
