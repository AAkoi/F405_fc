/**
 * @file    test_attitude_full.c
 * @brief   Attitude solving on MCU using gyro + accel + magnetometer.
 * @note    Uses task_gyro, task_acc, task_mag modules for data processing.
 */

#include "test_attitude_full.h"

#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "stm32f4xx_hal.h"
#include "attitude.h"
#include "icm42688p.h"
#include "hmc5883l.h"
#include "task_gyro.h"
#include "task_acc.h"
#include "task_mag.h"

extern icm42688p_dev_t icm;

static void init_attitude_from_sensors(bool use_mag)
{
    if (!accel_scaled.ready) {
        Attitude_Init();
        printf("[test_attitude_full] 姿态使用默认值初始化\r\n");
        return;
    }

#if USE_MAGNETOMETER
    // 优先使用加速度计+磁力计初始化（立即得到正确的yaw）
    if (use_mag && mag_calibrated.ready) {
        Attitude_InitFromAccelMag(
            accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
            mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z
        );
        printf("[test_attitude_full] 姿态已从加速度计+磁力计初始化（yaw立即有效）\r\n");
        printf("  初始姿态: Roll=%.1f° Pitch=%.1f° Yaw=%.1f°\r\n",
               euler_angles.roll, euler_angles.pitch, euler_angles.yaw);
        return;
    }
#endif

    // fallback: 仅用加速度计（yaw=0，需要缓慢收敛）
    Attitude_InitFromAccelerometer(accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z);
    printf("[test_attitude_full] 姿态已从加速度计初始化（yaw=0，将缓慢收敛）\r\n");
}

// 临时禁用磁力计融合的开关（如果磁力计未校准，设置为false可避免Yaw漂移）
#define USE_MAG_FUSION  true  // true=使用磁力计, false=仅使用IMU

void test_attitude_full_run(void)
{
#if USE_MAGNETOMETER
    printf("\r\n========================================\r\n");
    printf("[test_attitude_full] 完整姿态测试 (IMU+磁力计)\r\n");
    printf("磁力计融合: %s\r\n", USE_MAG_FUSION ? "已启用" : "已禁用(仅IMU)");
    printf("========================================\r\n\r\n");

    // 1. 初始化IMU
    printf("[1/5] 初始化 ICM42688P...\r\n");
    icm42688p_init_driver();
    HAL_Delay(100);

    // 2. 初始化磁力计
    printf("[2/5] 初始化 HMC5883L...\r\n");
    bool mag_available = hmc5883l_init_driver();
    if (!mag_available) {
        printf("[警告] HMC5883L 初始化失败，将仅使用IMU\r\n");
    }

    // 3. 校准传感器
    printf("[3/5] 校准陀螺仪（请保持设备静止，采集500个样本，约0.5秒）...\r\n");
    
    // 读取几个样本查看原始数据（用于调试）
    int16_t dbg_gx, dbg_gy, dbg_gz, dbg_ax, dbg_ay, dbg_az;
    float dbg_temp;
    if (icm42688p_get_all_data(&dbg_gx, &dbg_gy, &dbg_gz, &dbg_ax, &dbg_ay, &dbg_az, &dbg_temp)) {
        printf("      校准前原始数据 - Gyro: %d %d %d, Accel: %d %d %d\r\n", 
               dbg_gx, dbg_gy, dbg_gz, dbg_ax, dbg_ay, dbg_az);
    }
    
    if (icm42688p_calibrate(500)) {
        printf("  ✓ 陀螺仪零偏: [%d, %d, %d]\r\n", 
               icm.gyro_offset[0], icm.gyro_offset[1], icm.gyro_offset[2]);
    } else {
        printf("  ✗ 陀螺仪校准失败！\r\n");
    }
    
    printf("      加速度计无需校准（保留重力信号用于姿态解算）\r\n");
    icm42688p_calibrate_accel(&icm, 0);  // 不校准，零偏设为0
    printf("      加速度计零偏: [%d, %d, %d] (应全为0)\r\n", 
               icm.accel_offset[0], icm.accel_offset[1], icm.accel_offset[2]);
    
    // task_gyro/acc 会自动应用零偏，姿态模块不需要额外补偿
    Attitude_SetGyroBias(0.0f, 0.0f, 0.0f);
    
    // ========================================================================
    // 【磁力计校准参数设置区】
    // 说明：在上位机完成校准后，将校准参数粘贴到这里
    // ========================================================================
    if (mag_available) {
        // 方式1：设置 task_mag 的校准参数（推荐）
        // 注意：mag_set_calibration 的 offset 单位是 gauss，这里从 LSB 转 gauss
        const float mag_gain = (hmc_dev.gain_scale > 0.0f) ? hmc_dev.gain_scale : 1090.0f;
        mag_set_calibration(
            -2.0f/mag_gain, -327.0f/mag_gain, 20.0f/mag_gain,     // offset_x, offset_y, offset_z
            0.930f, 0.899f, 1.229f     // scale_x, scale_y, scale_z
        );

        
        // 方式2：同时设置 Attitude 模块的校准参数
        // Attitude_SetMagCalibration(
        //     0.0f, 0.0f, 0.0f,    // offset (gauss)
        //     1.0f, 1.0f, 1.0f     // scale
        // );
        
        printf("\r\n[磁力计校准状态]\r\n");
        printf("  如需校准，请：\r\n");
        printf("  1. 运行本程序，上位机会收到原始磁力计数据(mx,my,mz)\r\n");
        printf("  2. 在上位机使用校准工具（缓慢旋转设备30秒）\r\n");
        printf("  3. 上位机计算出校准参数后，复制到上面的代码中\r\n");
        printf("  4. 重新编译烧录，校准参数将永久生效\r\n");
        printf("  注意：如果暂不校准，可设置 USE_MAG_FUSION=false\r\n\r\n");
    }

    // 4. 初始化数据处理模块
    printf("[4/5] 初始化数据处理模块...\r\n");
    gyro_processing_init(1);  // 不降采样
    accel_processing_init();
    if (mag_available) {
        mag_processing_init();
    }

    // 5. 初始化姿态解算
    printf("[5/5] 初始化姿态解算...\r\n");
    Attitude_Init();
    
    // 读取几帧数据用于初始化，同时诊断
    // 【重要】必须在初始化姿态之前读取磁力计数据，否则 mag_calibrated.ready = false
    printf("\r\n[诊断] 读取初始数据（静止状态）...\r\n");
    int16_t mx_init = 0, my_init = 0, mz_init = 0;
    for (int i = 0; i < 5; i++) {  // 增加到5次，确保磁力计有数据
        int16_t gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw;
        float temp;
        if (icm42688p_get_all_data(&gx_raw, &gy_raw, &gz_raw, &ax_raw, &ay_raw, &az_raw, &temp)) {
            gyro_process_sample(gx_raw, gy_raw, gz_raw);
            accel_process_sample(ax_raw, ay_raw, az_raw);
            
            // 【关键】也要读取磁力计数据！
            if (mag_available) {
                if (hmc5883l_read_raw_data(&mx_init, &my_init, &mz_init)) {
                    mag_process_sample(mx_init, my_init, mz_init);
                }
            }
            
            // 计算补偿后的值（手动验证）
            int16_t gx_comp = gx_raw - icm.gyro_offset[0];
            int16_t gy_comp = gy_raw - icm.gyro_offset[1];
            int16_t gz_comp = gz_raw - icm.gyro_offset[2];
            
            printf("  样本%d:\r\n", i+1);
            printf("    原始RAW: G(%d,%d,%d) A(%d,%d,%d) T=%.1f°C\r\n",
                   gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw, temp);
            printf("    零偏补偿后: G(%d,%d,%d) [应该接近0]\r\n",
                   gx_comp, gy_comp, gz_comp);
            printf("    task输出: gyro(%.1f,%.1f,%.1f)dps acc(%.3f,%.3f,%.3f)g\r\n",
                   gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z,
                   accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z);
            if (mag_available && mag_calibrated.ready) {
                printf("    mag: raw(%d,%d,%d) gauss(%.3f,%.3f,%.3f)\r\n",
                       mx_init, my_init, mz_init,
                       mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z);
            }
        }
        HAL_Delay(100);  // 缩短延时，更快完成初始化
    }
    
    printf("\r\n[诊断] ICM42688P 配置:\r\n");
    printf("  gyro_scale  = %.2f (LSB/dps)\r\n", icm.gyro_scale);
    printf("  accel_scale = %.2f (LSB/g)\r\n", icm.accel_scale);
    printf("  gyro_offset = [%d, %d, %d] [陀螺仪零偏，通常<±500]\r\n", 
           icm.gyro_offset[0], icm.gyro_offset[1], icm.gyro_offset[2]);
    printf("  accel_offset= [%d, %d, %d] [加速度计零偏，应全为0]\r\n", 
           icm.accel_offset[0], icm.accel_offset[1], icm.accel_offset[2]);
    if (mag_available) {
        printf("  mag_calibrated.ready = %s\r\n", mag_calibrated.ready ? "true" : "false");
        printf("  mag_calibrated.gauss = (%.3f, %.3f, %.3f)\r\n\r\n",
               mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z);
    }
    
    init_attitude_from_sensors(USE_MAG_FUSION && mag_available);

    printf("\r\n[test_attitude_full] 开始实时输出姿态角...\r\n");
    printf("格式: ATTITUDE_FULL,时间戳,Roll,Pitch,Yaw,ax,ay,az,gx,gy,gz,mx,my,mz\r\n");
    printf("磁力计状态: %s\r\n", mag_available ? "已启用" : "未启用");
    printf("注意：如果数据持续饱和，请检查传感器配置和校准！\r\n\r\n");

    uint32_t last_print = HAL_GetTick();
    uint32_t loop_count = 0;
    uint32_t mag_read_count = 0;
    uint32_t sat_count = 0;  // 饱和计数器
    uint32_t last_perf = last_print;
    const float cycles_to_us = 1000000.0f / (float)SystemCoreClock;
    const uint32_t sat_guard_enable_ms = HAL_GetTick() + 800; // 上电后延迟一段时间再开始饱和检测

    while (1) {
        int16_t gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw;
        int16_t mx_raw = 0, my_raw = 0, mz_raw = 0;
        float temp_c;

        // 读取IMU原始数据
        if (!icm42688p_get_all_data(&gx_raw, &gy_raw, &gz_raw, 
                                    &ax_raw, &ay_raw, &az_raw, &temp_c)) {
            HAL_Delay(100); // 读取失败短暂等待，避免长时间停顿
            continue;
        }

        // 使用 task_gyro 和 task_acc 处理数据
        gyro_process_sample(gx_raw, gy_raw, gz_raw);
        accel_process_sample(ax_raw, ay_raw, az_raw);

        // 读取磁力计（降低频率，每2次读一次，响应更快）
        if (mag_available && (loop_count % 2 == 0)) {
            if (hmc5883l_read_raw_data(&mx_raw, &my_raw, &mz_raw)) {
                mag_process_sample(mx_raw, my_raw, mz_raw);
                mag_read_count++;
                
                // 首次成功读取时打印
                if (mag_read_count == 1) {
                    printf("[调试] 磁力计首次读取成功: raw(%d,%d,%d) gauss(%.3f,%.3f,%.3f)\r\n",
                           mx_raw, my_raw, mz_raw,
                           mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z);
                }
            } else {
                // 磁力计读取失败
                if (mag_read_count == 0 && loop_count == 100) {
                    printf("[警告] 磁力计连续读取失败，检查I2C连接\r\n");
                }
            }
        }
        loop_count++;

        // 等待数据就绪
        if (!accel_scaled.ready) {
            continue;
        }

        // 检测数据饱和（每100次打印一次警告），上电延时后才启用，遇到饱和直接丢弃本帧
        uint32_t now = HAL_GetTick();
        bool saturated = (fabsf(accel_scaled.g_x) > 15.5f || fabsf(accel_scaled.g_y) > 15.5f || fabsf(accel_scaled.g_z) > 15.5f ||
                          fabsf(gyro_scaled.dps_x) > 1950.0f || fabsf(gyro_scaled.dps_y) > 1950.0f || fabsf(gyro_scaled.dps_z) > 1950.0f);
        if (now >= sat_guard_enable_ms && saturated) {
            sat_count++;
            if (sat_count % 100 == 1) {
                printf("[警告#%lu] 传感器饱和！acc(%.1f,%.1f,%.1f)g gyro(%.0f,%.0f,%.0f)dps - 请检查传感器配置或零偏校准！\r\n",
                       (unsigned long)sat_count,
                       accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                       gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z);
            }
            continue; // 丢弃当前帧，避免污染姿态解算
        }

        // 更新姿态
        Euler_angles ang;
        #if USE_MAGNETOMETER
        if (USE_MAG_FUSION && mag_available && mag_calibrated.ready) {
            // 使用IMU + 磁力计（需要磁力计校准）
            ang = Attitude_Update(
                accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z,
                mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z
            );
        } else {
            // 仅使用IMU（不受磁力计影响，Yaw会缓慢漂移但稳定）
            ang = Attitude_Update_IMU_Only(
                accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z
            );
        }
        #else
        ang = Attitude_Update(
            accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
            gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z
        );
        #endif
        const AttitudeDiagnostics *diag = Attitude_GetDiagnostics();

        // 定时打印（上位机格式）
        if (now - last_print >= 100) {
            last_print = now;
            
            // ATTITUDE_FULL 格式（单片机计算的姿态 + 传感器数据）
            // 格式: ATTITUDE_FULL,时间戳,Roll,Pitch,Yaw,ax,ay,az,gx,gy,gz,mx,my,mz
            printf("ATTITUDE_FULL,%lu,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%d,%d,%d\r\n",
                   (unsigned long)now,
                   ang.roll, ang.pitch, ang.yaw,
                   accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                   gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z,
                   mag_raw.x, mag_raw.y, mag_raw.z);
        }

        if (now - last_perf >= 1000) {
            last_perf = now;
            float last_us = diag->cycles * cycles_to_us;
            float max_us  = diag->cycles_max * cycles_to_us;
            printf("[perf] dt=%.3fs spin=%.1f dps acc=%d mag_used=%d strength_ok=%d | |B|=%.3fG cycles=%lu (max %lu) => %.2fus / %.2fus\r\n",
                   diag->dt,
                   diag->spin_rate_dps,
                   diag->acc_valid,
                   diag->mag_used,
                   diag->mag_strength_ok,
                   mag_calibrated.magnitude_gauss,
                   (unsigned long)diag->cycles,
                   (unsigned long)diag->cycles_max,
                   last_us,
                   max_us);
                if(mag_available){
                    printf("ok");
                }else {
                    printf("not ok");
                }
        }
    }
#else
    printf("[test_attitude_full] USE_MAGNETOMETER 未启用，跳过测试\r\n");
    while (1) {
        HAL_Delay(1000);
    }
#endif
}
