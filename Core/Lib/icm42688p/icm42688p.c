#include "icm42688p.h"
#include "icm42688p_lib.h"
#include "bsp_pins.h"
#include "attitude.h"
#include "hmc5883l.h"
#include <stdlib.h>
#include <limits.h>

#define USE_DMA

// ============================================================================
// 配置常量（DRDY -> DMA 状态机）
// S1/S2: DRDY 预热计数，节奏不对则回退轮询
// S3: INT 到来，拉低 CS 启动 DMA 读帧
// S4: DMA 完成，标记 data_ready
// S5: 主循环消费数据，清标志
// ============================================================================
#define ICM_EXIT_COUNTER_HOLD      32U    // S1: 需要连续 hold 次 DRDY 才进入 DMA 模式
#define ICM_EXIT_COUNTER_MIN_RATIO 0.5f   // DRDY 窗口下限（相对理想周期）
#define ICM_EXIT_COUNTER_MAX_RATIO 1.5f   // DRDY 窗口上限（相对理想周期）
#define ICM_DMA_BURST_LEN          14U    // TEMP(2)+ACCEL(6)+GYRO(6)
extern SPI_HandleTypeDef hspi1;
icm42688p_dev_t icm;

// ============================================================================
// 共享状态（ISR + 主循环）
// ============================================================================
// DRDY计数：ISR累加，主循环消费
volatile uint32_t icm42688p_drdy_count = 0;
// DMA状态：0空闲 1完成 2忙
volatile uint8_t spi1_dma_flag = 0;
// 数据就绪标志：DMA完成后置1，消费后清0
volatile uint8_t icm42688p_data_ready = 0;

#ifdef USE_DMA
typedef enum {
    ICM_DMA_STATE_POLLING = 0,   // 回退为轮询
    ICM_DMA_STATE_PREFLIGHT,     // S1/S2: exit_counter 观察窗口
    ICM_DMA_STATE_WAIT_INT       // S3: 等待 INT，拉低 CS 并启动 DMA
} icm_dma_state_t;

static volatile icm_dma_state_t icm_dma_state = ICM_DMA_STATE_POLLING;
static volatile uint32_t icm_exit_counter = 0;
static uint32_t icm_hold_start_cycles = 0;
static uint32_t icm_hold_min_cycles = 0;
static uint32_t icm_hold_max_cycles = 0;
static uint32_t icm_last_dma_cycle = 0;
static uint32_t icm_last_dma_period_cycles = 0;
static uint8_t icm_dma_rx_buffer[ICM_DMA_BURST_LEN];
static bool icm_dma_mode_enabled = false;

// ============================================================================
// DMA 状态机辅助函数
// ============================================================================
static float icm42688p_odr_to_hz(uint8_t odr)
{
    switch (odr) {
        case ICM42688P_ODR_32KHZ:   return 32000.0f;
        case ICM42688P_ODR_16KHZ:   return 16000.0f;
        case ICM42688P_ODR_8KHZ:    return 8000.0f;
        case ICM42688P_ODR_4KHZ:    return 4000.0f;
        case ICM42688P_ODR_2KHZ:    return 2000.0f;
        case ICM42688P_ODR_1KHZ:    return 1000.0f;
        case ICM42688P_ODR_500HZ:   return 500.0f;
        case ICM42688P_ODR_200HZ:   return 200.0f;
        case ICM42688P_ODR_100HZ:   return 100.0f;
        case ICM42688P_ODR_50HZ:    return 50.0f;
        case ICM42688P_ODR_25HZ:    return 25.0f;
        case ICM42688P_ODR_12_5HZ:  return 12.5f;
        case ICM42688P_ODR_6_25HZ:  return 6.25f;
        case ICM42688P_ODR_3_125HZ: return 3.125f;
        case ICM42688P_ODR_1_5625HZ:return 1.5625f;
        default:                    return 1000.0f;
    }
}

static uint32_t icm_us_to_cycles(float us)
{
    const float cycles_per_us = (float)SystemCoreClock / 1000000.0f;
    return (uint32_t)(us * cycles_per_us);
}

static void icm_enable_dwt_counter(void)
{
    // 确保 DWT 计数开启，用于窗口计时和周期测量
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0) {
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

static void icm_dma_switch_to_polling(void)
{
    // 统一清理所有 DMA 状态，回到安全的轮询模式
    icm_dma_state = ICM_DMA_STATE_POLLING;
    icm_dma_mode_enabled = false;
    icm42688p_data_ready = 0;
    spi1_dma_flag = 0;
    icm_exit_counter = 0;
    icm_last_dma_period_cycles = 0;
}

static void icm_dma_start_preflight(void)
{
    // 进入 DRDY 节奏检测阶段（S1/S2），成功后自动切换到 DMA 模式
    icm_enable_dwt_counter();

    const float odr_hz = icm42688p_odr_to_hz(icm.config.gyro_odr);
    const float frame_us = (odr_hz > 0.0f) ? (1000000.0f / odr_hz) : 0.0f;
    const float hold_us = frame_us * (float)ICM_EXIT_COUNTER_HOLD;

    if (hold_us <= 0.0f) {
        icm_dma_switch_to_polling();
        return;
    }

    icm_hold_min_cycles = icm_us_to_cycles(hold_us * ICM_EXIT_COUNTER_MIN_RATIO);
    icm_hold_max_cycles = icm_us_to_cycles(hold_us * ICM_EXIT_COUNTER_MAX_RATIO);

    icm_exit_counter = 0;
    icm42688p_data_ready = 0;
    spi1_dma_flag = 0;
    icm_dma_state = ICM_DMA_STATE_PREFLIGHT;
    icm_hold_start_cycles = DWT->CYCCNT;
    icm_last_dma_cycle = icm_hold_start_cycles;
    icm_last_dma_period_cycles = 0;
    icm_dma_mode_enabled = true;
}

static void icm_parse_all_buffer(const uint8_t *buffer,
                                 icm42688p_gyro_data_t *gyro,
                                 icm42688p_accel_data_t *accel,
                                 icm42688p_temp_data_t *temp)
{
    temp->raw = (int16_t)((buffer[0] << 8) | buffer[1]);
    temp->celsius = (temp->raw / 132.48f) + 25.0f;

    accel->x = (int16_t)((buffer[2] << 8) | buffer[3]);
    accel->y = (int16_t)((buffer[4] << 8) | buffer[5]);
    accel->z = (int16_t)((buffer[6] << 8) | buffer[7]);

    gyro->x = (int16_t)((buffer[8] << 8) | buffer[9]);
    gyro->y = (int16_t)((buffer[10] << 8) | buffer[11]);
    gyro->z = (int16_t)((buffer[12] << 8) | buffer[13]);
}

static bool icm_dma_frame_available(icm42688p_gyro_data_t *gyro,
                                    icm42688p_accel_data_t *accel,
                                    icm42688p_temp_data_t *temp)
{
    // DMA 未启用或已回退
    if (!icm_dma_mode_enabled || icm_dma_state == ICM_DMA_STATE_POLLING) {
        return false;
    }

    // 预热窗口：只做节奏检测，超时则回退
    if (icm_dma_state == ICM_DMA_STATE_PREFLIGHT) {
        const uint32_t elapsed = DWT->CYCCNT - icm_hold_start_cycles;
        if (elapsed > icm_hold_max_cycles) {
            icm_dma_switch_to_polling();
        }
        return false;
    }

    // 数据就绪：解析固定 DMA 缓冲
    if (icm42688p_data_ready && spi1_dma_flag == 1) {
        icm_parse_all_buffer(icm_dma_rx_buffer, gyro, accel, temp);
        icm42688p_data_ready = 0; // S5：消费后清除
        spi1_dma_flag = 0;
        return true;
    }

    return false;
}

static void icm_dma_handle_int(uint32_t now)
{
    // 未开启 DMA 模式：回退为计数逻辑
    if (!icm_dma_mode_enabled) {
        icm42688p_drdy_count++;
        return;
    }

    // S1/S2：前期 exit_counter 窗口检测
    if (icm_dma_state == ICM_DMA_STATE_PREFLIGHT) {
        icm_exit_counter++;
        if (icm_exit_counter >= ICM_EXIT_COUNTER_HOLD) {
            const uint32_t elapsed = now - icm_hold_start_cycles;
            const bool within_window = (elapsed >= icm_hold_min_cycles) && (elapsed <= icm_hold_max_cycles);
            if (within_window) {
                icm_dma_state = ICM_DMA_STATE_WAIT_INT;  // S2 完成，等待下一次 INT 触发 DMA
                icm_last_dma_cycle = now;
            } else {
                icm_dma_switch_to_polling();  // 节奏不对，回退轮询
                icm42688p_drdy_count++;
            }
        }
        return;  // 预热阶段不启动 DMA
    }

    // S3：稳定后，每个 INT 拉低 CS 并启动 DMA
    if (icm_dma_state == ICM_DMA_STATE_WAIT_INT) {
        // 未消费完上一帧或 DMA 尚未释放，则直接丢弃本次中断
        if (spi1_dma_flag != 0 || icm42688p_data_ready) {
            return;
        }

        uint8_t addr = ICM42688P_REG_TEMP_DATA1 | 0x80;
        ICM42688P_CS_LOW();

        if (HAL_SPI_Transmit(&hspi1, &addr, 1, 100) != HAL_OK) {
            ICM42688P_CS_HIGH();
            icm_dma_switch_to_polling();
            icm42688p_drdy_count++;
            return;
        }

        spi1_dma_flag = 2;
        const HAL_StatusTypeDef dma_status = HAL_SPI_Receive_DMA(&hspi1, icm_dma_rx_buffer, ICM_DMA_BURST_LEN);
        if (dma_status != HAL_OK) {
            ICM42688P_CS_HIGH();
            icm_dma_switch_to_polling();
            icm42688p_drdy_count++;
            return;
        }

        icm_last_dma_period_cycles = now - icm_last_dma_cycle;
        icm_last_dma_cycle = now;
        return;
    }

    // 兜底：未知状态时回退计数
    icm42688p_drdy_count++;
}
#endif

// ============================================================================
// 低层 SPI 接口（轮询）
// ============================================================================
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

    // 使用稳定的轮询模式（DMA 由中断状态机驱动，此处只做地址+数据轮询）
    ICM42688P_CS_LOW();

    uint8_t tx_dummy = 0xFF;
    HAL_SPI_Transmit(&hspi1, &reg, 1, 100);
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
// 高层驱动初始化与配置
// ============================================================================
void icm42688p_init_driver(void)
{
    // Bind SPI helpers
    icm.spi_read_reg   = icm_spi_read_reg;
    icm.spi_write_reg  = icm_spi_write_reg;
    icm.spi_read_burst = icm_spi_read_burst;
    icm.delay_ms       = icm_delay_ms;

    // 清零校准数据（防止垃圾值或旧值干扰）
    icm.gyro_offset[0] = 0;
    icm.gyro_offset[1] = 0;
    icm.gyro_offset[2] = 0;
    icm.accel_offset[0] = 0;
    icm.accel_offset[1] = 0;
    icm.accel_offset[2] = 0;
    icm.gyro_scale = 0.0f;
    icm.accel_scale = 0.0f;

    HAL_Delay(100);  // 等待传感器上电稳定
    uint8_t whoami = icm_spi_read_reg(0x75);
    printf("ICM42688P WHO_AM_I=0x%02X\r\n", whoami);

    icm.config.gyro_fsr   = ICM42688P_GYRO_FSR_2000DPS;
    // 使用 ±2g 量程，静止时加速度应接近 1g，避免 8g 缩放误差
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
#ifdef USE_DMA
        icm_dma_switch_to_polling();
#else
        icm42688p_data_ready = 0;
        spi1_dma_flag = 0;
#endif
    } else {
        printf("ICM42688P init success\r\n");
    
#ifdef USE_DMA
        // 初始化后立即进入 exit_counter 计数阶段，验证 DRDY 节奏是否合理
        icm_dma_start_preflight();
#else
        icm42688p_data_ready = 0;
        spi1_dma_flag = 0;
#endif
    }
    
    // 验证传感器数据是否可读
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
#ifdef USE_DMA
    // 校准期间关闭 DMA 状态机，避免 SPI 争用
    const bool resume_dma = icm_dma_mode_enabled && icm_dma_state != ICM_DMA_STATE_POLLING;
    if (resume_dma) {
        icm_dma_switch_to_polling();
    }
#endif

    bool ok = icm42688p_calibrate_gyro(&icm, samples);

#ifdef USE_DMA
    if (resume_dma) {
        icm_dma_start_preflight();
    }
#endif
    return ok;
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

// ============================================================================
// 统一的数据获取入口（DMA 模式或轮询模式）
// ============================================================================
bool icm42688p_get_all_data(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                            int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                            float *temp_celsius)
{
    icm42688p_gyro_data_t  gd;
    icm42688p_accel_data_t ad;
    icm42688p_temp_data_t  td;

#ifdef USE_DMA
    // DMA 模式：仅在数据就绪时解析
    if (icm_dma_frame_available(&gd, &ad, &td)) {
        if (gyro_x)  *gyro_x  = gd.x;
        if (gyro_y)  *gyro_y  = gd.y;
        if (gyro_z)  *gyro_z  = gd.z;
        if (accel_x) *accel_x = ad.x;
        if (accel_y) *accel_y = ad.y;
        if (accel_z) *accel_z = ad.z;
        if (temp_celsius) *temp_celsius = td.celsius;
        return true;
    }

    // DMA 模式但未就绪，直接返回
    if (icm_dma_mode_enabled && icm_dma_state != ICM_DMA_STATE_POLLING) {
        return false;
    }
#endif

    // 轮询模式（未启用 DMA 或已回退）
    if (icm42688p_drdy_count == 0) {
        return false;
    }
    icm42688p_drdy_count--;

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

    if (icm42688p_drdy_count > 10) {
        icm42688p_drdy_count = 1;
    }

    return true;
}

// EXTI callback: data ready interrupt from ICM42688P
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ICM42688P_INT_PIN) {
#ifdef USE_DMA
        icm_dma_handle_int(DWT->CYCCNT);
        return;
#endif

        // 回退/轮询模式：只累加标志，主循环消费
        icm42688p_drdy_count++;
    }
#ifdef USE_HMC5883L_INT
    else if (GPIO_Pin == HMC5883l_INT_PIN) {
        hmc5883l_data_ready_flag = 1;
    }
#endif
}

// Update using data-ready flag
bool icm42688p_update(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                      int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                      float *temp_celsius)
{
    // 依赖 DMA/轮询状态机读取一帧数据，若无新数据则返回 false
    return icm42688p_get_all_data(gyro_x, gyro_y, gyro_z,
                                  accel_x, accel_y, accel_z,
                                  temp_celsius);
}

// ============================================================================
// 数据预处理（物理量转换、零偏处理）
// ============================================================================
// Read + convert to physical units
bool icm42688p_dataPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z,
                              int16_t *accel_x, int16_t *accel_y, int16_t *accel_z,
                              float *gyro_x_norm, float *gyro_y_norm, float *gyro_z_norm,
                              float *accel_x_norm, float *accel_y_norm, float *accel_z_norm,
                              float *temp_celsius)
{
    int16_t gx = 0, gy = 0, gz = 0;
    int16_t ax = 0, ay = 0, az = 0;
    float   t  = 0.0f;

    if (!icm42688p_get_all_data(&gx, &gy, &gz, &ax, &ay, &az, &t)) {
        return false;
    }

    if (gyro_x)  *gyro_x  = gx;
    if (gyro_y)  *gyro_y  = gy;
    if (gyro_z)  *gyro_z  = gz;
    if (accel_x) *accel_x = ax;
    if (accel_y) *accel_y = ay;
    if (accel_z) *accel_z = az;
    if (temp_celsius) *temp_celsius = t;

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

bool icm42688p_gyro_rawPreprocess(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    // Raw版本：仅做零偏补偿，不做刻度转换
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

// ============================================================================
// DMA 运行状态查询（调试用）
// ============================================================================
bool icm42688p_dma_active(void)
{
#ifdef USE_DMA
    return icm_dma_mode_enabled && icm_dma_state != ICM_DMA_STATE_POLLING;
#else
    return false;
#endif
}

uint32_t icm42688p_dma_period_cycles(void)
{
#ifdef USE_DMA
    return icm_last_dma_period_cycles;
#else
    return 0;
#endif
}

uint32_t icm42688p_dma_hold_target(void)
{
#ifdef USE_DMA
    return ICM_EXIT_COUNTER_HOLD;
#else
    return 0;
#endif
}

// ============================================================================
// HAL 回调（DMA 完成 / 错误）
// ============================================================================
// SPI1 RX DMA complete callback
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        __DSB();
        __ISB();
        ICM42688P_CS_HIGH();

        spi1_dma_flag = 1;
#ifdef USE_DMA
        icm42688p_data_ready = 1;   // S4：DMA 完成，数据就绪
        icm_dma_state = ICM_DMA_STATE_WAIT_INT;
#endif
        __DSB();
    }
}

// DMA错误回调（可选，用于调试）
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        ICM42688P_CS_HIGH();
        hspi->State = HAL_SPI_STATE_READY;
        spi1_dma_flag = 0;
#ifdef USE_DMA
        icm_dma_switch_to_polling();
#endif
    }
}
