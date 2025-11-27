#include <math.h>
#include "task_rc.h"

static elrs_rc_state_t rc_raw;
static rc_command_t rc_cmd;

static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void rc_update(float max_roll_deg, float max_pitch_deg, float max_yaw_deg, uint32_t timeout_ms)
{
    rc_cmd.link_active = false;

    // 拷贝 RC 快照
    ELRS_CRSF_CopyRCState(&rc_raw);

    // 链路检查
    if (!ELRS_CRSF_IsActive(timeout_ms)) {
        return;
    }

    rc_cmd.link_active = true;

    // 归一化输入 -> 期望角度
    rc_cmd.roll_deg  = clampf(rc_raw.roll,  -1.0f, 1.0f) * max_roll_deg;
    rc_cmd.pitch_deg = clampf(rc_raw.pitch, -1.0f, 1.0f) * max_pitch_deg;
    rc_cmd.yaw_deg   = clampf(rc_raw.yaw,   -1.0f, 1.0f) * max_yaw_deg;
    rc_cmd.throttle  = clampf(rc_raw.throttle, 0.0f, 1.0f);

    // 欧拉 -> 四元数（输入为弧度）
    float roll_rad  = rc_cmd.roll_deg  * DEG2RAD;
    float pitch_rad = rc_cmd.pitch_deg * DEG2RAD;
    float yaw_rad   = rc_cmd.yaw_deg   * DEG2RAD;
    rc_cmd.q_des = Attitude_EulerToQuat(roll_rad, pitch_rad, yaw_rad);
}

const rc_command_t *rc_get_command(void)
{
    return &rc_cmd;
}


/*暂时不用
    // 映射到 “us” 风格，并归一化 AUX
    rc_cmd.roll_us     = ELRS_CRSF_MapRaw11bToUs(rc_raw.raw[RC_CH_INDEX_ROLL]);
    rc_cmd.pitch_us    = ELRS_CRSF_MapRaw11bToUs(rc_raw.raw[RC_CH_INDEX_PITCH]);
    rc_cmd.yaw_us      = ELRS_CRSF_MapRaw11bToUs(rc_raw.raw[RC_CH_INDEX_YAW]);
    rc_cmd.throttle_us = ELRS_CRSF_MapRaw11bToUs(rc_raw.raw[RC_CH_INDEX_THROTTLE]);
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t ch = 4 + i; // AUX 从 CH5 开始
        uint16_t raw = (ch < rc_raw.count) ? rc_raw.raw[ch] : 1024;
        rc_cmd.aux_us[i] = ELRS_CRSF_MapRaw11bToUs(raw);
        rc_cmd.aux_norm[i] = clampf((float)raw / 2047.0f, 0.0f, 1.0f);
    }

*/
