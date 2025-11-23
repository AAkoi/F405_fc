#include "imu_task.h"
#include <stdbool.h>
#include <stdio.h>

pt1Filter_t pt1Filter_dev;
pt1raw pt1_raw;
gyro_antialias_t gyro_aa;
static biquadFilter_t aa_x, aa_y, aa_z;
static bool filter_ready = false;
static uint8_t decim_n = 1;
static uint8_t decim_count = 0;
static float decim_sum_x = 0.0f, decim_sum_y = 0.0f, decim_sum_z = 0.0f;
static float raw_sum_x = 0.0f, raw_sum_y = 0.0f, raw_sum_z = 0.0f;
gyro_decim_t gyro_decim;
gyro_trace_t gyro_trace;

// Filter and decimate one gyro sample
static bool gyro_filter_process_sample(int16_t gx, int16_t gy, int16_t gz)
{
    if (!filter_ready) {
        static uint8_t warn_count = 0;
        if (warn_count++ < 5) {
            printf("[gyro_raw_Flitter] Filter not ready!\r\n");
        }
        return false;
    }

    icm42688p_gyro_rawPreprocess(&gx, &gy, &gz);
    const float gscale = (icm.gyro_scale > 0.0f) ? icm.gyro_scale : 1.0f;
    raw_sum_x += (float)gx;
    raw_sum_y += (float)gy;
    raw_sum_z += (float)gz;

    pt1_raw.pt1_gyro_x = pt1FilterApply(&pt1Filter_dev, (float)gx);
    pt1_raw.pt1_gyro_y = pt1FilterApply(&pt1Filter_dev, (float)gy);
    pt1_raw.pt1_gyro_z = pt1FilterApply(&pt1Filter_dev, (float)gz);
    gyro_aa.x = biquadFilterApply(&aa_x, pt1_raw.pt1_gyro_x);
    gyro_aa.y = biquadFilterApply(&aa_y, pt1_raw.pt1_gyro_y);
    gyro_aa.z = biquadFilterApply(&aa_z, pt1_raw.pt1_gyro_z);

    // Accumulate for averaging
    decim_sum_x += gyro_aa.x;
    decim_sum_y += gyro_aa.y;
    decim_sum_z += gyro_aa.z;
    decim_count++;

    // Decimate: output once every decim_n samples
    if (decim_count >= decim_n) {
        decim_count = 0;
        const float inv = 1.0f / (float)decim_n;
        gyro_trace.raw_dps_x = (raw_sum_x * inv) / gscale;
        gyro_trace.raw_dps_y = (raw_sum_y * inv) / gscale;
        gyro_trace.raw_dps_z = (raw_sum_z * inv) / gscale;
        gyro_decim.dps_x = (decim_sum_x * inv) / gscale;
        gyro_decim.dps_y = (decim_sum_y * inv) / gscale;
        gyro_decim.dps_z = (decim_sum_z * inv) / gscale;
        gyro_decim.ready = true;
        decim_sum_x = decim_sum_y = decim_sum_z = 0.0f;
        raw_sum_x = raw_sum_y = raw_sum_z = 0.0f;
    } else {
        gyro_decim.ready = false;
    }

    return true;
}

// Feed an already-read gyro sample into the filter/decimator
bool gyro_filter_feed_sample(int16_t gyro_x, int16_t gyro_y, int16_t gyro_z)
{
    return gyro_filter_process_sample(gyro_x, gyro_y, gyro_z);
}

void gyro_filter_init(float sample_hz, float pt1_cut_hz, float aa_cut_hz, uint8_t decim_factor)
{
    if (sample_hz <= 0.0f) {
        return;
    }

    const float dt = 1.0f / sample_hz;
    const uint32_t refresh_us = (uint32_t)(1000000.0f / sample_hz);

    pt1FilterInit(&pt1Filter_dev, pt1FilterGain(pt1_cut_hz, dt));
    biquadFilterInitLPF(&aa_x, aa_cut_hz, refresh_us);
    biquadFilterInitLPF(&aa_y, aa_cut_hz, refresh_us);
    biquadFilterInitLPF(&aa_z, aa_cut_hz, refresh_us);

    if (decim_factor == 0) {
        decim_factor = 1;
    }

    decim_n = decim_factor;
    decim_count = 0;

    pt1_raw.pt1_gyro_x = pt1_raw.pt1_gyro_y = pt1_raw.pt1_gyro_z = 0.0f;
    gyro_aa.x = gyro_aa.y = gyro_aa.z = 0.0f;
    decim_sum_x = decim_sum_y = decim_sum_z = 0.0f;
    raw_sum_x = raw_sum_y = raw_sum_z = 0.0f;
    gyro_decim.dps_x = gyro_decim.dps_y = gyro_decim.dps_z = 0.0f;
    gyro_decim.ready = false;
    gyro_trace.raw_dps_x = gyro_trace.raw_dps_y = gyro_trace.raw_dps_z = 0.0f;
    filter_ready = true;
}

bool gyro_raw_Flitter(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z)
{
    int16_t gx = 0, gy = 0, gz = 0;
    if (!icm42688p_get_gyro_data(&gx, &gy, &gz)) {
        static uint8_t err_count = 0;
        if (err_count++ < 5) {
            printf("[gyro_raw_Flitter] Failed to get gyro data\r\n");
        }
        return false;
    }

    if (gyro_x) *gyro_x = gx;
    if (gyro_y) *gyro_y = gy;
    if (gyro_z) *gyro_z = gz;

    return gyro_filter_process_sample(gx, gy, gz);
}
