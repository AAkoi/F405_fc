/**
 * @file    hmc5883l.h
 * @brief   HMC5883L 磁力计应用层接口头文件
 * @author  Your Name
 * @date    2025
 * 
 * 这是 HMC5883L 的高层接口，提供简单易用的 API
 */

#ifndef HMC5883L_H
#define HMC5883L_H

#include <stdint.h>
#include <stdbool.h>
#include "hmc5883l_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 全局变量声明
 * ============================================================================ */

extern hmc5883l_dev_t hmc_dev;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 初始化 HMC5883L 驱动
 * @return true=成功, false=失败
 */
bool hmc5883l_init_driver(void);

/**
 * @brief 读取磁场数据（原始值）
 * @param mag_x X 轴磁场原始值
 * @param mag_y Y 轴磁场原始值
 * @param mag_z Z 轴磁场原始值
 * @return true=成功, false=失败
 */
bool hmc5883l_read_raw_data(int16_t *mag_x, int16_t *mag_y, int16_t *mag_z);

/**
 * @brief 读取磁场数据（Gauss）
 * @param mag_x X 轴磁场强度（Gauss）
 * @param mag_y Y 轴磁场强度（Gauss）
 * @param mag_z Z 轴磁场强度（Gauss）
 * @return true=成功, false=失败
 */
bool hmc5883l_read_gauss(float *mag_x, float *mag_y, float *mag_z);

/**
 * @brief 计算航向角（偏航角）
 * @return 航向角（度），范围 0-360
 */
float hmc5883l_get_heading(void);

/**
 * @brief 计算倾斜补偿后的航向角
 * @param roll 横滚角（度）
 * @param pitch 俯仰角（度）
 * @return 航向角（度），范围 0-360
 */
float hmc5883l_get_tilt_compensated_heading(float roll, float pitch);

/**
 * @brief 执行磁力计校准
 * @param samples 采样数量（建议 100-200）
 * @return true=校准成功, false=校准失败
 */
bool hmc5883l_calibrate_compass(uint16_t samples);

/**
 * @brief 执行自检
 * @return true=自检通过, false=自检失败
 */
bool hmc5883l_run_self_test(void);

/**
 * @brief 检查数据是否就绪
 * @return true=数据就绪, false=数据未就绪
 */
bool hmc5883l_is_data_ready(void);

#ifdef __cplusplus
}
#endif

#endif // HMC5883L_H

