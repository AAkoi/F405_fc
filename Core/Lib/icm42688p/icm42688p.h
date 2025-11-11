#include "bsp_spi.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>


// SPI底层函数
void icm_spi_write_reg(uint8_t reg, uint8_t value);
uint8_t icm_spi_read_reg(uint8_t reg);
void icm_spi_read_burst(uint8_t reg, uint8_t *buffer, uint16_t len);
void icm_delay_ms(uint32_t ms);

// 初始化函数
void icm42688p_init_driver(void);

// 数据读取函数
bool icm42688p_get_gyro_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);
bool icm42688p_get_accel_data(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z);
bool icm42688p_get_temperature(float *temp_celsius);
bool icm42688p_get_all_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                            int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                            float *temp_celsius);
