/**
 * @file    hmc5883l_lib.c
 * @brief   HMC5883L 三轴磁力计库实现文件
 * @author  Based on Betaflight implementation
 * @date    2025
 */

#include "hmc5883l_lib.h"
#include <string.h>
#include <math.h>

/* ============================================================================
 * 内部辅助函数
 * ============================================================================ */

/**
 * @brief 读取寄存器
 */
static uint8_t hmc5883l_read_reg(hmc5883l_dev_t *dev, uint8_t reg)
{
    return dev->i2c_read_reg(dev->i2c_addr, reg);
}

/**
 * @brief 写入寄存器
 */
static void hmc5883l_write_reg(hmc5883l_dev_t *dev, uint8_t reg, uint8_t value)
{
    dev->i2c_write_reg(dev->i2c_addr, reg, value);
}

/**
 * @brief 读取多个寄存器
 */
static void hmc5883l_read_burst(hmc5883l_dev_t *dev, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    dev->i2c_read_burst(dev->i2c_addr, reg, buffer, len);
}

/* ============================================================================
 * 公共 API 函数
 * ============================================================================ */

/**
 * @brief 检测 HMC5883L 设备
 */
bool hmc5883l_detect(hmc5883l_dev_t *dev)
{
    if (!dev || !dev->i2c_read_reg || !dev->delay_ms) {
        return false;
    }
    
    dev->delay_ms(20);
    
    // 读取 ID 寄存器 A
    uint8_t id_a = hmc5883l_read_reg(dev, HMC5883L_REG_IDA);
    
    if (id_a != HMC5883L_DEVICE_ID) {
        return false;
    }
    
    // 读取 ID 寄存器 B 和 C 进行进一步验证
    uint8_t id_b = hmc5883l_read_reg(dev, HMC5883L_REG_IDB);
    uint8_t id_c = hmc5883l_read_reg(dev, HMC5883L_REG_IDC);
    
    // ID 寄存器 B 应该是 '4' (0x34), C 应该是 '3' (0x33)
    if (id_b != 0x34 || id_c != 0x33) {
        return false;
    }
    
    return true;
}

/**
 * @brief 配置 HMC5883L
 */
void hmc5883l_configure(hmc5883l_dev_t *dev, const hmc5883l_config_t *config)
{
    if (!dev || !config) {
        return;
    }
    
    // 保存配置
    dev->config = *config;
    
    // 配置寄存器 A：采样数 + 输出率 + 测量模式
    uint8_t reg_a = (config->samples << 5) | (config->odr << 2) | config->meas_mode;
    hmc5883l_write_reg(dev, HMC5883L_REG_CONFA, reg_a);
    
    // 配置寄存器 B：增益
    uint8_t reg_b = config->gain << 5;
    hmc5883l_write_reg(dev, HMC5883L_REG_CONFB, reg_b);
    
    // 更新增益比例因子
    dev->gain_scale = hmc5883l_get_gain_scale(config->gain);
    
    // 配置工作模式
    hmc5883l_set_mode(dev, config->mode);
}

/**
 * @brief 初始化 HMC5883L 设备
 */
bool hmc5883l_init(hmc5883l_dev_t *dev)
{
    if (!dev) {
        return false;
    }
    
    // 检测设备
    if (!hmc5883l_detect(dev)) {
        return false;
    }
    
    // 如果没有配置，使用默认配置
    if (dev->config.odr == 0) {
        dev->config = hmc5883l_get_default_config();
    }
    
    // 应用配置
    hmc5883l_configure(dev, &dev->config);
    
    // 等待设备稳定
    dev->delay_ms(100);
    
    return true;
}

/**
 * @brief 设置工作模式
 */
void hmc5883l_set_mode(hmc5883l_dev_t *dev, hmc5883l_mode_t mode)
{
    hmc5883l_write_reg(dev, HMC5883L_REG_MODE, mode);
}

/**
 * @brief 读取原始磁场数据
 */
bool hmc5883l_read_raw(hmc5883l_dev_t *dev, hmc5883l_mag_data_t *data)
{
    if (!dev || !data) {
        return false;
    }
    
    uint8_t buffer[6];
    
    // 读取 6 个字节的数据（X, Z, Y 顺序）
    hmc5883l_read_burst(dev, HMC5883L_REG_DATA_X_MSB, buffer, 6);
    
    // 解析数据（注意：HMC5883L 的数据顺序是 X, Z, Y）
    data->x = (int16_t)((buffer[0] << 8) | buffer[1]) - dev->offset[0];
    data->z = (int16_t)((buffer[2] << 8) | buffer[3]) - dev->offset[2];
    data->y = (int16_t)((buffer[4] << 8) | buffer[5]) - dev->offset[1];
    
    return true;
}

/**
 * @brief 读取磁场数据（转换为 Gauss）
 */
bool hmc5883l_read(hmc5883l_dev_t *dev, hmc5883l_mag_data_float_t *data)
{
    if (!dev || !data) {
        return false;
    }
    
    hmc5883l_mag_data_t raw_data;
    
    if (!hmc5883l_read_raw(dev, &raw_data)) {
        return false;
    }
    
    // 转换为 Gauss
    data->x = (float)raw_data.x / dev->gain_scale;
    data->y = (float)raw_data.y / dev->gain_scale;
    data->z = (float)raw_data.z / dev->gain_scale;
    
    return true;
}

/**
 * @brief 读取状态寄存器
 */
uint8_t hmc5883l_read_status(hmc5883l_dev_t *dev)
{
    return hmc5883l_read_reg(dev, HMC5883L_REG_STATUS);
}

/**
 * @brief 检查数据是否就绪
 */
bool hmc5883l_data_ready(hmc5883l_dev_t *dev)
{
    uint8_t status = hmc5883l_read_status(dev);
    return (status & HMC5883L_STATUS_RDY) != 0;
}

/**
 * @brief 执行自检
 * 
 * 自检步骤：
 * 1. 配置为正偏置模式
 * 2. 读取数据
 * 3. 检查数据是否在预期范围内
 * 4. 恢复正常模式
 */
bool hmc5883l_self_test(hmc5883l_dev_t *dev)
{
    if (!dev) {
        return false;
    }
    
    // 保存当前配置
    hmc5883l_config_t original_config = dev->config;
    
    // 配置自检模式：正偏置 + 8 次采样 + 15Hz + 2.5Ga 增益
    hmc5883l_config_t test_config = {
        .odr = HMC5883L_ODR_15HZ,
        .samples = HMC5883L_SAMPLES_8,
        .gain = HMC5883L_GAIN_2_5GA,
        .mode = HMC5883L_SINGLE,
        .meas_mode = HMC5883L_MODE_POS_BIAS
    };
    
    hmc5883l_configure(dev, &test_config);
    dev->delay_ms(100);
    
    // 读取自检数据
    hmc5883l_mag_data_float_t test_data;
    if (!hmc5883l_read(dev, &test_data)) {
        hmc5883l_configure(dev, &original_config);
        return false;
    }
    
    // 自检阈值（Gauss）
    // 对于 2.5Ga 增益，正偏置应该产生：
    // X, Y: 约 +1.16 Ga
    // Z: 约 +1.08 Ga
    const float self_test_low = 0.6f;   // 243/390 ≈ 0.62
    const float self_test_high = 1.5f;  // 575/390 ≈ 1.47
    
    bool test_passed = true;
    
    // 检查 X 轴
    if (test_data.x < self_test_low || test_data.x > self_test_high) {
        test_passed = false;
    }
    
    // 检查 Y 轴
    if (test_data.y < self_test_low || test_data.y > self_test_high) {
        test_passed = false;
    }
    
    // 检查 Z 轴
    if (test_data.z < self_test_low || test_data.z > self_test_high) {
        test_passed = false;
    }
    
    // 恢复原始配置
    hmc5883l_configure(dev, &original_config);
    dev->delay_ms(100);
    
    return test_passed;
}

/**
 * @brief 校准磁力计
 */
bool hmc5883l_calibrate(hmc5883l_dev_t *dev, uint16_t samples)
{
    if (!dev || samples == 0) {
        return false;
    }
    
    int32_t sum[3] = {0, 0, 0};
    hmc5883l_mag_data_t data;
    
    // 临时清除偏移量
    int16_t temp_offset[3];
    memcpy(temp_offset, dev->offset, sizeof(temp_offset));
    memset(dev->offset, 0, sizeof(dev->offset));
    
    // 采集样本
    for (uint16_t i = 0; i < samples; i++) {
        // 如果是单次测量模式，需要触发新的测量
        if (dev->config.mode == HMC5883L_SINGLE) {
            hmc5883l_set_mode(dev, HMC5883L_SINGLE);
            dev->delay_ms(10);
        }
        
        if (!hmc5883l_read_raw(dev, &data)) {
            memcpy(dev->offset, temp_offset, sizeof(temp_offset));
            return false;
        }
        
        sum[0] += data.x;
        sum[1] += data.y;
        sum[2] += data.z;
        
        dev->delay_ms(10);
    }
    
    // 计算平均偏移量
    dev->offset[0] = sum[0] / samples;
    dev->offset[1] = sum[1] / samples;
    dev->offset[2] = sum[2] / samples;
    
    return true;
}

/**
 * @brief 获取增益比例因子
 * 
 * 根据数据手册，不同增益对应的 LSB/Gauss：
 * - 0.88 Ga: 1370 LSB/Gauss
 * - 1.3 Ga:  1090 LSB/Gauss
 * - 1.9 Ga:  820 LSB/Gauss
 * - 2.5 Ga:  660 LSB/Gauss
 * - 4.0 Ga:  440 LSB/Gauss
 * - 4.7 Ga:  390 LSB/Gauss
 * - 5.6 Ga:  330 LSB/Gauss
 * - 8.1 Ga:  230 LSB/Gauss
 */
float hmc5883l_get_gain_scale(hmc5883l_gain_t gain)
{
    switch (gain) {
        case HMC5883L_GAIN_0_88GA: return 1370.0f;
        case HMC5883L_GAIN_1_3GA:  return 1090.0f;
        case HMC5883L_GAIN_1_9GA:  return 820.0f;
        case HMC5883L_GAIN_2_5GA:  return 660.0f;
        case HMC5883L_GAIN_4_0GA:  return 440.0f;
        case HMC5883L_GAIN_4_7GA:  return 390.0f;
        case HMC5883L_GAIN_5_6GA:  return 330.0f;
        case HMC5883L_GAIN_8_1GA:  return 230.0f;
        default: return 1090.0f;  // 默认 1.3 Ga
    }
}

