/**
 * @file    task_mag.h
 * @brief   磁力计数据处理模块（校准 + 刻度转换）
 */

#ifndef TASK_MAG_H
#define TASK_MAG_H

#include <stdint.h>
#include <stdbool.h>

typedef struct mag_raw_s {
    int16_t x;
    int16_t y;
    int16_t z;
} mag_raw_t;

typedef struct mag_calibrated_s {
    float gauss_x;          // X轴磁场强度（gauss）
    float gauss_y;          // Y轴磁场强度（gauss）
    float gauss_z;          // Z轴磁场强度（gauss）
    float magnitude_gauss;  // 磁场模长（gauss）
    bool  ready;            // 数据就绪标志
    bool  calibrated;       // 是否已校准
} mag_calibrated_t;

extern mag_raw_t mag_raw;                   // 原始数据
extern mag_calibrated_t mag_calibrated;     // 校准后的数据（gauss）

// 初始化磁力计处理模块
void mag_processing_init(void);

// 磁力计校准（使用 hmc5883l lib 库的校准功能）
bool mag_calibrate(uint16_t samples);

// 手动设置磁力计校准参数
void mag_set_calibration(float offset_x, float offset_y, float offset_z,
                        float scale_x, float scale_y, float scale_z);

// 处理一个磁力计原始样本（校准 + 刻度转换）
bool mag_process_sample(int16_t raw_x, int16_t raw_y, int16_t raw_z);

// 获取归一化后的磁力计向量（单位向量）
bool mag_get_normalized(float *mx_unit, float *my_unit, float *mz_unit, float *strength_gauss);

#endif // TASK_MAG_H
