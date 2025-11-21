/**
 * @file    test_bmp280.h
 * @brief   BMP280气压传感器测试程序
 * @note    包含I2C通信测试、温度/气压/高度读取测试
 */

#ifndef TEST_BMP280_H
#define TEST_BMP280_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief BMP280完整测试循环
 * @note  持续读取并显示：温度、气压、海拔高度
 *        自动检测连续失败并尝试重新初始化
 */
void test_bmp280_full(void);

/**
 * @brief BMP280单次数据读取测试
 * @note  读取一次数据并显示，用于快速验证
 * @return true-成功, false-失败
 */
bool test_bmp280_single_read(void);

#endif // TEST_BMP280_H

