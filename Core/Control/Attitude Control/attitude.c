/*
 * 姿态解算（Mahony 互补滤波）
 */
#include <math.h>
#include "stm32f4xx_hal.h"
#include "attitude.h"
#include "icm42688p_lib.h"

// 外部IMU设备句柄
extern icm42688p_dev_t icm;

Euler_angles euler_angles;
Quaternion attitude_q;

// Mahony算法的积分项（误差积分）
static float exInt = 0.0f, eyInt = 0.0f, ezInt = 0.0f;
static uint32_t lastTick = 0;

// 陀螺仪零偏（单位：dps，在校准后会被设置）
static float gyro_bias_x = 0.0f;
static float gyro_bias_y = 0.0f;
static float gyro_bias_z = 0.0f;

// Mahony滤波器增益（可按响应/抑噪要求调整）。
// 实际使用的是 2*Kp 与 2*Ki，便于少一次乘法。
// 降低Kp值以减少对加速度计噪声的敏感度
static const float twoKp = 2.0f * 0.2f;   // 比例增益 2*Kp（Kp≈0.2，降低以减少噪声影响）
static const float twoKi = 2.0f * 0.01f;  // 积分增益 2*Ki（Ki≈0.01，降低以提高稳定性）

// 快速平方根倒数（1/sqrt(x)）
float fast_inv_sqrt(float x)
{
    float halfx = 0.5f * x;
    float y = x;
    int32_t i = *(int32_t*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (1.5f - (halfx * y * y));
    return y;
}

// 四元数归一化： q = q / |q|
static void quat_normalize(Quaternion *q)
{
    // |q| = sqrt(qw^2 + qx^2 + qy^2 + qz^2)
    float n2 = q->p0*q->p0 + q->p1*q->p1 + q->p2*q->p2 + q->p3*q->p3;
    if (n2 > 0.0f) {
        float inv = fast_inv_sqrt(n2);
        q->p0 *= inv; q->p1 *= inv; q->p2 *= inv; q->p3 *= inv;
    } else {
        q->p0 = 1.0f; q->p1 = q->p2 = q->p3 = 0.0f;
    }
}

void Attitude_Init(void)
{
    // 欧拉角清零，四元数置单位元 q=[1,0,0,0]
    euler_angles.pitch = 0.0f;
    euler_angles.roll  = 0.0f;
    euler_angles.yaw   = 0.0f;

    attitude_q.p0 = 1.0f;
    attitude_q.p1 = 0.0f;
    attitude_q.p2 = 0.0f;
    attitude_q.p3 = 0.0f;

    exInt = eyInt = ezInt = 0.0f;
    lastTick = HAL_GetTick();
}

void Attitude_CalibrateGyro(uint16_t samples)
{
    if (samples == 0) samples = 100;
    
    printf("开始陀螺仪校准，请保持传感器静止...\r\n");
    
    float gxSum = 0.0f, gySum = 0.0f, gzSum = 0.0f;
    uint16_t valid_count = 0;
    
    for (uint16_t i = 0; i < samples; ++i) {
        int16_t gx_r, gy_r, gz_r, ax_r, ay_r, az_r;
        float gx_dps, gy_dps, gz_dps, ax_g, ay_g, az_g, temp;
        if (icm42688p_dataPreprocess(&gx_r, &gy_r, &gz_r,
                                     &ax_r, &ay_r, &az_r,
                                     &gx_dps, &gy_dps, &gz_dps,
                                     &ax_g, &ay_g, &az_g,
                                     &temp)) {
            gxSum += gx_dps;
            gySum += gy_dps;
            gzSum += gz_dps;
            valid_count++;
        }
        HAL_Delay(5);
    }
    
    if (valid_count > 0) {
        gyro_bias_x = gxSum / valid_count;
        gyro_bias_y = gySum / valid_count;
        gyro_bias_z = gzSum / valid_count;
        
        printf("陀螺仪校准完成！零偏: X=%d.%d Y=%d.%d Z=%d.%d dps\r\n",
               (int)gyro_bias_x, (int)(fabsf(gyro_bias_x) * 10) % 10,
               (int)gyro_bias_y, (int)(fabsf(gyro_bias_y) * 10) % 10,
               (int)gyro_bias_z, (int)(fabsf(gyro_bias_z) * 10) % 10);
    } else {
        printf("陀螺仪校准失败：未获取到有效数据\r\n");
    }
}

void Attitude_SetGyroBias(float bias_x, float bias_y, float bias_z)
{
    gyro_bias_x = bias_x;
    gyro_bias_y = bias_y;
    gyro_bias_z = bias_z;
}

void Attitude_InitFromAccelerometer(uint16_t samples)
{
    if (samples == 0) samples = 50;

    float axSum = 0.0f, aySum = 0.0f, azSum = 0.0f;
    for (uint16_t i = 0; i < samples; ++i) {
        int16_t gx_r, gy_r, gz_r, ax_r, ay_r, az_r;
        float gx_dps, gy_dps, gz_dps, ax_g, ay_g, az_g, temp;
        if (icm42688p_dataPreprocess(&gx_r, &gy_r, &gz_r,
                                     &ax_r, &ay_r, &az_r,
                                     &gx_dps, &gy_dps, &gz_dps,
                                     &ax_g, &ay_g, &az_g,
                                     &temp)) {
            axSum += ax_g; aySum += ay_g; azSum += az_g;
        }
        HAL_Delay(2);
    }

    float ax = axSum / samples;
    float ay = aySum / samples;
    float az = azSum / samples;

    // 加速度向量归一化： a = a / ||a||
    float n2 = ax*ax + ay*ay + az*az;
    if (n2 > 1e-6f) {
        float inv = fast_inv_sqrt(n2);
        ax *= inv; ay *= inv; az *= inv;
    }

    // 通过加速度静态姿态计算初始姿态：
    // 常规：
    //   roll  = atan2(ay, az)
    //   pitch = atan2(-ax, sqrt(ay^2 + az^2))
    // 特殊：当 pitch ≈ ±90° 时，ay≈0, az≈0，出现欧拉奇异（横滚/航向不再可辨），
    // 为避免 atan2(0,0) 造成不稳定，这里在分母过小情况下固定 roll=0，仅设置 pitch=±90°。
    float denom = sqrtf(ay*ay + az*az);
    float roll, pitch;
    if (denom < 1e-6f) {
        // 近似 90° 俯仰：仅由 ax 决定方向
        roll = 0.0f;
        pitch = (ax < 0.0f) ? 1.57079633f : -1.57079633f; // ±pi/2
    } else {
        roll  = atan2f(ay, az);
        pitch = atan2f(-ax, denom);
    }
    float yaw   = 0.0f;

    // 欧拉角(Yaw-Pitch-Roll, 采用ZYX内旋) 转 四元数：
    // q0 = cy*cp*cr + sy*sp*sr
    // q1 = cy*cp*sr - sy*sp*cr
    // q2 = cy*sp*cr + sy*cp*sr
    // q3 = sy*cp*cr - cy*sp*sr
    float cr = cosf(roll * 0.5f),  sr = sinf(roll * 0.5f);
    float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
    float cy = cosf(yaw * 0.5f),   sy = sinf(yaw * 0.5f);

    attitude_q.p0 = cy*cp*cr + sy*sp*sr;
    attitude_q.p1 = cy*cp*sr - sy*sp*cr;
    attitude_q.p2 = cy*sp*cr + sy*cp*sr;
    attitude_q.p3 = sy*cp*cr - cy*sp*sr;
    quat_normalize(&attitude_q);

    exInt = eyInt = ezInt = 0.0f;
    lastTick = HAL_GetTick();
}

Euler_angles Attitude_Update(int16_t ax_in, int16_t ay_in, int16_t az_in,
                             int16_t gx_in, int16_t gy_in, int16_t gz_in)
{
    // 使用传入的原始传感器数据，不再重复读取
    int16_t gx_r = gx_in, gy_r = gy_in, gz_r = gz_in;
    int16_t ax_r = ax_in, ay_r = ay_in, az_r = az_in;
    
    // 将原始数据转换为物理单位
    float gx_dps, gy_dps, gz_dps;
    float ax_g, ay_g, az_g;
    
    if (!icm42688p_gyro_dataPreprocess(&gx_r, &gy_r, &gz_r,
                                       &gx_dps, &gy_dps, &gz_dps)) {
        return euler_angles; // 转换失败
    }
    
    // 手动转换加速度
    float ascale = (icm.accel_scale > 0.0f) ? icm.accel_scale : 1.0f;
    ax_g = (float)ax_r / ascale;
    ay_g = (float)ay_r / ascale;
    az_g = (float)az_r / ascale;

    // 时间步长 dt（s）并限制范围，避免突发抖动
    uint32_t now = HAL_GetTick();
    float dt = (now - lastTick) * 0.001f;
    lastTick = now;
    if (dt < 1e-4f) dt = 1e-4f;
    if (dt > 0.05f) dt = 0.05f;

    // 陀螺零偏补偿并从 °/s 转为 rad/s： rad/s = dps * pi/180
    float gx = (gx_dps - gyro_bias_x) * DEG2RAD;
    float gy = (gy_dps - gyro_bias_y) * DEG2RAD;
    float gz = (gz_dps - gyro_bias_z) * DEG2RAD;

    // 加速度向量归一化（当处于近自由落体或强机动时可能无效）
    float n2 = ax_g*ax_g + ay_g*ay_g + az_g*az_g;
    bool acc_valid = (n2 > 1e-6f);
    if (acc_valid) {
        float inv = fast_inv_sqrt(n2);
        ax_g *= inv; ay_g *= inv; az_g *= inv;
    }

    // 由当前四元数估计重力方向 v=[vx,vy,vz]
    float qw = attitude_q.p0, qx = attitude_q.p1, qy = attitude_q.p2, qz = attitude_q.p3;
    float vx = 2.0f * (qx*qz - qw*qy);
    float vy = 2.0f * (qw*qx + qy*qz);
    float vz = qw*qw - qx*qx - qy*qy + qz*qz;

    if (acc_valid) {
        // 误差为向量叉乘： e = a_meas x a_est
        float ex = (ay_g * vz - az_g * vy);
        float ey = (az_g * vx - ax_g * vz);
        float ez = (ax_g * vy - ay_g * vx);

        // 误差积分项（积分抗漂移）
        exInt += twoKi * ex * dt;
        eyInt += twoKi * ey * dt;
        ezInt += twoKi * ez * dt;

        // 比例 + 积分 反馈修正陀螺
        gx += twoKp * ex + exInt;
        gy += twoKp * ey + eyInt;
        gz += twoKp * ez + ezInt;
    }

    // 四元数微分： q_dot = 0.5 * q otimes [0,gx,gy,gz]
    float qw_dot = 0.5f * (-qx*gx - qy*gy - qz*gz);
    float qx_dot = 0.5f * ( qw*gx + qy*gz - qz*gy);
    float qy_dot = 0.5f * ( qw*gy - qx*gz + qz*gx);
    float qz_dot = 0.5f * ( qw*gz + qx*gy - qy*gx);

    // 四元数积分(更新四元数)
    attitude_q.p0 += qw_dot * dt;
    attitude_q.p1 += qx_dot * dt;
    attitude_q.p2 += qy_dot * dt;
    attitude_q.p3 += qz_dot * dt;
    //并归一化
    quat_normalize(&attitude_q);

    // 四元数转欧拉角（ZYX）
    qw = attitude_q.p0; qx = attitude_q.p1; qy = attitude_q.p2; qz = attitude_q.p3;
    float sinr_cosp = 2.0f * (qw*qx + qy*qz);
    float cosr_cosp = 1.0f - 2.0f * (qx*qx + qy*qy);
    float roll = atan2f(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (qw*qy - qz*qx);
    float pitch = (fabsf(sinp) >= 1.0f) ? copysignf((float)M_PI/2.0f, sinp) : asinf(sinp);

    float siny_cosp = 2.0f * (qw*qz + qx*qy);
    float cosy_cosp = 1.0f - 2.0f * (qy*qy + qz*qz);
    float yaw = atan2f(siny_cosp, cosy_cosp);

    euler_angles.roll  = roll  * RAD2DEG;
    euler_angles.pitch = pitch * RAD2DEG;
    euler_angles.yaw   = yaw   * RAD2DEG;

    return euler_angles;
}

/**
 * @brief 获取当前横滚角
 */
float Attitude_Get_Roll(void)
{
    return euler_angles.roll;
}

/**
 * @brief 获取当前俯仰角
 */
float Attitude_Get_Pitch(void)
{
    return euler_angles.pitch;
}

/**
 * @brief 获取当前航向角
 */
float Attitude_Get_Yaw(void)
{
    return euler_angles.yaw;
}

/**
 * @brief 获取当前完整姿态角
 */
Euler_angles Attitude_Get_Angles(void)
{
    return euler_angles;
}
