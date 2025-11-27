#include "test_tof.h"

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "tof.h"

void test_tof_run(void)
{
    printf("\r\n========================================\r\n");
    printf("[test_tof] VL53L0X 测距测试 (I2C)\r\n");
    printf("========================================\r\n\r\n");

    if (!tof_init_driver()) {
        printf("[test_tof] 传感器初始化失败，检查连线和I2C句柄配置。\r\n");
        while (1) { HAL_Delay(1000); }
    }

    printf("[test_tof] 初始化完成，开始读取距离（mm）...\r\n");

    while (1) {
        uint16_t dist = 0;
        if (tof_read_distance_mm(&dist)) {
            printf("TOF,%lu,%u\r\n", (unsigned long)HAL_GetTick(), dist);
        } else {
            printf("TOF ERR\r\n");
        }
        HAL_Delay(100);
    }
}
