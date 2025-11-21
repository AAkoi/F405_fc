#include "icm42688p.h"
#include "icm42688p_lib.h"
#include "bsp_pins.h"
#include "attitude.h"
#include <stdlib.h>

extern SPI_HandleTypeDef hspi1;
icm42688p_dev_t icm;

// Data ready flag (set in EXTI, cleared in update)
volatile uint8_t icm42688p_data_ready = 0;
volatile uint8_t spi1_dma_flag = 0;

// Low-level SPI helpers
void icm_spi_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    tx[0] = reg & 0x7F;   // bit7 = 0 -> write
    tx[1] = value;

    ICM42688P_CS_LOW();
    
    // 写操作用轮询模式即可（写不需要接收，开销很小）
    // 且写操作通常在初始化时使用，不是高频操作
    HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
    
    ICM42688P_CS_HIGH();
}

uint8_t icm_spi_read_reg(uint8_t reg)
{
    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = reg | 0x80;   // bit7 = 1 -> read
    tx[1] = 0xFF;         // dummy

    ICM42688P_CS_LOW();
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);
    ICM42688P_CS_HIGH();
    
    if (status != HAL_OK) {
        printf("[read_reg] HAL_SPI error, status=%d\r\n", status);
        return 0xFF;
    }

    return rx[1];
}

void icm_spi_read_burst(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    reg |= 0x80;  // read command

    ICM42688P_CS_LOW();
    spi1_dma_flag = 0;

    // 发出寄存器地址（同步）
    HAL_SPI_Transmit(&hspi1, &reg, 1, HAL_MAX_DELAY);

    // 清空地址阶段产生的残留
    if (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE)) {
        volatile uint8_t dummy = *(__IO uint8_t *)&hspi1.Instance->DR;
        (void)dummy;
    }
    if (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_OVR)) {
        __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
    }
    
    // 启动DMA接收
    HAL_StatusTypeDef dma_status = HAL_SPI_Receive_DMA(&hspi1, buffer, len);
    if (dma_status != HAL_OK) {  // 简单失败处理
        ICM42688P_CS_HIGH();
        return;
    }
    
    // 等待DMA完成
    uint32_t timeout_start = HAL_GetTick();
    while (spi1_dma_flag == 0) {
        if ((HAL_GetTick() - timeout_start) > 50) { // 最多等待50ms
            HAL_SPI_DMAStop(&hspi1);
            ICM42688P_CS_HIGH();
            hspi1.State = HAL_SPI_STATE_READY;
            break;
        }
    }
    __DSB();
}

void icm_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

// High-level driver init
void icm42688p_init_driver(void)
{
    // Bind SPI helpers
    icm.spi_read_reg   = icm_spi_read_reg;
    icm.spi_write_reg  = icm_spi_write_reg;
    icm.spi_read_burst = icm_spi_read_burst;
    icm.delay_ms       = icm_delay_ms;

    HAL_Delay(100);  // 等待传感器上电稳定
    uint8_t whoami = icm_spi_read_reg(0x75);
    printf("ICM42688P WHO_AM_I=0x%02X\r\n", whoami);

    icm.config.gyro_fsr   = ICM42688P_GYRO_FSR_2000DPS;
    icm.config.accel_fsr  = ICM42688P_ACCEL_FSR_16G;
    icm.config.gyro_odr   = ICM42688P_ODR_8KHZ;
    icm.config.accel_odr  = ICM42688P_ODR_1KHZ;
    icm.config.gyro_aaf   = ICM42688P_AAF_536HZ;
    icm.config.accel_aaf  = ICM42688P_AAF_536HZ;
    icm.config.enable_gyro  = true;
    icm.config.enable_accel = true;
    icm.config.enable_temp  = true;
    icm.config.use_ext_clk  = false;

    if (!icm42688p_init(&icm)) {
        printf("ICM42688P init failed\r\n");
    }
}

// Simple raw read helpers
bool icm42688p_get_gyro_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    icm42688p_gyro_data_t d;
    if (!icm42688p_read_gyro(&icm, &d)) {
        return false;
    }
    if (gyro_x) *gyro_x = d.x;
    if (gyro_y) *gyro_y = d.y;
    if (gyro_z) *gyro_z = d.z;
    return true;
}

bool icm42688p_get_accel_data(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z)
{
    icm42688p_accel_data_t d;
    if (!icm42688p_read_accel(&icm, &d)) {
        return false;
    }
    if (accel_x) *accel_x = d.x;
    if (accel_y) *accel_y = d.y;
    if (accel_z) *accel_z = d.z;
    return true;
}

bool icm42688p_get_temperature(float *temp_celsius)
{
    icm42688p_temp_data_t t;
    if (!icm42688p_read_temp(&icm, &t)) {
        return false;
    }
    if (temp_celsius) *temp_celsius = t.celsius;
    return true;
}

bool icm42688p_get_all_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                            int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                            float *temp_celsius)
{
    icm42688p_gyro_data_t  gd;
    icm42688p_accel_data_t ad;
    icm42688p_temp_data_t  td;

    if (!icm42688p_read_all(&icm, &gd, &ad, &td)) {
        return false;
    }

    if (gyro_x)  *gyro_x  = gd.x;
    if (gyro_y)  *gyro_y  = gd.y;
    if (gyro_z)  *gyro_z  = gd.z;
    if (accel_x) *accel_x = ad.x;
    if (accel_y) *accel_y = ad.y;
    if (accel_z) *accel_z = ad.z;
    if (temp_celsius) *temp_celsius = td.celsius;
    return true;
}

// EXTI callback: data ready interrupt from ICM42688P
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ICM42688P_INT_PIN) {
        icm42688p_data_ready = 1;
    }
}

// Update using data-ready flag
bool icm42688p_update(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                      int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                      float *temp_celsius)
{
    // 始终直接读取（即使没有中断标志）
    return icm42688p_get_all_data(gyro_x, gyro_y, gyro_z,
                                  accel_x, accel_y, accel_z,
                                  temp_celsius);
}

// Read + convert to physical units
bool icm42688p_dataPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                              int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                              float *gyro_x_norm, float *gyro_y_norm, float *gyro_z_norm,
                              float *accel_x_norm, float *accel_y_norm, float *accel_z_norm,
                              float *temp_celsius)
{
    icm42688p_gyro_data_t  gd;
    icm42688p_accel_data_t ad;
    icm42688p_temp_data_t  td;

    if (!icm42688p_read_all(&icm, &gd, &ad, &td)) {
        return false;
    }
    if (gyro_x)  *gyro_x  = gd.x;
    if (gyro_y)  *gyro_y  = gd.y;
    if (gyro_z)  *gyro_z  = gd.z;
    if (accel_x) *accel_x = ad.x;
    if (accel_y) *accel_y = ad.y;
    if (accel_z) *accel_z = ad.z;
    if (temp_celsius) *temp_celsius = td.celsius;

    // Convert to physical units using current scale factors
    float gscale = (icm.gyro_scale  > 0.0f) ? icm.gyro_scale  : 1.0f;
    float ascale = (icm.accel_scale > 0.0f) ? icm.accel_scale : 1.0f;

    if (gyro_x && gyro_x_norm)   *gyro_x_norm   = (float)(*gyro_x)  / gscale;
    if (gyro_y && gyro_y_norm)   *gyro_y_norm   = (float)(*gyro_y)  / gscale;
    if (gyro_z && gyro_z_norm)   *gyro_z_norm   = (float)(*gyro_z)  / gscale;
    if (accel_x && accel_x_norm) *accel_x_norm  = (float)(*accel_x) / ascale;
    if (accel_y && accel_y_norm) *accel_y_norm  = (float)(*accel_y) / ascale;
    if (accel_z && accel_z_norm) *accel_z_norm  = (float)(*accel_z) / ascale;

    return true;
}

bool icm42688p_gyro_dataPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                                   float *gyro_x_norm, float *gyro_y_norm, float *gyro_z_norm)
{
    float gscale = (icm.gyro_scale > 0.0f) ? icm.gyro_scale : 1.0f;

    if (gyro_x && gyro_x_norm) *gyro_x_norm = (float)(*gyro_x) / gscale;
    if (gyro_y && gyro_y_norm) *gyro_y_norm = (float)(*gyro_y) / gscale;
    if (gyro_z && gyro_z_norm) *gyro_z_norm = (float)(*gyro_z) / gscale;

    // At least one output written?
    return (gyro_x && gyro_x_norm) || (gyro_y && gyro_y_norm) || (gyro_z && gyro_z_norm);
}

// SPI1 RX DMA complete callback
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        // !! 关键1：确保DMA数据已写入RAM（内存屏障）
        __DSB();  // 数据同步屏障 - 确保DMA写入完成
        __ISB();  // 指令同步屏障 - 刷新流水线
        
        // !! 关键2：等待SPI硬件完全空闲
        uint32_t timeout = 10000;
        while ((__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_BSY)) && timeout--);
        
        ICM42688P_CS_HIGH();         // 释放片选
        
        // 清除可能残留的RXNE标志
        if (__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXNE)) {
            volatile uint8_t dummy = *(__IO uint8_t *)&hspi->Instance->DR;
            (void)dummy;
        }
        
        // 确保状态为READY
        hspi->State = HAL_SPI_STATE_READY;
        
        // !! 关键3：再次内存屏障，确保状态修改对主程序可见
        __DSB();
        
        spi1_dma_flag = 1;           // 最后设置完成标志
    }
}

// DMA错误回调（可选，用于调试）
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        ICM42688P_CS_HIGH();
        hspi->State = HAL_SPI_STATE_READY;
        spi1_dma_flag = 1;
    }
}
