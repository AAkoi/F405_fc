/**
 * @file    test_spi_hardware.h
 * @brief   SPI硬件底层诊断工具
 * @note    用于诊断SPI通信问题
 */

#ifndef TEST_SPI_HARDWARE_H
#define TEST_SPI_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief SPI1硬件诊断测试
 * @note  测试GPIO状态、SPI寄存器配置、时序等
 */
void test_spi1_hardware_diagnosis(void);

/**
 * @brief 测试ICM42688P的CS和中断引脚
 */
void test_icm42688p_gpio_pins(void);

/**
 * @brief 位操作方式测试SPI通信（绕过HAL库）
 */
void test_spi1_bitbang(void);

#endif // TEST_SPI_HARDWARE_H

