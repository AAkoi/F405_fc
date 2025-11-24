/**
 * @file    task_gyro.c
 * @brief   陀螺仪原始数据处理实现（零偏补偿 + 降采样）
 */

#include "task_gyro.h"
#include "icm42688p.h"
#include <stdio.h>
#include <string.h>



// 处理状态
static bool gyro_processing_ready = false;   // 是否已初始化

// 降采样参数
static uint8_t decim_n = 1;                  // 降采样因子
static uint8_t decim_count = 0;              // 降采样计数器

// 降采样累加缓冲区（刻度转换后的°/s）
static float sum_dps_x = 0.0f;
static float sum_dps_y = 0.0f;
static float sum_dps_z = 0.0f;

// 输出数据（全局变量，供外部访问）
gyro_compensated_t gyro_compensated;         // 零偏补偿后的数据（原始值）
gyro_scaled_t gyro_scaled;                   // 刻度转换后的数据（°/s）
gyro_decimated_t gyro_decimated;             // 降采样后的数据（°/s）



/**
 * @brief 对陀螺仪原始数据进行零偏补偿
 * @param raw_x 输入/输出：X轴原始数据
 * @param raw_y 输入/输出：Y轴原始数据
 * @param raw_z 输入/输出：Z轴原始数据
 * @note 使用 ICM42688P 的校准偏移量进行补偿
 */
static void gyro_compensate_offset(int16_t *raw_x, int16_t *raw_y, int16_t *raw_z)
{
    extern icm42688p_dev_t icm;
    
    // 减去零偏（带溢出保护）
    int32_t x = (int32_t)(*raw_x) - (int32_t)icm.gyro_offset[0];
    int32_t y = (int32_t)(*raw_y) - (int32_t)icm.gyro_offset[1];
    int32_t z = (int32_t)(*raw_z) - (int32_t)icm.gyro_offset[2];
    
    // 限幅到 int16_t 范围
    if (x > 32767) x = 32767;
    if (x < -32768) x = -32768;
    if (y > 32767) y = 32767;
    if (y < -32768) y = -32768;
    if (z > 32767) z = 32767;
    if (z < -32768) z = -32768;
    
    *raw_x = (int16_t)x;
    *raw_y = (int16_t)y;
    *raw_z = (int16_t)z;
}

/**
 * @brief 刻度转换：将补偿后的原始值转换为°/s
 * @param comp_x 零偏补偿后的X轴数据（原始值）
 * @param comp_y 零偏补偿后的Y轴数据（原始值）
 * @param comp_z 零偏补偿后的Z轴数据（原始值）
 * @param dps_x 输出：X轴角速度（°/s）
 * @param dps_y 输出：Y轴角速度（°/s）
 * @param dps_z 输出：Z轴角速度（°/s）
 */
static void gyro_scale_to_dps(int16_t comp_x, int16_t comp_y, int16_t comp_z,
                              float *dps_x, float *dps_y, float *dps_z)
{
    extern icm42688p_dev_t icm;
    const float gscale = (icm.gyro_scale > 0.0f) ? icm.gyro_scale : 1.0f;
    
    *dps_x = (float)comp_x / gscale;
    *dps_y = (float)comp_y / gscale;
    *dps_z = (float)comp_z / gscale;
}

/**
 * @brief 处理降采样
 * @param dps_x X轴角速度（°/s）
 * @param dps_y Y轴角速度（°/s）
 * @param dps_z Z轴角速度（°/s）
 * @return true=降采样窗口已满，输出数据就绪；false=继续累加
 */
static bool gyro_decimate(float dps_x, float dps_y, float dps_z)
{
    // 累加刻度转换后的数据（°/s）
    sum_dps_x += dps_x;
    sum_dps_y += dps_y;
    sum_dps_z += dps_z;
    decim_count++;
    
    // 当累积足够的样本后，计算平均值并输出
    if (decim_count >= decim_n) {
        const float inv = 1.0f / (float)decim_n;
        
        // 计算平均值（降采样输出，单位°/s）
        gyro_decimated.dps_x = sum_dps_x * inv;
        gyro_decimated.dps_y = sum_dps_y * inv;
        gyro_decimated.dps_z = sum_dps_z * inv;
        gyro_decimated.ready = true;
        
        // 重置计数器和累加器
        decim_count = 0;
        sum_dps_x = sum_dps_y = sum_dps_z = 0.0f;
        
        return true;  // 数据就绪
    }
    
    gyro_decimated.ready = false;
    return false;  // 继续累加
}


/**
 * @brief 初始化陀螺仪处理模块
 */
void gyro_processing_init(uint8_t decim_factor)
{
    // 设置降采样因子
    if (decim_factor == 0) {
        decim_factor = 1;  // 至少为1（不降采样）
    }
    decim_n = decim_factor;
    decim_count = 0;
    
    // 清空累加器
    sum_dps_x = sum_dps_y = sum_dps_z = 0.0f;
    
    // 清空输出数据
    memset(&gyro_compensated, 0, sizeof(gyro_compensated_t));
    memset(&gyro_scaled, 0, sizeof(gyro_scaled_t));
    memset(&gyro_decimated, 0, sizeof(gyro_decimated_t));
    gyro_decimated.ready = false;
    
    // 标记已就绪
    gyro_processing_ready = true;
    
    printf("[gyro_processing] Initialized: decimation %d:1\r\n", decim_factor);
}

/**
 * @brief 处理一个陀螺仪原始样本
 * @note 处理流程：原始值 → 零偏补偿 → 刻度转换(°/s) → 降采样
 */
bool gyro_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z)
{
    // 检查是否已初始化
    if (!gyro_processing_ready) {
        static uint8_t warn_count = 0;
        if (warn_count++ < 5) {
            printf("[gyro_processing] Not initialized!\r\n");
        }
        return false;
    }
    
    // 步骤1：零偏补偿
    gyro_compensate_offset(&raw_x, &raw_y, &raw_z);
    
    // 保存补偿后的数据（原始值）
    gyro_compensated.x = raw_x;
    gyro_compensated.y = raw_y;
    gyro_compensated.z = raw_z;
    
    // 步骤2：刻度转换（原始值 → °/s）
    gyro_scale_to_dps(raw_x, raw_y, raw_z,
                     &gyro_scaled.dps_x,
                     &gyro_scaled.dps_y,
                     &gyro_scaled.dps_z);
    
    // 步骤3：降采样（累加并求平均）
    gyro_decimate(gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z);
    
    return true;
}
