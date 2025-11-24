/**
 * @file    task_filter.h
 * @brief   陀螺仪滤波模块（PT1 + 抗混叠低通滤波）
 *          接收降采样后的数据，输出滤波后的角速度（°/s）
 */

#ifndef TASK_FILTER_H
#define TASK_FILTER_H

#include <stdint.h>
#include <stdbool.h>
#include "filter.h"

/**
 * @brief PT1滤波后的陀螺仪数据（°/s）
 */
typedef struct pt1raw_s {
    float pt1_gyro_x;  // °/s
    float pt1_gyro_y;  // °/s
    float pt1_gyro_z;  // °/s
} pt1raw;

/**
 * @brief 抗混叠滤波后的陀螺仪数据（°/s）
 */
typedef struct gyro_antialias_s {
    float x;  // °/s
    float y;  // °/s
    float z;  // °/s
} gyro_antialias_t;

/**
 * @brief 滤波并转换为物理单位后的陀螺仪数据
 */
typedef struct gyro_filtered_s {
    float dps_x;    // X轴角速度（度/秒）
    float dps_y;    // Y轴角速度（度/秒）
    float dps_z;    // Z轴角速度（度/秒）
    bool  ready;    // 数据就绪标志
} gyro_filtered_t;

extern pt1raw pt1_raw;                  // PT1滤波输出
extern gyro_antialias_t gyro_aa;        // 抗混叠滤波输出
extern gyro_filtered_t gyro_filtered;   // 最终滤波输出（°/s）


/**
 * @brief 初始化陀螺仪滤波器
 * @param sample_hz 滤波器输入频率（Hz，即降采样后的频率，例如1000Hz）
 * @param pt1_cut_hz PT1滤波器截止频率（Hz）
 * @param aa_cut_hz 抗混叠滤波器截止频率（Hz）
 * @note 必须在使用前调用
 * 
 * @example
 * // 降采样后1KHz输入，PT1截止100Hz，AA截止300Hz
 * gyro_filter_init(1000.0f, 100.0f, 300.0f);
 */
void gyro_filter_init(float sample_hz, float pt1_cut_hz, float aa_cut_hz);

/**
 * @brief 喂入一个降采样后的陀螺仪样本到滤波器
 * @param gyro_x X轴角速度（°/s，降采样后的平均值）
 * @param gyro_y Y轴角速度（°/s，降采样后的平均值）
 * @param gyro_z Z轴角速度（°/s，降采样后的平均值）
 * @return true=处理成功，false=滤波器未初始化
 * @note 
 * - 输入输出单位均为°/s
 * - 输入数据应该是降采样后的数据（来自 task_gyro 的 gyro_decimated）
 * - 滤波后的数据在 gyro_filtered 中
 */
bool gyro_filter_feed_sample(float gyro_x, float gyro_y, float gyro_z);

#endif // TASK_FILTER_H
