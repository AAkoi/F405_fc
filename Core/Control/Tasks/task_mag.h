/**
 * @file    task_mag.h
 * @brief   磁力计数据处理模块（校准 + 刻度转换）
 */

#ifndef TASK_MAG_H
#define TASK_MAG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 原始磁力计数据
 */
typedef struct mag_raw_s {
    int16_t x;
    int16_t y;
    int16_t z;
} mag_raw_t;

/**
 * @brief 校准后的磁力计数据（gauss）
 */
typedef struct mag_calibrated_s {
    float gauss_x;    // X轴磁场强度（gauss）
    float gauss_y;    // Y轴磁场强度（gauss）
    float gauss_z;    // Z轴磁场强度（gauss）
    float magnitude_gauss; // 磁场模长（gauss）
    bool  ready;      // 数据就绪标志
    bool  calibrated; // 是否已校准
} mag_calibrated_t;

extern mag_raw_t mag_raw;                   // 原始数据
extern mag_calibrated_t mag_calibrated;     // 校准后的数据（gauss）

/**
 * @brief 初始化磁力计处理模块
 * @note 必须在使用前调用
 */
void mag_processing_init(void);

/**
 * @brief 磁力计校准（使用 hmc5883l lib 库的校准功能）
 * @param samples 采样数量（建议 200）
 * @return true=校准成功，false=校准失败
 * @note 校准时需要慢速旋转设备（8字形）
 */
bool mag_calibrate(uint16_t samples);

/**
 * @brief 手动设置磁力计校准参数
 * @param offset_x X轴偏移（gauss）
 * @param offset_y Y轴偏移（gauss）
 * @param offset_z Z轴偏移（gauss）
 * @param scale_x X轴缩放因子
 * @param scale_y Y轴缩放因子
 * @param scale_z Z轴缩放因子
 */
void mag_set_calibration(float offset_x, float offset_y, float offset_z,
                        float scale_x, float scale_y, float scale_z);

/**
 * @brief 处理一个磁力计原始样本（校准 + 刻度转换）
 * @param raw_x X轴原始数据
 * @param raw_y Y轴原始数据
 * @param raw_z Z轴原始数据
 * @return true=成功，false=未初始化
 * @note 
 * - 处理流程：原始值 → 硬铁校准 → 软铁校准 → gauss
 * - 原始数据在 mag_raw 中
 * - 校准后的数据在 mag_calibrated 中（gauss）
 */
bool mag_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z);

#endif // TASK_MAG_H

