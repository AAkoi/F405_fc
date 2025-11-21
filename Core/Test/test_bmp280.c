/**
 * @file    test_bmp280.c
 * @brief   BMP280气压传感器测试实现
 */

#include "test_bmp280.h"
#include "bmp280.h"
#include <stdio.h>

/**
 * @brief BMP280完整测试循环
 */
void test_bmp280_full(void)
{
    printf("\r\n========================================\r\n");
    printf("       BMP280 气压传感器测试\r\n");
    printf("========================================\r\n\r\n");
    
    // 设置海平面气压（用于计算高度）
    // 标准大气压: 101325 Pa
    // 可根据当地天气调整，查询网站：https://www.weather.com.cn
    bmp280_set_sea_level_pressure_pa(101325.0f);
    printf("海平面气压设置为: 101325 Pa (标准大气压)\r\n");
    printf("如需精确高度，请根据当地实际气压调整\r\n\r\n");
    
    HAL_Delay(1000);
    
    printf("开始连续读取数据...\r\n");
    printf("------------------------------------------------------------\r\n");
    printf("  次数  |  温度(°C)  |  气压(Pa)  |  高度(m)  \r\n");
    printf("------------------------------------------------------------\r\n");
    
    uint32_t count = 0;
    uint32_t fail_count = 0;
    
    while (1)
    {
        float temperature = 0.0f;
        int32_t pressure = 0;
        float altitude = 0.0f;
        
        // 读取所有数据
        if (bmp280_get_all(&temperature, &pressure, &altitude))
        {
            // 成功读取
            fail_count = 0;
            count++;
            
            // 格式化输出（使用整数避免浮点数printf问题）
            int temp_int = (int)temperature;
            int temp_frac = (int)((temperature - temp_int) * 100);
            if (temp_frac < 0) temp_frac = -temp_frac;
            
            int alt_int = (int)altitude;
            int alt_frac = (int)((altitude - alt_int) * 100);
            if (alt_frac < 0) alt_frac = -alt_frac;
            
            printf("  %4d  | %3d.%02d °C | %6d Pa | %4d.%02d m\r\n",
                   (int)count,
                   temp_int, temp_frac,
                   (int)pressure,
                   alt_int, alt_frac);
            
            // 每10次输出一些额外信息
            if (count % 10 == 0) {
                printf("------------------------------------------------------------\r\n");
                printf("  统计: 已读取 %d 次 | 气压: %.2f hPa (百帕)\r\n",
                       (int)count, (float)pressure / 100.0f);
                printf("------------------------------------------------------------\r\n");
            }
        }
        else
        {
            // 读取失败
            fail_count++;
            printf("  [错误] 读取失败 (连续失败: %d 次)\r\n", (int)fail_count);
            
            if (fail_count >= 5) {
                printf("\r\n[严重错误] 连续读取失败！\r\n");
                printf("可能原因:\r\n");
                printf("  1. I2C连接断开\r\n");
                printf("  2. 传感器未正确初始化\r\n");
                printf("  3. 传感器故障\r\n");
                printf("\r\n正在尝试重新初始化...\r\n");
                
                bmp280_init_driver();
                fail_count = 0;
                HAL_Delay(1000);
            }
        }
        
        HAL_Delay(500); // 每0.5秒读取一次
    }
}

/**
 * @brief BMP280单次数据读取测试
 */
bool test_bmp280_single_read(void)
{
    float temperature = 0.0f;
    int32_t pressure = 0;
    float altitude = 0.0f;
    
    if (bmp280_get_all(&temperature, &pressure, &altitude))
    {
        printf("\r\n[BMP280 数据]\r\n");
        printf("  温度: %.2f °C\r\n", temperature);
        printf("  气压: %d Pa (%.2f hPa)\r\n", (int)pressure, (float)pressure / 100.0f);
        printf("  高度: %.2f m\r\n", altitude);
        return true;
    }
    else
    {
        printf("\r\n[BMP280 错误] 读取失败\r\n");
        return false;
    }
}

