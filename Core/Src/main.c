#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "bsp_System.h"
#include "bsp_IO.h"
#include "bsp_spi.h"
#include "bsp_iic.h"
#include "bsp_uart.h"
#include "icm42688p.h"
#include "attitude.h"
#include "bmp280.h"
#include "hmc5883l.h"
#include "test_icm42688p.h"
#include "test_bmp280.h"
#include "test_hmc5883l.h"
#include "test_attitude_mag.h"
#include "test_imu.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();
    BSP_UART_Init();

    printf("\r\n[boot] sensor stream starting...\r\n");

    icm42688p_init_driver();
    bmp280_init_driver();
    float bmp_t = 0.0f, bmp_alt = 0.0f;
    int32_t bmp_p = 0;
    const bool bmp_ok = bmp280_get_all(&bmp_t, &bmp_p, &bmp_alt);
    const bool hmc_ok = hmc5883l_init_driver();
    Attitude_Init();

    printf("[init] IMU ready\r\n");
    printf("[init] BMP280 %s\r\n", bmp_ok ? "ready" : "not detected");
    printf("[init] HMC5883L %s\r\n", hmc_ok ? "ready" : "not detected");

    // 如果需要上位机(html)可视化，发送统一数据格式
    printf("[mode] streaming ACC/GYR/MAG/BAR to serial\r\n");
    //test_attitude_mag_stream();

    // IMU 滤波/降采样测试，输出 IMU_CSV 便于 html 回放作图
    printf("[mode] gyro filtered/decimated test with IMU_CSV playback\r\n");
    test_imu_gyro_attitude();

    while (1) {
    }
}
