#include "bsp_spi.h"
#include "icm42688p_lib.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

// ICM42688P 设备实例
extern icm42688p_dev_t icm;

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

// 更新传感器数据（检查中断标志位并读取）
bool icm42688p_update(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                      int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                      float *temp_celsius);

// 仅做陀螺刻度转换（raw LSB -> dps）
bool icm42688p_gyro_dataPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                                    float *gyro_x_norm,float *gyro_y_norm,float *gyro_z_norm);

// 传感器数据预处理：读取原始值并给出规范化数值（gyro: dps, accel: g）
bool icm42688p_dataPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                      int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                       float *gyro_x_norm,float *gyro_y_norm,float *gyro_z_norm,
                      float *accel_x_norm,float *accel_y_norm,float *accel_z_norm,
                      float *temp_celsius);

// 数据就绪标志位（外部可访问，中断中设置）
extern volatile uint8_t icm42688p_data_ready;
extern volatile uint8_t spi1_dma_flag;
