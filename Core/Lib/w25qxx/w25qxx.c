/**
 * @file    w25qxx.c
 * @brief   W25Qxx SPI Flash 驱动（SPI1 + HAL）
 */

#include "w25qxx.h"
#include "bsp_spi.h"
#include "bsp_pins.h"
#include <string.h>

#define W25QXX_SPI_TIMEOUT  100U

// 简化的 GPIO 片选控制
#define W25QXX_CS_LOW()   GPIO_PIN_SET_LOW(W25QXX_CS_GPIO_PORT, W25QXX_CS_PIN)
#define W25QXX_CS_HIGH()  GPIO_PIN_SET_HIGH(W25QXX_CS_GPIO_PORT, W25QXX_CS_PIN)

static void w25qxx_cs_gpio_clock_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF) __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG) __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
}

// 全局设备实例
w25qxx_device_t w25qxx_dev = {
    .init = w25qxx_init,
    .wr   = w25qxx_write,
    .rd   = w25qxx_read,
    .type = 0x0000
};

static HAL_StatusTypeDef w25qxx_spi_tx(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    return HAL_SPI_TransmitReceive(&hspi1, (uint8_t *)tx, rx, len, W25QXX_SPI_TIMEOUT);
}

static uint8_t w25qxx_rw_byte(uint8_t tx)
{
    uint8_t rx = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, W25QXX_SPI_TIMEOUT);
    return rx;
}

static void w25qxx_write_enable(void)
{
    W25QXX_CS_LOW();
    w25qxx_rw_byte(W25X_WriteEnable);
    W25QXX_CS_HIGH();
}

static void w25qxx_write_disable(void)
{
    W25QXX_CS_LOW();
    w25qxx_rw_byte(W25X_WriteDisable);
    W25QXX_CS_HIGH();
}

uint8_t w25qxx_read_status(void)
{
    uint8_t status = 0;
    W25QXX_CS_LOW();
    w25qxx_rw_byte(W25X_ReadStatusReg);
    status = w25qxx_rw_byte(0xFF);
    W25QXX_CS_HIGH();
    return status;
}

void w25qxx_write_status(uint8_t sr)
{
    w25qxx_write_enable();
    W25QXX_CS_LOW();
    w25qxx_rw_byte(W25X_WriteStatusReg);
    w25qxx_rw_byte(sr);
    W25QXX_CS_HIGH();
}

static void w25qxx_wait_busy(void)
{
    while (w25qxx_read_status() & 0x01) {
        HAL_Delay(1);
    }
}

uint16_t w25qxx_read_id(void)
{
    uint8_t rx[3] = {0};
    uint8_t cmd[4] = {W25X_JedecDeviceID, 0x00, 0x00, 0x00};

    W25QXX_CS_LOW();
    w25qxx_spi_tx(cmd, rx, sizeof(cmd));
    W25QXX_CS_HIGH();

    return ((uint16_t)rx[1] << 8) | rx[2];
}

HAL_StatusTypeDef w25qxx_read(uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || len == 0) return HAL_ERROR;

    uint8_t cmd[4];
    cmd[0] = W25X_ReadData;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;

    W25QXX_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, cmd, sizeof(cmd), W25QXX_SPI_TIMEOUT);
    if (ret == HAL_OK) {
        ret = HAL_SPI_Receive(&hspi1, buf, len, W25QXX_SPI_TIMEOUT);
    }
    W25QXX_CS_HIGH();
    return ret;
}

static HAL_StatusTypeDef w25qxx_write_page(const uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || len == 0 || len > W25QXX_PAGE_SIZE) return HAL_ERROR;

    w25qxx_write_enable();

    uint8_t cmd[4];
    cmd[0] = W25X_PageProgram;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;

    W25QXX_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, cmd, sizeof(cmd), W25QXX_SPI_TIMEOUT);
    if (ret == HAL_OK) {
        ret = HAL_SPI_Transmit(&hspi1, (uint8_t *)buf, len, W25QXX_SPI_TIMEOUT);
    }
    W25QXX_CS_HIGH();

    w25qxx_wait_busy();
    w25qxx_write_disable();
    return ret;
}

HAL_StatusTypeDef w25qxx_write(const uint8_t *buf, uint32_t addr, uint16_t len)
{
    if (!buf || len == 0) return HAL_ERROR;

    uint16_t to_write = len;
    uint32_t cur_addr = addr;
    const uint8_t *p = buf;

    while (to_write > 0) {
        uint16_t page_off = cur_addr % W25QXX_PAGE_SIZE;
        uint16_t page_space = W25QXX_PAGE_SIZE - page_off;
        uint16_t chunk = (to_write < page_space) ? to_write : page_space;

        HAL_StatusTypeDef ret = w25qxx_write_page(p, cur_addr, chunk);
        if (ret != HAL_OK) {
            return ret;
        }

        p        += chunk;
        cur_addr += chunk;
        to_write -= chunk;
    }

    return HAL_OK;
}

HAL_StatusTypeDef w25qxx_erase_sector(uint32_t sector_idx)
{
    uint32_t addr = sector_idx * W25QXX_SECTOR_SIZE;
    w25qxx_write_enable();

    uint8_t cmd[4];
    cmd[0] = W25X_SectorErase;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;

    W25QXX_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, cmd, sizeof(cmd), W25QXX_SPI_TIMEOUT);
    W25QXX_CS_HIGH();

    w25qxx_wait_busy();
    return ret;
}

HAL_StatusTypeDef w25qxx_erase_chip(void)
{
    w25qxx_write_enable();
    W25QXX_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, (uint8_t[]){W25X_ChipErase}, 1, W25QXX_SPI_TIMEOUT);
    W25QXX_CS_HIGH();
    w25qxx_wait_busy();
    return ret;
}

HAL_StatusTypeDef w25qxx_power_down(void)
{
    W25QXX_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, (uint8_t[]){W25X_PowerDown}, 1, W25QXX_SPI_TIMEOUT);
    W25QXX_CS_HIGH();
    return ret;
}

HAL_StatusTypeDef w25qxx_wakeup(void)
{
    W25QXX_CS_LOW();
    HAL_StatusTypeDef ret = HAL_SPI_Transmit(&hspi1, (uint8_t[]){W25X_ReleasePowerDown}, 1, W25QXX_SPI_TIMEOUT);
    W25QXX_CS_HIGH();
    HAL_Delay(1);
    return ret;
}

void w25qxx_init(void)
{
    // SPI1 初始化（如果已经初始化过则不会有副作用）
    MX_SPI1_Init();

    // CS 引脚配置为推挽输出并默认拉高
    w25qxx_cs_gpio_clock_enable(W25QXX_CS_GPIO_PORT);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = W25QXX_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(W25QXX_CS_GPIO_PORT, &GPIO_InitStruct);
    W25QXX_CS_HIGH();

    w25qxx_dev.type = w25qxx_read_id();
}
