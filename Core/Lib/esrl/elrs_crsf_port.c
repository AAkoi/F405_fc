#include <string.h>
#include "elrs_crsf_uart.h"
#include "bsp_uart.h"
#include "stm32f4xx_hal.h"
#include "elrs_crsf_port.h"

// 全局上下文（简单起见，单实例）
static elrs_crsf_t g_crsf;

// RC 状态（由 ISR 回调更新）
static volatile elrs_rc_state_t g_rc;

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
static inline float clampf(float x, float lo, float hi) { return (x < lo) ? lo : (x > hi) ? hi : x; }

// 将 11bit 原始值映射到 -1..+1（中位 1024 -> 0）
static inline float rc_map_axis_11b(uint16_t v)
{
    // 转为 [-1,1]，并做轻微去抖/限幅
    float x = ((int32_t)v - 1024) / 1024.0f;
    return clampf(x, -1.0f, 1.0f);
}


// 油门 0..1
static inline float rc_map_throttle_11b(uint16_t v)
{
    float t = (float)v / 2047.0f;
    return clampf(t, 0.0f, 1.0f);
}

static void crsf_on_rc_channels(elrs_crsf_t *ctx, const uint16_t *ch, uint8_t count, uint32_t ts_us)
{
    (void)ctx;
    if (!ch || count == 0) return;

    // 拷贝原始值
    uint8_t n = (count > 16) ? 16 : count;

    // 关中断，保证一致写入
    __disable_irq();
    for (uint8_t i = 0; i < n; i++) {
        g_rc.raw[i] = ch[i];
    }
    for (uint8_t i = n; i < 16; i++) {
        g_rc.raw[i] = 1024; // 缺省置中
    }
    g_rc.count = n;
    g_rc.last_update_us = ts_us;

    // 轴向归一化
    g_rc.roll  = rc_map_axis_11b(g_rc.raw[RC_CH_INDEX_ROLL]);
    g_rc.pitch = rc_map_axis_11b(g_rc.raw[RC_CH_INDEX_PITCH]);
    g_rc.yaw   = rc_map_axis_11b(g_rc.raw[RC_CH_INDEX_YAW]);
    g_rc.throttle = rc_map_throttle_11b(g_rc.raw[RC_CH_INDEX_THROTTLE]);

    // AUX 位图（CH5..CH16）> 中位判定
    uint16_t aux_bits = 0;
    for (uint8_t ch_idx = 4; ch_idx < 16; ch_idx++) {
        if (g_rc.raw[ch_idx] > 1024) {
            aux_bits |= (uint16_t)(1u << (ch_idx - 4));
        }
    }
    g_rc.aux_bits = aux_bits;
    __enable_irq();

    // 可选：检测某个 AUX 触发绑定
    // 示例：AUX1（CH5，高电平）触发一次绑定命令
    if (aux_bits & 0x0001u) {
        // 为避免高频触发，可在外部调用处做去抖，这里仅做示例
        // elrs_crsf_send_bind(&g_crsf);
    }
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

void ELRS_CRSF_CopyRCState(elrs_rc_state_t *out)
{
    if (!out) return;
    __disable_irq();
    // 因 g_rc 为 volatile，使用 memcpy 获取一致快照
    elrs_rc_state_t tmp;
    memcpy(&tmp, (const void*)&g_rc, sizeof(tmp));
    __enable_irq();
    *out = tmp;
}

bool ELRS_CRSF_IsActive(uint32_t timeout_ms)
{
    uint32_t now_us = crsf_now_us(NULL);
    uint32_t dt_us = now_us - g_rc.last_update_us;
    return dt_us <= (timeout_ms * 1000u);
}

void ELRS_CRSF_SendBind(void)
{
    elrs_crsf_send_bind(&g_crsf);
}
