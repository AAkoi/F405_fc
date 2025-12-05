/**
 * @file    test_w25qxx.c
 * @brief   W25Qxx 读写小样本验证（SPI2，无 DMA）
 */

#include "test_w25qxx.h"
#include "w25qxx.h"
#include "bsp_spi.h"
#include "bsp_pins.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

#define W25Q_TEST_ADDR   0x000000
#define W25Q_TEST_LEN    128

static uint8_t tx_buf[W25Q_TEST_LEN];
static uint8_t rx_buf[W25Q_TEST_LEN];

void test_w25qxx_run(void)
{
    printf("\r\n===============================\r\n");
    printf("[test_w25q] W25Qxx SPI Flash 测试 (SPI2, no DMA)\r\n");
    printf("===============================\r\n");

    w25qxx_init();
    printf("[test_w25q] JEDEC ID: 0x%04X\r\n", w25qxx_dev.type);

    if (w25qxx_dev.type == 0x0000 || w25qxx_dev.type == 0xFFFF) {
        printf("[test_w25q] 未检测到有效设备，终止测试\r\n");
        return;
    }

    // 准备测试数据
    for (uint32_t i = 0; i < W25Q_TEST_LEN; i++) {
        tx_buf[i] = (uint8_t)i;
    }
    memset(rx_buf, 0, sizeof(rx_buf));

    printf("[test_w25q] 擦除扇区 @0x%06X...\r\n", W25Q_TEST_ADDR);
    if (w25qxx_erase_sector(W25Q_TEST_ADDR / W25QXX_SECTOR_SIZE) != HAL_OK) {
        printf("[test_w25q] 扇区擦除失败\r\n");
        return;
    }

    printf("[test_w25q] 写入 %u 字节...\r\n", W25Q_TEST_LEN);
    if (w25qxx_write(tx_buf, W25Q_TEST_ADDR, W25Q_TEST_LEN) != HAL_OK) {
        printf("[test_w25q] 写入失败\r\n");
        return;
    }

    printf("[test_w25q] 读取回验证...\r\n");
    if (w25qxx_read(rx_buf, W25Q_TEST_ADDR, W25Q_TEST_LEN) != HAL_OK) {
        printf("[test_w25q] 读取失败\r\n");
        return;
    }

    if (memcmp(tx_buf, rx_buf, W25Q_TEST_LEN) == 0) {
        printf("[test_w25q] 校验通过，数据一致\r\n");
    } else {
        printf("[test_w25q] 校验失败，存在差异\r\n");
        for (uint32_t i = 0; i < W25Q_TEST_LEN; i++) {
            if (tx_buf[i] != rx_buf[i]) {
                printf("  diff @%lu: write=%u read=%u\r\n",
                       (unsigned long)i, tx_buf[i], rx_buf[i]);
                break;
            }
        }
    }

    printf("[test_w25q] 测试完成\r\n");
}
