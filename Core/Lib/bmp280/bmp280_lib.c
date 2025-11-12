/**
 * @file    bmp280_lib.c
 * @brief   BMP280 气压计库实现文件
 * @author  Based on Betaflight implementation
 * @date    2025
 */

#include "bmp280_lib.h"
#include <string.h>
#include <math.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 读取寄存器（通用接口）
 */
static uint8_t bmp280_read_reg(bmp280_dev_t *dev, uint8_t reg)
{
    if (dev->interface == BMP280_INTERFACE_SPI) {
        return dev->spi_read_reg(reg);
    } else {
        return dev->i2c_read_reg(dev->i2c_addr, reg);
    }
}

/**
 * @brief 写入寄存器（通用接口）
 */
static void bmp280_write_reg(bmp280_dev_t *dev, uint8_t reg, uint8_t value)
{
    if (dev->interface == BMP280_INTERFACE_SPI) {
        dev->spi_write_reg(reg, value);
    } else {
        dev->i2c_write_reg(dev->i2c_addr, reg, value);
    }
}

/**
 * @brief 读取多个寄存器（通用接口）
 */
static void bmp280_read_burst(bmp280_dev_t *dev, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    if (dev->interface == BMP280_INTERFACE_SPI) {
        dev->spi_read_burst(reg, buffer, len);
    } else {
        dev->i2c_read_burst(dev->i2c_addr, reg, buffer, len);
    }
}

/**
 * @brief 读取校准数据
 */
static bool bmp280_read_calibration(bmp280_dev_t *dev)
{
    bmp280_read_burst(dev, BMP280_REG_CALIB_START, (uint8_t *)&dev->calib, BMP280_CALIB_DATA_LENGTH);
    return true;
}

/**
 * @brief 温度补偿计算
 * @return 温度（0.01°C）
 */
static int32_t bmp280_compensate_temperature(bmp280_dev_t *dev, int32_t adc_T)
{
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)dev->calib.dig_T1 << 1))) * ((int32_t)dev->calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dev->calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)dev->calib.dig_T1))) >> 12) * ((int32_t)dev->calib.dig_T3)) >> 14;
    
    dev->t_fine = var1 + var2;
    T = (dev->t_fine * 5 + 128) >> 8;
    
    return T;
}

/**
 * @brief 压力补偿计算
 * @return 压力（Pa，Q24.8 格式）
 */
static uint32_t bmp280_compensate_pressure(bmp280_dev_t *dev, int32_t adc_P)
{
    int64_t var1, var2, p;
    
    var1 = ((int64_t)dev->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)dev->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)dev->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)dev->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dev->calib.dig_P3) >> 8) + ((var1 * (int64_t)dev->calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dev->calib.dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0; // 避免除零
    }
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dev->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dev->calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dev->calib.dig_P7) << 4);
    
    return (uint32_t)p;
}

/* ============================================================================
 * 公共 API 函数
 * ============================================================================ */

/**
 * @brief 检测 BMP280 设备（SPI 接口）
 */
bool bmp280_detect_spi(bmp280_dev_t *dev)
{
    if (!dev || !dev->spi_read_reg || !dev->delay_ms) {
        return false;
    }
    
    dev->interface = BMP280_INTERFACE_SPI;
    dev->delay_ms(20);
    
    // 读取芯片 ID
    dev->chip_id = bmp280_read_reg(dev, BMP280_REG_CHIP_ID);
    
    if (dev->chip_id != BMP280_CHIP_ID && dev->chip_id != BME280_CHIP_ID) {
        return false;
    }
    
    return true;
}

/**
 * @brief 检测 BMP280 设备（I2C 接口）
 */
bool bmp280_detect_i2c(bmp280_dev_t *dev, uint8_t addr)
{
    if (!dev || !dev->i2c_read_reg || !dev->delay_ms) {
        return false;
    }
    
    dev->interface = BMP280_INTERFACE_I2C;
    dev->i2c_addr = addr;
    dev->delay_ms(20);
    
    // 读取芯片 ID
    dev->chip_id = bmp280_read_reg(dev, BMP280_REG_CHIP_ID);
    
    if (dev->chip_id != BMP280_CHIP_ID && dev->chip_id != BME280_CHIP_ID) {
        return false;
    }
    
    return true;
}

/**
 * @brief 软复位 BMP280
 */
void bmp280_reset(bmp280_dev_t *dev)
{
    bmp280_write_reg(dev, BMP280_REG_RESET, BMP280_RESET_VALUE);
    dev->delay_ms(10);
}

/**
 * @brief 配置 BMP280
 */
void bmp280_configure(bmp280_dev_t *dev, const bmp280_config_t *config)
{
    if (!dev || !config) {
        return;
    }
    
    // 保存配置
    dev->config = *config;
    
    // 配置 CONFIG 寄存器（IIR 滤波器 + 待机时间）
    uint8_t config_reg = (config->standby << 5) | (config->filter << 2);
    bmp280_write_reg(dev, BMP280_REG_CONFIG, config_reg);
    
    // 配置 CTRL_MEAS 寄存器（过采样 + 模式）
    uint8_t ctrl_meas = (config->temp_oversamp << 5) | (config->press_oversamp << 2) | config->mode;
    bmp280_write_reg(dev, BMP280_REG_CTRL_MEAS, ctrl_meas);
}

/**
 * @brief 初始化 BMP280 设备
 */
bool bmp280_init(bmp280_dev_t *dev)
{
    if (!dev) {
        return false;
    }
    
    // 检测设备（已在 detect 函数中完成）
    if (dev->chip_id != BMP280_CHIP_ID && dev->chip_id != BME280_CHIP_ID) {
        return false;
    }
    
    // 软复位
    bmp280_reset(dev);
    
    // 读取校准数据
    if (!bmp280_read_calibration(dev)) {
        return false;
    }
    
    // 设置默认海平面气压
    dev->sea_level_pressure = 101325.0f;
    
    // 如果没有配置，使用默认配置
    if (dev->config.mode == 0) {
        dev->config = bmp280_get_default_config();
    }
    
    // 应用配置
    bmp280_configure(dev, &dev->config);
    
    return true;
}

/**
 * @brief 启动单次测量（强制模式）
 */
void bmp280_start_measurement(bmp280_dev_t *dev)
{
    uint8_t ctrl_meas = (dev->config.temp_oversamp << 5) | 
                        (dev->config.press_oversamp << 2) | 
                        BMP280_MODE_FORCED;
    bmp280_write_reg(dev, BMP280_REG_CTRL_MEAS, ctrl_meas);
}

/**
 * @brief 读取原始数据
 */
bool bmp280_read_raw(bmp280_dev_t *dev)
{
    uint8_t data[6];
    
    // 读取压力和温度数据（6 字节）
    bmp280_read_burst(dev, BMP280_REG_PRESS_MSB, data, 6);
    
    // 解析压力数据（20 位）
    dev->adc_P = (int32_t)(data[0] << 12 | data[1] << 4 | data[2] >> 4);
    
    // 解析温度数据（20 位）
    dev->adc_T = (int32_t)(data[3] << 12 | data[4] << 4 | data[5] >> 4);
    
    return true;
}

/**
 * @brief 计算温度和压力
 */
void bmp280_calculate(bmp280_dev_t *dev, bmp280_data_t *data)
{
    if (!dev || !data) {
        return;
    }
    
    // 计算温度（必须先计算温度，因为压力补偿需要 t_fine）
    data->temperature = bmp280_compensate_temperature(dev, dev->adc_T);
    
    // 计算压力
    uint32_t p_q24_8 = bmp280_compensate_pressure(dev, dev->adc_P);
    data->pressure = (int32_t)(p_q24_8 / 256);
    
    // 计算海拔高度
    data->altitude = bmp280_calculate_altitude((float)data->pressure, dev->sea_level_pressure);
}

/**
 * @brief 读取温度和压力（一次性读取）
 */
bool bmp280_read(bmp280_dev_t *dev, bmp280_data_t *data)
{
    if (!dev || !data) {
        return false;
    }
    
    // 如果是强制模式，启动测量
    if (dev->config.mode == BMP280_MODE_FORCED) {
        bmp280_start_measurement(dev);
        
        // 等待测量完成
        uint32_t delay = bmp280_get_measurement_delay(dev);
        dev->delay_ms(delay);
    }
    
    // 读取原始数据
    if (!bmp280_read_raw(dev)) {
        return false;
    }
    
    // 计算结果
    bmp280_calculate(dev, data);
    
    return true;
}

/**
 * @brief 检查设备是否正在测量
 */
bool bmp280_is_measuring(bmp280_dev_t *dev)
{
    uint8_t status = bmp280_read_reg(dev, BMP280_REG_STATUS);
    return (status & BMP280_STATUS_MEASURING) != 0;
}

/**
 * @brief 设置海平面气压
 */
void bmp280_set_sea_level_pressure(bmp280_dev_t *dev, float pressure)
{
    if (dev) {
        dev->sea_level_pressure = pressure;
    }
}

/**
 * @brief 计算海拔高度
 * 
 * 使用国际标准大气压公式：
 * h = 44330 * (1 - (P/P0)^(1/5.255))
 * 
 * 其中：
 * - h: 海拔高度（m）
 * - P: 当前气压（Pa）
 * - P0: 海平面气压（Pa）
 */
float bmp280_calculate_altitude(float pressure, float sea_level_pressure)
{
    if (pressure <= 0 || sea_level_pressure <= 0) {
        return 0.0f;
    }
    
    return 44330.0f * (1.0f - powf(pressure / sea_level_pressure, 0.1903f));
}

/**
 * @brief 获取测量延迟时间
 * 
 * 根据数据手册：
 * - T_init_max = 20 / 16 = 1.25 ms
 * - T_measure = 2.3 ms * (temp_oversamp + press_oversamp)
 * - T_setup = 0.625 ms
 */
uint32_t bmp280_get_measurement_delay(const bmp280_dev_t *dev)
{
    if (!dev) {
        return 0;
    }
    
    // 初始化时间（ms）
    float t_init = 1.25f;
    
    // 测量时间（ms）
    uint8_t temp_samples = (1 << dev->config.temp_oversamp) >> 1;
    uint8_t press_samples = (1 << dev->config.press_oversamp) >> 1;
    float t_measure = 2.3f * (temp_samples + press_samples);
    
    // 设置时间（ms）
    float t_setup = (dev->config.press_oversamp > 0) ? 0.625f : 0.0f;
    
    // 总延迟（向上取整）
    uint32_t delay = (uint32_t)(t_init + t_measure + t_setup + 1.0f);
    
    return delay;
}
