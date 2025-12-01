#include "test_pwm_servo.h"
#include "bsp_pwm.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

// 舵机典型 50Hz，1~2ms 脉宽；占空比=脉宽/周期。
#define SERVO_PWM_HZ       50U
#define SERVO_PULSE_MIN_MS 1.0f
#define SERVO_PULSE_MID_MS 1.5f
#define SERVO_PULSE_MAX_MS 2.0f
#define SERVO_SWEEP_STEPS  20U

static float pulse_ms_to_duty(float pulse_ms)
{
    const float period_ms = 1000.0f / (float)SERVO_PWM_HZ; // e.g. 20ms
    float duty = pulse_ms / period_ms;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
    return duty;
}

void test_pwm_servo_run(void)
{
    printf("[servo_test] init PWM %u Hz\r\n", SERVO_PWM_HZ);
    if (!bsp_pwm_init(SERVO_PWM_HZ)) {
        printf("[servo_test] init failed\r\n");
        return;
    }

    const float duty_min = pulse_ms_to_duty(SERVO_PULSE_MIN_MS);
    const float duty_mid = pulse_ms_to_duty(SERVO_PULSE_MID_MS);
    const float duty_max = pulse_ms_to_duty(SERVO_PULSE_MAX_MS);

    float duty[BSP_PWM_CH_MAX];
    for (uint8_t i = 0; i < BSP_PWM_CH_MAX; i++) {
        duty[i] = duty_mid;
    }
    bsp_pwm_write_all(duty);
    printf("[servo_test] center duty=%.3f\r\n", duty_mid);
    HAL_Delay(1000);

    // 往返扫动
    while (1) {
        for (uint8_t step = 0; step <= SERVO_SWEEP_STEPS; step++) {
            float t = (float)step / (float)SERVO_SWEEP_STEPS;
            float d = duty_min + (duty_max - duty_min) * t;
            for (uint8_t i = 0; i < BSP_PWM_CH_MAX; i++) duty[i] = d;
            bsp_pwm_write_all(duty);
            HAL_Delay(200);
        }
        for (int step = (int)SERVO_SWEEP_STEPS; step >= 0; step--) {
            float t = (float)step / (float)SERVO_SWEEP_STEPS;
            float d = duty_min + (duty_max - duty_min) * t;
            for (uint8_t i = 0; i < BSP_PWM_CH_MAX; i++) duty[i] = d;
            bsp_pwm_write_all(duty);
            HAL_Delay(200);
        }
    }
}
