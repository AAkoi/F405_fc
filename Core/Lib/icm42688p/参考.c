#include "icm42688p.h"
#include "icm42688p_lib.h"
#include "bsp_pins.h"
#include "attitude.h"
#include "hmc5883l.h"
#include <stdlib.h>
#include <limits.h>

#define USE_DMA
extern SPI_HandleTypeDef hspi1;
icm42688p_dev_t icm;

// ============================================================================
// 状态机标志 - 按照"快路径/慢路径分离"原则
// ============================================================================

// DRDY 中断计数器（ISR 只增加，主循环消费）
volatile uint32_t icm42688p_drdy_count = 0;

// DMA 状态：0=空闲 1=数据就绪 2=DMA忙碌中
volatile uint8_t spi1_dma_flag = 0;

// ============================================================================
// 低层 SPI 操作
// ============================================================================

void icm_spi_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    tx[0] = reg & 0x7F;   // bit7 = 0 -> write
    tx[1] = value;

    ICM42688P_CS_LOW();
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

// ============================================================================
// 关键函数：icm_spi_read_burst - DMA 读取（按照稳定状态机设计）
// ============================================================================

void icm_spi_read_burst(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    reg |= 0x80;  // read command

#ifdef USE_DMA
    // ========================================================================
    // DMA 路径：非阻塞异步读取
    // ========================================================================
    
    // 检查 DMA 是否空闲
    if (spi1_dma_flag != 0) {
        // DMA 正忙，直接丢弃本次请求（主循环下次会重试）
        return;
    }
    
    // 标记 DMA 进入忙碌状态
    spi1_dma_flag = 2;
    
    // CS 拉低（只在这里拉低）
    ICM42688P_CS_LOW();
    
    // 先用轮询发送寄存器地址
    HAL_SPI_Transmit(&hspi1, &reg, 1, 100);
    
    // 启动 DMA 接收数据
    if (HAL_SPI_Receive_DMA(&hspi1, buffer, len) != HAL_OK) {
        // DMA 启动失败，清理状态并使用阻塞模式
        ICM42688P_CS_HIGH();
        spi1_dma_flag = 0;
        goto use_blocking_fallback;
    }
    
    // DMA 已启动，直接返回（回调会处理后续）
    return;
    
#endif

use_blocking_fallback:
    // ========================================================================
    // 阻塞模式后备方案
    // ========================================================================
    
    ICM42688P_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &reg, 1, 100);
    
    uint8_t tx_dummy = 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        HAL_SPI_TransmitReceive(&hspi1, &tx_dummy, &buffer[i], 1, 100);
    }
    
    ICM42688P_CS_HIGH();
}

void icm_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

// ============================================================================
// DMA 完成回调 - 只在这里拉高 CS
// ============================================================================

void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        // 确保 DMA 数据已写入 RAM
        __DSB();  // 数据同步屏障
        __ISB();  // 指令同步屏障
        
        // CS 拉高（只在这里拉高，与启动处对应）
        ICM42688P_CS_HIGH();
        
        // 标记数据就绪
        spi1_dma_flag = 1;
        
        __DSB();  // 确保状态修改对主程序可见
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        ICM42688P_CS_HIGH();
        hspi->State = HAL_SPI_STATE_READY;
        spi1_dma_flag = 0;  // 直接回到空闲
    }
}

// ============================================================================
// EXTI 中断回调 - 只计数，不做重活
// ============================================================================

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ICM42688P_INT_PIN) {
        // 只增加计数器
        icm42688p_drdy_count++;
    }
#ifdef USE_HMC5883L_INT
    else if (GPIO_Pin == HMC5883l_INT_PIN) {
        hmc5883l_data_ready_flag = 1;
    }
#endif
}

// ============================================================================
// 高层驱动初始化
// ============================================================================

void icm42688p_init_driver(void)
{
    // 绑定 SPI 回调
    icm.spi_read_reg   = icm_spi_read_reg;
    icm.spi_write_reg  = icm_spi_write_reg;
    icm.spi_read_burst = icm_spi_read_burst;
    icm.delay_ms       = icm_delay_ms;

    // 清零校准数据
    icm.gyro_offset[0] = 0;
    icm.gyro_offset[1] = 0;
    icm.gyro_offset[2] = 0;
    icm.accel_offset[0] = 0;
    icm.accel_offset[1] = 0;
    icm.accel_offset[2] = 0;
    icm.gyro_scale = 0.0f;
    icm.accel_scale = 0.0f;

    HAL_Delay(100);
    uint8_t whoami = icm_spi_read_reg(0x75);
    printf("ICM42688P WHO_AM_I=0x%02X\r\n", whoami);

    // 配置传感器参数
    icm.config.gyro_fsr   = ICM42688P_GYRO_FSR_2000DPS;
    icm.config.accel_fsr  = ICM42688P_ACCEL_FSR_2G;
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
    } else {
        printf("ICM42688P init success\r\n");
    }
    
    // 验证传感器数据
    printf("Testing sensor data read...\r\n");
    int16_t test_gx = 0, test_gy = 0, test_gz = 0;
    if (icm42688p_get_gyro_data(&test_gx, &test_gy, &test_gz)) {
        printf("Gyro test read OK: %d %d %d\r\n", test_gx, test_gy, test_gz);
    } else {
        printf("Gyro test read FAILED!\r\n");
    }
}

bool icm42688p_calibrate(uint16_t samples)
{
    return icm42688p_calibrate_gyro(&icm, samples);
}

// ============================================================================
// 简单读取接口（用于低频查询）
// ============================================================================

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

// ============================================================================
// 主循环调用函数 - 状态机核心
// ============================================================================

bool icm42688p_get_all_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                            int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                            float *temp_celsius)
{
    icm42688p_gyro_data_t  gd;
    icm42688p_accel_data_t ad;
    icm42688p_temp_data_t  td;

    // ========================================================================
    // 步骤1：有 DRDY 且 DMA 空闲 → 启动 DMA 读取
    // ========================================================================
    if (icm42688p_drdy_count > 0 && spi1_dma_flag == 0) {
        // 消费一次 DRDY
        icm42688p_drdy_count--;
        
        // 触发底层读取（会调用 icm_spi_read_burst 启动 DMA）
        icm42688p_read_all(&icm, &gd, &ad, &td);
    }
    
    // ========================================================================
    // 步骤2：检查 DMA 是否完成
    // ========================================================================
    if (spi1_dma_flag == 1) {
        // 先清状态（锁住这帧数据）
        spi1_dma_flag = 0;
        
        // 数据已经在 icm_spi_read_burst 调用时通过 buffer 写入
        // icm42688p_read_all 内部会解析，这里只需要再次读取解析后的结果
        if (!icm42688p_read_all(&icm, &gd, &ad, &td)) {
            return false;
        }
        
        // 填充输出参数
        if (gyro_x)  *gyro_x  = gd.x;
        if (gyro_y)  *gyro_y  = gd.y;
        if (gyro_z)  *gyro_z  = gd.z;
        if (accel_x) *accel_x = ad.x;
        if (accel_y) *accel_y = ad.y;
        if (accel_z) *accel_z = ad.z;
        if (temp_celsius) *temp_celsius = td.celsius;
        
        return true;
    }
    
    // ========================================================================
    // 步骤3：防丢帧保护（可选）
    // ========================================================================
    if (icm42688p_drdy_count > 10) {
        // 累积太多，只保留最新的
        icm42688p_drdy_count = 1;
    }
    
    return false;  // 本次无新数据
}

bool icm42688p_update(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                      int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                      float *temp_celsius)
{
    return icm42688p_get_all_data(gyro_x, gyro_y, gyro_z,
                                  accel_x, accel_y, accel_z,
                                  temp_celsius);
}

// ============================================================================
// 数据预处理
// ============================================================================

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

    // 转换为物理单位
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

bool icm42688p_gyro_rawPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    // Raw 版本：仅做零偏补偿
    bool wrote = false;
    if (gyro_x) {
        int32_t v = (int32_t)(*gyro_x) - (int32_t)icm.gyro_offset[0];
        if (v > INT16_MAX) v = INT16_MAX;
        if (v < INT16_MIN) v = INT16_MIN;
        *gyro_x = (int16_t)v;
        wrote = true;
    }
    if (gyro_y) {
        int32_t v = (int32_t)(*gyro_y) - (int32_t)icm.gyro_offset[1];
        if (v > INT16_MAX) v = INT16_MAX;
        if (v < INT16_MIN) v = INT16_MIN;
        *gyro_y = (int16_t)v;
        wrote = true;
    }
    if (gyro_z) {
        int32_t v = (int32_t)(*gyro_z) - (int32_t)icm.gyro_offset[2];
        if (v > INT16_MAX) v = INT16_MAX;
        if (v < INT16_MIN) v = INT16_MIN;
        *gyro_z = (int16_t)v;
        wrote = true;
    }
    return wrote;
}

bool icm42688p_gyro_dataPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                                   float *gyro_x_norm, float *gyro_y_norm, float *gyro_z_norm)
{
    const float gscale = (icm.gyro_scale > 0.0f) ? icm.gyro_scale : 1.0f;
    bool wrote = false;
    
    if (gyro_x) {
        *gyro_x -= icm.gyro_offset[0];
        if (gyro_x_norm) {
            *gyro_x_norm = (float)(*gyro_x) / gscale;
            wrote = true;
        }
    }
    if (gyro_y) {
        *gyro_y -= icm.gyro_offset[1];
        if (gyro_y_norm) {
            *gyro_y_norm = (float)(*gyro_y) / gscale;
            wrote = true;
        }
    }
    if (gyro_z) {
        *gyro_z -= icm.gyro_offset[2];
        if (gyro_z_norm) {
            *gyro_z_norm = (float)(*gyro_z) / gscale;
            wrote = true;
        }
    }
    return wrote;
}