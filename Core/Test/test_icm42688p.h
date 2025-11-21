/**
 * @file    test_icm42688p.h
 * @brief   ICM42688P IMU传感器测试程序
 * @note    包含SPI通信测试、传感器数据测试、姿态解算测试
 */

#ifndef TEST_ICM42688P_H
#define TEST_ICM42688P_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief ICM42688P完整测试流程
 * @note  包含：
 *        - SPI通信验证
 *        - 传感器初始化检查
 *        - 原始数据读取测试
 *        - 陀螺仪零偏校准
 *        - 姿态解算测试（Mahony滤波器）
 */
void test_icm42688p_full(void);

/**
 * @brief ICM42688P欧拉角测试（原euler_test_loop）
 * @note  持续输出欧拉角，用于调试姿态解算
 */
void test_icm42688p_euler_angles(void);

/**
 * @brief ICM42688P原始数据测试
 * @note  只输出原始加速度和陀螺仪数据，不进行姿态解算
 */
void test_icm42688p_raw_data(void);

#endif // TEST_ICM42688P_H

