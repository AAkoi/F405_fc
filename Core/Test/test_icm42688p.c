/**
 * @file    test_icm42688p.c
 * @brief   Minimal IMU streaming helper.
 */

#include "test_icm42688p.h"
#include "icm42688p.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

extern icm42688p_dev_t icm;

void test_icm42688p_raw_data(void)
{
    while (1) {
        icm42688p_gyro_data_t gd = {0};
        icm42688p_accel_data_t ad = {0};
        icm42688p_temp_data_t td = {0};

        if (icm42688p_read_all(&icm, &gd, &ad, &td)) {
            const int imu_temp_deci = (int)(td.celsius * 10.0f);  // 0.1 °C
            printf("ACC: %d %d %d | GYR: %d %d %d | MAG: 0 0 0 | BAR: 0 0 0 | T: %d\r\n",
                   ad.x, ad.y, ad.z,
                   gd.x, gd.y, gd.z,
                   imu_temp_deci);
        } else {
            printf("ACC: 0 0 0 | GYR: 0 0 0 | MAG: 0 0 0 | BAR: 0 0 0 | T: 0\r\n");
        }

        HAL_Delay(20);
    }
}
