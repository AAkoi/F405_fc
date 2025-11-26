/**
 * @file    task_acc.h
 * @brief   加速度计数据处理模块（零偏补偿 + 刻度转换）
 */

#ifndef TASK_ACC_H
#define TASK_ACC_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 零偏补偿后的加速度数据（原始值）
 */
typedef struct accel_compensated_s {
    int16_t x;
    int16_t y;
    int16_t z;
} accel_compensated_t;

/**
 * @brief 刻度转换后的加速度数据（g）
 */
typedef struct accel_scaled_s {
    float g_x;    // X轴加速度（g）
    float g_y;    // Y轴加速度（g）
    float g_z;    // Z轴加速度（g）
    bool  ready;  // 数据就绪标志
} accel_scaled_t;

extern accel_compensated_t accel_compensated;   // 零偏补偿后的数据（原始值）
extern accel_scaled_t accel_scaled;             // 刻度转换后的数据（g）

/**
 * @brief 初始化加速度计处理模块
 * @note 必须在使用前调用
 */
void accel_processing_init(void);

/**
 * @brief 处理一个加速度计原始样本（零偏补偿 + 刻度转换）
 * @param raw_x X轴原始数据（ADC值）
 * @param raw_y Y轴原始数据（ADC值）
 * @param raw_z Z轴原始数据（ADC值）
 * @return true=成功，false=未初始化
 * @note 
 * - 处理流程：原始值 → 零偏补偿 → 刻度转换(g)
 * - 每次IMU中断时调用
 * - 零偏补偿后的数据在 accel_compensated 中（原始值）
 * - 刻度转换后的数据在 accel_scaled 中（g）
 */
bool accel_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z);

#endif // TASK_ACC_H

