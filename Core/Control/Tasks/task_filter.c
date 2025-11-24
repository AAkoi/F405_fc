/**
 * @file    task_filter.c
 * @brief   陀螺仪滤波实现（PT1 + 抗混叠低通滤波）
 *          只负责纯滤波，输入输出单位均为°/s
 */

#include "task_fliter.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// 全局变量
// ============================================================================

// 滤波器实例
static pt1Filter_t pt1Filter_dev;         // PT1滤波器
static biquadFilter_t aa_x, aa_y, aa_z;   // 抗混叠滤波器（X/Y/Z轴）

// 滤波器状态
static bool filter_ready = false;         // 滤波器是否已初始化

// 滤波输出数据（全局变量，供外部访问）
pt1raw pt1_raw;                           // PT1滤波输出
gyro_antialias_t gyro_aa;                 // 抗混叠滤波输出
gyro_filtered_t gyro_filtered;            // 最终滤波输出（°/s）

// ============================================================================
// 内部函数
// ============================================================================

/**
 * @brief 处理一个陀螺仪样本（纯滤波）
 * @param gx X轴数据（降采样后的平均值，单位°/s）
 * @param gy Y轴数据（降采样后的平均值，单位°/s）
 * @param gz Z轴数据（降采样后的平均值，单位°/s）
 * @return true=成功，false=滤波器未初始化
 */
static bool gyro_filter_process_sample(float gx, float gy, float gz)
{
    // 检查滤波器是否已初始化
    if (!filter_ready) {
        static uint8_t warn_count = 0;
        if (warn_count++ < 5) {
            printf("[gyro_filter] Filter not ready!\r\n");
        }
        return false;
    }

    // PT1 低通滤波
    pt1_raw.pt1_gyro_x = pt1FilterApply(&pt1Filter_dev, gx);
    pt1_raw.pt1_gyro_y = pt1FilterApply(&pt1Filter_dev, gy);
    pt1_raw.pt1_gyro_z = pt1FilterApply(&pt1Filter_dev, gz);
    
    // 抗混叠滤波（Biquad LPF）
    gyro_aa.x = biquadFilterApply(&aa_x, pt1_raw.pt1_gyro_x);
    gyro_aa.y = biquadFilterApply(&aa_y, pt1_raw.pt1_gyro_y);
    gyro_aa.z = biquadFilterApply(&aa_z, pt1_raw.pt1_gyro_z);

    // 输出滤波后的数据（单位已是°/s）
    gyro_filtered.dps_x = gyro_aa.x;
    gyro_filtered.dps_y = gyro_aa.y;
    gyro_filtered.dps_z = gyro_aa.z;
    gyro_filtered.ready = true;

    return true;
}

// ============================================================================
// 外部接口实现
// ============================================================================

/**
 * @brief 喂入降采样后的陀螺仪样本到滤波器
 */
bool gyro_filter_feed_sample(float gyro_x, float gyro_y, float gyro_z)
{
    return gyro_filter_process_sample(gyro_x, gyro_y, gyro_z);
}

/**
 * @brief 初始化陀螺仪滤波器
 */
void gyro_filter_init(float sample_hz, float pt1_cut_hz, float aa_cut_hz)
{
    // 参数检查
    if (sample_hz <= 0.0f) {
        printf("[gyro_filter] Invalid sample rate: %.1f Hz\r\n", sample_hz);
        return;
    }

    // 计算采样周期
    const float dt = 1.0f / sample_hz;
    const uint32_t refresh_us = (uint32_t)(1000000.0f / sample_hz);

    // 初始化 PT1 滤波器
    pt1FilterInit(&pt1Filter_dev, pt1FilterGain(pt1_cut_hz, dt));
    
    // 初始化抗混叠 Biquad 低通滤波器（三轴）
    biquadFilterInitLPF(&aa_x, aa_cut_hz, refresh_us);
    biquadFilterInitLPF(&aa_y, aa_cut_hz, refresh_us);
    biquadFilterInitLPF(&aa_z, aa_cut_hz, refresh_us);

    // 清空所有状态
    memset(&pt1_raw, 0, sizeof(pt1raw));
    memset(&gyro_aa, 0, sizeof(gyro_antialias_t));
    memset(&gyro_filtered, 0, sizeof(gyro_filtered_t));
    gyro_filtered.ready = false;

    // 标记滤波器已就绪
    filter_ready = true;
    
    printf("[gyro_filter] Initialized: %.0f Hz input, PT1 cut %.0f Hz, AA cut %.0f Hz\r\n",
           sample_hz, pt1_cut_hz, aa_cut_hz);
}
