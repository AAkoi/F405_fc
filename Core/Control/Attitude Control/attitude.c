/*
 * 姿态解算（Mahony 互补滤波 + 磁力计融合）
 * 本模块只负责姿态融合算法，不涉及传感器数据读取
 */
#include "maths.h"
#include "stm32f4xx_hal.h"
#include "attitude.h"

Euler_angles euler_angles;
Quaternion attitude_q;

// Mahony 标准 9DoF：无需附加限幅或倾角/旋转屏蔽，仅做归一化
#define MAG_FIELD_MIN_GAUSS      0.05f   // 防止零向量
#define ACC_FIELD_MIN_G          0.05f

// Mahony算法的积分项（误差积分）
static float exInt = 0.0f, eyInt = 0.0f, ezInt = 0.0f;
static uint32_t lastTick = 0;

// Mahony标准增益（9DoF）：twoKp=2*Kp, twoKi=2*Ki
static const float twoKp = 2.0f * 4.0f;   // 比例增益 Kp=0.5
static const float twoKi = 2.0f * 0.01f;   // 积分增益（默认关闭，可按需开启）


// 四元数归一化： q = q / |q|
static void quat_normalize(Quaternion *q)
{
    float n2 = q->p0*q->p0 + q->p1*q->p1 + q->p2*q->p2 + q->p3*q->p3;
    if (n2 > 0.0f) {
        float inv = fast_inv_sqrt(n2);
        q->p0 *= inv; q->p1 *= inv; q->p2 *= inv; q->p3 *= inv;
    } else {
        q->p0 = 1.0f; q->p1 = q->p2 = q->p3 = 0.0f;
    }
}

// 运行诊断
static AttitudeDiagnostics attitude_diag = {0};

void Attitude_Init(void)
{
    euler_angles.pitch = 0.0f;
    euler_angles.roll  = 0.0f;
    euler_angles.yaw   = 0.0f;

    attitude_q.p0 = 1.0f;
    attitude_q.p1 = 0.0f;
    attitude_q.p2 = 0.0f;
    attitude_q.p3 = 0.0f;

    exInt = eyInt = ezInt = 0.0f;
    lastTick = HAL_GetTick();
    attitude_diag = (AttitudeDiagnostics){0};
}

void Attitude_InitFromAccelerometer(float ax, float ay, float az)
{
    float n2 = ax*ax + ay*ay + az*az;
    if (n2 > 1e-6f) {
        float inv = fast_inv_sqrt(n2);
        ax *= inv; ay *= inv; az *= inv;
    }

    float denom = sqrtf(ay*ay + az*az);
    float roll, pitch;
    if (denom < 1e-6f) {
        roll = 0.0f;
        pitch = (ax < 0.0f) ? 1.57079633f : -1.57079633f; // ±pi/2
    } else {
        roll  = atan2_approx(ay, az);
        pitch = atan2_approx(-ax, denom);
    }
    float yaw   = 0.0f;

    float cr = cos_approx(roll * 0.5f),  sr = sin_approx(roll * 0.5f);
    float cp = cos_approx(pitch * 0.5f), sp = sin_approx(pitch * 0.5f);
    float cy = cos_approx(yaw * 0.5f),   sy = sin_approx(yaw * 0.5f);

    attitude_q.p0 = cy*cp*cr + sy*sp*sr;
    attitude_q.p1 = cy*cp*sr - sy*sp*cr;
    attitude_q.p2 = cy*sp*cr + sy*cp*sr;
    attitude_q.p3 = sy*cp*cr - cy*sp*sr;
    quat_normalize(&attitude_q);

    exInt = eyInt = ezInt = 0.0f;
    lastTick = HAL_GetTick();
    attitude_diag = (AttitudeDiagnostics){0};
}

#if USE_MAGNETOMETER
/**
 * @brief 使用加速度计和磁力计初始化姿态（推荐用于reset）
 * @note  Roll/Pitch从加速度计计算，Yaw从磁力计计算，立即得到正确姿态
 */
void Attitude_InitFromAccelMag(float ax, float ay, float az,
                               float mx_gauss, float my_gauss, float mz_gauss)
{
    float acc_norm = sqrtf(ax*ax + ay*ay + az*az);
    if (acc_norm < 1e-6f) acc_norm = 1e-6f;
    ax /= acc_norm; ay /= acc_norm; az /= acc_norm;

    float denom = sqrtf(ay*ay + az*az);
    float roll, pitch;
    if (denom < 1e-6f) {
        roll = 0.0f;
        pitch = (ax < 0.0f) ? 1.57079633f : -1.57079633f;
    } else {
        roll  = atan2_approx(ay, az);
        pitch = atan2_approx(-ax, denom);
    }

    float mag_norm2 = mx_gauss*mx_gauss + my_gauss*my_gauss + mz_gauss*mz_gauss;
    if (mag_norm2 < 1e-6f) {
        Attitude_InitFromAccelerometer(ax, ay, az);
        return;
    }
    float inv_mag = fast_inv_sqrt(mag_norm2);
    float mx_unit = mx_gauss * inv_mag;
    float my_unit = my_gauss * inv_mag;
    float mz_unit = mz_gauss * inv_mag;

    float cos_roll  = cos_approx(roll);
    float sin_roll  = sin_approx(roll);
    float cos_pitch = cos_approx(pitch);
    float sin_pitch = sin_approx(pitch);

    float mx_h = mx_unit * cos_pitch + my_unit * sin_roll * sin_pitch + mz_unit * cos_roll * sin_pitch;
    float my_h = my_unit * cos_roll - mz_unit * sin_roll;

    float yaw = atan2_approx(-my_h, mx_h);

    float cr = cos_approx(roll * 0.5f),  sr = sin_approx(roll * 0.5f);
    float cp = cos_approx(pitch * 0.5f), sp = sin_approx(pitch * 0.5f);
    float cy = cos_approx(yaw * 0.5f),   sy = sin_approx(yaw * 0.5f);

    attitude_q.p0 = cy*cp*cr + sy*sp*sr;
    attitude_q.p1 = cy*cp*sr - sy*sp*cr;
    attitude_q.p2 = cy*sp*cr + sy*cp*sr;
    attitude_q.p3 = sy*cp*cr - cy*sp*sr;
    quat_normalize(&attitude_q);

    exInt = eyInt = ezInt = 0.0f;
    lastTick = HAL_GetTick();
    attitude_diag = (AttitudeDiagnostics){0};

    euler_angles.roll  = roll  * RAD2DEG;
    euler_angles.pitch = pitch * RAD2DEG;
    euler_angles.yaw   = yaw   * RAD2DEG;
}
#endif

#if USE_MAGNETOMETER
static Euler_angles Attitude_Update_Internal(float ax_g, float ay_g, float az_g,
                                             float gx_dps, float gy_dps, float gz_dps,
                                             float mx_gauss, float my_gauss, float mz_gauss, 
                                             bool use_mag)
#else
Euler_angles Attitude_Update(float ax_g, float ay_g, float az_g,
                             float gx_dps, float gy_dps, float gz_dps)
#endif
{
    const uint32_t cycle_start = DWT->CYCCNT;

    uint32_t now = HAL_GetTick();
    float dt = (now - lastTick) * 0.001f;
    lastTick = now;
    if (dt < 1e-4f) dt = 1e-4f;
    if (dt > 0.05f) dt = 0.05f;

    float spin_rate_dps = sqrtf(gx_dps*gx_dps + gy_dps*gy_dps + gz_dps*gz_dps);
    float gx = gx_dps * DEG2RAD;
    float gy = gy_dps * DEG2RAD;
    float gz = gz_dps * DEG2RAD;

    attitude_diag.mag_used = false;
    attitude_diag.mag_strength_ok = false;

    float acc_norm = sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
    if (acc_norm < ACC_FIELD_MIN_G) acc_norm = ACC_FIELD_MIN_G;
    ax_g /= acc_norm; ay_g /= acc_norm; az_g /= acc_norm;
    bool acc_valid = true;

    float qw = attitude_q.p0, qx = attitude_q.p1, qy = attitude_q.p2, qz = attitude_q.p3;
    float vx = 2.0f * (qx*qz - qw*qy);
    float vy = 2.0f * (qw*qx + qy*qz);
    float vz = qw*qw - qx*qx - qy*qy + qz*qz;

    float ex = 0.0f, ey = 0.0f, ez = 0.0f;

    if (acc_valid) {
        ex = (ay_g * vz - az_g * vy);
        ey = (az_g * vx - ax_g * vz);
        ez = (ax_g * vy - ay_g * vx);
    }

#if USE_MAGNETOMETER
    if (use_mag) {
        float mag_norm = sqrtf(mx_gauss*mx_gauss + my_gauss*my_gauss + mz_gauss*mz_gauss);
        const bool mag_strength_ok = mag_norm >= MAG_FIELD_MIN_GAUSS;
        if (!mag_strength_ok) {
            mag_norm = MAG_FIELD_MIN_GAUSS;
        }
        mx_gauss /= mag_norm; my_gauss /= mag_norm; mz_gauss /= mag_norm;

        float hx = 2.0f * (mx_gauss * (0.5f - qy*qy - qz*qz) + my_gauss * (qx*qy - qw*qz) + mz_gauss * (qx*qz + qw*qy));
        float hy = 2.0f * (mx_gauss * (qx*qy + qw*qz) + my_gauss * (0.5f - qx*qx - qz*qz) + mz_gauss * (qy*qz - qw*qx));
        float hz = 2.0f * (mx_gauss * (qx*qz - qw*qy) + my_gauss * (qy*qz + qw*qx) + mz_gauss * (0.5f - qx*qx - qy*qy));
        float bx = sqrtf(hx*hx + hy*hy);
        float bz = hz;

        float wx = 2.0f * (bx * (0.5f - qy*qy - qz*qz) + bz * (qx*qz - qw*qy));
        float wy = 2.0f * (bx * (qx*qy - qw*qz) + bz * (qw*qx + qy*qz));
        float wz = 2.0f * (bx * (qw*qy + qx*qz) + bz * (0.5f - qx*qx - qy*qy));

        float ex_mag = (my_gauss * wz - mz_gauss * wy);
        float ey_mag = (mz_gauss * wx - mx_gauss * wz);
        float ez_mag = (mx_gauss * wy - my_gauss * wx);

        ex += ex_mag;
        ey += ey_mag;
        ez += ez_mag;
        attitude_diag.mag_used = true;
        attitude_diag.mag_strength_ok = mag_strength_ok;
    }
#endif

    exInt += twoKi * ex * dt;
    eyInt += twoKi * ey * dt;
    ezInt += twoKi * ez * dt;

    gx += twoKp * ex + exInt;
    gy += twoKp * ey + eyInt;
    gz += twoKp * ez + ezInt;

    float qw_dot = 0.5f * (-qx*gx - qy*gy - qz*gz);
    float qx_dot = 0.5f * ( qw*gx + qy*gz - qz*gy);
    float qy_dot = 0.5f * ( qw*gy - qx*gz + qz*gx);
    float qz_dot = 0.5f * ( qw*gz + qx*gy - qy*gx);

    attitude_q.p0 += qw_dot * dt;
    attitude_q.p1 += qx_dot * dt;
    attitude_q.p2 += qy_dot * dt;
    attitude_q.p3 += qz_dot * dt;
    quat_normalize(&attitude_q);

    qw = attitude_q.p0; qx = attitude_q.p1; qy = attitude_q.p2; qz = attitude_q.p3;
    float sinr_cosp = 2.0f * (qw*qx + qy*qz);
    float cosr_cosp = 1.0f - 2.0f * (qx*qx + qy*qy);
    float roll = atan2_approx(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (qw*qy - qz*qx);
    float pitch = (fabsf(sinp) >= 1.0f) ? copysignf(0.5f * M_PIf, sinp) : asinf(sinp);

    float siny_cosp = 2.0f * (qw*qz + qx*qy);
    float cosy_cosp = 1.0f - 2.0f * (qy*qy + qz*qz);
    float yaw = atan2_approx(siny_cosp, cosy_cosp);

    euler_angles.roll  = roll  * RAD2DEG;
    euler_angles.pitch = pitch * RAD2DEG;
    euler_angles.yaw   = yaw   * RAD2DEG;

    const uint32_t cycle_end = DWT->CYCCNT;
    attitude_diag.dt = dt;
    attitude_diag.spin_rate_dps = spin_rate_dps;
    attitude_diag.acc_valid = acc_valid;
    attitude_diag.cycles = cycle_end - cycle_start;
    if (attitude_diag.cycles > attitude_diag.cycles_max) {
        attitude_diag.cycles_max = attitude_diag.cycles;
    }

    return euler_angles;
}

#if USE_MAGNETOMETER
Euler_angles Attitude_Update(float ax_g, float ay_g, float az_g,
                             float gx_dps, float gy_dps, float gz_dps,
                             float mx_gauss, float my_gauss, float mz_gauss)
{
    return Attitude_Update_Internal(ax_g, ay_g, az_g, 
                                   gx_dps, gy_dps, gz_dps, 
                                   mx_gauss, my_gauss, mz_gauss, true);
}

Euler_angles Attitude_Update_IMU_Only(float ax_g, float ay_g, float az_g,
                                      float gx_dps, float gy_dps, float gz_dps)
{
    return Attitude_Update_Internal(ax_g, ay_g, az_g, 
                                   gx_dps, gy_dps, gz_dps, 
                                   0.0f, 0.0f, 0.0f, false);
}
#endif

float Attitude_Get_Roll(void)
{
    return euler_angles.roll;
}

float Attitude_Get_Pitch(void)
{
    return euler_angles.pitch;
}

float Attitude_Get_Yaw(void)
{
    return euler_angles.yaw;
}

Euler_angles Attitude_Get_Angles(void)
{
    return euler_angles;
}

const AttitudeDiagnostics *Attitude_GetDiagnostics(void)
{
    return &attitude_diag;
}
