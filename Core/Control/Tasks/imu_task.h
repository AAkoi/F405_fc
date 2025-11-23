/**
 * @file    imu_task.h
 * @brief   Gyro raw -> filter -> decimation pipeline.
 */

#ifndef IMU_TASK_H
#define IMU_TASK_H

#include "icm42688p.h"
#include <stdbool.h>
#include "filter.h"

// Gyro raw -> PT1 -> anti-alias LPF -> decimation
typedef struct pt1raw_s {
    float pt1_gyro_x;
    float pt1_gyro_y;
    float pt1_gyro_z;
} pt1raw;

typedef struct gyro_antialias_s {
    float x;
    float y;
    float z;
} gyro_antialias_t;

typedef struct gyro_decim_s {
    float dps_x;
    float dps_y;
    float dps_z;
    bool  ready;
} gyro_decim_t; // decimated and scaled to dps

typedef struct gyro_trace_s {
    float raw_dps_x; // averaged raw window, scaled to dps
    float raw_dps_y;
    float raw_dps_z;
} gyro_trace_t;

// Init filters: sample Hz, PT1 cut Hz, anti-alias LPF cut Hz, decimation factor
void gyro_filter_init(float sample_hz, float pt1_cut_hz, float aa_cut_hz, uint8_t decim_factor);
bool gyro_raw_Flitter(int16_t *gyro_x, int16_t *gyro_y, int16_t *gyro_z);
// Feed already-read gyro raw data into filter/decimator
bool gyro_filter_feed_sample(int16_t gyro_x, int16_t gyro_y, int16_t gyro_z);

extern pt1raw pt1_raw;
extern gyro_antialias_t gyro_aa;
extern gyro_decim_t gyro_decim;
extern gyro_trace_t gyro_trace;

#endif // IMU_TASK_H
