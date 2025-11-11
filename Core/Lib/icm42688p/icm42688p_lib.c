/**
 * @file    icm42688p_lib.c
 * @brief   ICM42688P 6-Axis IMU Library Implementation
 * @author  Based on Betaflight implementation
 * @date    2025
 */

#include "icm42688p_lib.h"
#include <string.h>

/* ============================================================================
 * Private Helper Functions
 * ============================================================================ */

/**
 * @brief Set register bank
 */
void icm42688p_set_bank(icm42688p_dev_t *dev, uint8_t bank)
{
    dev->spi_write_reg(ICM42688P_REG_BANK_SEL, bank & 0x07);
}

/**
 * @brief Perform soft reset
 */
void icm42688p_soft_reset(icm42688p_dev_t *dev)
{
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_0);
    dev->spi_write_reg(ICM42688P_REG_DEVICE_CONFIG, ICM42688P_SOFT_RESET_BIT);
    dev->delay_ms(1);
}

/**
 * @brief Turn gyro and accel off
 */
static void icm42688p_turn_off(icm42688p_dev_t *dev)
{
    dev->spi_write_reg(ICM42688P_REG_PWR_MGMT0, 
                       ICM42688P_GYRO_MODE_OFF | ICM42688P_ACCEL_MODE_OFF);
}

/**
 * @brief Turn gyro and accel on in Low Noise mode
 */
static void icm42688p_turn_on(icm42688p_dev_t *dev)
{
    uint8_t pwr_mgmt = ICM42688P_TEMP_DISABLE_OFF;
    
    if (dev->config.enable_gyro) {
        pwr_mgmt |= ICM42688P_GYRO_MODE_LN;
    }
    
    if (dev->config.enable_accel) {
        pwr_mgmt |= ICM42688P_ACCEL_MODE_LN;
    }
    
    dev->spi_write_reg(ICM42688P_REG_PWR_MGMT0, pwr_mgmt);
    dev->delay_ms(1);
}

/* ============================================================================
 * Public API Functions
 * ============================================================================ */

/**
 * @brief Detect ICM42688P device
 */
bool icm42688p_detect(icm42688p_dev_t *dev)
{
    if (!dev || !dev->spi_read_reg || !dev->delay_ms) {
        return false;
    }
    
    dev->delay_ms(1);  // Power-on time
    icm42688p_soft_reset(dev);
    dev->spi_write_reg(ICM42688P_REG_PWR_MGMT0, 0x00);
    
    // Try to read WHO_AM_I register multiple times
    uint8_t attempts = 20;
    while (attempts--) {
        dev->delay_ms(1);
        uint8_t who_am_i = dev->spi_read_reg(ICM42688P_REG_WHO_AM_I);
        
        if (who_am_i == ICM42688P_WHO_AM_I_VALUE) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Configure gyroscope Anti-Alias Filter
 */
void icm42688p_config_gyro_aaf(icm42688p_dev_t *dev, icm42688p_aaf_config_t aaf_config)
{
    if (aaf_config >= ICM42688P_AAF_COUNT) {
        aaf_config = ICM42688P_AAF_258HZ;
    }
    
    icm42688p_aaf_params_t aaf = ICM42688P_AAF_LUT[aaf_config];
    
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_1);
    dev->spi_write_reg(ICM42688P_REG_GYRO_CONFIG_STATIC3, aaf.delt);
    dev->spi_write_reg(ICM42688P_REG_GYRO_CONFIG_STATIC4, aaf.deltSqr & 0xFF);
    dev->spi_write_reg(ICM42688P_REG_GYRO_CONFIG_STATIC5, 
                       (aaf.deltSqr >> 8) | (aaf.bitshift << 4));
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_0);
}

/**
 * @brief Configure accelerometer Anti-Alias Filter
 */
void icm42688p_config_accel_aaf(icm42688p_dev_t *dev, icm42688p_aaf_config_t aaf_config)
{
    if (aaf_config >= ICM42688P_AAF_COUNT) {
        aaf_config = ICM42688P_AAF_258HZ;
    }
    
    icm42688p_aaf_params_t aaf = ICM42688P_AAF_LUT[aaf_config];
    
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_2);
    dev->spi_write_reg(ICM42688P_REG_ACCEL_CONFIG_STATIC2, aaf.delt << 1);
    dev->spi_write_reg(ICM42688P_REG_ACCEL_CONFIG_STATIC3, aaf.deltSqr & 0xFF);
    dev->spi_write_reg(ICM42688P_REG_ACCEL_CONFIG_STATIC4, 
                       (aaf.deltSqr >> 8) | (aaf.bitshift << 4));
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_0);
}

/**
 * @brief Configure gyroscope
 */
void icm42688p_config_gyro(icm42688p_dev_t *dev, uint8_t fsr, uint8_t odr)
{
    dev->spi_write_reg(ICM42688P_REG_GYRO_CONFIG0, (fsr << 5) | (odr & 0x0F));
    dev->delay_ms(15);
    
    // Update scale factor
    dev->gyro_scale = icm42688p_get_gyro_scale(fsr);
}

/**
 * @brief Configure accelerometer
 */
void icm42688p_config_accel(icm42688p_dev_t *dev, uint8_t fsr, uint8_t odr)
{
    dev->spi_write_reg(ICM42688P_REG_ACCEL_CONFIG0, (fsr << 5) | (odr & 0x0F));
    dev->delay_ms(15);
    
    // Update scale factor
    dev->accel_scale = icm42688p_get_accel_scale(fsr);
}

/**
 * @brief Configure interrupt pin
 */
void icm42688p_config_interrupt(icm42688p_dev_t *dev, uint8_t mode, uint8_t polarity, uint8_t drive)
{
    dev->spi_write_reg(ICM42688P_REG_INT_CONFIG, mode | polarity | drive);
    dev->spi_write_reg(ICM42688P_REG_INT_CONFIG0, ICM42688P_INT_CLEAR_ON_SBR);
    
    // Configure INT_CONFIG1
    uint8_t int_config1 = dev->spi_read_reg(ICM42688P_REG_INT_CONFIG1);
    int_config1 &= ~(1 << ICM42688P_INT_ASYNC_RESET_BIT);  // Clear async reset bit
    int_config1 |= (ICM42688P_INT_TPULSE_8US | ICM42688P_INT_TDEASSERT_DIS);
    dev->spi_write_reg(ICM42688P_REG_INT_CONFIG1, int_config1);
}

/**
 * @brief Enable/disable data ready interrupt
 */
void icm42688p_enable_data_ready_interrupt(icm42688p_dev_t *dev, bool enable)
{
    uint8_t value = enable ? ICM42688P_UI_DRDY_INT1_ENABLE : ICM42688P_UI_DRDY_INT1_DISABLE;
    dev->spi_write_reg(ICM42688P_REG_INT_SOURCE0, value);
}

/**
 * @brief Set power mode
 */
void icm42688p_set_power_mode(icm42688p_dev_t *dev, uint8_t gyro_mode, uint8_t accel_mode)
{
    uint8_t pwr_mgmt = ICM42688P_TEMP_DISABLE_OFF | gyro_mode | accel_mode;
    dev->spi_write_reg(ICM42688P_REG_PWR_MGMT0, pwr_mgmt);
    dev->delay_ms(1);
}

/**
 * @brief Enable external clock input
 */
bool icm42688p_enable_external_clock(icm42688p_dev_t *dev)
{
    // Switch to Bank 1 and configure PIN9 as CLKIN
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_1);
    uint8_t intf_config5 = dev->spi_read_reg(ICM42688P_REG_INTF_CONFIG5);
    intf_config5 = (intf_config5 & ~ICM42688P_PIN9_FUNCTION_MASK) | ICM42688P_PIN9_FUNCTION_CLKIN;
    dev->spi_write_reg(ICM42688P_REG_INTF_CONFIG5, intf_config5);
    
    // Switch to Bank 0 and enable external clock
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_0);
    uint8_t intf_config1 = dev->spi_read_reg(ICM42688P_REG_INTF_CONFIG1);
    intf_config1 |= ICM42688P_INTF_CONFIG1_CLKIN;
    dev->spi_write_reg(ICM42688P_REG_INTF_CONFIG1, intf_config1);
    
    return true;
}

/**
 * @brief Initialize ICM42688P device
 */
bool icm42688p_init(icm42688p_dev_t *dev)
{
    if (!dev || !dev->spi_read_reg || !dev->spi_write_reg || !dev->delay_ms) {
        return false;
    }
    
    // Detect device
    if (!icm42688p_detect(dev)) {
        return false;
    }
    
    // Set default configuration if not set
    if (dev->config.gyro_odr == 0) {
        dev->config.gyro_odr = ICM42688P_ODR_1KHZ;
    }
    if (dev->config.accel_odr == 0) {
        dev->config.accel_odr = ICM42688P_ODR_1KHZ;
    }
    
    // Enable external clock if configured
    if (dev->config.use_ext_clk) {
        icm42688p_enable_external_clock(dev);
    }
    
    // Turn off gyro and accel for configuration
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_0);
    icm42688p_turn_off(dev);
    
    // Configure gyro Anti-Alias Filter
    icm42688p_config_gyro_aaf(dev, dev->config.gyro_aaf);
    
    // Configure accel Anti-Alias Filter
    icm42688p_config_accel_aaf(dev, dev->config.accel_aaf);
    
    // Configure UI filters for low latency
    icm42688p_set_bank(dev, ICM42688P_BANK_SEL_0);
    dev->spi_write_reg(ICM42688P_REG_GYRO_ACCEL_CONFIG0, 
                       ICM42688P_ACCEL_UI_FILT_BW_LOW_LATENCY | 
                       ICM42688P_GYRO_UI_FILT_BW_LOW_LATENCY);
    
    // Configure interrupt pin
    icm42688p_config_interrupt(dev, 
                               ICM42688P_INT1_MODE_PULSED,
                               ICM42688P_INT1_POLARITY_HIGH,
                               ICM42688P_INT1_DRIVE_PP);
    
    // Enable data ready interrupt
    icm42688p_enable_data_ready_interrupt(dev, true);
    
    // Disable AFSR to prevent stalls in gyro output
    uint8_t intf_config1 = dev->spi_read_reg(ICM42688P_REG_INTF_CONFIG1);
    intf_config1 &= ~ICM42688P_INTF_CONFIG1_AFSR_MASK;
    intf_config1 |= ICM42688P_INTF_CONFIG1_AFSR_DISABLE;
    dev->spi_write_reg(ICM42688P_REG_INTF_CONFIG1, intf_config1);
    
    // Turn on gyro and accel
    icm42688p_turn_on(dev);
    
    // Configure gyro (FSR and ODR)
    icm42688p_config_gyro(dev, dev->config.gyro_fsr, dev->config.gyro_odr);
    
    // Configure accel (FSR and ODR)
    icm42688p_config_accel(dev, dev->config.accel_fsr, dev->config.accel_odr);
    
    return true;
}

/**
 * @brief Read gyroscope data
 */
bool icm42688p_read_gyro(icm42688p_dev_t *dev, icm42688p_gyro_data_t *data)
{
    if (!dev || !data || !dev->spi_read_burst) {
        return false;
    }
    
    uint8_t buffer[6];
    dev->spi_read_burst(ICM42688P_REG_GYRO_DATA_X1, buffer, 6);
    
    data->x = (int16_t)((buffer[0] << 8) | buffer[1]) - dev->gyro_offset[0];
    data->y = (int16_t)((buffer[2] << 8) | buffer[3]) - dev->gyro_offset[1];
    data->z = (int16_t)((buffer[4] << 8) | buffer[5]) - dev->gyro_offset[2];
    
    return true;
}

/**
 * @brief Read accelerometer data
 */
bool icm42688p_read_accel(icm42688p_dev_t *dev, icm42688p_accel_data_t *data)
{
    if (!dev || !data || !dev->spi_read_burst) {
        return false;
    }
    
    uint8_t buffer[6];
    dev->spi_read_burst(ICM42688P_REG_ACCEL_DATA_X1, buffer, 6);
    
    data->x = (int16_t)((buffer[0] << 8) | buffer[1]) - dev->accel_offset[0];
    data->y = (int16_t)((buffer[2] << 8) | buffer[3]) - dev->accel_offset[1];
    data->z = (int16_t)((buffer[4] << 8) | buffer[5]) - dev->accel_offset[2];
    
    return true;
}

/**
 * @brief Read temperature data
 */
bool icm42688p_read_temp(icm42688p_dev_t *dev, icm42688p_temp_data_t *data)
{
    if (!dev || !data || !dev->spi_read_burst) {
        return false;
    }
    
    uint8_t buffer[2];
    dev->spi_read_burst(ICM42688P_REG_TEMP_DATA1, buffer, 2);
    
    data->raw = (int16_t)((buffer[0] << 8) | buffer[1]);
    
    // Temperature in Celsius = (TEMP_DATA / 132.48) + 25
    data->celsius = (data->raw / 132.48f) + 25.0f;
    
    return true;
}

/**
 * @brief Read all sensor data in one burst
 */
bool icm42688p_read_all(icm42688p_dev_t *dev, 
                        icm42688p_gyro_data_t *gyro,
                        icm42688p_accel_data_t *accel,
                        icm42688p_temp_data_t *temp)
{
    if (!dev || !gyro || !accel || !temp || !dev->spi_read_burst) {
        return false;
    }
    
    // Read all data in one burst: ACCEL(6) + GYRO(6) + TEMP(2) = 14 bytes
    uint8_t buffer[14];
    dev->spi_read_burst(ICM42688P_REG_ACCEL_DATA_X1, buffer, 14);
    
    // Parse accelerometer data
    accel->x = (int16_t)((buffer[0] << 8) | buffer[1]) - dev->accel_offset[0];
    accel->y = (int16_t)((buffer[2] << 8) | buffer[3]) - dev->accel_offset[1];
    accel->z = (int16_t)((buffer[4] << 8) | buffer[5]) - dev->accel_offset[2];
    
    // Parse gyroscope data
    gyro->x = (int16_t)((buffer[6] << 8) | buffer[7]) - dev->gyro_offset[0];
    gyro->y = (int16_t)((buffer[8] << 8) | buffer[9]) - dev->gyro_offset[1];
    gyro->z = (int16_t)((buffer[10] << 8) | buffer[11]) - dev->gyro_offset[2];
    
    // Parse temperature data
    temp->raw = (int16_t)((buffer[12] << 8) | buffer[13]);
    temp->celsius = (temp->raw / 132.48f) + 25.0f;
    
    return true;
}

/**
 * @brief Calibrate gyroscope
 */
bool icm42688p_calibrate_gyro(icm42688p_dev_t *dev, uint16_t samples)
{
    if (!dev || samples == 0) {
        return false;
    }
    
    int32_t sum[3] = {0, 0, 0};
    icm42688p_gyro_data_t data;
    
    // Temporarily clear offsets
    int16_t temp_offset[3];
    memcpy(temp_offset, dev->gyro_offset, sizeof(temp_offset));
    memset(dev->gyro_offset, 0, sizeof(dev->gyro_offset));
    
    // Collect samples
    for (uint16_t i = 0; i < samples; i++) {
        if (!icm42688p_read_gyro(dev, &data)) {
            memcpy(dev->gyro_offset, temp_offset, sizeof(temp_offset));
            return false;
        }
        
        sum[0] += data.x;
        sum[1] += data.y;
        sum[2] += data.z;
        
        dev->delay_ms(1);
    }
    
    // Calculate average offsets
    dev->gyro_offset[0] = sum[0] / samples;
    dev->gyro_offset[1] = sum[1] / samples;
    dev->gyro_offset[2] = sum[2] / samples;
    
    return true;
}

/**
 * @brief Calibrate accelerometer
 */
bool icm42688p_calibrate_accel(icm42688p_dev_t *dev, uint16_t samples)
{
    if (!dev || samples == 0) {
        return false;
    }
    
    int32_t sum[3] = {0, 0, 0};
    icm42688p_accel_data_t data;
    
    // Temporarily clear offsets
    int16_t temp_offset[3];
    memcpy(temp_offset, dev->accel_offset, sizeof(temp_offset));
    memset(dev->accel_offset, 0, sizeof(dev->accel_offset));
    
    // Collect samples
    for (uint16_t i = 0; i < samples; i++) {
        if (!icm42688p_read_accel(dev, &data)) {
            memcpy(dev->accel_offset, temp_offset, sizeof(temp_offset));
            return false;
        }
        
        sum[0] += data.x;
        sum[1] += data.y;
        sum[2] += data.z;
        
        dev->delay_ms(1);
    }
    
    // Calculate average offsets
    // Note: For Z-axis, subtract 1g (gravity) from the average
    dev->accel_offset[0] = sum[0] / samples;
    dev->accel_offset[1] = sum[1] / samples;
    dev->accel_offset[2] = (sum[2] / samples) - (int16_t)dev->accel_scale;  // Subtract 1g
    
    return true;
}

/**
 * @brief Get gyro scale factor
 */
float icm42688p_get_gyro_scale(uint8_t fsr)
{
    // LSB per dps for ICM42688P
    switch (fsr) {
        case 0: return 16.4f;    // ±2000 dps
        case 1: return 32.8f;    // ±1000 dps
        case 2: return 65.5f;    // ±500 dps
        case 3: return 131.0f;   // ±250 dps
        default: return 16.4f;
    }
}

/**
 * @brief Get accel scale factor
 */
float icm42688p_get_accel_scale(uint8_t fsr)
{
    // LSB per g for ICM42688P
    switch (fsr) {
        case 0: return 2048.0f;  // ±16g
        case 1: return 4096.0f;  // ±8g
        case 2: return 8192.0f;  // ±4g
        case 3: return 16384.0f; // ±2g
        default: return 2048.0f;
    }
}
