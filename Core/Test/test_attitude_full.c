/**
 * @file    test_attitude_full.c
 * @brief   完整姿态解算测试（陀螺仪 + 加速度计 + 磁力计）
 * @note    配合优化后的attitude.c使用，陀螺仪零偏已在底层处理
 */

#include "test_attitude_full.h"

#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "stm32f4xx_hal.h"
#include "attitude.h"
#include "icm42688p.h"
#include "hmc5883l.h"
#include "bmp280.h"
#include "task_gyro.h"
#include "task_acc.h"
#include "task_mag.h"

extern icm42688p_dev_t icm;
extern hmc5883l_dev_t hmc_dev;

// 磁力计融合开关：如果磁力计未校准，建议设为false避免yaw漂移
#define USE_MAG_FUSION  true  // true=使用磁力计融合, false=仅IMU

/**
 * @brief 从传感器数据初始化姿态
 * @param use_mag 是否使用磁力计初始化yaw角
 */
static void init_attitude_from_sensors(bool use_mag)
{
    if (!accel_scaled.ready) {
        Attitude_Init();
        printf("[姿态] 使用默认值初始化\r\n");
        return;
    }

#if USE_MAGNETOMETER
    if (use_mag && mag_calibrated.ready) {
        float mx_unit = 0.0f, my_unit = 0.0f, mz_unit = 0.0f;
        float mag_strength = 0.0f;
        if (mag_get_normalized(&mx_unit, &my_unit, &mz_unit, &mag_strength)) {
            Attitude_InitFromAccelMag(
                accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                mx_unit, my_unit, mz_unit
            );
            printf("[姿态] 已从加速度+磁力计初始化（yaw立即有效）\r\n");
            printf("  初始姿态: Roll=%.1f° Pitch=%.1f° Yaw=%.1f° |B|=%.3fG\r\n",
                   euler_angles.roll, euler_angles.pitch, euler_angles.yaw, mag_strength);
            return;
        }
    }
#endif

    // fallback: 仅用加速度计（yaw=0，需要缓慢收敛）
    Attitude_InitFromAccelerometer(accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z);
    printf("[姿态] 已从加速度计初始化（yaw=0，将缓慢收敛）\r\n");
}

void test_attitude_full_run(void)
{
#if USE_MAGNETOMETER
    printf("\r\n========================================\r\n");
    printf("[测试] 完整姿态解算 (IMU+磁力计)\r\n");
    printf("磁力计融合: %s\r\n", USE_MAG_FUSION ? "已启用" : "已禁用(仅IMU)");
    printf("========================================\r\n\r\n");

    // ============ 步骤0: 初始化气压计 ============
    printf("[0/5] 初始化 BMP280...\r\n");
    bmp280_init_driver(); // 若失败只提示，不影响IMU

    // ============ 步骤1: 初始化IMU ============
    printf("[1/5] 初始化 ICM42688P...\r\n");
    icm42688p_init_driver();
    HAL_Delay(100);

    // ============ 步骤2: 初始化磁力计 ============
    printf("[2/5] 初始化 HMC5883L...\r\n");
    bool mag_available = hmc5883l_init_driver();
    if (!mag_available) {
        printf("[警告] HMC5883L 初始化失败，将仅使用IMU\r\n");
    }

    // ============ 步骤3: 传感器校准 ============
    printf("[3/5] 校准传感器...\r\n");
    
    // 3.1 陀螺仪零偏校准（静止状态）
    printf("  >> 陀螺仪零偏校准（请保持设备静止，采样500个样本，约2.5秒）...\r\n");
    
    // 显示校准前的原始数据（用于调试）
    int16_t dbg_gx, dbg_gy, dbg_gz, dbg_ax, dbg_ay, dbg_az;
    float dbg_temp;
    if (icm42688p_get_all_data(&dbg_gx, &dbg_gy, &dbg_gz, &dbg_ax, &dbg_ay, &dbg_az, &dbg_temp)) {
        printf("      校准前原始数据 - Gyro: %d %d %d, Accel: %d %d %d\r\n", 
               dbg_gx, dbg_gy, dbg_gz, dbg_ax, dbg_ay, dbg_az);
    }
    
    if (icm42688p_calibrate(500)) {
        printf("      ✓ 陀螺仪零偏: [%d, %d, %d] (已自动应用)\r\n", 
               icm.gyro_offset[0], icm.gyro_offset[1], icm.gyro_offset[2]);
    } else {
        printf("      ✗ 陀螺仪校准失败！\r\n");
    }
    
    // 3.2 加速度计（不校准，保留重力信号用于姿态解算）
    printf("  >> 加速度计无需校准（保留重力信号用于姿态解算）\r\n");
    icm42688p_calibrate_accel(&icm, 0);  // 零偏设为0（不校准）
    printf("      加速度计零偏: [%d, %d, %d] (应全为0)\r\n", 
           icm.accel_offset[0], icm.accel_offset[1], icm.accel_offset[2]);
    
    // 3.3 磁力计校准参数（硬铁/软铁）
    if (mag_available) {
        const float mag_gain = (hmc_dev.gain_scale > 0.0f) ? hmc_dev.gain_scale : 1090.0f;
        
        // 示例校准参数（需要根据实际校准结果修改）
        mag_set_calibration(
            -2.0f/mag_gain, -327.0f/mag_gain, 20.0f/mag_gain,  // offset (硬铁偏移)
            0.930f, 0.899f, 1.229f                              // scale (软铁缩放)
        );
        
        printf("\r\n[磁力计校准提示]\r\n");
        printf("  1. 运行本程序，上位机收到原始磁力计数据(mx,my,mz)\r\n");
        printf("  2. 在上位机使用校准工具（缓慢旋转设备约60秒）\r\n");
        printf("  3. 将拟合出的校准参数填入 mag_set_calibration\r\n");
        printf("  4. 重新编译烧录后参数生效；若暂不校准，可设 USE_MAG_FUSION=false\r\n\r\n");
    }

    // ============ 步骤4: 初始化数据处理模块 ============
    printf("[4/5] 初始化数据处理模块...\r\n");
    gyro_processing_init(1);  // 不降采样
    accel_processing_init();
    if (mag_available) {
        mag_processing_init();
    }

    // ============ 步骤5: 初始化姿态解算 ============
    printf("[5/5] 初始化姿态解算...\r\n");
    Attitude_Init();
    
    // ============ 诊断: 读取初始数据 ============
    printf("\r\n[诊断] 读取初始数据（静止状态）...\r\n");
    int16_t mx_init = 0, my_init = 0, mz_init = 0;
    
    for (int i = 0; i < 5; i++) {
        int16_t gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw;
        float temp;
        
        if (icm42688p_get_all_data(&gx_raw, &gy_raw, &gz_raw, &ax_raw, &ay_raw, &az_raw, &temp)) {
            gyro_process_sample(gx_raw, gy_raw, gz_raw);
            accel_process_sample(ax_raw, ay_raw, az_raw);
            
            if (mag_available) {
                if (hmc5883l_read_raw_data(&mx_init, &my_init, &mz_init)) {
                    mag_process_sample(mx_init, my_init, mz_init);
                }
            }
            
            // 计算零偏补偿后的值
            int16_t gx_comp = gx_raw - icm.gyro_offset[0];
            int16_t gy_comp = gy_raw - icm.gyro_offset[1];
            int16_t gz_comp = gz_raw - icm.gyro_offset[2];
            
            printf("  样本%d:\r\n", i+1);
            printf("    原始RAW: G(%d,%d,%d) A(%d,%d,%d) T=%.1f°C\r\n",
                   gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw, temp);
            printf("    零偏补偿后: G(%d,%d,%d) [应接近0]\r\n",
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
        HAL_Delay(100);
    }
    
    // ============ 诊断: 传感器配置信息 ============
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
    
    // 从传感器数据初始化姿态
    init_attitude_from_sensors(USE_MAG_FUSION && mag_available);

    // ============ 主循环: 实时姿态输出 ============
    printf("\r\n[测试] 开始实时输出姿态角...\r\n");
    printf("格式: ATTITUDE_FULL,时间,Roll,Pitch,Yaw,ax,ay,az,gx,gy,gz,mx,my,mz\r\n");
    printf("磁力计状态: %s\r\n", mag_available ? "已启用" : "未启用");
    printf("注意：如果数据持续饱和，请检查传感器配置和校准！\r\n\r\n");

    uint32_t last_print = HAL_GetTick();
    uint32_t loop_count = 0;
    uint32_t mag_read_count = 0;
    uint32_t sat_count = 0;  // 饱和计数
    uint32_t last_perf = last_print;
    uint32_t last_baro_print = last_print;
    const uint32_t sat_guard_enable_ms = HAL_GetTick() + 800; // 上电后延迟一段时间再开始饱和检测
    const float cycles_to_us = 1000000.0f / (float)SystemCoreClock;
    float last_mag_strength = 0.0f;

    while (1) {
        int16_t gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw;
        int16_t mx_raw = 0, my_raw = 0, mz_raw = 0;
        float temp_c;

        // ---- 读取IMU原始数据 ----
        if (!icm42688p_get_all_data(&gx_raw, &gy_raw, &gz_raw, 
                                    &ax_raw, &ay_raw, &az_raw, &temp_c)) {
            HAL_Delay(100);
            continue;
        }

        // 处理陀螺仪和加速度计数据
        gyro_process_sample(gx_raw, gy_raw, gz_raw);
        accel_process_sample(ax_raw, ay_raw, az_raw);

        // ---- 读取磁力计（降低频率，每2次读一次，响应更快） ----
        if (mag_available && (loop_count % 2 == 0)) {
            if (hmc5883l_read_raw_data(&mx_raw, &my_raw, &mz_raw)) {
                mag_process_sample(mx_raw, my_raw, mz_raw);
                mag_read_count++;
                
                if (mag_read_count == 1) {
                    printf("[调试] 磁力计首次读取成功: raw(%d,%d,%d) gauss(%.3f,%.3f,%.3f)\r\n",
                           mx_raw, my_raw, mz_raw,
                           mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z);
                }
            } else {
                if (mag_read_count == 0 && loop_count == 100) {
                    printf("[警告] 磁力计连续读取失败，检查I2C连接\r\n");
                }
            }
        }
        loop_count++;

        if (!accel_scaled.ready) {
            continue;
        }

        // ---- 饱和检测 ----
        uint32_t now = HAL_GetTick();
        bool saturated = (fabsf(accel_scaled.g_x) > 15.5f || 
                         fabsf(accel_scaled.g_y) > 15.5f || 
                         fabsf(accel_scaled.g_z) > 15.5f ||
                         fabsf(gyro_scaled.dps_x) > 1950.0f || 
                         fabsf(gyro_scaled.dps_y) > 1950.0f || 
                         fabsf(gyro_scaled.dps_z) > 1950.0f);
        
        if (now >= sat_guard_enable_ms && saturated) {
            sat_count++;
            if (sat_count % 100 == 1) {
                printf("[警告#%lu] 传感器饱和！acc(%.1f,%.1f,%.1f)g gyro(%.0f,%.0f,%.0f)dps - 请检查传感器配置或零偏校准！\r\n",
                       (unsigned long)sat_count,
                       accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                       gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z);
            }
            continue;  // 跳过饱和数据
        }

        // ---- 获取归一化磁力计数据 ----
        bool mag_ready = false;
        float mx_unit = 0.0f, my_unit = 0.0f, mz_unit = 0.0f;
        if (mag_available && mag_calibrated.ready) {
            mag_ready = mag_get_normalized(&mx_unit, &my_unit, &mz_unit, &last_mag_strength);
        }

        // ---- 姿态更新 ----
        Euler_angles ang;
        if (USE_MAG_FUSION && mag_ready) {
            // 使用磁力计融合（9DoF）
            ang = Attitude_Update(
                accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z,
                mx_unit, my_unit, mz_unit
            );
        } else {
            // 仅使用IMU（6DoF）
            ang = Attitude_Update_IMU_Only(
                accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z
            );
        }
        
        const AttitudeDiagnostics *diag = Attitude_GetDiagnostics();

        // ---- 定期输出姿态数据（100ms） ----
        if (now - last_print >= 100) {
            last_print = now;
            printf("ATTITUDE_FULL,%lu,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%d,%d,%d\r\n",
                   (unsigned long)now,
                   ang.roll, ang.pitch, ang.yaw,
                   accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z,
                   gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z,
                   mag_raw.x, mag_raw.y, mag_raw.z);
        }

        // ---- 定期输出气压计数据（200ms） ----
        if (now - last_baro_print >= 200) {
            last_baro_print = now;
            float baro_temp_c = 0.0f;
            int32_t baro_pa = 0;
            float baro_alt_m = 0.0f;
            if (bmp280_get_all(&baro_temp_c, &baro_pa, &baro_alt_m)) {
                int32_t baro_temp_deci = (int32_t)(baro_temp_c * 10.0f);   // 0.1°C
                int32_t baro_alt_deci = (int32_t)(baro_alt_m * 10.0f);     // 0.1m
                printf("BAR: %ld %ld %ld\r\n",
                       (long)baro_temp_deci,
                       (long)baro_pa,
                       (long)baro_alt_deci);
            }
        }

        // ---- 定期输出性能诊断（1000ms） ----
        if (now - last_perf >= 1000) {
            last_perf = now;
            float last_us = diag->cycles * cycles_to_us;
            float max_us  = diag->cycles_max * cycles_to_us;
            printf("[perf] dt=%.3fs spin=%.1fdps acc=%d mag_used=%d strength_ok=%d |B|=%.3fG cycles=%lu(max %lu) => %.2fus/%.2fus\r\n",
                   diag->dt,
                   diag->spin_rate_dps,
                   diag->acc_valid,
                   diag->mag_used,
                   diag->mag_strength_ok,
                   last_mag_strength,
                   (unsigned long)diag->cycles,
                   (unsigned long)diag->cycles_max,
                   last_us,
                   max_us);
        }
    }
#else
    printf("[test_attitude_full] USE_MAGNETOMETER 未启用，跳过测试\r\n");
    while (1) {
        HAL_Delay(1000);
    }
#endif
}




