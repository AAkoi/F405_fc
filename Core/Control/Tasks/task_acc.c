/**
 * @file    task_acc.c
 * @brief   加速度计数据处理实现（仅刻度转换，不做零偏补偿）
 * @note    加速度计保留重力分量，因为重力是姿态解算的必需信号
 */

#include "task_acc.h"
#include "icm42688p.h"
#include <stdio.h>
#include <string.h>

// 处理状态
static bool accel_processing_ready = false;   // 是否已初始化

// 输出数据（全局变量，供外部访问）
accel_compensated_t accel_compensated;         // 原始数据（不做零偏补偿）
accel_scaled_t accel_scaled;                   // 刻度转换后的数据（g）

/**
 * @brief 刻度转换：将补偿后的原始值转换为 g
 * @param comp_x 零偏补偿后的X轴数据（原始值）
 * @param comp_y 零偏补偿后的Y轴数据（原始值）
 * @param comp_z 零偏补偿后的Z轴数据（原始值）
 * @param g_x 输出：X轴加速度（g）
 * @param g_y 输出：Y轴加速度（g）
 * @param g_z 输出：Z轴加速度（g）
 */
static void accel_scale_to_g(int16_t comp_x, int16_t comp_y, int16_t comp_z,
                             float *g_x, float *g_y, float *g_z)
{
    extern icm42688p_dev_t icm;
    const float ascale = (icm.accel_scale > 0.0f) ? icm.accel_scale : 1.0f;
    
    *g_x = (float)comp_x / ascale;
    *g_y = (float)comp_y / ascale;
    *g_z = (float)comp_z / ascale;
}

/**
 * @brief 初始化加速度计处理模块
 */
void accel_processing_init(void)
{
    // 清空输出数据
    memset(&accel_compensated, 0, sizeof(accel_compensated_t));
    memset(&accel_scaled, 0, sizeof(accel_scaled_t));
    accel_scaled.ready = false;
    
    // 标记已就绪
    accel_processing_ready = true;
    
    printf("[accel_processing] Initialized\r\n");
}

/**
 * @brief 处理一个加速度计原始样本
 * @note 处理流程：原始值 → 刻度转换(g)
 *       不做零偏补偿，因为重力是姿态解算的必需信号
 */
bool accel_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z)
{
    // 检查是否已初始化
    if (!accel_processing_ready) {
        static uint8_t warn_count = 0;
        if (warn_count++ < 5) {
            printf("[accel_processing] Not initialized!\r\n");
        }
        return false;
    }
    
    // 保存原始数据（不做零偏补偿）
    accel_compensated.x = raw_x;
    accel_compensated.y = raw_y;
    accel_compensated.z = raw_z;
    
    // 刻度转换（原始值 → g）
    accel_scale_to_g(raw_x, raw_y, raw_z,
                    &accel_scaled.g_x,
                    &accel_scaled.g_y,
                    &accel_scaled.g_z);
    
    accel_scaled.ready = true;
    
    return true;
}

