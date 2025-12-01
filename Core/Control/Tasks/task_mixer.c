#include "task_mixer.h"

typedef struct {
    float throttle;
    float roll;
    float pitch;
    float yaw;
} mixer_coeff_t;

// throttle, roll, pitch, yaw
static const mixer_coeff_t mixerQuadX[MIXER_MOTOR_COUNT] = {
    { 1.0f, -1.0f,  1.0f, -1.0f },          // REAR_R
    { 1.0f, -1.0f, -1.0f,  1.0f },          // FRONT_R
    { 1.0f,  1.0f,  1.0f,  1.0f },          // REAR_L
    { 1.0f,  1.0f, -1.0f, -1.0f },          // FRONT_L
};

static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

void mixer_mix_quad_x(float base_throttle, float roll, float pitch, float yaw,
                      float motor_out[MIXER_MOTOR_COUNT])
{
    if (!motor_out) {
        return;
    }

    const float base = clampf(base_throttle, 0.0f, 1.0f);

    for (uint8_t i = 0; i < MIXER_MOTOR_COUNT; i++) {
        const mixer_coeff_t *m = &mixerQuadX[i];
        float out = base * m->throttle + roll * m->roll + pitch * m->pitch + yaw * m->yaw;
        motor_out[i] = clampf(out, 0.0f, 1.0f);
    }
}
