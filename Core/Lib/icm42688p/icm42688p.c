#include "icm42688p.h"
#include "icm42688p_lib.h"
#include "bsp_pins.h"

extern SPI_HandleTypeDef hspi1;
icm42688p_dev_t icm;

/* === 数据就绪标志位 === */
// 数据就绪标志位（中断中设置，主循环中清除）
volatile uint8_t icm42688p_data_ready = 0;
volatile uint8_t spi1_dma_flag = 0;

/* === 写寄存器函数 === */
void icm_spi_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t tx[2];
    tx[0] = reg & 0x7F;   // bit7=0 表示写
    tx[1] = value;

    ICM42688P_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);
    ICM42688P_CS_HIGH();
}

/* === 读单个寄存器函数 === */
uint8_t icm_spi_read_reg(uint8_t reg)
{
    uint8_t tx[2];
    uint8_t rx[2];
    tx[0] = reg | 0x80;   // bit7=1 表示读
    tx[1] = 0xFF;         // dummy

    ICM42688P_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 2, HAL_MAX_DELAY);
    ICM42688P_CS_HIGH();

    return rx[1];  // 返回寄存器数据
}

/* === 连续读寄存器（DMA方式） === */
void icm_spi_read_burst(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    reg |= 0x80;  // 读命令

    ICM42688P_CS_LOW();
    spi1_dma_flag=0;

    // 先发出起始寄存器地址
    HAL_SPI_Transmit(&hspi1, &reg, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive_DMA(&hspi1, buffer, len);
}

/* === 延时函数 === */
void icm_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}


void icm42688p_init_driver(void)
{
    // 绑定SPI读写函数
    icm.spi_read_reg  = icm_spi_read_reg;
    icm.spi_write_reg = icm_spi_write_reg;
    icm.spi_read_burst = icm_spi_read_burst;
    icm.delay_ms = icm_delay_ms;

    // ===== 默认配置 =====
    icm.config.gyro_fsr   = ICM42688P_GYRO_FSR_2000DPS;   // 陀螺仪量程 ±2000 dps
    icm.config.accel_fsr  = ICM42688P_ACCEL_FSR_16G;      // 加速度计量程 ±16g
    icm.config.gyro_odr   = ICM42688P_ODR_8KHZ;           // 陀螺仪采样率 8kHz
    icm.config.accel_odr  = ICM42688P_ODR_1KHZ;           // 加速度计采样率 1kHz

    // AAF（抗混叠滤波器）配置
    icm.config.gyro_aaf  = ICM42688P_AAF_536HZ;   // 陀螺仪 AAF 536Hz
    icm.config.accel_aaf = ICM42688P_AAF_536HZ;   // 加速度计 AAF 536Hz

    // 启用哪些模块
    icm.config.enable_gyro  = true;
    icm.config.enable_accel = true;
    icm.config.enable_temp  = true;
    icm.config.use_ext_clk  = false; // 不使用外部时钟

    // ===== 检测并初始化芯片 =====
    if (icm42688p_detect(&icm))
    {
        printf("ICM42688P detected!\r\n");
        
        // 执行完整初始化
        if (icm42688p_init(&icm))
        {
            printf("ICM42688P initialized successfully!\r\n");
        }
        else
        {
            printf("ICM42688P initialization failed!\r\n");
        }
    }
    else
    {
        printf("ICM42688P not found!\r\n");
    }
}

/**
 * @brief 读取陀螺仪数据示例
 * @param gyro_x, gyro_y, gyro_z 输出陀螺仪三轴原始数据
 * @return 读取成功返回true
 */
bool icm42688p_get_gyro_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    icm42688p_gyro_data_t gyro_data;
    
    if (icm42688p_read_gyro(&icm, &gyro_data))
    {
        *gyro_x = gyro_data.x;
        *gyro_y = gyro_data.y;
        *gyro_z = gyro_data.z;
        return true;
    }
    return false;
}

/**
 * @brief 读取加速度计数据示例
 * @param accel_x, accel_y, accel_z 输出加速度计三轴原始数据
 * @return 读取成功返回true
 */
bool icm42688p_get_accel_data(int16_t *accel_x, int16_t *accel_y, int16_t *accel_z)
{
    icm42688p_accel_data_t accel_data;
    
    if (icm42688p_read_accel(&icm, &accel_data))
    {
        *accel_x = accel_data.x;
        *accel_y = accel_data.y;
        *accel_z = accel_data.z;
        return true;
    }
    return false;
}

/**
 * @brief 读取温度数据示例
 * @param temp_celsius 输出温度值（摄氏度）
 * @return 读取成功返回true
 */
bool icm42688p_get_temperature(float *temp_celsius)
{
    icm42688p_temp_data_t temp_data;
    
    if (icm42688p_read_temp(&icm, &temp_data))
    {
        *temp_celsius = temp_data.celsius;
        return true;
    }
    return false;
}

/**
 * @brief 一次性读取所有传感器数据
 * @param gyro_x, gyro_y, gyro_z 陀螺仪三轴数据
 * @param accel_x, accel_y, accel_z 加速度计三轴数据
 * @param temp_celsius 温度数据
 * @return 读取成功返回true
 */
bool icm42688p_get_all_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                            int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                            float *temp_celsius)
{
    icm42688p_gyro_data_t gyro_data;
    icm42688p_accel_data_t accel_data;
    icm42688p_temp_data_t temp_data;
    
    if (icm42688p_read_all(&icm, &gyro_data, &accel_data, &temp_data))
    {
        *gyro_x = gyro_data.x;
        *gyro_y = gyro_data.y;
        *gyro_z = gyro_data.z;
        
        *accel_x = accel_data.x;
        *accel_y = accel_data.y;
        *accel_z = accel_data.z;
        
        *temp_celsius = temp_data.celsius;
        return true;
    }
    return false;
}

/**
 * @brief GPIO外部中断回调函数 - ICM42688P数据就绪中断
 * @param GPIO_Pin 触发中断的引脚
 * @note 中断中只设置标志位，数据读取在主循环中进行
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == ICM42688P_INT_PIN)
  {
    /* ICM42688P数据就绪中断 - 设置标志位 */
    icm42688p_data_ready = 1;
  }
}

/**
 * @brief 更新ICM42688P传感器数据
 * @param gyro_x, gyro_y, gyro_z 陀螺仪三轴数据
 * @param accel_x, accel_y, accel_z 加速度计三轴数据
 * @param temp_celsius 温度数据
 * @return 如果有新数据并读取成功返回true，否则返回false
 * @note 检查标志位，如果有新数据则读取并清除标志位
 */
bool icm42688p_update(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                      int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                      float *temp_celsius)
{
    // 检查数据就绪标志位
    if (!icm42688p_data_ready)
    {
        return false;
    }
    
    // 清除标志位
    icm42688p_data_ready = 0;
    
    // 读取传感器数据
    icm42688p_gyro_data_t gyro_data;
    icm42688p_accel_data_t accel_data;
    icm42688p_temp_data_t temp_data;
    
    if (icm42688p_read_all(&icm, &gyro_data, &accel_data, &temp_data))
    {
        // 复制数据到输出参数
        *gyro_x = gyro_data.x;
        *gyro_y = gyro_data.y;
        *gyro_z = gyro_data.z;
        
        *accel_x = accel_data.x;
        *accel_y = accel_data.y;
        *accel_z = accel_data.z;
        
        *temp_celsius = temp_data.celsius;
        
        return true;
    }
    
    return false;
}

/**
 * @brief SPI DMA接收完成回调函数
 * @param hspi SPI句柄
 * @note DMA传输完成后拉高CS引脚
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        // DMA接收完成，拉高CS
        spi1_dma_flag=1;
        ICM42688P_CS_HIGH();
    }
}