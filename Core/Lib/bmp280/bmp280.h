#include "bsp_iic.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef BMP280_H
#define BMP280_H

// I2C 底层函数（由本驱动提供，供库绑定）
uint8_t bmp_i2c_read_reg(uint8_t addr, uint8_t reg);
void bmp_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);
void bmp_i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *buffer, uint16_t len);
void bmp_delay_ms(uint32_t ms);

// 初始化 BMP280 传感器（I2C）
void bmp280_init_driver(void);

// 数据读取接口
bool bmp280_get_temperature(float *temp_celsius);
bool bmp280_get_pressure(int32_t *pressure_pa);
bool bmp280_get_altitude(float *altitude_m);
bool bmp280_get_all(float *temp_celsius, int32_t *pressure_pa, float *altitude_m);

// 参数设置
void bmp280_set_sea_level_pressure_pa(float sea_level_pressure_pa);

#endif // BMP280_H
