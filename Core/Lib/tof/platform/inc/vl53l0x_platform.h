/**
 * @file    vl53l0x_platform.h
 * @brief   VL53L0X 平台层接口定义（简化版，基于 BSP_IIC）
 * @date    2025
 */

#ifndef __VL53L0X_PLATFORM_H
#define __VL53L0X_PLATFORM_H

#include "vl53l0x_def.h"
#include "vl53l0x_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 设备结构体定义
 * ============================================================================ */

// VL53L0X 设备结构体
typedef struct {
    VL53L0X_DevData_t Data;          /*!< ST Ewok Dev data */
    uint8_t   I2cDevAddr;            /*!< I2C device address */
    uint8_t   comms_type;            /*!< Type of comms : I2C or SPI */
    uint16_t  comms_speed_khz;       /*!< Comms speed [kHz] */
} VL53L0X_Dev_t;

typedef VL53L0X_Dev_t* VL53L0X_DEV;

/* ============================================================================
 * 宏定义
 * ============================================================================ */

#define VL53L0X_MAX_I2C_XFER_SIZE  64  // 最大 I2C 传输字节数

#define PALDevDataGet(Dev, field) (Dev->Data.field)
#define PALDevDataSet(Dev, field, data) ((Dev->Data.field) = (data))

/* ============================================================================
 * 日志系统宏定义（禁用）
 * ============================================================================ */

// 禁用日志功能
#define VL53L0X_LOG_ENABLE 0

// 日志模块定义
#define TRACE_MODULE_NONE              0x00000000
#define TRACE_MODULE_API               0x00000001
#define TRACE_MODULE_CORE              0x00000002
#define TRACE_MODULE_PLATFORM          0x00000004
#define TRACE_MODULE_ALL               0x7fffffff

// 日志级别定义
#define TRACE_LEVEL_NONE               0x00000000
#define TRACE_LEVEL_ERRORS             0x00000001
#define TRACE_LEVEL_WARNING            0x00000002
#define TRACE_LEVEL_INFO               0x00000004
#define TRACE_LEVEL_DEBUG              0x00000008
#define TRACE_LEVEL_ALL                0x00000010
#define TRACE_LEVEL_IGNORE             0x00000020

// 日志函数宏（使用ifndef保护，避免重定义警告）
#ifndef TRACE_PRINT
#define TRACE_PRINT(module, level, ...)  (void)0
#endif

#ifndef LOG_FUNCTION_START
#define LOG_FUNCTION_START(fmt, ...)     (void)0
#endif

#ifndef LOG_FUNCTION_END
#define LOG_FUNCTION_END(status)         (void)0
#endif

#ifndef LOG_FUNCTION_END_FMT
#define LOG_FUNCTION_END_FMT(status, fmt, ...) (void)0
#endif

#ifndef VL53L0X_COPYSTRING
#define VL53L0X_COPYSTRING(str, ...)     (void)0
#endif

// 内部日志宏
#ifndef _LOG_FUNCTION_START
#define _LOG_FUNCTION_START(module, fmt, ...) (void)0
#endif

#ifndef _LOG_FUNCTION_END
#define _LOG_FUNCTION_END(module, status)     (void)0
#endif

#ifndef _LOG_FUNCTION_END_FMT
#define _LOG_FUNCTION_END_FMT(module, status, fmt, ...) (void)0
#endif

/* ============================================================================
 * 平台层函数声明（在 tof.c 中实现）
 * ============================================================================ */

/**
 * @brief 写入多个寄存器
 */
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, 
                                  uint8_t *pdata, uint32_t count);

/**
 * @brief 读取多个寄存器
 */
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, 
                                 uint8_t *pdata, uint32_t count);

/**
 * @brief 写入单字节寄存器
 */
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data);

/**
 * @brief 写入字（16位）寄存器
 */
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data);

/**
 * @brief 写入双字（32位）寄存器
 */
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data);

/**
 * @brief 更新字节寄存器（读-改-写）
 */
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, 
                                  uint8_t AndData, uint8_t OrData);

/**
 * @brief 读取单字节寄存器
 */
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data);

/**
 * @brief 读取字（16位）寄存器
 */
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data);

/**
 * @brief 读取双字（32位）寄存器
 */
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data);

/**
 * @brief 轮询延时
 */
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev);

#ifdef __cplusplus
}
#endif

#endif // __VL53L0X_PLATFORM_H

