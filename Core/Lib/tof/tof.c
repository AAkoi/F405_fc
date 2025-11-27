/**
 * @file    tof.c
 * @brief   VL53L0X ToF 测距传感器应用层接口实现
 * @author  Based on ST VL53L0X API
 * @date    2025
 * 
 * 使用 I2C2 接口通信（参考 BSP_IIC 实现风格）
 */

#include "tof.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * 全局变量
 * ============================================================================ */

// VL53L0X 设备结构体
static VL53L0X_Dev_t vl53l0x_dev;

// 设备信息
static VL53L0X_DeviceInfo_t device_info;

// 当前测量模式
static tof_mode_t current_mode = TOF_MODE_DEFAULT;

// 外部 I2C2 句柄声明 (由 STM32 CubeMX 生成)
extern I2C_HandleTypeDef hi2c2;

/* ============================================================================
 * 模式配置参数
 * ============================================================================ */

typedef struct {
    uint32_t signal_limit;           // Signal 限制值 (FixPoint1616)
    uint32_t sigma_limit;            // Sigma 限制值 (FixPoint1616)
    uint32_t timing_budget;          // 测量时间预算（微秒）
    uint8_t pre_range_vcsel_period;  // VCSEL 预范围周期
    uint8_t final_range_vcsel_period;// VCSEL 最终范围周期
} tof_mode_config_t;

// 各种模式的配置参数
static const tof_mode_config_t mode_configs[] = {
    // DEFAULT 模式 (标准)
    {
        .signal_limit = (uint32_t)(0.25 * 65536),
        .sigma_limit = (uint32_t)(18 * 65536),
        .timing_budget = 33000,
        .pre_range_vcsel_period = 14,
        .final_range_vcsel_period = 10
    },
    // HIGH_ACCURACY 模式 (高精度)
    {
        .signal_limit = (uint32_t)(0.25 * 65536),
        .sigma_limit = (uint32_t)(18 * 65536),
        .timing_budget = 200000,
        .pre_range_vcsel_period = 14,
        .final_range_vcsel_period = 10
    },
    // LONG_RANGE 模式 (长距离)
    {
        .signal_limit = (uint32_t)(0.1 * 65536),
        .sigma_limit = (uint32_t)(60 * 65536),
        .timing_budget = 33000,
        .pre_range_vcsel_period = 18,
        .final_range_vcsel_period = 14
    },
    // HIGH_SPEED 模式 (高速)
    {
        .signal_limit = (uint32_t)(0.25 * 65536),
        .sigma_limit = (uint32_t)(32 * 65536),
        .timing_budget = 20000,
        .pre_range_vcsel_period = 14,
        .final_range_vcsel_period = 10
    }
};

/* ============================================================================
 * I2C2 底层函数（基于 BSP_IIC 风格的浅封装）
 * ============================================================================ */

/**
 * @brief I2C2 写入单个字节
 */
static inline uint8_t tof_i2c2_write_byte(uint8_t dev_addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c2, dev_addr, data, 2, 100);
    return (status == HAL_OK) ? 0 : 1;
}

/**
 * @brief I2C2 读取单个字节
 */
static inline uint8_t tof_i2c2_read_byte(uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    HAL_StatusTypeDef status;
    
    // 发送寄存器地址
    status = HAL_I2C_Master_Transmit(&hi2c2, dev_addr, &reg, 1, 100);
    if (status != HAL_OK) return 1;
    
    // 读取数据
    status = HAL_I2C_Master_Receive(&hi2c2, dev_addr, data, 1, 100);
    return (status == HAL_OK) ? 0 : 1;
}

/**
 * @brief I2C2 读取多个字节
 */
static inline uint8_t tof_i2c2_read_burst(uint8_t dev_addr, uint8_t reg, 
                                           uint8_t *buffer, uint16_t len)
{
    HAL_StatusTypeDef status;
    
    // 发送寄存器地址
    status = HAL_I2C_Master_Transmit(&hi2c2, dev_addr, &reg, 1, 100);
    if (status != HAL_OK) return 1;
    
    // 读取数据
    status = HAL_I2C_Master_Receive(&hi2c2, dev_addr, buffer, len, 100);
    return (status == HAL_OK) ? 0 : 1;
}

/**
 * @brief I2C2 写入多个字节
 */
static inline uint8_t tof_i2c2_write_multi(uint8_t dev_addr, uint8_t reg, 
                                            uint8_t *pdata, uint16_t count)
{
    if (count > 64) return 1;  // 防止缓冲区溢出
    
    // 发送寄存器地址 + 数据
    uint8_t buffer[65];
    buffer[0] = reg;
    memcpy(&buffer[1], pdata, count);
    
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c2, dev_addr, buffer, count + 1, 100);
    return (status == HAL_OK) ? 0 : 1;
}

/* ============================================================================
 * VL53L0X 平台层接口实现（供 API 调用）
 * ============================================================================ */

/**
 * @brief 写入多个寄存器
 */
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, 
                                  uint8_t *pdata, uint32_t count)
{
    if (count >= VL53L0X_MAX_I2C_XFER_SIZE) {
        return VL53L0X_ERROR_INVALID_PARAMS;
    }
    
    int32_t status = tof_i2c2_write_multi(Dev->I2cDevAddr, index, pdata, (uint16_t)count);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 读取多个寄存器
 */
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, 
                                 uint8_t *pdata, uint32_t count)
{
    if (count >= VL53L0X_MAX_I2C_XFER_SIZE) {
        return VL53L0X_ERROR_INVALID_PARAMS;
    }
    
    int32_t status = tof_i2c2_read_burst(Dev->I2cDevAddr, index, pdata, (uint16_t)count);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 写入单字节寄存器
 */
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
    int32_t status = tof_i2c2_write_byte(Dev->I2cDevAddr, index, data);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 写入字（16位）寄存器
 */
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(data >> 8);   // 高字节
    buffer[1] = (uint8_t)(data & 0xFF); // 低字节
    
    int32_t status = tof_i2c2_write_multi(Dev->I2cDevAddr, index, buffer, 2);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 写入双字（32位）寄存器
 */
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    uint8_t buffer[4];
    buffer[0] = (uint8_t)(data >> 24);
    buffer[1] = (uint8_t)((data >> 16) & 0xFF);
    buffer[2] = (uint8_t)((data >> 8) & 0xFF);
    buffer[3] = (uint8_t)(data & 0xFF);
    
    int32_t status = tof_i2c2_write_multi(Dev->I2cDevAddr, index, buffer, 4);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 更新字节寄存器（读-改-写）
 */
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, 
                                  uint8_t AndData, uint8_t OrData)
{
    uint8_t data;
    int32_t status;
    
    // 读取
    status = tof_i2c2_read_byte(Dev->I2cDevAddr, index, &data);
    if (status != 0) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    
    // 修改
    data = (data & AndData) | OrData;
    
    // 写入
    status = tof_i2c2_write_byte(Dev->I2cDevAddr, index, data);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 读取单字节寄存器
 */
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    int32_t status = tof_i2c2_read_byte(Dev->I2cDevAddr, index, data);
    
    return (status == 0) ? VL53L0X_ERROR_NONE : VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 读取字（16位）寄存器
 */
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    uint8_t buffer[2];
    int32_t status = tof_i2c2_read_burst(Dev->I2cDevAddr, index, buffer, 2);
    
    if (status == 0) {
        *data = ((uint16_t)buffer[0] << 8) | (uint16_t)buffer[1];
        return VL53L0X_ERROR_NONE;
    }
    
    return VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 读取双字（32位）寄存器
 */
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    uint8_t buffer[4];
    int32_t status = tof_i2c2_read_burst(Dev->I2cDevAddr, index, buffer, 4);
    
    if (status == 0) {
        *data = ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
                ((uint32_t)buffer[2] << 8)  | (uint32_t)buffer[3];
        return VL53L0X_ERROR_NONE;
    }
    
    return VL53L0X_ERROR_CONTROL_INTERFACE;
}

/**
 * @brief 轮询延时
 */
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
    volatile uint32_t i;
    for (i = 0; i < 250; i++) {
        // 简单的延时循环，实际应用中建议使用定时器或 HAL_Delay
    }
    return VL53L0X_ERROR_NONE;
}

/* ============================================================================
 * 辅助函数
 * ============================================================================ */

/**
 * @brief 获取测量模式字符串声明
 */
const char* tof_get_mode_string(tof_mode_t mode);

/**
 * @brief 打印 API 错误信息
 */
static void print_api_error(VL53L0X_Error status)
{
    if (status != VL53L0X_ERROR_NONE) {
        printf("[VL53L0X] Error: %d\r\n", status);
    }
}

/**
 * @brief 应用测量模式配置
 */
static bool apply_mode_config(tof_mode_t mode)
{
    VL53L0X_Error status;
    const tof_mode_config_t *config = &mode_configs[mode];
    
    // 设置信号速率限制
    status = VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
        VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, config->signal_limit);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 设置 Sigma 限制
    status = VL53L0X_SetLimitCheckValue(&vl53l0x_dev,
        VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, config->sigma_limit);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 设置测量时间预算
    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 
                                                              config->timing_budget);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 设置 VCSEL 周期
    status = VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev,
        VL53L0X_VCSEL_PERIOD_PRE_RANGE, config->pre_range_vcsel_period);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    status = VL53L0X_SetVcselPulsePeriod(&vl53l0x_dev,
        VL53L0X_VCSEL_PERIOD_FINAL_RANGE, config->final_range_vcsel_period);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    return true;
}
/* ============================================================================
 * 公共 API 函数
 * ============================================================================ */

/**
 * @brief 初始化 VL53L0X ToF 传感器
 */
bool tof_init(void)
{
    VL53L0X_Error status;
    
    printf("========== VL53L0X ToF 初始化 ==========\r\n");
    
    // 清零设备结构体
    memset(&vl53l0x_dev, 0, sizeof(vl53l0x_dev));
    
    // 设置 I2C 参数
    vl53l0x_dev.I2cDevAddr = TOF_I2C_ADDRESS_DEFAULT;
    vl53l0x_dev.comms_type = 1;        // I2C 通信
    vl53l0x_dev.comms_speed_khz = 400; // 400kHz
    
    printf("[步骤1] 检测设备...\r\n");
    
    // 等待设备启动
    HAL_Delay(20);
    
    // 尝试读取设备 ID
    uint16_t model_id = 0;
    status = VL53L0X_RdWord(&vl53l0x_dev, 0xC0, &model_id);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ 无法通过 I2C2 通信\r\n");
        printf("    可能原因:\r\n");
        printf("    1. I2C2 未正确初始化\r\n");
        printf("    2. VL53L0X 未连接或未上电\r\n");
        printf("    3. I2C 地址不正确\r\n");
        printf("=====================================\r\n\r\n");
        return false;
    }
    
    if (model_id != 0xEEAA) {
        printf("  ✗ 设备 ID 不匹配: 0x%04X (期望: 0xEEAA)\r\n", model_id);
        printf("=====================================\r\n\r\n");
        return false;
    }
    
    printf("  ✓ 检测到 VL53L0X (Model ID: 0x%04X)\r\n", model_id);
    
    printf("\r\n[步骤2] 初始化数据结构...\r\n");
    
    // 数据初始化
    status = VL53L0X_DataInit(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ 数据初始化失败\r\n");
        print_api_error(status);
        printf("=====================================\r\n\r\n");
        return false;
    }
    printf("  ✓ 数据初始化完成\r\n");
    
    printf("\r\n[步骤3] 静态初始化...\r\n");
    
    // 静态初始化
    status = VL53L0X_StaticInit(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ 静态初始化失败\r\n");
        print_api_error(status);
        printf("=====================================\r\n\r\n");
        return false;
    }
    printf("  ✓ 静态初始化完成\r\n");
    
    printf("\r\n[步骤4] 获取设备信息...\r\n");
    
    // 获取设备信息
    status = VL53L0X_GetDeviceInfo(&vl53l0x_dev, &device_info);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ 获取设备信息失败\r\n");
        print_api_error(status);
    } else {
        printf("  ✓ 设备名称: %s\r\n", device_info.Name);
        printf("    产品 ID: %s\r\n", device_info.ProductId);
        printf("    版本: %d.%d\r\n", device_info.ProductRevisionMajor, 
                                      device_info.ProductRevisionMinor);
    }
    
    printf("\r\n[步骤5] 设备校准...\r\n");
    
    // 参考 SPAD 校准
    uint32_t ref_spad_count;
    uint8_t is_aperture_spads;
    status = VL53L0X_PerformRefSpadManagement(&vl53l0x_dev,
                                               &ref_spad_count,
                                               &is_aperture_spads);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ SPAD 校准失败\r\n");
        print_api_error(status);
        printf("=====================================\r\n\r\n");
        return false;
    }
    printf("  ✓ SPAD 校准完成 (Count: %u, Aperture: %u)\r\n",
           (unsigned int)ref_spad_count, (unsigned int)is_aperture_spads);

    // 参考温度/电压校准
    uint8_t vhv_settings;
    uint8_t phase_cal;
    status = VL53L0X_PerformRefCalibration(&vl53l0x_dev, &vhv_settings, &phase_cal);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ 参考校准失败\r\n");
        print_api_error(status);
        printf("=====================================\r\n\r\n");
        return false;
    }
    printf("  ✓ 参考校准完成 (VHV: %d, Phase: %d)\r\n", vhv_settings, phase_cal);
    
    printf("\r\n[步骤6] 设置测量模式...\r\n");
    
    // 设置设备模式为单次测量（作为初始状态）
    status = VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (status != VL53L0X_ERROR_NONE) {
        printf("  ✗ 设置设备模式失败\r\n");
        print_api_error(status);
        printf("=====================================\r\n\r\n");
        return false;
    }
    printf("  ✓ 设备模式: 单次测量\r\n");
    
    // 应用默认测量模式配置
    if (!apply_mode_config(TOF_MODE_DEFAULT)) {
        printf("  ✗ 应用模式配置失败\r\n");
        printf("=====================================\r\n\r\n");
        return false;
    }
    printf("  ✓ 测量模式: 默认\r\n");
    current_mode = TOF_MODE_DEFAULT;
    
    printf("\r\n========== VL53L0X 初始化成功 ==========\r\n\r\n");
    
    return true;
}

/**
 * @brief 设置测量模式
 */
bool tof_set_mode(tof_mode_t mode)
{
    if (mode > TOF_MODE_HIGH_SPEED) {
        return false;
    }
    
    if (apply_mode_config(mode)) {
        current_mode = mode;
        printf("[VL53L0X] 模式切换: %s\r\n", tof_get_mode_string(mode));
        return true;
    }
    
    return false;
}

/**
 * @brief 执行单次测距
 */
bool tof_read_distance(uint16_t *distance_mm)
{
    if (!distance_mm) {
        return false;
    }
    
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_Error status;
    
    // 执行单次测距
    status = VL53L0X_PerformSingleRangingMeasurement(&vl53l0x_dev, &measurement);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 检查测量状态
    if (measurement.RangeStatus == 0) {
        *distance_mm = measurement.RangeMilliMeter;
        return true;
    }
    
    return false;
}

/**
 * @brief 执行单次测距（详细数据）
 */
bool tof_read_data(tof_data_t *data)
{
    if (!data) {
        return false;
    }
    
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_Error status;
    
    // 执行单次测距
    status = VL53L0X_PerformSingleRangingMeasurement(&vl53l0x_dev, &measurement);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 填充数据结构
    data->range_mm = measurement.RangeMilliMeter;
    data->range_status = measurement.RangeStatus;
    data->signal_rate = (float)measurement.SignalRateRtnMegaCps / 65536.0f;
    data->measurement_time = measurement.MeasurementTimeUsec;
    
    return (measurement.RangeStatus == 0);
}

/**
 * @brief 启动连续测距模式
 */
bool tof_start_continuous(uint32_t period_ms)
{
    VL53L0X_Error status;
    
    // 设置为连续测量模式
    status = VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 设置测量间隔
    if (period_ms > 0) {
        status = VL53L0X_SetInterMeasurementPeriodMilliSeconds(&vl53l0x_dev, period_ms);
        if (status != VL53L0X_ERROR_NONE) {
            print_api_error(status);
            return false;
        }
    }
    
    // 启动连续测量
    status = VL53L0X_StartMeasurement(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    printf("[VL53L0X] 连续测量模式已启动\r\n");
    return true;
}

/**
 * @brief 停止连续测距模式
 */
bool tof_stop_continuous(void)
{
    VL53L0X_Error status;
    
    // 停止测量
    status = VL53L0X_StopMeasurement(&vl53l0x_dev);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 切换回单次测量模式
    status = VL53L0X_SetDeviceMode(&vl53l0x_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    printf("[VL53L0X] 连续测量模式已停止\r\n");
    return true;
}

/**
 * @brief 读取连续测距数据（连续模式下使用）
 */
bool tof_get_continuous_distance(uint16_t *distance_mm)
{
    if (!distance_mm) {
        return false;
    }
    
    VL53L0X_RangingMeasurementData_t measurement;
    VL53L0X_Error status;
    
    // 等待数据就绪
    status = VL53L0X_GetRangingMeasurementData(&vl53l0x_dev, &measurement);
    if (status != VL53L0X_ERROR_NONE) {
        print_api_error(status);
        return false;
    }
    
    // 清除中断
    VL53L0X_ClearInterruptMask(&vl53l0x_dev, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
    
    // 检查测量状态
    if (measurement.RangeStatus == 0) {
        *distance_mm = measurement.RangeMilliMeter;
        return true;
    }
    
    return false;
}

/**
 * @brief 获取设备信息
 */
bool tof_get_device_info(char *name, uint8_t *revision_major, uint8_t *revision_minor)
{
    if (name) {
        strncpy(name, device_info.Name, 32);
    }
    if (revision_major) {
        *revision_major = device_info.ProductRevisionMajor;
    }
    if (revision_minor) {
        *revision_minor = device_info.ProductRevisionMinor;
    }
    return true;
}

/**
 * @brief 执行偏移校准
 */
bool tof_calibrate_offset(uint16_t target_distance_mm)
{
    VL53L0X_Error status;
    int32_t offset_microm;
    
    printf("[VL53L0X] 开始偏移校准(目标: %d mm)...\r\n", target_distance_mm);
    
    status = VL53L0X_PerformOffsetCalibration(&vl53l0x_dev,
                                               target_distance_mm * 1000,
                                               &offset_microm);
    if (status == VL53L0X_ERROR_NONE) {
        printf("[VL53L0X] 偏移校准完成 (Offset: %d μm)\r\n", (int)offset_microm);
        return true;
    }
    
    print_api_error(status);
    return false;
}

/**
 * @brief 执行串扰校准
 */
bool tof_calibrate_xtalk(uint16_t target_distance_mm)
{
    VL53L0X_Error status;
    uint32_t xtalk_comp_rate;

    printf("[VL53L0X] 开始串扰校准(目标: %d mm)...\r\n", target_distance_mm);
    
    status = VL53L0X_PerformXTalkCalibration(&vl53l0x_dev,
                                              target_distance_mm * 1000,
                                              &xtalk_comp_rate);
    if (status == VL53L0X_ERROR_NONE) {
        printf("[VL53L0X] 串扰校准完成 (XTalk: %u)\r\n", (unsigned int)xtalk_comp_rate);
        return true;
    }
    
    print_api_error(status);
    return false;
}

/**
 * @brief 软复位传感器
 */
bool tof_reset(void)
{
    VL53L0X_Error status;
    
    status = VL53L0X_ResetDevice(&vl53l0x_dev);
    if (status == VL53L0X_ERROR_NONE) {
        HAL_Delay(10);
        return true;
    }
    
    print_api_error(status);
    return false;
}

/**
 * @brief 设置测量超时时间
 */
bool tof_set_measurement_timing_budget(uint32_t timeout_ms)
{
    VL53L0X_Error status;
    
    status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53l0x_dev, 
                                                              timeout_ms * 1000);
    
    if (status == VL53L0X_ERROR_NONE) {
        printf("[VL53L0X] 测量超时设置: %u ms\r\n", (unsigned int)timeout_ms);
        return true;
    }
    
    print_api_error(status);
    return false;
}

/**
 * @brief 获取当前测量模式字符串
 */
const char* tof_get_mode_string(tof_mode_t mode)
{
    switch (mode) {
        case TOF_MODE_DEFAULT:       return "默认";
        case TOF_MODE_HIGH_ACCURACY: return "高精度";
        case TOF_MODE_LONG_RANGE:    return "长距离";
        case TOF_MODE_HIGH_SPEED:    return "高速";
        default:                     return "未知";
    }
}

/**
 * @brief 获取测量状态描述字符串
 */
const char* tof_get_status_string(uint8_t status)
{
    switch (status) {
        case 0: return "有效";
        case 1: return "信号失败";
        case 2: return "Sigma 失败";
        case 3: return "信号和 Sigma 失败";
        case 4: return "超出范围";
        case 5: return "环境光影响";
        default: return "未知错误";
    }
}





