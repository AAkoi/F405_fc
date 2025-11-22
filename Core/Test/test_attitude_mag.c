/**
 * @file    test_attitude_mag.c
 * @brief   Compact sensor stream for the web visualizer.
 */

#include "test_attitude_mag.h"
#include "icm42688p.h"
#include "hmc5883l.h"
#include "bmp280.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdint.h>

extern icm42688p_dev_t icm;

static void print_frame(const icm42688p_accel_data_t *ad,
                        const icm42688p_gyro_data_t *gd,
                        int16_t mx, int16_t my, int16_t mz,
                        int bar_temp_deci, int32_t pressure, int alt_deci,
                        int imu_temp_deci)
{
    // 统一输出整数，0.1 精度交给上位机计算
    printf("ACC: %d %d %d | GYR: %d %d %d | MAG: %d %d %d | BAR: %d %ld %d | T: %d\r\n",
           ad->x, ad->y, ad->z,
           gd->x, gd->y, gd->z,
           mx, my, mz,
           bar_temp_deci, (long)pressure, alt_deci,
           imu_temp_deci);
}

void test_attitude_mag_stream(void)
{
    while (1) {
        icm42688p_gyro_data_t gd = {0};
        icm42688p_accel_data_t ad = {0};
        icm42688p_temp_data_t td = {0};

        int16_t mag_x = 0, mag_y = 0, mag_z = 0;
        float bar_temp = 0.0f, altitude = 0.0f;
        int32_t pressure = 0;

        if (icm42688p_read_all(&icm, &gd, &ad, &td)) {
            hmc5883l_read_raw_data(&mag_x, &mag_y, &mag_z);
            bmp280_get_all(&bar_temp, &pressure, &altitude);

            const int bar_temp_deci = (int)(bar_temp * 10.0f);
            const int alt_deci = (int)(altitude * 10.0f);
            const int imu_temp_deci = (int)(td.celsius * 10.0f);  // 0.1 °C

            print_frame(&ad, &gd, mag_x, mag_y, mag_z,
                        bar_temp_deci, pressure, alt_deci, imu_temp_deci);
        } else {
            printf("ACC: 0 0 0 | GYR: 0 0 0 | MAG: 0 0 0 | BAR: 0 0 0 | T: 0\r\n");
        }

        HAL_Delay(20);
    }
}

bool test_attitude_mag_single_read(void)
{
    icm42688p_gyro_data_t gd = {0};
    icm42688p_accel_data_t ad = {0};
    icm42688p_temp_data_t td = {0};

    int16_t mag_x = 0, mag_y = 0, mag_z = 0;
    float bar_temp = 0.0f, altitude = 0.0f;
    int32_t pressure = 0;

    if (!icm42688p_read_all(&icm, &gd, &ad, &td)) {
        printf("[sensor] IMU read failed\r\n");
        return false;
    }

    hmc5883l_read_raw_data(&mag_x, &mag_y, &mag_z);
    bmp280_get_all(&bar_temp, &pressure, &altitude);

    const int bar_temp_deci = (int)(bar_temp * 10.0f);
    const int alt_deci = (int)(altitude * 10.0f);
    const int imu_temp_deci = (int)(td.celsius * 10.0f);  // 0.1 °C

    print_frame(&ad, &gd, mag_x, mag_y, mag_z,
                bar_temp_deci, pressure, alt_deci, imu_temp_deci);
    return true;
}
