/**
 * @file    test_bmp280.c
 * @brief   Minimal BMP280 single read helper.
 */

#include "test_bmp280.h"
#include "bmp280.h"
#include <stdio.h>

bool test_bmp280_single_read(void)
{
    float temperature = 0.0f;
    int32_t pressure = 0;
    float altitude = 0.0f;

    if (!bmp280_get_all(&temperature, &pressure, &altitude)) {
        printf("[bmp280] read failed\r\n");
        return false;
    }

    const int temp_deci = (int)(temperature * 10.0f);
    const int alt_deci = (int)(altitude * 10.0f);

    printf("BAR: %d %ld %d (temp 0.1C, pressure Pa, altitude 0.1m)\r\n",
           temp_deci, (long)pressure, alt_deci);
    return true;
}
