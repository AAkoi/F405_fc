/**
 * @file    hmc5883l_lib.h
 * @brief   HMC5883L 三轴磁力计库头文件
 * @author  Based on Betaflight implementation
 * @date    2025
 * 
 * 功能特性：
 * - 支持 HMC5883L 三轴数字磁力计
 * - I2C 接口（地址 0x1E）
 * - 可配置的输出数据率（0.75Hz - 75Hz）
 * - 可配置的增益（0.88Ga - 8.1Ga）
 * - 连续测量模式和单次测量模式
 * - 自检功能
 */

#ifndef HMC5883L_LIB_H
#define HMC5883L_LIB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 寄存器定义
 * ============================================================================ */

// HMC5883L I2C 地址
#define HMC5883L_I2C_ADDRESS        0x1E

// 设备 ID
#define HMC5883L_DEVICE_ID          0x48

// 寄存器地址
#define HMC5883L_REG_CONFA          0x00    // 配置寄存器 A
#define HMC5883L_REG_CONFB          0x01    // 配置寄存器 B
#define HMC5883L_REG_MODE           0x02    // 模式寄存器
#define HMC5883L_REG_DATA_X_MSB     0x03    // X 轴数据 MSB
#define HMC5883L_REG_DATA_X_LSB     0x04    // X 轴数据 LSB
#define HMC5883L_REG_DATA_Z_MSB     0x05    // Z 轴数据 MSB
#define HMC5883L_REG_DATA_Z_LSB     0x06    // Z 轴数据 LSB
#define HMC5883L_REG_DATA_Y_MSB     0x07    // Y 轴数据 MSB
#define HMC5883L_REG_DATA_Y_LSB     0x08    // Y 轴数据 LSB
#define HMC5883L_REG_STATUS         0x09    // 状态寄存器
#define HMC5883L_REG_IDA            0x0A    // ID 寄存器 A
#define HMC5883L_REG_IDB            0x0B    // ID 寄存器 B
#define HMC5883L_REG_IDC            0x0C    // ID 寄存器 C

/* ============================================================================
 * 配置常量
 * ============================================================================ */

// 数据输出率（配置寄存器 A，位 4:2）
typedef enum {
    HMC5883L_ODR_0_75HZ  = 0x00,    // 0.75 Hz
    HMC5883L_ODR_1_5HZ   = 0x01,    // 1.5 Hz
    HMC5883L_ODR_3HZ     = 0x02,    // 3 Hz
    HMC5883L_ODR_7_5HZ   = 0x03,    // 7.5 Hz
    HMC5883L_ODR_15HZ    = 0x04,    // 15 Hz（默认）
    HMC5883L_ODR_30HZ    = 0x05,    // 30 Hz
    HMC5883L_ODR_75HZ    = 0x06     // 75 Hz
} hmc5883l_odr_t;

// 测量模式（配置寄存器 A，位 1:0）
typedef enum {
    HMC5883L_MODE_NORMAL     = 0x00,    // 正常测量模式
    HMC5883L_MODE_POS_BIAS   = 0x01,    // 正偏置自检模式
    HMC5883L_MODE_NEG_BIAS   = 0x02     // 负偏置自检模式
} hmc5883l_measurement_mode_t;

// 采样平均数（配置寄存器 A，位 6:5）
typedef enum {
    HMC5883L_SAMPLES_1   = 0x00,    // 1 次采样（默认）
    HMC5883L_SAMPLES_2   = 0x01,    // 2 次采样平均
    HMC5883L_SAMPLES_4   = 0x02,    // 4 次采样平均
    HMC5883L_SAMPLES_8   = 0x03     // 8 次采样平均
} hmc5883l_samples_t;

// 增益配置（配置寄存器 B，位 7:5）
typedef enum {
    HMC5883L_GAIN_0_88GA = 0x00,    // ±0.88 Ga, 1370 LSB/Gauss
    HMC5883L_GAIN_1_3GA  = 0x01,    // ±1.3 Ga,  1090 LSB/Gauss（默认）
    HMC5883L_GAIN_1_9GA  = 0x02,    // ±1.9 Ga,  820 LSB/Gauss
    HMC5883L_GAIN_2_5GA  = 0x03,    // ±2.5 Ga,  660 LSB/Gauss
    HMC5883L_GAIN_4_0GA  = 0x04,    // ±4.0 Ga,  440 LSB/Gauss
    HMC5883L_GAIN_4_7GA  = 0x05,    // ±4.7 Ga,  390 LSB/Gauss
    HMC5883L_GAIN_5_6GA  = 0x06,    // ±5.6 Ga,  330 LSB/Gauss
    HMC5883L_GAIN_8_1GA  = 0x07     // ±8.1 Ga,  230 LSB/Gauss
} hmc5883l_gain_t;

// 工作模式（模式寄存器，位 1:0）
typedef enum {
    HMC5883L_CONTINUOUS  = 0x00,    // 连续测量模式
    HMC5883L_SINGLE      = 0x01,    // 单次测量模式
    HMC5883L_IDLE        = 0x02     // 空闲模式
} hmc5883l_mode_t;

// 状态寄存器位定义
#define HMC5883L_STATUS_RDY         (1 << 0)    // 数据就绪标志
#define HMC5883L_STATUS_LOCK        (1 << 1)    // 数据输出寄存器锁定

/* ============================================================================
 * 数据结构
 * ============================================================================ */

/**
 * @brief 磁场数据结构体
 */
typedef struct {
    int16_t x;      // X 轴原始数据
    int16_t y;      // Y 轴原始数据
    int16_t z;      // Z 轴原始数据
} hmc5883l_mag_data_t;

/**
 * @brief 磁场数据（浮点型，单位：Gauss）
 */
typedef struct {
    float x;        // X 轴磁场强度（Gauss）
    float y;        // Y 轴磁场强度（Gauss）
    float z;        // Z 轴磁场强度（Gauss）
} hmc5883l_mag_data_float_t;

/**
 * @brief 配置结构体
 */
typedef struct {
    hmc5883l_odr_t odr;                     // 数据输出率
    hmc5883l_samples_t samples;             // 采样平均数
    hmc5883l_gain_t gain;                   // 增益
    hmc5883l_mode_t mode;                   // 工作模式
    hmc5883l_measurement_mode_t meas_mode;  // 测量模式
} hmc5883l_config_t;

/**
 * @brief HMC5883L 设备结构体
 */
typedef struct {
    // I2C 地址
    uint8_t i2c_addr;
    
    // 通信函数指针（用户实现）
    uint8_t (*i2c_read_reg)(uint8_t addr, uint8_t reg);
    void (*i2c_write_reg)(uint8_t addr, uint8_t reg, uint8_t value);
    void (*i2c_read_burst)(uint8_t addr, uint8_t reg, uint8_t *buffer, uint16_t len);
    void (*delay_ms)(uint32_t ms);
    
    // 配置
    hmc5883l_config_t config;
    
    // 增益比例因子（LSB per Gauss）
    float gain_scale;
    
    // 校准偏移量
    int16_t offset[3];
    
} hmc5883l_dev_t;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 检测 HMC5883L 设备
 * @param dev 设备结构体指针
 * @return true=检测成功, false=检测失败
 */
bool hmc5883l_detect(hmc5883l_dev_t *dev);

/**
 * @brief 初始化 HMC5883L 设备
 * @param dev 设备结构体指针
 * @return true=初始化成功, false=初始化失败
 */
bool hmc5883l_init(hmc5883l_dev_t *dev);

/**
 * @brief 配置 HMC5883L
 * @param dev 设备结构体指针
 * @param config 配置参数
 */
void hmc5883l_configure(hmc5883l_dev_t *dev, const hmc5883l_config_t *config);

/**
 * @brief 设置工作模式
 * @param dev 设备结构体指针
 * @param mode 工作模式
 */
void hmc5883l_set_mode(hmc5883l_dev_t *dev, hmc5883l_mode_t mode);

/**
 * @brief 读取原始磁场数据
 * @param dev 设备结构体指针
 * @param data 输出数据结构体指针
 * @return true=读取成功, false=读取失败
 */
bool hmc5883l_read_raw(hmc5883l_dev_t *dev, hmc5883l_mag_data_t *data);

/**
 * @brief 读取磁场数据（转换为 Gauss）
 * @param dev 设备结构体指针
 * @param data 输出数据结构体指针
 * @return true=读取成功, false=读取失败
 */
bool hmc5883l_read(hmc5883l_dev_t *dev, hmc5883l_mag_data_float_t *data);

/**
 * @brief 读取状态寄存器
 * @param dev 设备结构体指针
 * @return 状态寄存器值
 */
uint8_t hmc5883l_read_status(hmc5883l_dev_t *dev);

/**
 * @brief 检查数据是否就绪
 * @param dev 设备结构体指针
 * @return true=数据就绪, false=数据未就绪
 */
bool hmc5883l_data_ready(hmc5883l_dev_t *dev);

/**
 * @brief 执行自检
 * @param dev 设备结构体指针
 * @return true=自检通过, false=自检失败
 */
bool hmc5883l_self_test(hmc5883l_dev_t *dev);

/**
 * @brief 校准磁力计（计算偏移量）
 * @param dev 设备结构体指针
 * @param samples 采样数量
 * @return true=校准成功, false=校准失败
 */
bool hmc5883l_calibrate(hmc5883l_dev_t *dev, uint16_t samples);

/**
 * @brief 获取增益比例因子
 * @param gain 增益配置
 * @return 比例因子（LSB per Gauss）
 */
float hmc5883l_get_gain_scale(hmc5883l_gain_t gain);

/* ============================================================================
 * 默认配置
 * ============================================================================ */

/**
 * @brief 获取默认配置
 */
static inline hmc5883l_config_t hmc5883l_get_default_config(void)
{
    hmc5883l_config_t config = {
        .odr = HMC5883L_ODR_15HZ,           // 15 Hz 输出率（更抗抖）
        .samples = HMC5883L_SAMPLES_8,      // 8 次采样平均以压噪
        .gain = HMC5883L_GAIN_1_3GA,        // ±1.3 Ga 增益
        .mode = HMC5883L_CONTINUOUS,        // 连续测量模式
        .meas_mode = HMC5883L_MODE_NORMAL   // 正常测量模式
    };
    return config;
}

/**
 * @brief 获取低功耗配置
 */
static inline hmc5883l_config_t hmc5883l_get_low_power_config(void)
{
    hmc5883l_config_t config = {
        .odr = HMC5883L_ODR_7_5HZ,
        .samples = HMC5883L_SAMPLES_1,
        .gain = HMC5883L_GAIN_1_3GA,
        .mode = HMC5883L_SINGLE,            // 单次测量模式（低功耗）
        .meas_mode = HMC5883L_MODE_NORMAL
    };
    return config;
}

/**
 * @brief 获取高精度配置
 */
static inline hmc5883l_config_t hmc5883l_get_high_precision_config(void)
{
    hmc5883l_config_t config = {
        .odr = HMC5883L_ODR_15HZ,
        .samples = HMC5883L_SAMPLES_8,      // 8 次采样平均
        .gain = HMC5883L_GAIN_1_3GA,
        .mode = HMC5883L_CONTINUOUS,
        .meas_mode = HMC5883L_MODE_NORMAL
    };
    return config;
}

#ifdef __cplusplus
}
#endif

#endif // HMC5883L_LIB_H

