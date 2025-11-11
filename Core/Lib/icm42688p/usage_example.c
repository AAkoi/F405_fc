/**
 * @file    usage_example.c
 * @brief   ICM42688P 使用示例
 * @note    这个文件展示如何使用ICM42688P读取数据
 */

#include "icm42688p.h"
#include <stdio.h>

/**
 * @brief 示例1：在main函数中初始化ICM42688P
 */
void example_init(void)
{
    // 在main函数或初始化函数中调用
    icm42688p_init_driver();
    
    // 初始化完成后，芯片已经配置好并开始工作
}

/**
 * @brief 示例2：读取陀螺仪数据
 */
void example_read_gyro(void)
{
    int16_t gyro_x, gyro_y, gyro_z;
    
    if (icm42688p_get_gyro_data(&gyro_x, &gyro_y, &gyro_z))
    {
        printf("Gyro: X=%d, Y=%d, Z=%d\r\n", gyro_x, gyro_y, gyro_z);
    }
    else
    {
        printf("Failed to read gyro data\r\n");
    }
}

/**
 * @brief 示例3：读取加速度计数据
 */
void example_read_accel(void)
{
    int16_t accel_x, accel_y, accel_z;
    
    if (icm42688p_get_accel_data(&accel_x, &accel_y, &accel_z))
    {
        printf("Accel: X=%d, Y=%d, Z=%d\r\n", accel_x, accel_y, accel_z);
    }
    else
    {
        printf("Failed to read accel data\r\n");
    }
}

/**
 * @brief 示例4：读取温度数据
 */
void example_read_temperature(void)
{
    float temperature;
    
    if (icm42688p_get_temperature(&temperature))
    {
        printf("Temperature: %.2f °C\r\n", temperature);
    }
    else
    {
        printf("Failed to read temperature\r\n");
    }
}

/**
 * @brief 示例5：一次性读取所有数据（推荐方式，效率最高）
 */
void example_read_all(void)
{
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t accel_x, accel_y, accel_z;
    float temperature;
    
    if (icm42688p_get_all_data(&gyro_x, &gyro_y, &gyro_z,
                               &accel_x, &accel_y, &accel_z,
                               &temperature))
    {
        printf("===== ICM42688P Data =====\r\n");
        printf("Gyro:  X=%6d, Y=%6d, Z=%6d\r\n", gyro_x, gyro_y, gyro_z);
        printf("Accel: X=%6d, Y=%6d, Z=%6d\r\n", accel_x, accel_y, accel_z);
        printf("Temp:  %.2f °C\r\n", temperature);
        printf("==========================\r\n");
    }
    else
    {
        printf("Failed to read sensor data\r\n");
    }
}

/**
 * @brief 示例6：在定时器中断或主循环中周期性读取数据
 */
void example_periodic_read(void)
{
    // 方法1：在定时器中断中调用（例如1kHz定时器）
    // void TIM_IRQHandler(void)
    // {
    //     if (timer_flag)
    //     {
    //         example_read_all();
    //     }
    // }
    
    // 方法2：在主循环中调用
    // while(1)
    // {
    //     example_read_all();
    //     HAL_Delay(10);  // 每10ms读取一次
    // }
}

/**
 * @brief 示例7：将原始数据转换为物理单位
 */
void example_convert_to_physical_units(void)
{
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t accel_x, accel_y, accel_z;
    float temperature;
    
    if (icm42688p_get_all_data(&gyro_x, &gyro_y, &gyro_z,
                               &accel_x, &accel_y, &accel_z,
                               &temperature))
    {
        // 陀螺仪：±2000 dps 量程，16位ADC
        // 灵敏度 = 65536 / (2 * 2000) = 16.384 LSB/dps
        float gyro_scale = 16.384f;
        float gyro_x_dps = gyro_x / gyro_scale;
        float gyro_y_dps = gyro_y / gyro_scale;
        float gyro_z_dps = gyro_z / gyro_scale;
        
        // 加速度计：±16g 量程，16位ADC
        // 灵敏度 = 65536 / (2 * 16) = 2048 LSB/g
        float accel_scale = 2048.0f;
        float accel_x_g = accel_x / accel_scale;
        float accel_y_g = accel_y / accel_scale;
        float accel_z_g = accel_z / accel_scale;
        
        printf("Gyro (dps):   X=%.2f, Y=%.2f, Z=%.2f\r\n", 
               gyro_x_dps, gyro_y_dps, gyro_z_dps);
        printf("Accel (g):    X=%.2f, Y=%.2f, Z=%.2f\r\n", 
               accel_x_g, accel_y_g, accel_z_g);
        printf("Temperature:  %.2f °C\r\n", temperature);
    }
}

/**
 * @brief 完整的main函数示例
 */
void complete_main_example(void)
{
    // 系统初始化
    // HAL_Init();
    // SystemClock_Config();
    // MX_GPIO_Init();
    // MX_SPI1_Init();
    // MX_USART1_UART_Init();
    
    // 初始化ICM42688P
    icm42688p_init_driver();
    
    // 主循环
    while(1)
    {
        // 读取所有传感器数据
        int16_t gyro_x, gyro_y, gyro_z;
        int16_t accel_x, accel_y, accel_z;
        float temperature;
        
        if (icm42688p_get_all_data(&gyro_x, &gyro_y, &gyro_z,
                                   &accel_x, &accel_y, &accel_z,
                                   &temperature))
        {
            // 数据处理...
            printf("Gyro: %d,%d,%d | Accel: %d,%d,%d | Temp: %.1f\r\n",
                   gyro_x, gyro_y, gyro_z,
                   accel_x, accel_y, accel_z,
                   temperature);
        }
        
        HAL_Delay(100);  // 100ms读取一次
    }
}
