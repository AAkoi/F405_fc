/**
 * @file    bsp_pwm.h
 * @brief   通用 PWM 底层驱动（TIM3/TIM8），供上层任务使用
 */

#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdbool.h>
#include <stdint.h>

#define BSP_PWM_DEFAULT_HZ 20000U

typedef enum {
    BSP_PWM_CH1 = 0, // PC6  TIM8_CH1
    BSP_PWM_CH2,     // PC7  TIM8_CH2
    BSP_PWM_CH3,     // PC8  TIM3_CH3
    BSP_PWM_CH4,     // PB1  TIM3_CH4
    BSP_PWM_CH_MAX
} bsp_pwm_channel_t;

/**
 * @brief 初始化 PWM 输出
 * @param pwm_hz 目标 PWM 频率，0 使用默认 20kHz
 * @return true 成功，false 失败
 */
bool bsp_pwm_init(uint32_t pwm_hz);

/**
 * @brief 写入单路占空比
 * @param ch 通道
 * @param duty 占空比 0..1
 */
void bsp_pwm_write(bsp_pwm_channel_t ch, float duty);

/**
 * @brief 批量写入，占空比数组长度至少 BSP_PWM_CH_MAX
 */
void bsp_pwm_write_all(const float duty[BSP_PWM_CH_MAX]);

/**
 * @brief 输出是否已就绪
 */
bool bsp_pwm_ready(void);

/**
 * @brief 全部停止（占空比清零）
 */
void bsp_pwm_stop_all(void);

#endif // BSP_PWM_H
