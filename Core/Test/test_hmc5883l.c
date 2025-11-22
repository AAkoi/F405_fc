/**
 * @file    test_hmc5883l.c
 * @brief   Minimal magnetometer output helpers.
 */

#include "test_hmc5883l.h"
#include "hmc5883l.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdint.h>

void test_hmc5883l_raw_data(void)
{
    while (1) {
        int16_t mx = 0, my = 0, mz = 0;

        if (hmc5883l_read_raw_data(&mx, &my, &mz)) {
            printf("ACC: 0 0 0 | GYR: 0 0 0 | MAG: %d %d %d | BAR: 0 0 0 | T: 0\r\n",
                   mx, my, mz);
        } else {
            printf("ACC: 0 0 0 | GYR: 0 0 0 | MAG: 0 0 0 | BAR: 0 0 0 | T: 0\r\n");
        }

        HAL_Delay(50);
    }
}

bool test_hmc5883l_single_read(void)
{
    int16_t mx = 0, my = 0, mz = 0;

    if (!hmc5883l_read_raw_data(&mx, &my, &mz)) {
        printf("[hmc5883l] read failed\r\n");
        return false;
    }

    printf("ACC: 0 0 0 | GYR: 0 0 0 | MAG: %d %d %d | BAR: 0 0 0 | T: 0\r\n",
           mx, my, mz);
    return true;
}
