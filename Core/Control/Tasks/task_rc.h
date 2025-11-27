#ifndef TASK_RC_H
#define TASK_RC_H

#include <stdbool.h>
#include "elrs_crsf_port.h"
#include "attitude.h"

typedef struct {
    float throttle;   // 0..1
    float roll_deg;   // 期望角度（度）
    float pitch_deg;  // 期望角度（度）
    float yaw_deg;    // 期望角度（度）
    // 原始 11bit 通道映射到近似 1000-2000us（兼容 Betaflight 风格）
    uint16_t roll_us;
    uint16_t pitch_us;
    uint16_t yaw_us;
    uint16_t throttle_us;
    uint16_t aux_us[8];
    float aux_norm[8]; // 0..1，方便 UI 条形显示
    Quaternion q_des; // 期望姿态四元数
    bool link_active;
} rc_command_t;

/**
 * @brief 读取 ELRS/CRSF 指令并转换为期望姿态四元数
 * @param max_roll_deg  roll 最大角度
 * @param max_pitch_deg pitch 最大角度
 * @param max_yaw_deg   yaw 最大角度（若用速率模式，可传 0 忽略）
 * @param timeout_ms    链路超时判定
 */
void rc_update(float max_roll_deg, float max_pitch_deg, float max_yaw_deg, uint32_t timeout_ms);

/**
 * @brief 获取最新期望指令
 */
const rc_command_t *rc_get_command(void);

#endif // TASK_RC_H
