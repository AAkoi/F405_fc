/**
 * @file    test_imu.c
 * @brief   Gyro filter/decimation stream (ACC/GYR... + IMU_CSV playback helper).
 */

#include "test_imu.h"
#include "imu_task.h"
#include "icm42688p.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>

void test_imu_gyro_attitude(void)
{
    // ICM42688P: 8 kHz gyro -> decimate to 1 kHz (decim=8)
    const float sample_hz = 8000.0f;
    const float pt1_cut_hz = 200.0f;
    const float aa_cut_hz  = 400.0f;
    const uint8_t decim_factor = 8;
    const uint32_t print_interval_ms = 50; // throttle UART prints

    gyro_filter_init(sample_hz, pt1_cut_hz, aa_cut_hz, decim_factor);

    printf("\r\n[IMU] gyro filtered/decimated stream (ACC|GYR|MAG|BAR|T)\r\n");
    printf("sample=%.0f Hz, pt1=%.0f Hz, anti-alias=%.0f Hz, decim=%u => %.0f Hz\r\n",
           sample_hz, pt1_cut_hz, aa_cut_hz, decim_factor, sample_hz / decim_factor);
    printf("Log replay: IMU_CSV,t_ms,raw_gx,raw_gy,raw_gz,filt_gx,filt_gy,filt_gz\r\n");

    while (1) {
        int16_t gx = 0, gy = 0, gz = 0;
        int16_t ax = 0, ay = 0, az = 0;
        float temp_c = 0.0f;

        // One-shot read to reduce bus traffic
        if (!icm42688p_get_all_data(&gx, &gy, &gz, &ax, &ay, &az, &temp_c)) {
            HAL_Delay(print_interval_ms);
            continue;
        }

        // Feed filter/decimator
        if (!gyro_filter_feed_sample(gx, gy, gz)) {
            HAL_Delay(print_interval_ms);
            continue;
        }

        if (!gyro_decim.ready) {
            continue; // wait for decimator window
        }

        const int imu_temp_deci = (int)lroundf(temp_c * 10.0f);

        // Keep html parser compatible; GYR is filtered+decimated dps
        const int raw_x_dps = (int)lroundf(gyro_trace.raw_dps_x);
        const int raw_y_dps = (int)lroundf(gyro_trace.raw_dps_y);
        const int raw_z_dps = (int)lroundf(gyro_trace.raw_dps_z);
        const int gyr_x_int = (int)lroundf(gyro_decim.dps_x);
        const int gyr_y_int = (int)lroundf(gyro_decim.dps_y);
        const int gyr_z_int = (int)lroundf(gyro_decim.dps_z);

        printf("ACC: %d %d %d | GYR_RAW: %d %d %d | GYR: %d %d %d | MAG: 0 0 0 | BAR: 0 0 0 | T: %d\r\n",
               ax, ay, az,
               raw_x_dps, raw_y_dps, raw_z_dps,
               gyr_x_int, gyr_y_int, gyr_z_int,
               imu_temp_deci);

        // CSV: easier for html playback (time uses HAL_GetTick ms)
        printf("IMU_CSV,%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",
               (unsigned long)HAL_GetTick(),
               gyro_trace.raw_dps_x, gyro_trace.raw_dps_y, gyro_trace.raw_dps_z,
               gyro_decim.dps_x, gyro_decim.dps_y, gyro_decim.dps_z);

        HAL_Delay(print_interval_ms);
    }
}
