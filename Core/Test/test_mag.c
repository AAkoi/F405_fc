/**
 * @file    test_mag.c
 * @brief   Standalone magnetometer test/stream for host-side calibration.
 *
 * Output format (10 Hz):
 *   MAG_RAW,<ms>,rawX,rawY,rawZ,gaussX,gaussY,gaussZ,magG
 */

#include "test_mag.h"

#include <math.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "hmc5883l.h"
#include "task_mag.h"

void test_mag_run(void)
{
    printf("\r\n========================================\r\n");
    printf("[test_mag] 磁力计原始数据测试/标定输出\r\n");
    printf("========================================\r\n\r\n");

    printf("[1/3] 初始化 HMC5883L...\r\n");
    bool mag_available = hmc5883l_init_driver();
    if (!mag_available) {
        printf("[错误] HMC5883L 初始化失败，退出测试\r\n");
        while (1) { HAL_Delay(1000); }
    }

    printf("[2/3] 初始化磁力计处理模块...\r\n");
    mag_processing_init(); // 清零校准参数，留待上位机拟合

    printf("[3/3] 开始输出，格式:\r\n");
    printf("MAG_RAW,时间戳ms,rawX,rawY,rawZ,gaussX,gaussY,gaussZ,|B|G\r\n");
    printf("ATTITUDE_FULL,时间戳,0,0,0,0,0,0,0,0,0,mx,my,mz (兼容上位机实时显示)\r\n");
    printf("请在上位机做'8字'挥动采集数据，用于硬铁/软铁标定。\r\n\r\n");

    uint32_t last_print = HAL_GetTick();
    while (1) {
        int16_t mx_raw = 0, my_raw = 0, mz_raw = 0;
        if (hmc5883l_read_raw_data(&mx_raw, &my_raw, &mz_raw)) {
            mag_process_sample(mx_raw, my_raw, mz_raw);

            if (mag_calibrated.ready) {
                float heading = atan2f(mag_calibrated.gauss_y, mag_calibrated.gauss_x);
                heading = heading * 180.0f / M_PI;
                    if (heading < 0) {
                         heading += 360.0f;
                    }
                float magG = sqrtf(
                    mag_calibrated.gauss_x * mag_calibrated.gauss_x +
                    mag_calibrated.gauss_y * mag_calibrated.gauss_y +
                    mag_calibrated.gauss_z * mag_calibrated.gauss_z
                );

                uint32_t now = HAL_GetTick();
                if (now - last_print >= 100) { // 10 Hz 输出
                    last_print = now;
                    printf("MAG_RAW,%lu,%d,%d,%d,%.4f,%.4f,%.4f,%.4f\r\n",
                           (unsigned long)now,
                           mx_raw, my_raw, mz_raw,
                           mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z,
                           magG);
                    // 兼容上位机解析 ATTITUDE_FULL 的磁力计实时显示
                    printf("ATTITUDE_FULL,%lu,0,0,0,0,0,0,0,0,0,%d,%d,%d\r\n",
                           (unsigned long)now, mx_raw, my_raw, mz_raw);
                    
                }
            }
        } else {
            HAL_Delay(10); // 读取失败短暂等待
        }

        HAL_Delay(5);
    }
}


