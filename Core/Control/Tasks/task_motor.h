/**
 * @file    task_motor.h
 * @brief   电机输出任务：将混控结果送到底层 PWM 驱动
 */

#ifndef TASK_MOTOR_H
#define TASK_MOTOR_H

#include <stdbool.h>
#include <stdint.h>
#include "task_mixer.h"
#include "bsp_pwm.h"

/**
 * @brief 初始化电机输出（内部调用 BSP PWM）
 * @param pwm_hz PWM 频率，0 则使用默认值
 */
void task_motor_init(uint32_t pwm_hz);

/**
 * @brief 更新电机占空比（0..1）
 * @param motor 长度 MIXER_MOTOR_COUNT
 */
void task_motor_update(const float motor[MIXER_MOTOR_COUNT]);

/**
 * @brief 停止所有电机输出
 */
void task_motor_stop(void);

/**
 * @brief 查询电机输出是否就绪
 */
bool task_motor_ready(void);

#endif // TASK_MOTOR_H
