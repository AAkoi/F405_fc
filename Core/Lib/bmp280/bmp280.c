#include "bmp280.h"
#include "bmp280_lib.h"
#include "bsp_pins.h"

#include <stdio.h>

// BMP280 设备实例
static bmp280_dev_t bmp;

/* === I2C 底层封装 === */
uint8_t bmp_i2c_read_reg(uint8_t addr, uint8_t reg)
{
    return bsp_i2c_read_reg(addr, reg);
}

void bmp_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    bsp_i2c_write_reg(addr, reg, value);
}

void bmp_i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    bsp_i2c_read_burst(addr, reg, buffer, len);
}

void bmp_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* === 初始化 === */
void bmp280_init_driver(void)
{
    printf("========== BMP280 初始化 ==========\r\n");
    
    // 绑定 I2C 与延时函数
    bmp.i2c_read_reg  = bmp_i2c_read_reg;
    bmp.i2c_write_reg = bmp_i2c_write_reg;
    bmp.i2c_read_burst = bmp_i2c_read_burst;
    bmp.delay_ms = bmp_delay_ms;

    // 默认配置（高精度强制模式）
    bmp.config = bmp280_get_high_precision_config();

    printf("[步骤1] I2C通信测试...\r\n");
    printf("  尝试地址 0x76...\r\n");
    
    // 依次尝试 0x76 与 0x77 地址
    bool ok = false;
    if (bmp280_detect_i2c(&bmp, BMP280_I2C_ADDR_PRIMARY)) {
        printf("  ✓ 在地址 0x76 检测到设备\r\n");
        ok = true;
    } else {
        printf("  ✗ 地址 0x76 无响应\r\n");
        printf("  尝试地址 0x77...\r\n");
        
        if (bmp280_detect_i2c(&bmp, BMP280_I2C_ADDR_SECONDARY)) {
            printf("  ✓ 在地址 0x77 检测到设备\r\n");
            ok = true;
        } else {
            printf("  ✗ 地址 0x77 无响应\r\n");
        }
    }

    if (!ok) {
        printf("\r\n[错误] BMP280 未找到！\r\n");
        printf("可能原因:\r\n");
        printf("  1. I2C接线错误 (SDA/SCL)\r\n");
        printf("  2. 传感器未上电\r\n");
        printf("  3. I2C地址不是 0x76 或 0x77\r\n");
        printf("  4. I2C上拉电阻缺失\r\n");
        printf("  5. I2C时钟频率过高\r\n");
        printf("=====================================\r\n\r\n");
        return;
    }

    printf("\r\n[步骤2] 初始化传感器...\r\n");
    
    // 初始化设备并应用配置
    if (bmp280_init(&bmp)) {
        printf("  ✓ BMP280 初始化成功!\r\n");
        printf("    - Chip ID: 0x%02X", bmp.chip_id);
        
        if (bmp.chip_id == 0x58) {
            printf(" (BMP280)\r\n");
        } else if (bmp.chip_id == 0x60) {
            printf(" (BME280)\r\n");
        } else {
            printf(" (未知型号)\r\n");
        }
        
        printf("    - I2C地址: 0x%02X\r\n", bmp.i2c_addr);
        printf("    - 工作模式: 高精度强制模式\r\n");
        printf("    - 过采样: 温度x2, 气压x16\r\n");
    } else {
        printf("  ✗ BMP280 初始化失败!\r\n");
        printf("    传感器检测到但配置失败\r\n");
    }
    
    printf("=====================================\r\n\r\n");
}

/* === 读取接口 === */
bool bmp280_get_temperature(float *temp_celsius)
{
    if (!temp_celsius) return false;

    bmp280_data_t data;
    if (!bmp280_read(&bmp, &data)) {
        return false;
    }
    *temp_celsius = data.temperature / 100.0f; // 0.01°C → °C
    return true;
}

bool bmp280_get_pressure(int32_t *pressure_pa)
{
    if (!pressure_pa) return false;

    bmp280_data_t data;
    if (!bmp280_read(&bmp, &data)) {
        return false;
    }
    *pressure_pa = data.pressure; // Pa
    return true;
}

bool bmp280_get_altitude(float *altitude_m)
{
    if (!altitude_m) return false;

    bmp280_data_t data;
    if (!bmp280_read(&bmp, &data)) {
        return false;
    }
    *altitude_m = data.altitude;
    return true;
}

bool bmp280_get_all(float *temp_celsius, int32_t *pressure_pa, float *altitude_m)
{
    bmp280_data_t data;
    if (!bmp280_read(&bmp, &data)) {
        return false;
    }

    if (temp_celsius) {
        *temp_celsius = data.temperature / 100.0f;
    }
    if (pressure_pa) {
        *pressure_pa = data.pressure;
    }
    if (altitude_m) {
        *altitude_m = data.altitude;
    }
    return true;
}

/* === 参数设置 === */
void bmp280_set_sea_level_pressure_pa(float sea_level_pressure_pa)
{
    bmp280_set_sea_level_pressure(&bmp, sea_level_pressure_pa);
}
