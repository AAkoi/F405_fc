/**
 * @file    icm42688p_example.c
 * @brief   ICM42688P Library Usage Example
 * @author  Example code
 * @date    2025
 * 
 * This example demonstrates how to use the ICM42688P library
 */

#include "icm42688p_lib.h"
#include <stdio.h>

/* ============================================================================
 * SPI Communication Functions (User Implementation Required)
 * ============================================================================ */

/**
 * @brief Read a single register via SPI
 * @param reg Register address
 * @return Register value
 */
uint8_t my_spi_read_reg(uint8_t reg)
{
    uint8_t value;
    
    // Example implementation (adjust for your hardware):
    // 1. Assert CS (Chip Select) low
    // 2. Send register address with read bit (reg | 0x80)
    // 3. Read one byte
    // 4. Deassert CS high
    
    // CS_LOW();
    // spi_transfer(reg | 0x80);
    // value = spi_transfer(0x00);
    // CS_HIGH();
    
    return value;
}

/**
 * @brief Write a single register via SPI
 * @param reg Register address
 * @param value Value to write
 */
void my_spi_write_reg(uint8_t reg, uint8_t value)
{
    // Example implementation (adjust for your hardware):
    // 1. Assert CS low
    // 2. Send register address (without read bit)
    // 3. Send data byte
    // 4. Deassert CS high
    
    // CS_LOW();
    // spi_transfer(reg & 0x7F);
    // spi_transfer(value);
    // CS_HIGH();
}

/**
 * @brief Read multiple registers via SPI (burst read)
 * @param reg Starting register address
 * @param buffer Buffer to store read data
 * @param len Number of bytes to read
 */
void my_spi_read_burst(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    // Example implementation (adjust for your hardware):
    // 1. Assert CS low
    // 2. Send register address with read bit
    // 3. Read len bytes into buffer
    // 4. Deassert CS high
    
    // CS_LOW();
    // spi_transfer(reg | 0x80);
    // for (uint16_t i = 0; i < len; i++) {
    //     buffer[i] = spi_transfer(0x00);
    // }
    // CS_HIGH();
}

/**
 * @brief Delay in milliseconds
 * @param ms Milliseconds to delay
 */
void my_delay_ms(uint32_t ms)
{
    // Implement your delay function here
    // For example: HAL_Delay(ms) on STM32
}

/* ============================================================================
 * Example 1: Basic Initialization and Reading
 * ============================================================================ */

void example_basic_usage(void)
{
    icm42688p_dev_t imu;
    
    // 1. Initialize device structure
    memset(&imu, 0, sizeof(imu));
    
    // 2. Set SPI function pointers
    imu.spi_read_reg = my_spi_read_reg;
    imu.spi_write_reg = my_spi_write_reg;
    imu.spi_read_burst = my_spi_read_burst;
    imu.delay_ms = my_delay_ms;
    
    // 3. Configure device settings
    imu.config.gyro_fsr = 0;                        // ±2000 dps
    imu.config.accel_fsr = 0;                       // ±16g
    imu.config.gyro_odr = ICM42688P_ODR_1KHZ;      // 1 kHz
    imu.config.accel_odr = ICM42688P_ODR_1KHZ;     // 1 kHz
    imu.config.gyro_aaf = ICM42688P_AAF_258HZ;     // 258 Hz AAF
    imu.config.accel_aaf = ICM42688P_AAF_258HZ;    // 258 Hz AAF
    imu.config.enable_gyro = true;
    imu.config.enable_accel = true;
    imu.config.enable_temp = true;
    imu.config.use_ext_clk = false;
    
    // 4. Initialize the device
    if (!icm42688p_init(&imu)) {
        printf("ICM42688P initialization failed!\n");
        return;
    }
    
    printf("ICM42688P initialized successfully!\n");
    
    // 5. Read sensor data
    icm42688p_gyro_data_t gyro;
    icm42688p_accel_data_t accel;
    icm42688p_temp_data_t temp;
    
    while (1) {
        // Read all sensors at once
        if (icm42688p_read_all(&imu, &gyro, &accel, &temp)) {
            printf("Gyro: X=%d Y=%d Z=%d\n", gyro.x, gyro.y, gyro.z);
            printf("Accel: X=%d Y=%d Z=%d\n", accel.x, accel.y, accel.z);
            printf("Temp: %.2f°C\n", temp.celsius);
        }
        
        my_delay_ms(10);  // 100 Hz read rate
    }
}

/* ============================================================================
 * Example 2: High-Speed Configuration (8 kHz)
 * ============================================================================ */

void example_high_speed(void)
{
    icm42688p_dev_t imu;
    
    memset(&imu, 0, sizeof(imu));
    
    // Set function pointers
    imu.spi_read_reg = my_spi_read_reg;
    imu.spi_write_reg = my_spi_write_reg;
    imu.spi_read_burst = my_spi_read_burst;
    imu.delay_ms = my_delay_ms;
    
    // High-speed configuration
    imu.config.gyro_fsr = 0;                        // ±2000 dps
    imu.config.accel_fsr = 0;                       // ±16g
    imu.config.gyro_odr = ICM42688P_ODR_8KHZ;      // 8 kHz (maximum)
    imu.config.accel_odr = ICM42688P_ODR_8KHZ;     // 8 kHz
    imu.config.gyro_aaf = ICM42688P_AAF_1962HZ;    // 1962 Hz AAF (wide bandwidth)
    imu.config.accel_aaf = ICM42688P_AAF_997HZ;    // 997 Hz AAF
    imu.config.enable_gyro = true;
    imu.config.enable_accel = true;
    imu.config.enable_temp = false;                 // Disable temp for max speed
    imu.config.use_ext_clk = false;
    
    if (!icm42688p_init(&imu)) {
        printf("ICM42688P initialization failed!\n");
        return;
    }
    
    printf("ICM42688P configured for 8 kHz operation\n");
    
    // High-speed data reading
    icm42688p_gyro_data_t gyro;
    
    while (1) {
        if (icm42688p_read_gyro(&imu, &gyro)) {
            // Process gyro data at 8 kHz
            // Convert to dps: value / gyro_scale
            float gyro_x_dps = gyro.x / imu.gyro_scale;
            float gyro_y_dps = gyro.y / imu.gyro_scale;
            float gyro_z_dps = gyro.z / imu.gyro_scale;
            
            // Your high-speed processing here
        }
        
        // No delay - read as fast as possible (limited by SPI speed)
    }
}

/* ============================================================================
 * Example 3: Calibration
 * ============================================================================ */

void example_calibration(void)
{
    icm42688p_dev_t imu;
    
    memset(&imu, 0, sizeof(imu));
    
    // Set function pointers
    imu.spi_read_reg = my_spi_read_reg;
    imu.spi_write_reg = my_spi_write_reg;
    imu.spi_read_burst = my_spi_read_burst;
    imu.delay_ms = my_delay_ms;
    
    // Standard configuration
    imu.config.gyro_fsr = 0;
    imu.config.accel_fsr = 0;
    imu.config.gyro_odr = ICM42688P_ODR_1KHZ;
    imu.config.accel_odr = ICM42688P_ODR_1KHZ;
    imu.config.gyro_aaf = ICM42688P_AAF_258HZ;
    imu.config.accel_aaf = ICM42688P_AAF_258HZ;
    imu.config.enable_gyro = true;
    imu.config.enable_accel = true;
    imu.config.enable_temp = true;
    imu.config.use_ext_clk = false;
    
    if (!icm42688p_init(&imu)) {
        printf("ICM42688P initialization failed!\n");
        return;
    }
    
    printf("Starting calibration...\n");
    printf("Keep the device stationary!\n");
    
    // Calibrate gyroscope (1000 samples)
    if (icm42688p_calibrate_gyro(&imu, 1000)) {
        printf("Gyro calibration complete!\n");
        printf("Offsets: X=%d Y=%d Z=%d\n", 
               imu.gyro_offset[0], 
               imu.gyro_offset[1], 
               imu.gyro_offset[2]);
    } else {
        printf("Gyro calibration failed!\n");
    }
    
    // Calibrate accelerometer (1000 samples)
    // Note: Device must be level (Z-axis pointing up)
    printf("Place device level for accel calibration...\n");
    my_delay_ms(2000);
    
    if (icm42688p_calibrate_accel(&imu, 1000)) {
        printf("Accel calibration complete!\n");
        printf("Offsets: X=%d Y=%d Z=%d\n", 
               imu.accel_offset[0], 
               imu.accel_offset[1], 
               imu.accel_offset[2]);
    } else {
        printf("Accel calibration failed!\n");
    }
    
    // Now read calibrated data
    icm42688p_gyro_data_t gyro;
    icm42688p_accel_data_t accel;
    
    while (1) {
        if (icm42688p_read_gyro(&imu, &gyro)) {
            // Gyro data is now offset-corrected
            printf("Gyro (calibrated): X=%d Y=%d Z=%d\n", gyro.x, gyro.y, gyro.z);
        }
        
        if (icm42688p_read_accel(&imu, &accel)) {
            // Accel data is now offset-corrected
            printf("Accel (calibrated): X=%d Y=%d Z=%d\n", accel.x, accel.y, accel.z);
        }
        
        my_delay_ms(100);
    }
}

/* ============================================================================
 * Example 4: Interrupt-Driven Reading
 * ============================================================================ */

volatile bool data_ready = false;

// This should be called from your EXTI interrupt handler
void icm42688p_data_ready_isr(void)
{
    data_ready = true;
}

void example_interrupt_driven(void)
{
    icm42688p_dev_t imu;
    
    memset(&imu, 0, sizeof(imu));
    
    // Set function pointers
    imu.spi_read_reg = my_spi_read_reg;
    imu.spi_write_reg = my_spi_write_reg;
    imu.spi_read_burst = my_spi_read_burst;
    imu.delay_ms = my_delay_ms;
    
    // Configuration
    imu.config.gyro_fsr = 0;
    imu.config.accel_fsr = 0;
    imu.config.gyro_odr = ICM42688P_ODR_1KHZ;
    imu.config.accel_odr = ICM42688P_ODR_1KHZ;
    imu.config.gyro_aaf = ICM42688P_AAF_258HZ;
    imu.config.accel_aaf = ICM42688P_AAF_258HZ;
    imu.config.enable_gyro = true;
    imu.config.enable_accel = true;
    imu.config.enable_temp = true;
    imu.config.use_ext_clk = false;
    
    if (!icm42688p_init(&imu)) {
        printf("ICM42688P initialization failed!\n");
        return;
    }
    
    // Configure interrupt (already done in init, but shown here for clarity)
    icm42688p_config_interrupt(&imu, 
                               ICM42688P_INT1_MODE_PULSED,
                               ICM42688P_INT1_POLARITY_HIGH,
                               ICM42688P_INT1_DRIVE_PP);
    
    icm42688p_enable_data_ready_interrupt(&imu, true);
    
    printf("Interrupt-driven mode enabled\n");
    
    // Main loop - only read when data is ready
    icm42688p_gyro_data_t gyro;
    icm42688p_accel_data_t accel;
    
    while (1) {
        if (data_ready) {
            data_ready = false;
            
            // Read sensor data
            if (icm42688p_read_gyro(&imu, &gyro) && 
                icm42688p_read_accel(&imu, &accel)) {
                
                // Process data
                printf("Gyro: X=%d Y=%d Z=%d | Accel: X=%d Y=%d Z=%d\n",
                       gyro.x, gyro.y, gyro.z,
                       accel.x, accel.y, accel.z);
            }
        }
        
        // Do other tasks while waiting for interrupt
    }
}

/* ============================================================================
 * Example 5: Converting to Physical Units
 * ============================================================================ */

void example_physical_units(void)
{
    icm42688p_dev_t imu;
    
    memset(&imu, 0, sizeof(imu));
    
    // Set function pointers
    imu.spi_read_reg = my_spi_read_reg;
    imu.spi_write_reg = my_spi_write_reg;
    imu.spi_read_burst = my_spi_read_burst;
    imu.delay_ms = my_delay_ms;
    
    // Configuration
    imu.config.gyro_fsr = 0;                        // ±2000 dps
    imu.config.accel_fsr = 0;                       // ±16g
    imu.config.gyro_odr = ICM42688P_ODR_1KHZ;
    imu.config.accel_odr = ICM42688P_ODR_1KHZ;
    imu.config.gyro_aaf = ICM42688P_AAF_258HZ;
    imu.config.accel_aaf = ICM42688P_AAF_258HZ;
    imu.config.enable_gyro = true;
    imu.config.enable_accel = true;
    imu.config.enable_temp = true;
    imu.config.use_ext_clk = false;
    
    if (!icm42688p_init(&imu)) {
        printf("ICM42688P initialization failed!\n");
        return;
    }
    
    printf("Reading sensor data in physical units...\n");
    
    icm42688p_gyro_data_t gyro;
    icm42688p_accel_data_t accel;
    icm42688p_temp_data_t temp;
    
    while (1) {
        if (icm42688p_read_all(&imu, &gyro, &accel, &temp)) {
            // Convert gyro to degrees per second (dps)
            float gyro_x_dps = gyro.x / imu.gyro_scale;
            float gyro_y_dps = gyro.y / imu.gyro_scale;
            float gyro_z_dps = gyro.z / imu.gyro_scale;
            
            // Convert accel to g (gravity units)
            float accel_x_g = accel.x / imu.accel_scale;
            float accel_y_g = accel.y / imu.accel_scale;
            float accel_z_g = accel.z / imu.accel_scale;
            
            // Convert accel to m/s² (multiply by 9.81)
            float accel_x_ms2 = accel_x_g * 9.81f;
            float accel_y_ms2 = accel_y_g * 9.81f;
            float accel_z_ms2 = accel_z_g * 9.81f;
            
            printf("Gyro (dps): X=%.2f Y=%.2f Z=%.2f\n", 
                   gyro_x_dps, gyro_y_dps, gyro_z_dps);
            printf("Accel (g): X=%.2f Y=%.2f Z=%.2f\n", 
                   accel_x_g, accel_y_g, accel_z_g);
            printf("Accel (m/s²): X=%.2f Y=%.2f Z=%.2f\n", 
                   accel_x_ms2, accel_y_ms2, accel_z_ms2);
            printf("Temperature: %.2f°C\n\n", temp.celsius);
        }
        
        my_delay_ms(100);
    }
}

/* ============================================================================
 * Main Function
 * ============================================================================ */

int main(void)
{
    // Initialize your hardware (SPI, GPIO, etc.)
    // ...
    
    // Run one of the examples:
    example_basic_usage();
    // example_high_speed();
    // example_calibration();
    // example_interrupt_driven();
    // example_physical_units();
    
    return 0;
}
