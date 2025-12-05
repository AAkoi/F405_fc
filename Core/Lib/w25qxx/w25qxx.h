/**
 * @file    w25qxx.h
 * @brief   W25Qxx SPI Flash 驱动（基于 SPI2 + HAL）
 */

#ifndef W25QXX_H
#define W25QXX_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define W25QXX_PAGE_SIZE      256U
#define W25QXX_SECTOR_SIZE    4096U

// 命令集
#define W25X_WriteEnable        0x06
#define W25X_WriteDisable       0x04
#define W25X_ReadStatusReg      0x05
#define W25X_WriteStatusReg     0x01
#define W25X_ReadData           0x03
#define W25X_FastReadData       0x0B
#define W25X_PageProgram        0x02
#define W25X_BlockErase         0xD8
#define W25X_SectorErase        0x20
#define W25X_ChipErase          0xC7
#define W25X_PowerDown          0xB9
#define W25X_ReleasePowerDown   0xAB
#define W25X_ManufactDeviceID   0x90
#define W25X_JedecDeviceID      0x9F

typedef struct w25qxx_device_s {
    void     (*init)(void);
    HAL_StatusTypeDef (*wr)(const uint8_t *buf, uint32_t addr, uint16_t len);
    HAL_StatusTypeDef (*rd)(uint8_t *buf, uint32_t addr, uint16_t len);
    uint16_t type;
} w25qxx_device_t;

extern w25qxx_device_t w25qxx_dev;

void w25qxx_init(void);
uint16_t w25qxx_read_id(void);
uint8_t  w25qxx_read_status(void);
void     w25qxx_write_status(uint8_t sr);
HAL_StatusTypeDef w25qxx_read(uint8_t *buf, uint32_t addr, uint16_t len);
HAL_StatusTypeDef w25qxx_write(const uint8_t *buf, uint32_t addr, uint16_t len);
HAL_StatusTypeDef w25qxx_erase_sector(uint32_t sector_idx);
HAL_StatusTypeDef w25qxx_erase_chip(void);
HAL_StatusTypeDef w25qxx_power_down(void);
HAL_StatusTypeDef w25qxx_wakeup(void);

#endif // W25QXX_H
