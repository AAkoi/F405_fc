/**
 * @file    bmp280_lib.h
 * @brief   BMP280 气压计库头文件（支持 SPI 和 I2C）
 * @author  Based on Betaflight implementation
 * @date    2025
 * 
 * 功能特性：
 * - 支持 BMP280 和 BME280
 * - SPI 接口（最高 10 MHz）
 * - I2C 接口（地址 0x76 或 0x77）
 * - 温度补偿
 * - 压力测量（Pa）
 * - 高度计算
 */

#ifndef BMP280_LIB_H
#define BMP280_LIB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 寄存器定义
 * ============================================================================ */

// 芯片 ID
#define BMP280_CHIP_ID                  0x58    // BMP280
#define BME280_CHIP_ID                  0x60    // BME280（带湿度传感器）

// 寄存器地址
#define BMP280_REG_CHIP_ID              0xD0    // 芯片 ID 寄存器
#define BMP280_REG_RESET                0xE0    // 软复位寄存器
#define BMP280_REG_STATUS               0xF3    // 状态寄存器
#define BMP280_REG_CTRL_MEAS            0xF4    // 控制测量寄存器
#define BMP280_REG_CONFIG               0xF5    // 配置寄存器
#define BMP280_REG_PRESS_MSB            0xF7    // 压力 MSB
#define BMP280_REG_PRESS_LSB            0xF8    // 压力 LSB
#define BMP280_REG_PRESS_XLSB           0xF9    // 压力 XLSB
#define BMP280_REG_TEMP_MSB             0xFA    // 温度 MSB
#define BMP280_REG_TEMP_LSB             0xFB    // 温度 LSB
#define BMP280_REG_TEMP_XLSB            0xFC    // 温度 XLSB

// 校准数据寄存器
#define BMP280_REG_CALIB_START          0x88    // 校准数据起始地址
#define BMP280_CALIB_DATA_LENGTH        24      // 校准数据长度

// I2C 地址
#define BMP280_I2C_ADDR_PRIMARY         0x76    // SDO 接地
#define BMP280_I2C_ADDR_SECONDARY       0x77    // SDO 接 VCC

// 软复位命令
#define BMP280_RESET_VALUE              0xB6

// 状态寄存器位
#define BMP280_STATUS_MEASURING         (1 << 3)
#define BMP280_STATUS_IM_UPDATE         (1 << 0)

/* ============================================================================
 * 配置常量
 * ============================================================================ */

// 过采样率（Oversampling）
typedef enum {
    BMP280_OVERSAMP_SKIP = 0x00,    // 跳过测量
    BMP280_OVERSAMP_1X   = 0x01,    // 1x 过采样
    BMP280_OVERSAMP_2X   = 0x02,    // 2x 过采样
    BMP280_OVERSAMP_4X   = 0x03,    // 4x 过采样
    BMP280_OVERSAMP_8X   = 0x04,    // 8x 过采样
    BMP280_OVERSAMP_16X  = 0x05     // 16x 过采样
} bmp280_oversamp_t;

// 工作模式
typedef enum {
    BMP280_MODE_SLEEP  = 0x00,      // 睡眠模式
    BMP280_MODE_FORCED = 0x01,      // 强制模式（单次测量）
    BMP280_MODE_NORMAL = 0x03       // 正常模式（连续测量）
} bmp280_mode_t;

// IIR 滤波器系数
typedef enum {
    BMP280_FILTER_OFF = 0x00,       // 关闭滤波
    BMP280_FILTER_2   = 0x01,       // 系数 2
    BMP280_FILTER_4   = 0x02,       // 系数 4
    BMP280_FILTER_8   = 0x03,       // 系数 8
    BMP280_FILTER_16  = 0x04        // 系数 16
} bmp280_filter_t;

// 待机时间（正常模式下）
typedef enum {
    BMP280_STANDBY_0_5MS  = 0x00,   // 0.5 ms
    BMP280_STANDBY_62_5MS = 0x01,   // 62.5 ms
    BMP280_STANDBY_125MS  = 0x02,   // 125 ms
    BMP280_STANDBY_250MS  = 0x03,   // 250 ms
    BMP280_STANDBY_500MS  = 0x04,   // 500 ms
    BMP280_STANDBY_1000MS = 0x05,   // 1000 ms
    BMP280_STANDBY_2000MS = 0x06,   // 2000 ms
    BMP280_STANDBY_4000MS = 0x07    // 4000 ms
} bmp280_standby_t;

// 接口类型
typedef enum {
    BMP280_INTERFACE_SPI,           // SPI 接口
    BMP280_INTERFACE_I2C            // I2C 接口
} bmp280_interface_t;

/* ============================================================================
 * 数据结构
 * ============================================================================ */

/**
 * @brief 校准参数结构体
 */
typedef struct {
    uint16_t dig_T1;    // 温度校准参数 T1
    int16_t  dig_T2;    // 温度校准参数 T2
    int16_t  dig_T3;    // 温度校准参数 T3
    uint16_t dig_P1;    // 压力校准参数 P1
    int16_t  dig_P2;    // 压力校准参数 P2
    int16_t  dig_P3;    // 压力校准参数 P3
    int16_t  dig_P4;    // 压力校准参数 P4
    int16_t  dig_P5;    // 压力校准参数 P5
    int16_t  dig_P6;    // 压力校准参数 P6
    int16_t  dig_P7;    // 压力校准参数 P7
    int16_t  dig_P8;    // 压力校准参数 P8
    int16_t  dig_P9;    // 压力校准参数 P9
} __attribute__((packed)) bmp280_calib_t;

/**
 * @brief 配置结构体
 */
typedef struct {
    bmp280_oversamp_t temp_oversamp;    // 温度过采样率
    bmp280_oversamp_t press_oversamp;   // 压力过采样率
    bmp280_mode_t mode;                 // 工作模式
    bmp280_filter_t filter;             // IIR 滤波器
    bmp280_standby_t standby;           // 待机时间
} bmp280_config_t;

/**
 * @brief 测量数据结构体
 */
typedef struct {
    int32_t temperature;    // 温度（0.01°C）例如：2523 = 25.23°C
    int32_t pressure;       // 压力（Pa）例如：101325 = 101325 Pa
    float altitude;         // 海拔高度（m）
} bmp280_data_t;

/**
 * @brief BMP280 设备结构体
 */
typedef struct {
    // 接口类型
    bmp280_interface_t interface;
    
    // I2C 配置
    uint8_t i2c_addr;       // I2C 地址（0x76 或 0x77）
    
    // 通信函数指针（用户实现）
    uint8_t (*spi_read_reg)(uint8_t reg);
    void (*spi_write_reg)(uint8_t reg, uint8_t value);
    void (*spi_read_burst)(uint8_t reg, uint8_t *buffer, uint16_t len);
    
    uint8_t (*i2c_read_reg)(uint8_t addr, uint8_t reg);
    void (*i2c_write_reg)(uint8_t addr, uint8_t reg, uint8_t value);
    void (*i2c_read_burst)(uint8_t addr, uint8_t reg, uint8_t *buffer, uint16_t len);
    
    void (*delay_ms)(uint32_t ms);
    
    // 设备信息
    uint8_t chip_id;        // 芯片 ID（0x58 或 0x60）
    
    // 校准参数
    bmp280_calib_t calib;
    
    // 配置
    bmp280_config_t config;
    
    // 内部状态
    int32_t t_fine;         // 温度补偿中间值
    int32_t adc_T;          // 原始温度 ADC 值
    int32_t adc_P;          // 原始压力 ADC 值
    
    // 海拔计算基准
    float sea_level_pressure;   // 海平面气压（Pa），默认 101325
    
} bmp280_dev_t;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 检测 BMP280 设备（SPI 接口）
 * @param dev 设备结构体指针
 * @return true=检测成功, false=检测失败
 */
bool bmp280_detect_spi(bmp280_dev_t *dev);

/**
 * @brief 检测 BMP280 设备（I2C 接口）
 * @param dev 设备结构体指针
 * @param addr I2C 地址（0x76 或 0x77）
 * @return true=检测成功, false=检测失败
 */
bool bmp280_detect_i2c(bmp280_dev_t *dev, uint8_t addr);

/**
 * @brief 初始化 BMP280 设备
 * @param dev 设备结构体指针
 * @return true=初始化成功, false=初始化失败
 */
bool bmp280_init(bmp280_dev_t *dev);

/**
 * @brief 软复位 BMP280
 * @param dev 设备结构体指针
 */
void bmp280_reset(bmp280_dev_t *dev);

/**
 * @brief 配置 BMP280
 * @param dev 设备结构体指针
 * @param config 配置参数
 */
void bmp280_configure(bmp280_dev_t *dev, const bmp280_config_t *config);

/**
 * @brief 启动单次测量（强制模式）
 * @param dev 设备结构体指针
 */
void bmp280_start_measurement(bmp280_dev_t *dev);

/**
 * @brief 读取原始数据
 * @param dev 设备结构体指针
 * @return true=读取成功, false=读取失败
 */
bool bmp280_read_raw(bmp280_dev_t *dev);

/**
 * @brief 计算温度和压力
 * @param dev 设备结构体指针
 * @param data 输出数据结构体指针
 */
void bmp280_calculate(bmp280_dev_t *dev, bmp280_data_t *data);

/**
 * @brief 读取温度和压力（一次性读取）
 * @param dev 设备结构体指针
 * @param data 输出数据结构体指针
 * @return true=读取成功, false=读取失败
 */
bool bmp280_read(bmp280_dev_t *dev, bmp280_data_t *data);

/**
 * @brief 检查设备是否正在测量
 * @param dev 设备结构体指针
 * @return true=正在测量, false=空闲
 */
bool bmp280_is_measuring(bmp280_dev_t *dev);

/**
 * @brief 设置海平面气压（用于高度计算）
 * @param dev 设备结构体指针
 * @param pressure 海平面气压（Pa）
 */
void bmp280_set_sea_level_pressure(bmp280_dev_t *dev, float pressure);

/**
 * @brief 计算海拔高度
 * @param pressure 当前气压（Pa）
 * @param sea_level_pressure 海平面气压（Pa）
 * @return 海拔高度（m）
 */
float bmp280_calculate_altitude(float pressure, float sea_level_pressure);

/**
 * @brief 获取测量延迟时间（ms）
 * @param dev 设备结构体指针
 * @return 延迟时间（ms）
 */
uint32_t bmp280_get_measurement_delay(const bmp280_dev_t *dev);

/* ============================================================================
 * 默认配置
 * ============================================================================ */

/**
 * @brief 获取默认配置（高精度模式）
 */
static inline bmp280_config_t bmp280_get_default_config(void)
{
    bmp280_config_t config = {
        .temp_oversamp = BMP280_OVERSAMP_1X,    // 温度 1x 过采样
        .press_oversamp = BMP280_OVERSAMP_8X,   // 压力 8x 过采样
        .mode = BMP280_MODE_FORCED,             // 强制模式（按需测量）
        .filter = BMP280_FILTER_OFF,            // 关闭滤波
        .standby = BMP280_STANDBY_0_5MS         // 待机 0.5ms
    };
    return config;
}

/**
 * @brief 获取低功耗配置
 */
static inline bmp280_config_t bmp280_get_low_power_config(void)
{
    bmp280_config_t config = {
        .temp_oversamp = BMP280_OVERSAMP_1X,
        .press_oversamp = BMP280_OVERSAMP_1X,
        .mode = BMP280_MODE_FORCED,
        .filter = BMP280_FILTER_OFF,
        .standby = BMP280_STANDBY_0_5MS
    };
    return config;
}

/**
 * @brief 获取高精度配置
 */
static inline bmp280_config_t bmp280_get_high_precision_config(void)
{
    bmp280_config_t config = {
        .temp_oversamp = BMP280_OVERSAMP_2X,
        .press_oversamp = BMP280_OVERSAMP_16X,
        .mode = BMP280_MODE_FORCED,
        .filter = BMP280_FILTER_4,
        .standby = BMP280_STANDBY_0_5MS
    };
    return config;
}

/**
 * @brief 获取连续测量配置
 */
static inline bmp280_config_t bmp280_get_continuous_config(void)
{
    bmp280_config_t config = {
        .temp_oversamp = BMP280_OVERSAMP_1X,
        .press_oversamp = BMP280_OVERSAMP_8X,
        .mode = BMP280_MODE_NORMAL,             // 连续测量模式
        .filter = BMP280_FILTER_4,
        .standby = BMP280_STANDBY_125MS         // 125ms 间隔
    };
    return config;
}

#ifdef __cplusplus
}
#endif

#endif // BMP280_LIB_H
