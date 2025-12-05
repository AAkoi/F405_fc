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
#include "test_timer_log.h"
#include "test_scheduler.h"
#include "test_w25qxx.h"
#include "test_esrl.h"
#include "test_pwm_servo.h"

#define RUN_MODE 7 // 0: gyro+acc attitude test, 1: gyro+acc+mag attitude test, 2: magnetometer stream test, 3: gyro raw test, 4: scheduler pipeline test, 5: W25Qxx flash test, 6: ELRS/CRSF on UART2 test, 7: PWM servo test

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
    } else if(RUN_MODE==3) {
        test_gyro_run();
    } else if (RUN_MODE==4) {
        test_scheduler_run();
    } else if (RUN_MODE==5) {
        test_w25qxx_run();
    } else if (RUN_MODE==6) {
        test_esrl_run();
    } else if (RUN_MODE==7) {
        test_pwm_servo_run();
    }else{
        test_timer_log_run();
    }

    // Should never reach here
    while (1) {}
}
