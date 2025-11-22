/**
 * @file    test_attitude_mag.h
 * @brief   Minimal sensor bridge for web visualization.
 *
 * Output format (one line, integers only):
 * ACC: ax ay az | GYR: gx gy gz | MAG: mx my mz | BAR: temp_decic pa alt_decim | T: imu_temp_decic
 * - BAR temp is 0.1 °C, altitude is 0.1 m, IMU temp is 0.1 °C
 */

#ifndef TEST_ATTITUDE_MAG_H
#define TEST_ATTITUDE_MAG_H

#include <stdbool.h>

void test_attitude_mag_stream(void);
bool test_attitude_mag_single_read(void);

#endif // TEST_ATTITUDE_MAG_H
