/**
 * @file    elrs_crsf_port.h
 * @brief   将 ELRS/CRSF 绑定到指定 UART 的移植层（UART1/2 均可）
 */
#ifndef ELRS_CRSF_PORT_H
#define ELRS_CRSF_PORT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化并将 elrs_crsf 绑定到 UART1（默认 420000 波特）
void ELRS_CRSF_InitOnUART1(void);

// 初始化并将 elrs_crsf 绑定到 UART2（默认 420000 波特）
void ELRS_CRSF_InitOnUART2(void);

// 在主循环中调用的处理函数（执行挂起的动作，如发送Bind）
void ELRS_CRSF_Process(void);

// 请求一次绑定（安全：可在中断环境调用，仅置标志）
void ELRS_CRSF_RequestBind(void);

// ========================= RC 映射与访问接口 =========================

// 通道顺序映射（可按需在编译选项中覆盖为 TAER 等）
// 缺省为 AETR：
//   CH1: Roll(A)
//   CH2: Pitch(E)
//   CH3: Throttle(T)
//   CH4: Yaw(R)
#ifndef RC_CH_INDEX_ROLL
#define RC_CH_INDEX_ROLL      0
#endif
#ifndef RC_CH_INDEX_PITCH
#define RC_CH_INDEX_PITCH     1
#endif
#ifndef RC_CH_INDEX_THROTTLE
#define RC_CH_INDEX_THROTTLE  2
#endif
#ifndef RC_CH_INDEX_YAW
#define RC_CH_INDEX_YAW       3
#endif

typedef struct {
    // 原始 CRSF 11bit 数值（0..2047）
    uint16_t raw[16];
    uint8_t  count;            // 有效通道数（通常 16）
    uint32_t last_update_us;   // 最近更新的时间戳（us）

    // 归一化指令
    //   roll/pitch/yaw: -1..+1（0 为居中）
    //   throttle: 0..1
    float roll;
    float pitch;
    float yaw;
    float throttle;

    // 逻辑开关（AUX）位图：当通道值 > 中位时置位
    //   bit0 -> CH5, bit1 -> CH6, ...
    uint16_t aux_bits;
} elrs_rc_state_t;

// 拷贝当前 RC 状态快照（在关中断下进行，确保一致性）
void ELRS_CRSF_CopyRCState(elrs_rc_state_t *out);

// 链路是否活跃：最近一帧在 timeout_ms 内更新
bool ELRS_CRSF_IsActive(uint32_t timeout_ms);

// 发送一次 RX 绑定命令（让接收机进入绑定，部分接收机支持）
void ELRS_CRSF_SendBind(void);

// 将 0..2047 原始通道映射到近似 1000..2000us（中心 ~1500）
uint16_t ELRS_CRSF_MapRaw11bToUs(uint16_t v11b);

// ========================= Link 状态接口（基于 LinkStatistics） =========================
typedef struct {
    uint8_t  lq;               // uplink LQ 0..100
    int8_t   rssi_dbm;         // 上行 RSSI dBm（负值）
    uint32_t last_stats_us;    // 最近一次收到 LinkStatistics 的时间（us）
    uint32_t last_rc_us;       // 最近一次收到 RC 通道帧的时间（us）
} elrs_link_info_t;

// “无线链路是否已连接”判断：要求在 timeout_ms 内收到 LinkStats 且 LQ>0
bool ELRS_CRSF_IsRadioLinked(uint32_t timeout_ms);

// 获取最近的链路信息（LQ/RSSI/时间戳），若未收到则置零
void ELRS_CRSF_GetLinkInfo(elrs_link_info_t *out);

#ifdef __cplusplus
}
#endif

#endif // ELRS_CRSF_PORT_H

