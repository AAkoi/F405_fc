/**
 * @file    hmc5883l.c
 * @brief   HMC5883L 磁力计应用层接口实现
 * @author  Your Name
 * @date    2025
 */

#include "hmc5883l.h"
#include "bsp_iic.h"
#include "bsp_pins.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * 全局变量
 * ============================================================================ */

hmc5883l_dev_t hmc_dev;

/* ============================================================================
 * I2C 通信函数（适配 BSP）
 * ============================================================================ */

/**
 * @brief I2C 读取单个寄存器（适配 I2C3）
 */
static uint8_t hmc5883l_i2c_read_reg(uint8_t addr, uint8_t reg)
{
    uint8_t data = 0;
    // 使用 HAL 直接操作 I2C3
    HAL_I2C_Mem_Read(&hi2c3, addr << 1, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    return data;
}

/**
 * @brief I2C 写入单个寄存器（适配 I2C3）
 */
static void hmc5883l_i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    // 使用 HAL 直接操作 I2C3
    HAL_I2C_Mem_Write(&hi2c3, addr << 1, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 100);
}

/**
 * @brief I2C 读取多个寄存器（适配 I2C3）
 */
static void hmc5883l_i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    // 使用 HAL 直接操作 I2C3
    HAL_I2C_Mem_Read(&hi2c3, addr << 1, reg, I2C_MEMADD_SIZE_8BIT, buffer, len, 100);
}

/**
 * @brief 延时函数
 */
static void hmc5883l_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/* ============================================================================
 * 公共 API 函数
 * ============================================================================ */

/**
 * @brief 初始化 HMC5883L 驱动
 */
bool hmc5883l_init_driver(void)
{
    // 清零设备结构体
    memset(&hmc_dev, 0, sizeof(hmc_dev));
    
    // 设置 I2C 地址
    hmc_dev.i2c_addr = HMC5883L_I2C_ADDRESS;
    
    // 注册 I2C 通信函数
    hmc_dev.i2c_read_reg = hmc5883l_i2c_read_reg;
    hmc_dev.i2c_write_reg = hmc5883l_i2c_write_reg;
    hmc_dev.i2c_read_burst = hmc5883l_i2c_read_burst;
    hmc_dev.delay_ms = hmc5883l_delay_ms;
    
    // 设置默认配置
    hmc_dev.config = hmc5883l_get_default_config();
    
    // 初始化设备
    if (!hmc5883l_init(&hmc_dev)) {
        return false;
    }
    
    return true;
}

/**
 * @brief 读取磁场数据（原始值）
 */
bool hmc5883l_read_raw_data(int16_t *mag_x, int16_t *mag_y, int16_t *mag_z)
{
    if (!mag_x || !mag_y || !mag_z) {
        return false;
    }
    
    hmc5883l_mag_data_t data;
    
    if (!hmc5883l_read_raw(&hmc_dev, &data)) {
        return false;
    }
    
    *mag_x = data.x;
    *mag_y = data.y;
    *mag_z = data.z;
    
    return true;
}

/**
 * @brief 读取磁场数据（Gauss）
 */
bool hmc5883l_read_gauss(float *mag_x, float *mag_y, float *mag_z)
{
    if (!mag_x || !mag_y || !mag_z) {
        return false;
    }
    
    hmc5883l_mag_data_float_t data;
    
    if (!hmc5883l_read(&hmc_dev, &data)) {
        return false;
    }
    
    *mag_x = data.x;
    *mag_y = data.y;
    *mag_z = data.z;
    
    return true;
}

/**
 * @brief 计算航向角（偏航角）
 * 
 * 航向角计算公式：
 * heading = atan2(y, x) * 180 / PI
 * 
 * 注意：此函数假设磁力计水平放置
 */
float hmc5883l_get_heading(void)
{
    float mag_x, mag_y, mag_z;
    
    if (!hmc5883l_read_gauss(&mag_x, &mag_y, &mag_z)) {
        return 0.0f;
    }
    
    // 计算航向角（弧度）
    float heading = atan2f(mag_y, mag_x);
    
    // 转换为度数
    heading = heading * 180.0f / M_PI;
    
    // 归一化到 0-360 度
    if (heading < 0) {
        heading += 360.0f;
    }
    
    return heading;
}

/**
 * @brief 计算倾斜补偿后的航向角
 * 
 * 当磁力计倾斜时，需要使用加速度计数据进行补偿
 * 
 * @param roll 横滚角（度）
 * @param pitch 俯仰角（度）
 * @return 航向角（度），范围 0-360
 */
float hmc5883l_get_tilt_compensated_heading(float roll, float pitch)
{
    float mag_x, mag_y, mag_z;
    
    if (!hmc5883l_read_gauss(&mag_x, &mag_y, &mag_z)) {
        return 0.0f;
    }
    
    // 转换为弧度
    float roll_rad = roll * M_PI / 180.0f;
    float pitch_rad = pitch * M_PI / 180.0f;
    
    // 计算倾斜补偿后的磁场分量
    float mag_x_comp = mag_x * cosf(pitch_rad) + mag_z * sinf(pitch_rad);
    float mag_y_comp = mag_x * sinf(roll_rad) * sinf(pitch_rad) + 
                       mag_y * cosf(roll_rad) - 
                       mag_z * sinf(roll_rad) * cosf(pitch_rad);
    
    // 计算航向角（弧度）
    float heading = atan2f(mag_y_comp, mag_x_comp);
    
    // 转换为度数
    heading = heading * 180.0f / M_PI;
    
    // 归一化到 0-360 度
    if (heading < 0) {
        heading += 360.0f;
    }
    
    return heading;
}

/**
 * @brief 执行磁力计校准
 */
bool hmc5883l_calibrate_compass(uint16_t samples)
{
    return hmc5883l_calibrate(&hmc_dev, samples);
}

/**
 * @brief 执行自检
 */
bool hmc5883l_run_self_test(void)
{
    return hmc5883l_self_test(&hmc_dev);
}

/**
 * @brief 检查数据是否就绪
 */
bool hmc5883l_is_data_ready(void)
{
    return hmc5883l_data_ready(&hmc_dev);
}

