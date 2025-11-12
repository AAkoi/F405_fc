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
#define ICM42688P_SPI1_GPIO_PORT          GPIOA
#define ICM42688P_SCK_PIN               GPIO_PIN_5 //PA5
#define ICM42688P_MISO_PIN              GPIO_PIN_6 //PA6
#define ICM42688P_MOSI_PIN              GPIO_PIN_7 //PA7

// 中断引脚 (INT1)
#define ICM42688P_INT_GPIO_PORT         GPIOC
#define ICM42688P_INT_PIN               GPIO_PIN_3

#define BMP280_IIC1_GPIO_PORT          GPIOB
#define BMP280_IIC1_SCL                GPIO_PIN_6
#define BMP280_IIC1_SDA                GPIO_PIN_7

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
