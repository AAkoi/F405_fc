#include <math.h>
#include <string.h>
#include "task_pid.h"
#include "task_rc.h"
#include "task_gyro.h"

// 轴索引
enum { AXIS_ROLL = 0, AXIS_PITCH = 1, AXIS_YAW = 2 };

static pid_controller_t pid_angle[2]; // roll, pitch
static pid_controller_t pid_rate[3];  // roll, pitch, yaw
static pid_output_t pid_out;

static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static void set_pid_dt(pid_controller_t *pid, float dt)
{
    if (pid) pid->dt = dt;
}

void task_pid_init(float control_rate_hz)
{
    // 使用默认配置并调整限幅/滤波
    pid_config_t cfg_angle;
    pid_config_t cfg_rate;
    pid_gains_t g_roll, g_pitch, g_yaw;

    pid_get_default_config(&cfg_angle);
    pid_get_default_config(&cfg_rate);
    pid_get_default_gains_roll(&g_roll);
    pid_get_default_gains_pitch(&g_pitch);
    pid_get_default_gains_yaw(&g_yaw);

    // 角度环：输出期望角速度，输出限幅适中
    cfg_angle.output_limit = 400.0f; // dps setpoint 最大幅度
    cfg_angle.iterm_limit = 200.0f;
    cfg_angle.enable_dterm_filter = false;
    cfg_angle.enable_feedforward = false;

    // 速率环：直接驱动力矩，限制在较小范围便于归一化混控
    cfg_rate.output_limit = 1.0f;
    cfg_rate.iterm_limit = 0.5f;
    cfg_rate.enable_dterm_filter = false;
    cfg_rate.enable_feedforward = false;

    pid_init(&pid_angle[AXIS_ROLL],  &cfg_angle, control_rate_hz);
    pid_init(&pid_angle[AXIS_PITCH], &cfg_angle, control_rate_hz);

    pid_update_gains(&pid_angle[AXIS_ROLL],  &g_roll);
    pid_update_gains(&pid_angle[AXIS_PITCH], &g_pitch);

    pid_init(&pid_rate[AXIS_ROLL],  &cfg_rate, control_rate_hz);
    pid_init(&pid_rate[AXIS_PITCH], &cfg_rate, control_rate_hz);
    pid_init(&pid_rate[AXIS_YAW],   &cfg_rate, control_rate_hz);

    pid_update_gains(&pid_rate[AXIS_ROLL],  &g_roll);
    pid_update_gains(&pid_rate[AXIS_PITCH], &g_pitch);
    pid_update_gains(&pid_rate[AXIS_YAW],   &g_yaw);

    memset(&pid_out, 0, sizeof(pid_out));
}

const pid_output_t *task_pid_step(float dt, float max_rate_dps)
{
    // 清零输出
    memset(&pid_out, 0, sizeof(pid_out));

    // 更新 PID 内部 dt
    set_pid_dt(&pid_angle[AXIS_ROLL], dt);
    set_pid_dt(&pid_angle[AXIS_PITCH], dt);
    for (int i = 0; i < 3; i++) set_pid_dt(&pid_rate[i], dt);

    // 取 RC 期望
    const rc_command_t *rc = rc_get_command();
    if (!rc || !rc->link_active) {
        return &pid_out;
    }
    pid_out.link_active = true;

    // 姿态反馈（deg）
    Euler_angles ang = Attitude_Get_Angles();
    pid_out.angle_meas[0] = ang.roll;
    pid_out.angle_meas[1] = ang.pitch;
    pid_out.angle_meas[2] = ang.yaw;

    // 角度期望（deg），yaw 此处也走角度环，保持简单
    pid_out.angle_sp[0] = rc->roll_deg;
    pid_out.angle_sp[1] = rc->pitch_deg;
    pid_out.angle_sp[2] = rc->yaw_deg;

    // 角度环输出期望角速度（dps）
    float sp_rate_roll  = pid_update(&pid_angle[AXIS_ROLL],  pid_out.angle_sp[0], pid_out.angle_meas[0]);
    float sp_rate_pitch = pid_update(&pid_angle[AXIS_PITCH], pid_out.angle_sp[1], pid_out.angle_meas[1]);
    // Yaw 简化为速率模式，直接使用期望角度差的比例映射
    float sp_rate_yaw   = pid_out.angle_sp[2] - pid_out.angle_meas[2];

    sp_rate_roll  = clampf(sp_rate_roll,  -max_rate_dps, max_rate_dps);
    sp_rate_pitch = clampf(sp_rate_pitch, -max_rate_dps, max_rate_dps);
    sp_rate_yaw   = clampf(sp_rate_yaw,   -max_rate_dps, max_rate_dps);

    pid_out.rate_sp[0] = sp_rate_roll;
    pid_out.rate_sp[1] = sp_rate_pitch;
    pid_out.rate_sp[2] = sp_rate_yaw;

    // 角速度反馈（dps）
    pid_out.rate_meas[0] = gyro_scaled.dps_x;
    pid_out.rate_meas[1] = gyro_scaled.dps_y;
    pid_out.rate_meas[2] = gyro_scaled.dps_z;

    // 速率环输出力矩指令
    float u_roll  = pid_update(&pid_rate[AXIS_ROLL],  pid_out.rate_sp[0], pid_out.rate_meas[0]);
    float u_pitch = pid_update(&pid_rate[AXIS_PITCH], pid_out.rate_sp[1], pid_out.rate_meas[1]);
    float u_yaw   = pid_update(&pid_rate[AXIS_YAW],   pid_out.rate_sp[2], pid_out.rate_meas[2]);

    // 混控（X 架示例），输出归一化 0..1
    float base_thr = clampf(rc->throttle, 0.0f, 1.0f);
    float m1 = base_thr + u_roll + u_pitch - u_yaw;
    float m2 = base_thr - u_roll + u_pitch + u_yaw;
    float m3 = base_thr - u_roll - u_pitch - u_yaw;
    float m4 = base_thr + u_roll - u_pitch + u_yaw;

    pid_out.motor[0] = clampf(m1, 0.0f, 1.0f);
    pid_out.motor[1] = clampf(m2, 0.0f, 1.0f);
    pid_out.motor[2] = clampf(m3, 0.0f, 1.0f);
    pid_out.motor[3] = clampf(m4, 0.0f, 1.0f);

    return &pid_out;
}
