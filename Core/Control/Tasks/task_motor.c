#include "task_motor.h"
#include "bsp_pwm.h"

void task_motor_init(uint32_t pwm_hz)
{
    (void)bsp_pwm_init(pwm_hz);
}

void task_motor_update(const float motor[MIXER_MOTOR_COUNT])
{
    if (!motor || !bsp_pwm_ready()) {
        return;
    }
    bsp_pwm_write_all(motor);
}

void task_motor_stop(void)
{
    if (!bsp_pwm_ready()) {
        return;
    }
    bsp_pwm_stop_all();
}

bool task_motor_ready(void)
{
    return bsp_pwm_ready();
}
