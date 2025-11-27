#ifndef TASK_PID_H
#define TASK_PID_H

#include <stdbool.h>
#include <stdint.h>
#include "attitude.h"
#include "task_rc.h"
#include "pid.h"

typedef struct {
    float motor[4];      // 归一化电机输出 0..1
    float rate_sp[3];    // 期望角速度 dps
    float rate_meas[3];  // 实际角速度 dps
    float angle_sp[3];   // 期望角度 deg
    float angle_meas[3]; // 实际角度 deg
    bool link_active;
} pid_output_t;

void task_pid_init(float control_rate_hz);
const pid_output_t *task_pid_step(float dt, float max_rate_dps);

#endif // TASK_PID_H
