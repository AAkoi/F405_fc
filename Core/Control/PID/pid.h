/**
 * @file    pid_lib.h
 * @brief   通用PID控制器库（基于Betaflight PID实现）
 * @author  Extracted from Betaflight
 * @date    2025
 * 
 * 功能特性：
 * - 标准PID控制（P、I、D项）
 * - 前馈控制（Feedforward）
 * - 积分限幅（Anti-windup）
 * - 微分滤波（D-term filtering）
 * - 输出限幅
 * - 多轴支持
 * - 可配置参数
 */

#ifndef PID_LIB_H
#define PID_LIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 配置选项
 * ============================================================================ */

// 最大轴数（默认3轴：Roll, Pitch, Yaw）
#ifndef PID_MAX_AXIS
#define PID_MAX_AXIS 3
#endif

// 是否启用前馈控制
#ifndef PID_ENABLE_FEEDFORWARD
#define PID_ENABLE_FEEDFORWARD 1
#endif

// 是否启用微分滤波
#ifndef PID_ENABLE_DTERM_FILTER
#define PID_ENABLE_DTERM_FILTER 1
#endif

/* ============================================================================
 * 常量定义
 * ============================================================================ */

// PID增益缩放因子（用于更好的调参范围）
#define PID_PTERM_SCALE     0.032029f
#define PID_ITERM_SCALE     0.244381f
#define PID_DTERM_SCALE     0.000529f
#define PID_FEEDFORWARD_SCALE 0.013754f

// 默认PID参数
#define PID_P_DEFAULT       45
#define PID_I_DEFAULT       80
#define PID_D_DEFAULT       30
#define PID_F_DEFAULT       120

// 输出限幅
#define PID_OUTPUT_LIMIT_DEFAULT    500
#define PID_OUTPUT_LIMIT_MIN        100
#define PID_OUTPUT_LIMIT_MAX        1000

// 积分限幅
#define PID_ITERM_LIMIT_DEFAULT     400
#define PID_ITERM_WINDUP_DEFAULT    80  // 百分比

// 增益范围
#define PID_GAIN_MIN        0
#define PID_GAIN_MAX        250
#define PID_F_GAIN_MAX      1000

/* ============================================================================
 * 数据结构
 * ============================================================================ */

/**
 * @brief PID增益系数（原始值，0-250）
 */
typedef struct {
    uint8_t P;      // 比例增益
    uint8_t I;      // 积分增益
    uint8_t D;      // 微分增益
    uint16_t F;     // 前馈增益（0-1000）
} pid_gains_t;

/**
 * @brief PID系数（缩放后的浮点值）
 */
typedef struct {
    float Kp;       // 比例系数
    float Ki;       // 积分系数
    float Kd;       // 微分系数
    float Kf;       // 前馈系数
} pid_coefficients_t;

/**
 * @brief PID配置参数
 */
typedef struct {
    pid_gains_t gains;              // PID增益
    float output_limit;             // 输出限幅
    float iterm_limit;              // 积分限幅
    uint8_t iterm_windup;           // 积分抗饱和阈值（百分比）
    float dterm_lpf_hz;             // D项低通滤波频率（Hz）
    bool enable_feedforward;        // 是否启用前馈
    bool enable_dterm_filter;       // 是否启用D项滤波
} pid_config_t;

/**
 * @brief 简单的一阶低通滤波器
 */
typedef struct {
    float state;                    // 滤波器状态
    float k;                        // 滤波系数
} pid_lpf_t;

/**
 * @brief PID运行时数据
 */
typedef struct {
    // PID项
    float P;                        // 比例项
    float I;                        // 积分项
    float D;                        // 微分项
    float F;                        // 前馈项
    float output;                   // 总输出
    
    // 内部状态
    float previous_error;           // 上次误差
    float previous_measurement;     // 上次测量值
    float previous_setpoint;        // 上次设定值
    
    // 滤波器
    pid_lpf_t dterm_filter;         // D项滤波器
    
    // 统计
    uint32_t update_count;          // 更新次数
} pid_runtime_t;

/**
 * @brief PID控制器实例
 */
typedef struct {
    pid_config_t config;            // 配置参数
    pid_coefficients_t coeffs;      // 缩放后的系数
    pid_runtime_t runtime;          // 运行时数据
    float dt;                       // 时间步长（秒）
    bool initialized;               // 是否已初始化
} pid_controller_t;

/**
 * @brief 多轴PID控制器
 */
typedef struct {
    pid_controller_t axis[PID_MAX_AXIS];  // 各轴控制器
    uint8_t axis_count;                   // 轴数量
    float sample_rate_hz;                 // 采样率（Hz）
} pid_multi_axis_t;

/* ============================================================================
 * 函数声明 - 单轴PID控制器
 * ============================================================================ */

/**
 * @brief 初始化PID控制器
 * @param pid PID控制器指针
 * @param config 配置参数
 * @param sample_rate_hz 采样率（Hz）
 */
void pid_init(pid_controller_t *pid, const pid_config_t *config, float sample_rate_hz);

/**
 * @brief 重置PID控制器状态
 * @param pid PID控制器指针
 */
void pid_reset(pid_controller_t *pid);

/**
 * @brief 重置积分项
 * @param pid PID控制器指针
 */
void pid_reset_iterm(pid_controller_t *pid);

/**
 * @brief 更新PID配置
 * @param pid PID控制器指针
 * @param config 新配置参数
 */
void pid_update_config(pid_controller_t *pid, const pid_config_t *config);

/**
 * @brief 更新PID增益
 * @param pid PID控制器指针
 * @param gains 新增益值
 */
void pid_update_gains(pid_controller_t *pid, const pid_gains_t *gains);

/**
 * @brief PID控制器计算（标准形式）
 * @param pid PID控制器指针
 * @param setpoint 设定值
 * @param measurement 测量值
 * @return PID输出
 */
float pid_update(pid_controller_t *pid, float setpoint, float measurement);

/**
 * @brief PID控制器计算（带前馈）
 * @param pid PID控制器指针
 * @param setpoint 设定值
 * @param measurement 测量值
 * @param feedforward 前馈值
 * @return PID输出
 */
float pid_update_with_feedforward(pid_controller_t *pid, float setpoint, float measurement, float feedforward);

/**
 * @brief 获取PID各项值
 * @param pid PID控制器指针
 * @param p_term 输出P项（可为NULL）
 * @param i_term 输出I项（可为NULL）
 * @param d_term 输出D项（可为NULL）
 * @param f_term 输出F项（可为NULL）
 */
void pid_get_terms(const pid_controller_t *pid, float *p_term, float *i_term, float *d_term, float *f_term);

/**
 * @brief 设置积分限幅
 * @param pid PID控制器指针
 * @param limit 积分限幅值
 */
void pid_set_iterm_limit(pid_controller_t *pid, float limit);

/**
 * @brief 设置输出限幅
 * @param pid PID控制器指针
 * @param limit 输出限幅值
 */
void pid_set_output_limit(pid_controller_t *pid, float limit);

/**
 * @brief 获取PID输出
 * @param pid PID控制器指针
 * @return 当前输出值
 */
float pid_get_output(const pid_controller_t *pid);

/**
 * @brief 获取积分项
 * @param pid PID控制器指针
 * @return 当前积分项值
 */
float pid_get_iterm(const pid_controller_t *pid);

/* ============================================================================
 * 函数声明 - 多轴PID控制器
 * ============================================================================ */

/**
 * @brief 初始化多轴PID控制器
 * @param multi_pid 多轴PID控制器指针
 * @param axis_count 轴数量（1-PID_MAX_AXIS）
 * @param sample_rate_hz 采样率（Hz）
 */
void pid_multi_init(pid_multi_axis_t *multi_pid, uint8_t axis_count, float sample_rate_hz);

/**
 * @brief 配置指定轴的PID参数
 * @param multi_pid 多轴PID控制器指针
 * @param axis 轴索引（0开始）
 * @param config 配置参数
 */
void pid_multi_config_axis(pid_multi_axis_t *multi_pid, uint8_t axis, const pid_config_t *config);

/**
 * @brief 重置所有轴
 * @param multi_pid 多轴PID控制器指针
 */
void pid_multi_reset_all(pid_multi_axis_t *multi_pid);

/**
 * @brief 重置指定轴
 * @param multi_pid 多轴PID控制器指针
 * @param axis 轴索引
 */
void pid_multi_reset_axis(pid_multi_axis_t *multi_pid, uint8_t axis);

/**
 * @brief 更新所有轴
 * @param multi_pid 多轴PID控制器指针
 * @param setpoints 设定值数组
 * @param measurements 测量值数组
 * @param outputs 输出数组
 */
void pid_multi_update(pid_multi_axis_t *multi_pid, const float *setpoints, 
                      const float *measurements, float *outputs);

/**
 * @brief 更新指定轴
 * @param multi_pid 多轴PID控制器指针
 * @param axis 轴索引
 * @param setpoint 设定值
 * @param measurement 测量值
 * @return PID输出
 */
float pid_multi_update_axis(pid_multi_axis_t *multi_pid, uint8_t axis, 
                            float setpoint, float measurement);

/* ============================================================================
 * 工具函数
 * ============================================================================ */

/**
 * @brief 将原始增益转换为系数
 * @param gains 原始增益
 * @param coeffs 输出系数
 */
void pid_gains_to_coefficients(const pid_gains_t *gains, pid_coefficients_t *coeffs);

/**
 * @brief 限幅函数
 * @param value 输入值
 * @param limit 限幅值
 * @return 限幅后的值
 */
float pid_constrain(float value, float limit);

/**
 * @brief 限幅函数（带范围）
 * @param value 输入值
 * @param min 最小值
 * @param max 最大值
 * @return 限幅后的值
 */
float pid_constrain_range(float value, float min, float max);

/**
 * @brief 初始化低通滤波器
 * @param lpf 滤波器指针
 * @param cutoff_hz 截止频率（Hz）
 * @param sample_rate_hz 采样率（Hz）
 */
void pid_lpf_init(pid_lpf_t *lpf, float cutoff_hz, float sample_rate_hz);

/**
 * @brief 应用低通滤波
 * @param lpf 滤波器指针
 * @param input 输入值
 * @return 滤波后的值
 */
float pid_lpf_apply(pid_lpf_t *lpf, float input);

/**
 * @brief 重置低通滤波器
 * @param lpf 滤波器指针
 * @param value 初始值
 */
void pid_lpf_reset(pid_lpf_t *lpf, float value);

/* ============================================================================
 * 默认配置
 * ============================================================================ */

/**
 * @brief 获取默认PID配置
 * @param config 配置参数指针
 */
void pid_get_default_config(pid_config_t *config);

/**
 * @brief 获取默认增益（Roll轴）
 * @param gains 增益指针
 */
void pid_get_default_gains_roll(pid_gains_t *gains);

/**
 * @brief 获取默认增益（Pitch轴）
 * @param gains 增益指针
 */
void pid_get_default_gains_pitch(pid_gains_t *gains);

/**
 * @brief 获取默认增益（Yaw轴）
 * @param gains 增益指针
 */
void pid_get_default_gains_yaw(pid_gains_t *gains);

/* ============================================================================
 * 调试和诊断
 * ============================================================================ */

/**
 * @brief 打印PID状态
 * @param pid PID控制器指针
 * @param output_func 输出函数（如printf）
 */
void pid_print_status(const pid_controller_t *pid, void (*output_func)(const char *fmt, ...));

/**
 * @brief 格式化PID状态到字符串
 * @param pid PID控制器指针
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @return 写入的字符数
 */
int pid_format_status(const pid_controller_t *pid, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif // PID_LIB_H
