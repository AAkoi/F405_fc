#include "elrs_crsf_uart.h"
#include "bsp_uart.h"
#include "stm32f4xx_hal.h"

// 全局上下文（简单起见，单实例）
static elrs_crsf_t g_crsf;

// 启用 DWT 周期计数器（CYCCNT）用于微秒时间戳
static void dwt_setup_cycle_counter(void)
{
    // 使能 DWT / ITM 时钟域
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    // 复位计数器
    DWT->CYCCNT = 0;
    // 使能周期计数
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// 时间戳（us）：使用 HAL_GetTick() 粗略转换为微秒
static uint32_t crsf_now_us(void *user)
{
    (void)user;
    // 使用 DWT 周期计数换算微秒：us = CYCCNT / (SystemCoreClock / 1e6)
    // 对于 168MHz，除数为 168。
    uint32_t div = (SystemCoreClock / 1000000u);
    if (div == 0) {
        // 兜底（不应发生），退回 HAL_GetTick
        return HAL_GetTick() * 1000u;
    }
    return DWT->CYCCNT / div;
}

// 发送：通过 UART1 发送缓冲区
static void crsf_tx_write(void *user, const uint8_t *data, uint16_t len)
{
    (void)user;
    (void)BSP_UART_Write(1, data, len);
}

// 可选：解析到了 0x16 RC 通道时的回调（占位，可在此做通道映射）
static void crsf_on_rc_channels(elrs_crsf_t *ctx, const uint16_t *ch, uint8_t count, uint32_t ts_us)
{
    (void)ctx; (void)ch; (void)count; (void)ts_us;
    // TODO: 将 CRSF 通道映射到飞控通道/期望命令
}

// 可选：链路统计（0x14）回调（占位）
static void crsf_on_link_stats(elrs_crsf_t *ctx, const elrs_crsf_link_stats_t *stats, uint32_t ts_us)
{
    (void)ctx; (void)stats; (void)ts_us;
    // TODO: 记录 RSSI / LQ 等指标
}

// 串口字节回调：绑定到 UART1 -> 输入到 CRSF 解析器
void BSP_UART_RxByteCallback(uint8_t uart_id, uint8_t byte)
{
    if (uart_id == 1) {
        elrs_crsf_input_byte(&g_crsf, byte);
    }
}

void ELRS_CRSF_InitOnUART1(void)
{
    // 启用 DWT 周期计数
    dwt_setup_cycle_counter();

    elrs_crsf_config_t cfg = {0};
    cfg.now_us   = crsf_now_us;
    cfg.tx_write = crsf_tx_write;
    cfg.user     = NULL;
    cfg.on_rc_channels = crsf_on_rc_channels;
    cfg.on_link_stats  = crsf_on_link_stats;
    cfg.on_frame       = NULL; // 其他帧可选处理
    cfg.frame_timeout_us = 0;  // 使用缺省

    elrs_crsf_init(&g_crsf, &cfg);

    // 设置 UART1 波特率为 CRSF 推荐值，并确保使能接收中断
    BSP_UART_Open(1, ELRS_CRSF_BAUD_DEFAULT);
}
