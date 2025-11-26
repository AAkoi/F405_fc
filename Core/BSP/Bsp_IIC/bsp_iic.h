#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"
#include <stdint.h>
#include <stdbool.h>

// I2C句柄声明
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3;

// I2C IT完成标志位
extern volatile uint8_t i2c1_it_flag;

// I2C状态枚举
typedef enum {
    I2C_STATE_IDLE = 0,
    I2C_STATE_BUSY,
    I2C_STATE_DONE,
    I2C_STATE_ERROR
} i2c_state_t;

// 获取I2C状态
i2c_state_t bsp_i2c_get_state(void);

// 初始化函数
void MX_I2C1_Init(void);
void MX_I2C3_Init(void);

// I2C读写函数（阻塞）
uint8_t bsp_i2c_read_reg(uint8_t dev_addr, uint8_t reg);
void bsp_i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t value);
void bsp_i2c_read_burst(uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint16_t len);

// I2C非阻塞读写函数
bool bsp_i2c_write_reg_start(uint8_t dev_addr, uint8_t reg, uint8_t value);
bool bsp_i2c_read_burst_start(uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint16_t len);
bool bsp_i2c_is_busy(void);

#endif // BSP_IIC_H
