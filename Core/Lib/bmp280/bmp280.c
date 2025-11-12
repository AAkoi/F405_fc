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
    // 绑定 I2C 与延时函数
    bmp.i2c_read_reg  = bmp_i2c_read_reg;
    bmp.i2c_write_reg = bmp_i2c_write_reg;
    bmp.i2c_read_burst = bmp_i2c_read_burst;
    bmp.delay_ms = bmp_delay_ms;

    // 默认配置（高精度强制模式）
    bmp.config = bmp280_get_high_precision_config();

    // 依次尝试 0x76 与 0x77 地址
    bool ok = false;
    if (bmp280_detect_i2c(&bmp, BMP280_I2C_ADDR_PRIMARY)) {
        ok = true;
    } else if (bmp280_detect_i2c(&bmp, BMP280_I2C_ADDR_SECONDARY)) {
        ok = true;
    }

    if (!ok) {
        printf("BMP280 not found on I2C (0x76/0x77)!\r\n");
        return;
    }

    // 初始化设备并应用配置
    if (bmp280_init(&bmp)) {
        printf("BMP280 detected (chip_id=0x%02X) at 0x%02X\r\n", bmp.chip_id, bmp.i2c_addr);
    } else {
        printf("BMP280 init failed!\r\n");
    }
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
