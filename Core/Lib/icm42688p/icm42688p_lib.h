/**
 * @file    icm42688p_lib.h
 * @brief   ICM42688P 6-Axis IMU Library Header
 * @author  Based on Betaflight implementation
 * @date    2025
 * 
 * ICM42688P is a high-performance 6-axis MEMS MotionTracking device
 * - 3-axis gyroscope with ±2000 dps range
 * - 3-axis accelerometer with ±16g range
 * - SPI interface up to 24 MHz
 * - Programmable Anti-Alias Filter (AAF)
 * - Output Data Rate (ODR) up to 8 kHz
 */

#ifndef ICM42688P_LIB_H
#define ICM42688P_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * ICM42688P Configuration Constants
 * ============================================================================ */

// Device Identification
#define ICM42688P_WHO_AM_I_VALUE        0x47
#define ICM42688P_DEVICE_ID             ICM42688P_WHO_AM_I_VALUE

// SPI Communication
#define ICM42688P_MAX_SPI_CLK_HZ        24000000    // 24 MHz max SPI frequency
#define ICM42688P_SPI_READ_BIT          0x80        // Set bit 7 for read operation

// External Clock (optional)
#define ICM42688P_CLKIN_FREQ            32000       // 32 kHz external clock

/* ============================================================================
 * Register Map - User Bank 0 (Default)
 * ============================================================================ */

#define ICM42688P_REG_DEVICE_CONFIG     0x11        // Device configuration
#define ICM42688P_REG_INT_CONFIG        0x14        // Interrupt configuration
#define ICM42688P_REG_ACCEL_DATA_X1     0x1F        // Accel X-axis data [15:8]
#define ICM42688P_REG_ACCEL_DATA_X0     0x20        // Accel X-axis data [7:0]
#define ICM42688P_REG_ACCEL_DATA_Y1     0x21        // Accel Y-axis data [15:8]
#define ICM42688P_REG_ACCEL_DATA_Y0     0x22        // Accel Y-axis data [7:0]
#define ICM42688P_REG_ACCEL_DATA_Z1     0x23        // Accel Z-axis data [15:8]
#define ICM42688P_REG_ACCEL_DATA_Z0     0x24        // Accel Z-axis data [7:0]
#define ICM42688P_REG_GYRO_DATA_X1      0x25        // Gyro X-axis data [15:8]
#define ICM42688P_REG_GYRO_DATA_X0      0x26        // Gyro X-axis data [7:0]
#define ICM42688P_REG_GYRO_DATA_Y1      0x27        // Gyro Y-axis data [15:8]
#define ICM42688P_REG_GYRO_DATA_Y0      0x28        // Gyro Y-axis data [7:0]
#define ICM42688P_REG_GYRO_DATA_Z1      0x29        // Gyro Z-axis data [15:8]
#define ICM42688P_REG_GYRO_DATA_Z0      0x2A        // Gyro Z-axis data [7:0]
#define ICM42688P_REG_TEMP_DATA1        0x1D        // Temperature data [15:8]
#define ICM42688P_REG_TEMP_DATA0        0x1E        // Temperature data [7:0]
#define ICM42688P_REG_INTF_CONFIG1      0x4D        // Interface configuration 1
#define ICM42688P_REG_PWR_MGMT0         0x4E        // Power management 0
#define ICM42688P_REG_GYRO_CONFIG0      0x4F        // Gyro configuration 0
#define ICM42688P_REG_ACCEL_CONFIG0     0x50        // Accel configuration 0
#define ICM42688P_REG_GYRO_ACCEL_CONFIG0 0x52       // Gyro/Accel UI filter config
#define ICM42688P_REG_INT_CONFIG0       0x63        // Interrupt config 0
#define ICM42688P_REG_INT_CONFIG1       0x64        // Interrupt config 1
#define ICM42688P_REG_INT_SOURCE0       0x65        // Interrupt source 0
#define ICM42688P_REG_WHO_AM_I          0x75        // Device ID
#define ICM42688P_REG_BANK_SEL          0x76        // Register bank selection

/* ============================================================================
 * Register Map - User Bank 1
 * ============================================================================ */

#define ICM42688P_REG_GYRO_CONFIG_STATIC3   0x0C    // Gyro AAF DELT
#define ICM42688P_REG_GYRO_CONFIG_STATIC4   0x0D    // Gyro AAF DELTSQR[7:0]
#define ICM42688P_REG_GYRO_CONFIG_STATIC5   0x0E    // Gyro AAF DELTSQR[15:8] & BITSHIFT
#define ICM42688P_REG_INTF_CONFIG5          0x7B    // Interface config 5 (CLKIN)

/* ============================================================================
 * Register Map - User Bank 2
 * ============================================================================ */

#define ICM42688P_REG_ACCEL_CONFIG_STATIC2  0x03    // Accel AAF DELT
#define ICM42688P_REG_ACCEL_CONFIG_STATIC3  0x04    // Accel AAF DELTSQR[7:0]
#define ICM42688P_REG_ACCEL_CONFIG_STATIC4  0x05    // Accel AAF DELTSQR[15:8] & BITSHIFT

/* ============================================================================
 * Register Bank Selection
 * ============================================================================ */

#define ICM42688P_BANK_SEL_0            0x00
#define ICM42688P_BANK_SEL_1            0x01
#define ICM42688P_BANK_SEL_2            0x02
#define ICM42688P_BANK_SEL_3            0x03
#define ICM42688P_BANK_SEL_4            0x04

/* ============================================================================
 * DEVICE_CONFIG Register (0x11) - Soft Reset
 * ============================================================================ */

#define ICM42688P_SOFT_RESET_BIT        (1 << 0)

/* ============================================================================
 * PWR_MGMT0 Register (0x4E) - Power Management
 * ============================================================================ */

#define ICM42688P_GYRO_MODE_OFF         (0 << 2)
#define ICM42688P_GYRO_MODE_STANDBY     (1 << 2)
#define ICM42688P_GYRO_MODE_LN          (3 << 2)    // Low Noise mode
#define ICM42688P_ACCEL_MODE_OFF        (0 << 0)
#define ICM42688P_ACCEL_MODE_LP         (2 << 0)    // Low Power mode
#define ICM42688P_ACCEL_MODE_LN         (3 << 0)    // Low Noise mode
#define ICM42688P_TEMP_DISABLE_OFF      (0 << 5)
#define ICM42688P_TEMP_DISABLE_ON       (1 << 5)

/* ============================================================================
 * GYRO_CONFIG0 & ACCEL_CONFIG0 Registers (0x4F, 0x50)
 * ============================================================================ */

// Full Scale Range (FSR) - Gyro
#define ICM42688P_GYRO_FSR_2000DPS      (0 << 5)    // ±2000 dps
#define ICM42688P_GYRO_FSR_1000DPS      (1 << 5)    // ±1000 dps
#define ICM42688P_GYRO_FSR_500DPS       (2 << 5)    // ±500 dps
#define ICM42688P_GYRO_FSR_250DPS       (3 << 5)    // ±250 dps

// Full Scale Range (FSR) - Accel
#define ICM42688P_ACCEL_FSR_16G         (0 << 5)    // ±16g
#define ICM42688P_ACCEL_FSR_8G          (1 << 5)    // ±8g
#define ICM42688P_ACCEL_FSR_4G          (2 << 5)    // ±4g
#define ICM42688P_ACCEL_FSR_2G          (3 << 5)    // ±2g

// Output Data Rate (ODR)
#define ICM42688P_ODR_32KHZ             1
#define ICM42688P_ODR_16KHZ             2
#define ICM42688P_ODR_8KHZ              3
#define ICM42688P_ODR_4KHZ              4
#define ICM42688P_ODR_2KHZ              5
#define ICM42688P_ODR_1KHZ              6
#define ICM42688P_ODR_200HZ             7
#define ICM42688P_ODR_100HZ             8
#define ICM42688P_ODR_50HZ              9
#define ICM42688P_ODR_25HZ              10
#define ICM42688P_ODR_12_5HZ            11
#define ICM42688P_ODR_6_25HZ            12
#define ICM42688P_ODR_3_125HZ           13
#define ICM42688P_ODR_1_5625HZ          14
#define ICM42688P_ODR_500HZ             15

/* ============================================================================
 * GYRO_ACCEL_CONFIG0 Register (0x52) - UI Filter Configuration
 * ============================================================================ */

#define ICM42688P_ACCEL_UI_FILT_BW_LOW_LATENCY  (15 << 4)
#define ICM42688P_GYRO_UI_FILT_BW_LOW_LATENCY   (15 << 0)

/* ============================================================================
 * INT_CONFIG Register (0x14) - Interrupt Pin Configuration
 * ============================================================================ */

#define ICM42688P_INT1_MODE_PULSED      (0 << 2)
#define ICM42688P_INT1_MODE_LATCHED     (1 << 2)
#define ICM42688P_INT1_DRIVE_OD         (0 << 1)    // Open Drain
#define ICM42688P_INT1_DRIVE_PP         (1 << 1)    // Push-Pull
#define ICM42688P_INT1_POLARITY_LOW     (0 << 0)
#define ICM42688P_INT1_POLARITY_HIGH    (1 << 0)

/* ============================================================================
 * INT_CONFIG0 Register (0x63) - Interrupt Clear Configuration
 * ============================================================================ */

#define ICM42688P_INT_CLEAR_ON_SBR      ((0 << 5) | (0 << 4))
#define ICM42688P_INT_CLEAR_ON_F1BR     ((1 << 5) | (0 << 4))

/* ============================================================================
 * INT_CONFIG1 Register (0x64) - Interrupt Timing Configuration
 * ============================================================================ */

#define ICM42688P_INT_ASYNC_RESET_BIT   4
#define ICM42688P_INT_TDEASSERT_BIT     5
#define ICM42688P_INT_TPULSE_BIT        6
#define ICM42688P_INT_TPULSE_100US      (0 << 6)
#define ICM42688P_INT_TPULSE_8US        (1 << 6)
#define ICM42688P_INT_TDEASSERT_EN      (0 << 5)
#define ICM42688P_INT_TDEASSERT_DIS     (1 << 5)

/* ============================================================================
 * INT_SOURCE0 Register (0x65) - Interrupt Source Enable
 * ============================================================================ */

#define ICM42688P_UI_DRDY_INT1_DISABLE  (0 << 3)
#define ICM42688P_UI_DRDY_INT1_ENABLE   (1 << 3)

/* ============================================================================
 * INTF_CONFIG1 Register (0x4D) - Interface Configuration
 * ============================================================================ */

#define ICM42688P_INTF_CONFIG1_AFSR_MASK    0xC0
#define ICM42688P_INTF_CONFIG1_AFSR_DISABLE 0x40
#define ICM42688P_INTF_CONFIG1_CLKIN        (1 << 2)

/* ============================================================================
 * INTF_CONFIG5 Register (0x7B, Bank 1) - PIN9 Configuration
 * ============================================================================ */

#define ICM42688P_PIN9_FUNCTION_MASK    (3 << 1)
#define ICM42688P_PIN9_FUNCTION_CLKIN   (2 << 1)

/* ============================================================================
 * Anti-Alias Filter (AAF) Configuration
 * ============================================================================ */

typedef enum {
    ICM42688P_AAF_258HZ = 0,    // 258 Hz cutoff
    ICM42688P_AAF_536HZ,        // 536 Hz cutoff
    ICM42688P_AAF_997HZ,        // 997 Hz cutoff
    ICM42688P_AAF_1962HZ,       // 1962 Hz cutoff
    ICM42688P_AAF_COUNT
} icm42688p_aaf_config_t;

typedef struct {
    uint8_t delt;           // DELT parameter
    uint16_t deltSqr;       // DELTSQR parameter
    uint8_t bitshift;       // BITSHIFT parameter
} icm42688p_aaf_params_t;

// AAF lookup table for ICM42688P
static const icm42688p_aaf_params_t ICM42688P_AAF_LUT[] = {
    [ICM42688P_AAF_258HZ]  = {  6,   36, 10 },
    [ICM42688P_AAF_536HZ]  = { 12,  144,  8 },
    [ICM42688P_AAF_997HZ]  = { 21,  440,  6 },
    [ICM42688P_AAF_1962HZ] = { 37, 1376,  4 },
};

/* ============================================================================
 * Data Structures
 * ============================================================================ */

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} icm42688p_gyro_data_t;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} icm42688p_accel_data_t;

typedef struct {
    int16_t raw;            // Raw temperature value
    float celsius;          // Temperature in Celsius
} icm42688p_temp_data_t;

typedef struct {
    uint8_t gyro_fsr;       // Gyro full scale range
    uint8_t accel_fsr;      // Accel full scale range
    uint8_t gyro_odr;       // Gyro output data rate
    uint8_t accel_odr;      // Accel output data rate
    icm42688p_aaf_config_t gyro_aaf;    // Gyro AAF config
    icm42688p_aaf_config_t accel_aaf;   // Accel AAF config
    bool enable_gyro;       // Enable gyroscope
    bool enable_accel;      // Enable accelerometer
    bool enable_temp;       // Enable temperature sensor
    bool use_ext_clk;       // Use external clock (CLKIN)
} icm42688p_config_t;

typedef struct {
    // SPI communication function pointers (user must implement)
    uint8_t (*spi_read_reg)(uint8_t reg);
    void (*spi_write_reg)(uint8_t reg, uint8_t value);
    void (*spi_read_burst)(uint8_t reg, uint8_t *buffer, uint16_t len);
    void (*delay_ms)(uint32_t ms);
    
    // Device configuration
    icm42688p_config_t config;
    
    // Calibration data
    int16_t gyro_offset[3];
    int16_t accel_offset[3];
    
    // Scale factors
    float gyro_scale;       // LSB per dps
    float accel_scale;      // LSB per g
} icm42688p_dev_t;

/* ============================================================================
 * 函数原型（Function Prototypes）
 * ============================================================================ */

/**
 * @brief 初始化 ICM42688P 设备
 * @param dev 指向设备结构体的指针
 * @return 如果初始化成功返回 true，否则返回 false
 */
bool icm42688p_init(icm42688p_dev_t *dev);

/**
 * @brief 检测 ICM42688P 设备是否存在
 * @param dev 指向设备结构体的指针
 * @return 如果检测到 ICM42688P 返回 true，否则返回 false
 */
bool icm42688p_detect(icm42688p_dev_t *dev);

/**
 * @brief 执行软复位操作
 * @param dev 指向设备结构体的指针
 */
void icm42688p_soft_reset(icm42688p_dev_t *dev);

/**
 * @brief 设置寄存器访问的 Bank
 * @param dev 指向设备结构体的指针
 * @param bank 寄存器组编号（0-4）
 */
void icm42688p_set_bank(icm42688p_dev_t *dev, uint8_t bank);

/**
 * @brief 配置陀螺仪
 * @param dev 指向设备结构体的指针
 * @param fsr 陀螺仪量程设置（Full Scale Range，例如 ±250/500/1000/2000 dps）
 * @param odr 输出数据速率（Output Data Rate，单位 Hz）
 */
void icm42688p_config_gyro(icm42688p_dev_t *dev, uint8_t fsr, uint8_t odr);

/**
 * @brief 配置加速度计
 * @param dev 指向设备结构体的指针
 * @param fsr 加速度计量程设置（Full Scale Range，例如 ±2/4/8/16 g）
 * @param odr 输出数据速率（Output Data Rate，单位 Hz）
 */
void icm42688p_config_accel(icm42688p_dev_t *dev, uint8_t fsr, uint8_t odr);

/**
 * @brief 配置陀螺仪的抗混叠滤波器（Anti-Alias Filter, AAF）
 * @param dev 指向设备结构体的指针
 * @param aaf_config AAF 滤波器配置结构体
 */
void icm42688p_config_gyro_aaf(icm42688p_dev_t *dev, icm42688p_aaf_config_t aaf_config);

/**
 * @brief 配置加速度计的抗混叠滤波器（Anti-Alias Filter, AAF）
 * @param dev 指向设备结构体的指针
 * @param aaf_config AAF 滤波器配置结构体
 */
void icm42688p_config_accel_aaf(icm42688p_dev_t *dev, icm42688p_aaf_config_t aaf_config);

/**
 * @brief 配置中断引脚参数
 * @param dev 指向设备结构体的指针
 * @param mode 中断模式（脉冲模式或锁存模式）
 * @param polarity 中断极性（高电平触发或低电平触发）
 * @param drive 中断输出驱动类型（推挽输出或开漏输出）
 */
void icm42688p_config_interrupt(icm42688p_dev_t *dev, uint8_t mode, uint8_t polarity, uint8_t drive);

/**
 * @brief 启用或禁用数据就绪中断（Data Ready Interrupt）
 * @param dev 指向设备结构体的指针
 * @param enable 设置为 true 启用中断，false 禁用中断
 */
void icm42688p_enable_data_ready_interrupt(icm42688p_dev_t *dev, bool enable);

/**
 * @brief 读取陀螺仪数据
 * @param dev 指向设备结构体的指针
 * @param data 指向陀螺仪数据结构体的指针
 * @return 如果读取成功返回 true，否则返回 false
 */
bool icm42688p_read_gyro(icm42688p_dev_t *dev, icm42688p_gyro_data_t *data);

/**
 * @brief 读取加速度计数据
 * @param dev 指向设备结构体的指针
 * @param data 指向加速度计数据结构体的指针
 * @return 如果读取成功返回 true，否则返回 false
 */
bool icm42688p_read_accel(icm42688p_dev_t *dev, icm42688p_accel_data_t *data);

/**
 * @brief 读取温度数据
 * @param dev 指向设备结构体的指针
 * @param data 指向温度数据结构体的指针
 * @return 如果读取成功返回 true，否则返回 false
 */
bool icm42688p_read_temp(icm42688p_dev_t *dev, icm42688p_temp_data_t *data);

/**
 * @brief 一次性读取全部传感器数据（陀螺仪 + 加速度计 + 温度）
 * @param dev 指向设备结构体的指针
 * @param gyro 指向陀螺仪数据结构体的指针
 * @param accel 指向加速度计数据结构体的指针
 * @param temp 指向温度数据结构体的指针
 * @return 如果读取成功返回 true，否则返回 false
 */
bool icm42688p_read_all(icm42688p_dev_t *dev, 
                        icm42688p_gyro_data_t *gyro,
                        icm42688p_accel_data_t *accel,
                        icm42688p_temp_data_t *temp);

/**
 * @brief 设置传感器的电源模式
 * @param dev 指向设备结构体的指针
 * @param gyro_mode 陀螺仪功耗模式（睡眠、待机、工作）
 * @param accel_mode 加速度计功耗模式（睡眠、待机、工作）
 */
void icm42688p_set_power_mode(icm42688p_dev_t *dev, uint8_t gyro_mode, uint8_t accel_mode);

/**
 * @brief 启用外部时钟输入（CLKIN）
 * @param dev 指向设备结构体的指针
 * @return 如果启用成功返回 true，否则返回 false
 */
bool icm42688p_enable_external_clock(icm42688p_dev_t *dev);

/**
 * @brief 校准陀螺仪（计算偏移量）
 * @param dev 指向设备结构体的指针
 * @param samples 平均采样数量，用于计算偏置
 * @return 如果校准成功返回 true，否则返回 false
 */
bool icm42688p_calibrate_gyro(icm42688p_dev_t *dev, uint16_t samples);

/**
 * @brief 校准加速度计（计算偏移量）
 * @param dev 指向设备结构体的指针
 * @param samples 平均采样数量，用于计算偏置
 * @return 如果校准成功返回 true，否则返回 false
 */
bool icm42688p_calibrate_accel(icm42688p_dev_t *dev, uint16_t samples);

/**
 * @brief 获取陀螺仪的量程对应比例因子（LSB/dps）
 * @param fsr 陀螺仪量程设置
 * @return 返回比例因子（用于将原始值转换为 dps）
 */
float icm42688p_get_gyro_scale(uint8_t fsr);

/**
 * @brief 获取加速度计的量程对应比例因子（LSB/g）
 * @param fsr 加速度计量程设置
 * @return 返回比例因子（用于将原始值转换为 g）
 */
float icm42688p_get_accel_scale(uint8_t fsr);


#ifdef __cplusplus
}
#endif

#endif // ICM42688P_LIB_H
