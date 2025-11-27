/**
 * @file    task_mag.c
 * @brief   磁力计数据处理实现（校准 + 刻度转换）
 */

#include "task_mag.h"
#include "hmc5883l.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32f4xx_hal.h"

#define MAG_FIELD_MIN_GAUSS 0.05f

// 处理状态
static bool mag_processing_ready = false;   // 是否已初始化

// 校准参数（硬铁偏移 + 软铁缩放）
static float mag_offset_x = 0.0f;
static float mag_offset_y = 0.0f;
static float mag_offset_z = 0.0f;
static float mag_scale_x = 1.0f;
static float mag_scale_y = 1.0f;
static float mag_scale_z = 1.0f;

// 输出数据（全局变量，供外部访问）
mag_raw_t mag_raw;                           // 原始数据
mag_calibrated_t mag_calibrated;             // 校准后的数据（gauss）

/**
 * @brief 初始化磁力计处理模块
 */
void mag_processing_init(void)
{
    // 清空输出数据
    memset(&mag_raw, 0, sizeof(mag_raw_t));
    memset(&mag_calibrated, 0, sizeof(mag_calibrated_t));
    mag_calibrated.ready = false;
    mag_calibrated.calibrated = false;
    
    // 标记已就绪
    mag_processing_ready = true;
    
    printf("[mag_processing] Initialized\r\n");
}

/**
 * @brief 磁力计校准（使用 hmc5883l lib 库）
 */
bool mag_calibrate(uint16_t samples)
{
    if (!mag_processing_ready) {
        printf("[mag_calibrate] Not initialized!\r\n");
        return false;
    }
    
    if (samples == 0) samples = 200;
    
    printf("[mag_calibrate] 开始磁力计校准，请慢速旋转设备（8字形）...\r\n");
    
    // 使用 hmc5883l lib 库的校准功能
    if (!hmc5883l_calibrate_compass(samples)) {
        printf("[mag_calibrate] 校准失败！\r\n");
        return false;
    }
    
    printf("[mag_calibrate] 校准完成！\r\n");
    mag_calibrated.calibrated = true;
    
    return true;
}

/**
 * @brief 手动设置磁力计校准参数
 */
void mag_set_calibration(float offset_x, float offset_y, float offset_z,
                        float scale_x, float scale_y, float scale_z)
{
    mag_offset_x = offset_x;
    mag_offset_y = offset_y;
    mag_offset_z = offset_z;
    mag_scale_x = scale_x;
    mag_scale_y = scale_y;
    mag_scale_z = scale_z;
    
    mag_calibrated.calibrated = true;
    
    printf("[mag_set_calibration] 校准参数已设置\r\n");
}

/**
 * @brief 应用校准参数并转换为 gauss
 */
static void mag_apply_calibration(int16_t raw_x, int16_t raw_y, int16_t raw_z,
                                  float *gauss_x, float *gauss_y, float *gauss_z)
{
    extern hmc5883l_dev_t hmc_dev;
    const float gain_scale = (hmc_dev.gain_scale > 0.0f) ? hmc_dev.gain_scale : 1.0f;
    
    // 步骤1：应用硬铁偏置校准（原始值层面）
    // 注意：hmc_dev.offset 应该在初始化时设为0，这里使用用户校准的 mag_offset
    float comp_x = (float)raw_x - mag_offset_x;
    float comp_y = (float)raw_y - mag_offset_y;
    float comp_z = (float)raw_z - mag_offset_z;
    
    // 步骤2：转换为 gauss
    float mx_gauss = comp_x / gain_scale;
    float my_gauss = comp_y / gain_scale;
    float mz_gauss = comp_z / gain_scale;
    
    // 步骤3：应用软铁缩放
    *gauss_x = mx_gauss * mag_scale_x;
    *gauss_y = my_gauss * mag_scale_y;
    *gauss_z = mz_gauss * mag_scale_z;
}

/**
 * @brief 处理一个磁力计原始样本
 */
bool mag_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z)
{
    // 检查是否已初始化
    if (!mag_processing_ready) {
        static uint8_t warn_count = 0;
        if (warn_count++ < 5) {
            printf("[mag_processing] Not initialized!\r\n");
        }
        return false;
    }
    
    // 保存原始数据
    mag_raw.x = raw_x;
    mag_raw.y = raw_y;
    mag_raw.z = raw_z;
    
    // 应用校准并转换为 gauss
    mag_apply_calibration(raw_x, raw_y, raw_z,
                         &mag_calibrated.gauss_x,
                         &mag_calibrated.gauss_y,
                         &mag_calibrated.gauss_z);

    mag_calibrated.magnitude_gauss = sqrtf(
        mag_calibrated.gauss_x * mag_calibrated.gauss_x +
        mag_calibrated.gauss_y * mag_calibrated.gauss_y +
        mag_calibrated.gauss_z * mag_calibrated.gauss_z
    );
    
    mag_calibrated.ready = true;
    
    return true;
}

/**
 * @brief 获取归一化后的磁力计向量
 */
bool mag_get_normalized(float *mx_unit, float *my_unit, float *mz_unit, float *strength_gauss)
{
    if (!mag_calibrated.ready) {
        return false;
    }

    float norm = mag_calibrated.magnitude_gauss;
    if (strength_gauss) {
        *strength_gauss = norm;
    }
    if (norm < MAG_FIELD_MIN_GAUSS) {
        norm = MAG_FIELD_MIN_GAUSS;
    }

    const float inv = 1.0f / norm;
    *mx_unit = mag_calibrated.gauss_x * inv;
    *my_unit = mag_calibrated.gauss_y * inv;
    *mz_unit = mag_calibrated.gauss_z * inv;
    return true;
}
