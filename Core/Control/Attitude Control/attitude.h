/**
 * @file    attitude.h
 * @brief   姿态解算模块（Mahony 互补滤波 + 磁力计融合）接口
 * @note    本模块只负责姿态融合算法，不涉及传感器数据读取
 *          所有输入数据应为物理单位（dps, g, gauss）
 */
#ifndef ATTITUDE_H
#define ATTITUDE_H

#include <stdint.h>
#include <stdbool.h>

// 编译宏：是否启用磁力计融合（默认启用）
#ifndef USE_MAGNETOMETER
#define USE_MAGNETOMETER 1
#endif

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

// 运行诊断信息（性能 / 传感器使用状态）
typedef struct {
    float dt;               // 上次更新的时间步长(s)
    float spin_rate_dps;    // 上次更新时陀螺旋转率 (deg/s)
    bool acc_valid;         // 是否使用了加速度计
    bool mag_used;          // 是否使用了磁力计
    bool mag_strength_ok;   // 磁场幅值是否在合理范围
    uint32_t cycles;        // 本次姿态更新耗费的DWT 时钟周期数
    uint32_t cycles_max;    // 运行以来的最大周期数
} AttitudeDiagnostics;

// 模块状态（由 attitude.c 定义）
extern Euler_angles euler_angles;
extern Quaternion attitude_q;

// 快速平方根倒数（Fast inverse sqrt）
float fast_inv_sqrt(float x);

// 姿态初始化（设为单位四元数，清零积分项）
void Attitude_Init(void);

// 利用加速度计静止姿态进行初始化（传入已归一化的加速度向量）
// 注意：此函数会将yaw初始化为0，然后靠磁力计缓慢收敛
void Attitude_InitFromAccelerometer(float ax_g, float ay_g, float az_g);

#if USE_MAGNETOMETER
// 使用加速度 + 磁力计初始化姿态（推荐！立即得到正确的yaw）
void Attitude_InitFromAccelMag(float ax_g, float ay_g, float az_g,
                               float mx, float my, float mz);

// 更新姿态（带磁力计融合）
Euler_angles Attitude_Update(float ax_g, float ay_g, float az_g,
                             float gx_dps, float gy_dps, float gz_dps,
                             float mx_gauss, float my_gauss, float mz_gauss);

// 更新姿态（仅使用IMU，不使用磁力计）
Euler_angles Attitude_Update_IMU_Only(float ax_g, float ay_g, float az_g,
                                      float gx_dps, float gy_dps, float gz_dps);
#else
// 更新姿态（不带磁力计）
Euler_angles Attitude_Update(float ax_g, float ay_g, float az_g,
                             float gx_dps, float gy_dps, float gz_dps);
#endif

// 获取当前姿态角
float Attitude_Get_Roll(void);
float Attitude_Get_Pitch(void);
float Attitude_Get_Yaw(void);
Euler_angles Attitude_Get_Angles(void);

// 获取诊断信息（只读指针）
const AttitudeDiagnostics *Attitude_GetDiagnostics(void);

// 欧拉角(rad) 转 四元数
Quaternion Attitude_EulerToQuat(float roll_rad, float pitch_rad, float yaw_rad);

#endif // ATTITUDE_H
