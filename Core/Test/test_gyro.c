/**
 * @file    test_gyro.c
 * @brief   Attitude solving on MCU using only gyro + accel (Mahony IMU-only).
 * @note    Uses task_gyro and task_acc modules for data processing.
 */

#include "test_gyro.h"

#include <stdbool.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "attitude.h"
#include "icm42688p.h"
#include "task_gyro.h"
#include "task_acc.h"

extern icm42688p_dev_t icm;

static void init_attitude_from_static_accel(void)
{
    if (accel_scaled.ready) {
        Attitude_InitFromAccelerometer(accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z);
        printf("[test_gyro] 姿态已从加速度计初始化\r\n");
    } else {
        Attitude_Init();
        printf("[test_gyro] 姿态使用默认值初始化\r\n");
    }
}

void test_gyro_run(void)
{
    printf("\r\n========================================\r\n");
    printf("[test_gyro] IMU姿态测量(仅陀螺仪+加速度计)\r\n");
    printf("========================================\r\n\r\n");

    // 1. 初始化IMU
    printf("[1/4] 初始化 ICM42688P...\r\n");
    icm42688p_init_driver();
    HAL_Delay(100);

    // 2. 校准陀螺仪（加速度计不需要校准，因为重力是真实信号）
    printf("[2/4] 校准陀螺仪零偏（请保持设备静止，采集500个样本，约0.5秒）...\r\n");
    
    // 读取几个样本查看原始数据（用于调试）
    int16_t dbg_gx, dbg_gy, dbg_gz, dbg_ax, dbg_ay, dbg_az;
    float dbg_temp;
    if (icm42688p_get_all_data(&dbg_gx, &dbg_gy, &dbg_gz, &dbg_ax, &dbg_ay, &dbg_az, &dbg_temp)) {
        printf("      校准前原始数据 - Gyro: %d %d %d, Accel: %d %d %d\r\n", 
               dbg_gx, dbg_gy, dbg_gz, dbg_ax, dbg_ay, dbg_az);
    }
    
    icm42688p_calibrate(500);  // 500个样本足以获得良好精度
    printf("      陀螺仪零偏: %d %d %d\r\n", icm.gyro_offset[0], icm.gyro_offset[1], icm.gyro_offset[2]);
    
    printf("      加速度计无需校准（保留重力信号用于姿态解算）\r\n");
    icm42688p_calibrate_accel(&icm, 0);  // 不校准，零偏设为0
    printf("      加速度计零偏: %d %d %d (应全为0)\r\n", icm.accel_offset[0], icm.accel_offset[1], icm.accel_offset[2]);
    
    // 零偏由 task_gyro/task_acc 处理，姿态模块直接使用补偿后的数据

    // 3. 初始化数据处理模块
    printf("[3/4] 初始化数据处理模块..\r\n");
    gyro_processing_init(1);  // 不降采样:1
    accel_processing_init();

    // 4. 初始化姿态解算
    printf("[4/4] 初始化姿态解算..\r\n");
    Attitude_Init();
    
    // 读取几帧数据用于初始化姿态
    for (int i = 0; i < 10; i++) {
        int16_t gx, gy, gz, ax, ay, az;
        float temp;
        if (icm42688p_get_all_data(&gx, &gy, &gz, &ax, &ay, &az, &temp)) {
            accel_process_sample(ax, ay, az);
        }
        HAL_Delay(10);
    }
    init_attitude_from_static_accel();

    printf("\r\n[test_gyro] 开始实时输出姿态角...\r\n");
    printf("格式: ATTITUDE_FULL,时间,Roll,Pitch,Yaw,ax,ay,az,gx,gy,gz,0,0,0\r\n");
    printf("注意: 姿态解算不再单独处理零偏，完全依赖 task_gyro/task_acc 校准结果\r\n\r\n");

    uint32_t last_print = HAL_GetTick();
    uint32_t last_perf = last_print;
    const float cycles_to_us = 1000000.0f / (float)SystemCoreClock;

    while (1) {
        int16_t gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw;
        float temp_c;

        // 读取IMU原始数据
        if (!icm42688p_get_all_data(&gx_raw, &gy_raw, &gz_raw, 
                                    &ax_raw, &ay_raw, &az_raw, &temp_c)) {
            HAL_Delay(1);
            continue;
        }

        // 使用 task_gyro 和 task_acc 处理数据
        gyro_process_sample(gx_raw, gy_raw, gz_raw);
        accel_process_sample(ax_raw, ay_raw, az_raw);

        // 使用处理后的数据更新姿态
        if (!accel_scaled.ready) {
            continue;
        }

#if USE_MAGNETOMETER
        Euler_angles ang = Attitude_Update_IMU_Only(
            accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
            gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z
        );
#else
        Euler_angles ang = Attitude_Update(
            accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
            gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z
        );
#endif
        const AttitudeDiagnostics *diag = Attitude_GetDiagnostics();

        // 定时打印（上位机格式）
        uint32_t now = HAL_GetTick();
        if (now - last_print >= 100) {
            last_print = now;
            
            // ATTITUDE_FULL 格式（单片机计算的姿态）
            printf("ATTITUDE_FULL,%lu,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,0,0,0\r\n",
                   (unsigned long)now,
                   ang.roll, ang.pitch, ang.yaw,
                   accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                   gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z);
        }

        if (now - last_perf >= 1000) {
            last_perf = now;
            float last_us = diag->cycles * cycles_to_us;
            float max_us  = diag->cycles_max * cycles_to_us;
            printf("[perf] dt=%.3fs spin=%.1f dps acc=%d mag=%d strength_ok=%d cycles=%lu (max %lu) => %.2fus / %.2fus\r\n",
                   diag->dt,
                   diag->spin_rate_dps,
                   diag->acc_valid,
                   diag->mag_used,
                   diag->mag_strength_ok,
                   (unsigned long)diag->cycles,
                   (unsigned long)diag->cycles_max,
                   last_us,
                   max_us);
        }
    }
}
