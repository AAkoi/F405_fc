/**
 * @file    elrs_crsf_port.h
 * @brief   将 ELRS/CRSF 绑定到 UART1 的移植层
 */
#ifndef ELRS_CRSF_PORT_H
#define ELRS_CRSF_PORT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 初始化并将 elrs_crsf 绑定到 UART1（默认 420000 波特）
void ELRS_CRSF_InitOnUART1(void);

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

#ifdef __cplusplus
}
#endif

#endif // ELRS_CRSF_PORT_H

