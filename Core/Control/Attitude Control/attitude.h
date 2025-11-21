/**
 * @file    attitude.h
 * @brief   姿态解算模块（Mahony 互补滤波）接口
 */
#ifndef ATTITUDE_H
#define ATTITUDE_H

#include <stdint.h>
#include "icm42688p.h"

#ifndef DEG2RAD
#define DEG2RAD 0.01745329251994329577f
#endif
#ifndef RAD2DEG
#define RAD2DEG 57.2957795130823208768f
#endif

// 欧拉角（单位：deg）
typedef struct {
    float pitch;  // 俯仰角
    float roll;   // 横滚角
    float yaw;    // 航向角
} Euler_angles;

// 四元数 q=[p0,p1,p2,p3] = [w,x,y,z]
typedef struct {
    float p0; // w
    float p1; // x
    float p2; // y
    float p3; // z
} Quaternion;

// 模块状态（在 attitude.c 中定义）
extern Euler_angles euler_angles;
extern Quaternion attitude_q;

// 快速平方根倒数（Fast inverse sqrt）
float fast_inv_sqrt(float x);

// 姿态初始化（设为单位四元数，清零积分项）
void Attitude_Init(void);

// 利用加速度计静止姿态进行初始化（可选）
void Attitude_InitFromAccelerometer(uint16_t samples);

// 陀螺仪零偏校准（静止状态下采样多次求平均）
void Attitude_CalibrateGyro(uint16_t samples);

// 设置陀螺仪零偏（手动设置）
void Attitude_SetGyroBias(float bias_x, float bias_y, float bias_z);

// 更新姿态（如有新数据自动读取）
Euler_angles Attitude_Update(int16_t ax, int16_t ay, int16_t az,
                             int16_t gx, int16_t gy, int16_t gz);

#endif // ATTITUDE_H

