/**
 * @file    task_mixer.h
 * @brief   电机混控任务（Quad X）
 */

#ifndef TASK_MIXER_H
#define TASK_MIXER_H

#include <stdint.h>

#define MIXER_MOTOR_COUNT 4

/**
 * @brief Quad X 架构混控
 * @param base_throttle 基础油门（0..1）
 * @param roll 归一化横滚力矩指令
 * @param pitch 归一化俯仰力矩指令
 * @param yaw 归一化偏航力矩指令
 * @param motor_out 输出数组，长度为4，范围0..1
 */
void mixer_mix_quad_x(float base_throttle, float roll, float pitch, float yaw,
                      float motor_out[MIXER_MOTOR_COUNT]);

#endif // TASK_MIXER_H
