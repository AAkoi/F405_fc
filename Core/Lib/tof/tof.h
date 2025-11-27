/**
 * @file    tof.h
 * @brief   VL53L0X ToF 测距传感器应用层接口头文件
 * @author  Based on ST VL53L0X API
 * @date    2025
 * 
 * 这是 VL53L0X 的高层接口，提供简单易用的 API
 * 使用 I2C3 接口通信
 */

#ifndef TOF_H
#define TOF_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 类型定义
 * ============================================================================ */

/**
 * @brief 测量模式枚举
 */
typedef enum {
    TOF_MODE_DEFAULT      = 0,  // 默认模式
    TOF_MODE_HIGH_ACCURACY = 1,  // 高精度模式
    TOF_MODE_LONG_RANGE   = 2,  // 长距离模式
    TOF_MODE_HIGH_SPEED   = 3   // 高速模式
} tof_mode_t;

/**
 * @brief 测距数据结构体
 */
typedef struct {
    uint16_t range_mm;           // 距离（毫米）
    uint16_t range_status;       // 测量状态
    float signal_rate;           // 信号强度
    uint32_t measurement_time;   // 测量时间（微秒）
} tof_data_t;

/* ============================================================================
 * 常量定义
 * ============================================================================ */

#define TOF_I2C_ADDRESS_DEFAULT     0x52    // VL53L0X 默认 I2C 地址（8位）
#define TOF_MAX_RANGE_MM            2000    // 最大测量范围（mm）
#define TOF_MIN_RANGE_MM            30      // 最小测量范围（mm）

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 初始化 VL53L0X ToF 传感器
 * @return true=初始化成功, false=初始化失败
 * 
 * 注意：需要先初始化 I2C3
 */
bool tof_init(void);

/**
 * @brief 设置测量模式
 * @param mode 测量模式（DEFAULT/HIGH_ACCURACY/LONG_RANGE/HIGH_SPEED）
 * @return true=设置成功, false=设置失败
 */
bool tof_set_mode(tof_mode_t mode);

/**
 * @brief 执行单次测距
 * @param distance_mm 输出：测量距离（毫米）
 * @return true=测量成功, false=测量失败
 */
bool tof_read_distance(uint16_t *distance_mm);

/**
 * @brief 执行单次测距（详细数据）
 * @param data 输出：测距数据结构体指针
 * @return true=测量成功, false=测量失败
 */
bool tof_read_data(tof_data_t *data);

/**
 * @brief 启动连续测距模式
 * @param period_ms 测量周期（毫秒），0 = 尽可能快
 * @return true=启动成功, false=启动失败
 */
bool tof_start_continuous(uint32_t period_ms);

/**
 * @brief 停止连续测距模式
 * @return true=停止成功, false=停止失败
 */
bool tof_stop_continuous(void);

/**
 * @brief 读取连续测距数据（连续模式下使用）
 * @param distance_mm 输出：测量距离（毫米）
 * @return true=测量成功, false=测量失败
 */
bool tof_get_continuous_distance(uint16_t *distance_mm);

/**
 * @brief 获取设备信息
 * @param name 输出：设备名称（至少32字节）
 * @param revision_major 输出：主版本号
 * @param revision_minor 输出：次版本号
 * @return true=成功, false=失败
 */
bool tof_get_device_info(char *name, uint8_t *revision_major, uint8_t *revision_minor);

/**
 * @brief 执行偏移校准
 * @param target_distance_mm 目标距离（毫米），建议使用100mm
 * @return true=校准成功, false=校准失败
 * 
 * 注意：校准时需要在传感器前方放置指定距离的白色目标板
 */
bool tof_calibrate_offset(uint16_t target_distance_mm);

/**
 * @brief 执行串扰校准
 * @param target_distance_mm 目标距离（毫米），建议使用400mm
 * @return true=校准成功, false=校准失败
 * 
 * 注意：校准时需要在传感器前方放置指定距离的白色目标板
 */
bool tof_calibrate_xtalk(uint16_t target_distance_mm);

/**
 * @brief 软复位传感器
 * @return true=复位成功, false=复位失败
 */
bool tof_reset(void);

/**
 * @brief 设置测量超时时间
 * @param timeout_ms 超时时间（毫秒）
 * @return true=设置成功, false=设置失败
 */
bool tof_set_measurement_timing_budget(uint32_t timeout_ms);

/**
 * @brief 获取当前测量模式字符串
 * @param mode 测量模式
 * @return 模式名称字符串
 */
const char* tof_get_mode_string(tof_mode_t mode);

/**
 * @brief 获取测量状态字符串
 * @param status 测量状态代码
 * @return 状态描述字符串
 */
const char* tof_get_status_string(uint8_t status);

#ifdef __cplusplus
}
#endif

#endif // TOF_H

