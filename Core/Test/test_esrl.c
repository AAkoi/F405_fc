/**
 * @file    test_esrl.c
 * @brief   使用 UART2 进行 ELRS/CRSF 基本联调（与上位机串口通信）
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "elrs_crsf_port.h"
#include "elrs_crsf_uart.h"  // for ELRS_CRSF_BAUD_DEFAULT
#include "bsp_uart.h"

static void print_channels_us(void)
{
    elrs_rc_state_t rc;
    ELRS_CRSF_CopyRCState(&rc);
    uint16_t ch_us[8];
    for (int i = 0; i < 8; i++) {
        ch_us[i] = ELRS_CRSF_MapRaw11bToUs(rc.raw[i]);
    }
    printf("CH[1..8] us: %u %u %u %u %u %u %u %u\r\n",
           ch_us[0], ch_us[1], ch_us[2], ch_us[3],
           ch_us[4], ch_us[5], ch_us[6], ch_us[7]);
}

void test_esrl_run(void)
{
    printf("\r\n===============================\r\n");
    printf("[test_esrl] ELRS/CRSF on USART2 @%u baud\r\n", (unsigned)ELRS_CRSF_BAUD_DEFAULT);
    printf("===============================\r\n");
    printf("[test_esrl] tip: press 'b' on UART1 to send bind cmd\r\n");

    // 绑定到 UART2，BSP 将打开串口并使能中断接收
    ELRS_CRSF_InitOnUART2();

    uint32_t last_print_ms = 0;
    uint32_t start_ms = HAL_GetTick();
    uint32_t last_active_ms = HAL_GetTick();       // 记录最后一次收到帧的时间
    const uint32_t uart2_reopen_ms = 10000;        // 超过 10 秒未收到帧则重开 UART2
    while (1) {
        // 处理挂起命令（例如由 UART1 发来的 'b' 触发Bind）
        ELRS_CRSF_Process();
        uint32_t now = HAL_GetTick();
        // 注意：默认不自动发送 Bind，避免阻塞已绑定的自动重连。
        if (now - last_print_ms >= 500) {
            last_print_ms = now;
            bool rc_active = ELRS_CRSF_IsActive(800);
            bool radio_ok  = ELRS_CRSF_IsRadioLinked(1200);
            if (rc_active) {
                elrs_link_info_t info; ELRS_CRSF_GetLinkInfo(&info);
                printf("[ESRL] rc=OK radio=%s LQ=%u RSSI=%ddBm  ", radio_ok?"OK":"NO", info.lq, (int)info.rssi_dbm);
                print_channels_us();
                last_active_ms = now;
            } else {
                printf("[ESRL] rc=TIMEOUT, waiting frames...\r\n");
                // 若超过阈值未收到任何帧，重开 UART2 以恢复可能的 RX 卡死
                if (now - last_active_ms > uart2_reopen_ms) {
                    BSP_UART_Open(2, ELRS_CRSF_BAUD_DEFAULT);
                    last_active_ms = now; // 避免频繁重启
                }
            }
        }
    }
}
