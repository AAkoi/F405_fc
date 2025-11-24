/**
 * @file    task_gyro.h
 * @brief   陀螺仪原始数据处理模块（零偏补偿 + 降采样）
 */

#ifndef TASK_GYRO_H
#define TASK_GYRO_H

#include <stdint.h>
#include <stdbool.h>


/**
 * @brief 零偏补偿后的陀螺仪数据（原始值）
 */
typedef struct gyro_compensated_s {
    int16_t x;
    int16_t y;
    int16_t z;
} gyro_compensated_t;

/**
 * @brief 刻度转换后的陀螺仪数据（°/s）
 */
typedef struct gyro_scaled_s {
    float dps_x;    // X轴角速度（度/秒）
    float dps_y;    // Y轴角速度（度/秒）
    float dps_z;    // Z轴角速度（度/秒）
} gyro_scaled_t;

/**
 * @brief 降采样后的陀螺仪数据（°/s，未滤波）
 */
typedef struct gyro_decimated_s {
    float dps_x;    // X轴平均角速度（度/秒）
    float dps_y;    // Y轴平均角速度（度/秒）
    float dps_z;    // Z轴平均角速度（度/秒）
    bool  ready;    // 数据就绪标志
} gyro_decimated_t;


extern gyro_compensated_t gyro_compensated;   // 零偏补偿后的数据（原始值）
extern gyro_scaled_t gyro_scaled;             // 刻度转换后的数据（°/s）
extern gyro_decimated_t gyro_decimated;       // 降采样后的数据（°/s）



/**
 * @brief 初始化陀螺仪处理模块
 * @param decim_factor 降采样因子（例如8表示8:1降采样）
 * @note 必须在使用前调用
 * 
 * @example
 * // 8:1降采样（8KHz -> 1KHz）
 * gyro_processing_init(8);
 */
void gyro_processing_init(uint8_t decim_factor);

/**
 * @brief 处理一个陀螺仪原始样本（零偏补偿 + 刻度转换 + 降采样）
 * @param raw_x X轴原始数据（ADC值）
 * @param raw_y Y轴原始数据（ADC值）
 * @param raw_z Z轴原始数据（ADC值）
 * @return true=成功，false=未初始化
 * @note 
 * - 处理流程：原始值 → 零偏补偿 → 刻度转换(°/s) → 降采样
 * - 每次IMU中断时调用
 * - 零偏补偿后的数据在 gyro_compensated 中（原始值）
 * - 刻度转换后的数据在 gyro_scaled 中（°/s）
 * - 降采样后的数据在 gyro_decimated 中（°/s，当 ready=true 时）
 * - 降采样后的数据可以喂给滤波器
 */
bool gyro_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z);

#endif // TASK_GYRO_H
