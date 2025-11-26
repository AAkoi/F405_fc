#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "bsp_System.h"
#include "bsp_IO.h"
#include "bsp_spi.h"
#include "bsp_iic.h"
#include "bsp_uart.h"
#include "test_gyro.h"
#include "test_attitude_full.h"
#include "test_mag.h"

#define RUN_MODE 1  // 0: gyro+acc attitude test, 1: gyro+acc+mag attitude test, 2: magnetometer stream test

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_I2C3_Init();
    BSP_UART_Init();
    cycleCounterInit();

    printf("\r\n[boot] System Ready\r\n");

    if (RUN_MODE == 1) {
        test_attitude_full_run();
    } else if (RUN_MODE == 2) {
        test_mag_run();
    } else {
        test_gyro_run();
    }

    // Should never reach here
    while (1) {}
}
