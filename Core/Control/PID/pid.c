/**
 * @file    pid_lib.c
 * @brief   通用PID控制器库实现
 * @author  Extracted from Betaflight
 * @date    2025
 */

#include "pid.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 计算低通滤波器系数
 */
static float calculate_lpf_coefficient(float cutoff_hz, float sample_rate_hz)
{
    if (cutoff_hz <= 0.0f || sample_rate_hz <= 0.0f) {
        return 1.0f;  // 无滤波
    }
    
    const float rc = 1.0f / (2.0f * 3.14159265f * cutoff_hz);
    const float dt = 1.0f / sample_rate_hz;
    return dt / (rc + dt);
}

/**
 * @brief 更新PID系数
 */
static void update_pid_coefficients(pid_controller_t *pid)
{
    pid_gains_to_coefficients(&pid->config.gains, &pid->coeffs);
}

/* ============================================================================
 * 工具函数实现
 * ============================================================================ */

void pid_gains_to_coefficients(const pid_gains_t *gains, pid_coefficients_t *coeffs)
{
    coeffs->Kp = gains->P * PID_PTERM_SCALE;
    coeffs->Ki = gains->I * PID_ITERM_SCALE;
    coeffs->Kd = gains->D * PID_DTERM_SCALE;
    coeffs->Kf = gains->F * PID_FEEDFORWARD_SCALE;
}

float pid_constrain(float value, float limit)
{
    if (value > limit) {
        return limit;
    } else if (value < -limit) {
        return -limit;
    }
    return value;
}

float pid_constrain_range(float value, float min, float max)
{
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    }
    return value;
}

void pid_lpf_init(pid_lpf_t *lpf, float cutoff_hz, float sample_rate_hz)
{
    lpf->state = 0.0f;
    lpf->k = calculate_lpf_coefficient(cutoff_hz, sample_rate_hz);
}

float pid_lpf_apply(pid_lpf_t *lpf, float input)
{
    lpf->state = lpf->state + lpf->k * (input - lpf->state);
    return lpf->state;
}

void pid_lpf_reset(pid_lpf_t *lpf, float value)
{
    lpf->state = value;
}

/* ============================================================================
 * 单轴PID控制器实现
 * ============================================================================ */

void pid_init(pid_controller_t *pid, const pid_config_t *config, float sample_rate_hz)
{
    // 清零所有数据
    memset(pid, 0, sizeof(pid_controller_t));
    
    // 保存配置
    memcpy(&pid->config, config, sizeof(pid_config_t));
    
    // 计算时间步长
    pid->dt = 1.0f / sample_rate_hz;
    
    // 更新系数
    update_pid_coefficients(pid);
    
    // 初始化D项滤波器
    if (config->enable_dterm_filter && config->dterm_lpf_hz > 0.0f) {
        pid_lpf_init(&pid->runtime.dterm_filter, config->dterm_lpf_hz, sample_rate_hz);
    }
    
    pid->initialized = true;
}

void pid_reset(pid_controller_t *pid)
{
    if (!pid->initialized) {
        return;
    }
    
    // 重置运行时数据
    pid->runtime.P = 0.0f;
    pid->runtime.I = 0.0f;
    pid->runtime.D = 0.0f;
    pid->runtime.F = 0.0f;
    pid->runtime.output = 0.0f;
    pid->runtime.previous_error = 0.0f;
    pid->runtime.previous_measurement = 0.0f;
    pid->runtime.previous_setpoint = 0.0f;
    pid->runtime.update_count = 0;
    
    // 重置滤波器
    pid_lpf_reset(&pid->runtime.dterm_filter, 0.0f);
}

void pid_reset_iterm(pid_controller_t *pid)
{
    if (pid->initialized) {
        pid->runtime.I = 0.0f;
    }
}

void pid_update_config(pid_controller_t *pid, const pid_config_t *config)
{
    if (!pid->initialized) {
        return;
    }
    
    // 保存旧的积分项
    float old_iterm = pid->runtime.I;
    
    // 更新配置
    memcpy(&pid->config, config, sizeof(pid_config_t));
    
    // 更新系数
    update_pid_coefficients(pid);
    
    // 重新初始化D项滤波器
    if (config->enable_dterm_filter && config->dterm_lpf_hz > 0.0f) {
        float sample_rate_hz = 1.0f / pid->dt;
        pid_lpf_init(&pid->runtime.dterm_filter, config->dterm_lpf_hz, sample_rate_hz);
    }
    
    // 恢复积分项
    pid->runtime.I = old_iterm;
}

void pid_update_gains(pid_controller_t *pid, const pid_gains_t *gains)
{
    if (!pid->initialized) {
        return;
    }
    
    memcpy(&pid->config.gains, gains, sizeof(pid_gains_t));
    update_pid_coefficients(pid);
}

float pid_update(pid_controller_t *pid, float setpoint, float measurement)
{
    return pid_update_with_feedforward(pid, setpoint, measurement, 0.0f);
}

float pid_update_with_feedforward(pid_controller_t *pid, float setpoint, float measurement, float feedforward)
{
    if (!pid->initialized) {
        return 0.0f;
    }
    
    // 计算误差
    float error = setpoint - measurement;
    
    // ========== P项 ==========
    pid->runtime.P = pid->coeffs.Kp * error;
    
    // ========== I项 ==========
    // 积分累加
    pid->runtime.I += pid->coeffs.Ki * error * pid->dt;
    
    // 积分限幅（Anti-windup）
    if (pid->config.iterm_limit > 0.0f) {
        pid->runtime.I = pid_constrain(pid->runtime.I, pid->config.iterm_limit);
    }
    
    // 积分抗饱和（基于输出限幅）
    if (pid->config.iterm_windup > 0 && pid->config.output_limit > 0.0f) {
        float windup_threshold = pid->config.output_limit * (pid->config.iterm_windup / 100.0f);
        float pid_sum_without_i = pid->runtime.P + pid->runtime.D + pid->runtime.F;
        
        if (fabsf(pid_sum_without_i) > windup_threshold) {
            // 接近输出限幅时，停止积分累加
            pid->runtime.I -= pid->coeffs.Ki * error * pid->dt;
        }
    }
    
    // ========== D项 ==========
    // 使用测量值微分（避免设定值突变导致的微分冲击）
    float measurement_derivative = (measurement - pid->runtime.previous_measurement) / pid->dt;
    float d_term_raw = -pid->coeffs.Kd * measurement_derivative;  // 负号因为是测量值微分
    
    // D项滤波
    if (pid->config.enable_dterm_filter && pid->config.dterm_lpf_hz > 0.0f) {
        pid->runtime.D = pid_lpf_apply(&pid->runtime.dterm_filter, d_term_raw);
    } else {
        pid->runtime.D = d_term_raw;
    }
    
    // ========== F项（前馈）==========
    if (pid->config.enable_feedforward) {
        // 可以使用外部提供的前馈值，或基于设定值变化率
        if (feedforward != 0.0f) {
            pid->runtime.F = pid->coeffs.Kf * feedforward;
        } else {
            // 基于设定值变化率的前馈
            float setpoint_derivative = (setpoint - pid->runtime.previous_setpoint) / pid->dt;
            pid->runtime.F = pid->coeffs.Kf * setpoint_derivative;
        }
    } else {
        pid->runtime.F = 0.0f;
    }
    
    // ========== 总输出 ==========
    pid->runtime.output = pid->runtime.P + pid->runtime.I + pid->runtime.D + pid->runtime.F;
    
    // 输出限幅
    if (pid->config.output_limit > 0.0f) {
        pid->runtime.output = pid_constrain(pid->runtime.output, pid->config.output_limit);
    }
    
    // 保存状态
    pid->runtime.previous_error = error;
    pid->runtime.previous_measurement = measurement;
    pid->runtime.previous_setpoint = setpoint;
    pid->runtime.update_count++;
    
    return pid->runtime.output;
}

void pid_get_terms(const pid_controller_t *pid, float *p_term, float *i_term, float *d_term, float *f_term)
{
    if (!pid->initialized) {
        return;
    }
    
    if (p_term) *p_term = pid->runtime.P;
    if (i_term) *i_term = pid->runtime.I;
    if (d_term) *d_term = pid->runtime.D;
    if (f_term) *f_term = pid->runtime.F;
}

void pid_set_iterm_limit(pid_controller_t *pid, float limit)
{
    if (pid->initialized) {
        pid->config.iterm_limit = limit;
    }
}

void pid_set_output_limit(pid_controller_t *pid, float limit)
{
    if (pid->initialized) {
        pid->config.output_limit = limit;
    }
}

float pid_get_output(const pid_controller_t *pid)
{
    return pid->initialized ? pid->runtime.output : 0.0f;
}

float pid_get_iterm(const pid_controller_t *pid)
{
    return pid->initialized ? pid->runtime.I : 0.0f;
}

/* ============================================================================
 * 多轴PID控制器实现
 * ============================================================================ */

void pid_multi_init(pid_multi_axis_t *multi_pid, uint8_t axis_count, float sample_rate_hz)
{
    if (axis_count > PID_MAX_AXIS) {
        axis_count = PID_MAX_AXIS;
    }
    
    memset(multi_pid, 0, sizeof(pid_multi_axis_t));
    multi_pid->axis_count = axis_count;
    multi_pid->sample_rate_hz = sample_rate_hz;
    
    // 使用默认配置初始化所有轴
    pid_config_t default_config;
    pid_get_default_config(&default_config);
    
    for (uint8_t i = 0; i < axis_count; i++) {
        pid_init(&multi_pid->axis[i], &default_config, sample_rate_hz);
    }
}

void pid_multi_config_axis(pid_multi_axis_t *multi_pid, uint8_t axis, const pid_config_t *config)
{
    if (axis < multi_pid->axis_count) {
        pid_update_config(&multi_pid->axis[axis], config);
    }
}

void pid_multi_reset_all(pid_multi_axis_t *multi_pid)
{
    for (uint8_t i = 0; i < multi_pid->axis_count; i++) {
        pid_reset(&multi_pid->axis[i]);
    }
}

void pid_multi_reset_axis(pid_multi_axis_t *multi_pid, uint8_t axis)
{
    if (axis < multi_pid->axis_count) {
        pid_reset(&multi_pid->axis[axis]);
    }
}

void pid_multi_update(pid_multi_axis_t *multi_pid, const float *setpoints, 
                      const float *measurements, float *outputs)
{
    for (uint8_t i = 0; i < multi_pid->axis_count; i++) {
        outputs[i] = pid_update(&multi_pid->axis[i], setpoints[i], measurements[i]);
    }
}

float pid_multi_update_axis(pid_multi_axis_t *multi_pid, uint8_t axis, 
                            float setpoint, float measurement)
{
    if (axis < multi_pid->axis_count) {
        return pid_update(&multi_pid->axis[axis], setpoint, measurement);
    }
    return 0.0f;
}

/* ============================================================================
 * 默认配置
 * ============================================================================ */

void pid_get_default_config(pid_config_t *config)
{
    config->gains.P = PID_P_DEFAULT;
    config->gains.I = PID_I_DEFAULT;
    config->gains.D = PID_D_DEFAULT;
    config->gains.F = PID_F_DEFAULT;
    config->output_limit = PID_OUTPUT_LIMIT_DEFAULT;
    config->iterm_limit = PID_ITERM_LIMIT_DEFAULT;
    config->iterm_windup = PID_ITERM_WINDUP_DEFAULT;
    config->dterm_lpf_hz = 100.0f;  // 100Hz低通滤波
    config->enable_feedforward = true;
    config->enable_dterm_filter = true;
}

void pid_get_default_gains_roll(pid_gains_t *gains)
{
    gains->P = 45;
    gains->I = 80;
    gains->D = 30;
    gains->F = 120;
}

void pid_get_default_gains_pitch(pid_gains_t *gains)
{
    gains->P = 47;
    gains->I = 84;
    gains->D = 34;
    gains->F = 125;
}

void pid_get_default_gains_yaw(pid_gains_t *gains)
{
    gains->P = 45;
    gains->I = 80;
    gains->D = 0;
    gains->F = 120;
}

/* ============================================================================
 * 调试和诊断
 * ============================================================================ */

void pid_print_status(const pid_controller_t *pid, void (*output_func)(const char *fmt, ...))
{
    if (!pid->initialized || !output_func) {
        return;
    }
    
    output_func("PID Status:\n");
    output_func("  Gains: P=%d I=%d D=%d F=%d\n", 
                pid->config.gains.P, pid->config.gains.I, 
                pid->config.gains.D, pid->config.gains.F);
    output_func("  Coefficients: Kp=%.6f Ki=%.6f Kd=%.6f Kf=%.6f\n",
                pid->coeffs.Kp, pid->coeffs.Ki, pid->coeffs.Kd, pid->coeffs.Kf);
    output_func("  Terms: P=%.3f I=%.3f D=%.3f F=%.3f\n",
                pid->runtime.P, pid->runtime.I, pid->runtime.D, pid->runtime.F);
    output_func("  Output: %.3f\n", pid->runtime.output);
    output_func("  Limits: Output=%.1f ITerm=%.1f\n",
                pid->config.output_limit, pid->config.iterm_limit);
    output_func("  Updates: %u\n", pid->runtime.update_count);
}

int pid_format_status(const pid_controller_t *pid, char *buffer, size_t size)
{
    if (!pid->initialized || !buffer || size == 0) {
        return 0;
    }
    
    return snprintf(buffer, size,
        "PID: P=%.3f I=%.3f D=%.3f F=%.3f Out=%.3f [Gains: %d/%d/%d/%d]",
        pid->runtime.P, pid->runtime.I, pid->runtime.D, pid->runtime.F,
        pid->runtime.output,
        pid->config.gains.P, pid->config.gains.I, 
        pid->config.gains.D, pid->config.gains.F);
}
