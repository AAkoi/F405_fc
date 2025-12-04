/**
 * @file    test_scheduler.c
 * @brief   验证任务调度器：gyro/acc/mag/filter 组合流水线
 *
 * 任务列表：
 *  - IMU_POLL      周期1kHz：读取 ICM42688P，喂给 gyro/acc 处理
 *  - MAG_POLL      周期50Hz：读取 HMC5883L，喂给磁力计处理
 *  - GYRO_FILTER   事件：当 gyro_decimated.ready 时运行，做滤波
 *  - STREAM        周期20Hz：串口输出最近一次的处理结果
 *  - STATS         周期1Hz：打印调度器统计（可选）
 *
 * 使用方法：在 main.c 中选择 RUN_MODE=4 运行该测试。
 */

#include "test_scheduler.h"

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "scheduler.h"
#include "task_register.h"
#include "task_gyro.h"
#include "task_acc.h"
#include "task_mag.h"
#include "task_fliter.h"
#include "icm42688p.h"
#include "hmc5883l.h"
#include "bsp_System.h"

#define SCHED_TEST_CAPACITY   8
#define IMU_PERIOD_US         125     // 8 kHz IMU 采样
#define MAG_PERIOD_US         66667   // 15 Hz 磁力计（与默认 HMC ODR 一致）
#define STREAM_PERIOD_US      50000   // 20 Hz 串口输出
#define STATS_PERIOD_US       1000000 // 1 Hz 调度器统计

static task_entry_t task_pool[SCHED_TEST_CAPACITY];
static task_scheduler_fc_t sched;
static bool mag_available = false;
static bool dma_status_printed = false;
static uint32_t dma_period_a = 0;  // 约125us计数
static uint32_t dma_period_b = 0;  // 约250us计数
static uint32_t dma_period_other = 0;
static uint32_t dma_period_last_seen = 0;
static uint32_t dma_window_start_ms = 0;

// ----------------------------------------------------------------------------
// 任务：读取 IMU 数据并喂入 gyro/acc 管线
// ----------------------------------------------------------------------------
static void task_imu_poll(void *user)
{
    (void)user;
    int16_t gx = 0, gy = 0, gz = 0;
    int16_t ax = 0, ay = 0, az = 0;
    float   temp = 0.0f;

    if (!icm42688p_get_all_data(&gx, &gy, &gz, &ax, &ay, &az, &temp)) {
        return; // 本周期没有新数据
    }

    // 喂入姿态前端管线
    gyro_process_sample(gx, gy, gz);
    accel_process_sample(ax, ay, az);
}

// ----------------------------------------------------------------------------
// 任务：读取磁力计并喂入处理管线
// ----------------------------------------------------------------------------
static void task_mag_poll(void *user)
{
    (void)user;
    if (!mag_available) {
        return;
    }

    int16_t mx = 0, my = 0, mz = 0;
    if (hmc5883l_read_raw_data(&mx, &my, &mz)) {
        mag_process_sample(mx, my, mz);
    }
}

// ----------------------------------------------------------------------------
// 任务：当 gyro_decimated.ready 时执行滤波
// ----------------------------------------------------------------------------
static bool should_run_filter(void *user)
{
    (void)user;
    return gyro_decimated.ready;
}

static void task_gyro_filter(void *user)
{
    (void)user;
    // 仅在 ready 时处理，并手动清除 ready 以避免重复触发
    if (!gyro_decimated.ready) {
        return;
    }

    gyro_filter_feed_sample(
        gyro_decimated.dps_x,
        gyro_decimated.dps_y,
        gyro_decimated.dps_z
    );

    gyro_decimated.ready = false;
}

// ----------------------------------------------------------------------------
// 任务：输出当前数据（用于观察流水线是否正常）
// ----------------------------------------------------------------------------
static void task_stream(void *user)
{
    (void)user;
/*     printf("STREAM,"
           "gyro_raw(dps)=%.2f,%.2f,%.2f;"
           "gyro_filt(dps)=%.2f,%.2f,%.2f;"
           "acc(g)=%.3f,%.3f,%.3f",
           gyro_scaled.dps_x, gyro_scaled.dps_y, gyro_scaled.dps_z,
           gyro_filtered.dps_x, gyro_filtered.dps_y, gyro_filtered.dps_z,
           accel_scaled.g_x, accel_scaled.g_y, accel_scaled.g_z); */

    if (mag_available && mag_calibrated.ready) {
/*         printf(";mag(raw)=%d,%d,%d;mag(G)=%.3f,%.3f,%.3f;|B|=%.3f",
               mag_raw.x, mag_raw.y, mag_raw.z,
               mag_calibrated.gauss_x, mag_calibrated.gauss_y, mag_calibrated.gauss_z,
               mag_calibrated.magnitude_gauss); */
    }
    printf("\r\n");
}

// ----------------------------------------------------------------------------
// 任务：周期打印调度器统计
// ----------------------------------------------------------------------------
static void task_print_stats(void *user)
{
    (void)user;
    scheduler_print_stats(&sched);
}

// ----------------------------------------------------------------------------
// 定时输出 DMA 状态（避免反复查询）
// ----------------------------------------------------------------------------
static void task_dma_status(void *user)
{
    (void)user;
    const bool dma_on = icm42688p_dma_active();
    const uint32_t period_cycles = icm42688p_dma_period_cycles();
    if (period_cycles != dma_period_last_seen) {
        dma_period_last_seen = period_cycles;
        float period_us = 0.0f;
        if (period_cycles > 0) {
            period_us = (float)period_cycles * 1000000.0f / (float)SystemCoreClock;
        }
        if (period_us > 80.0f && period_us < 170.0f) {
            dma_period_a++;
        } else if (period_us > 170.0f && period_us < 330.0f) {
            dma_period_b++;
        } else {
            dma_period_other++;
        }
    }

    uint32_t now_ms = HAL_GetTick();
    if (dma_window_start_ms == 0) {
        dma_window_start_ms = now_ms;
    }
    if ((now_ms - dma_window_start_ms) >= 1000U) { // 每秒输出一次
        printf("[dma] mode=%s, last=%.2fus, a(125us)=%lu, b(250us)=%lu, other=%lu, ideal_a~8000, ideal_b~0\r\n",
               dma_on ? "DMA" : "POLL",
               (float)period_cycles * 1000000.0f / (float)SystemCoreClock,
               (unsigned long)dma_period_a,
               (unsigned long)dma_period_b,
               (unsigned long)dma_period_other);
        dma_period_a = dma_period_b = dma_period_other = 0;
        dma_window_start_ms = now_ms;
    }
}

// ----------------------------------------------------------------------------
// 初始化传感器与处理链
// ----------------------------------------------------------------------------
static void init_sensor_pipeline(void)
{
    printf("[sched_test] 初始化 ICM42688P...\r\n");
    icm42688p_init_driver();
    HAL_Delay(50);

    printf("[sched_test] 初始化 HMC5883L...\r\n");
    mag_available = hmc5883l_init_driver();
    if (!mag_available) {
        printf("[sched_test] 磁力计初始化失败，跳过 MAG_POLL 任务\r\n");
    }

    // 初始化处理链
    gyro_processing_init(1);  // 不降采样，直接按原始 ODR 处理
    accel_processing_init();
    mag_processing_init();
    gyro_filter_init(8000.0f, 400.0f, 800.0f); // 8kHz 输入时示例的截止频率，可按需要调整

    // 打印 DMA 运行模式信息（一次性）
    if (!dma_status_printed) {
        const bool dma_on = icm42688p_dma_active();
        if (dma_on) {
            const uint32_t period_cycles = icm42688p_dma_period_cycles();
            float period_us = 0.0f;
            if (period_cycles > 0) {
                period_us = (float)period_cycles * 1000000.0f / (float)SystemCoreClock;
            }
            printf("[sched_test] ICM42688P DMA 模式: ON, DRDY 周期≈%.2fus (hold=%lu)\r\n",
                   period_us, (unsigned long)icm42688p_dma_hold_target());
        } else {
            printf("[sched_test] ICM42688P DMA 模式: OFF (轮询)\r\n");
        }
        dma_status_printed = true;
    }
}

// ----------------------------------------------------------------------------
// 注册任务
// ----------------------------------------------------------------------------
static void register_tasks(void)
{
    task_register_clear();

    task_register_periodic("IMU_POLL", task_imu_poll, NULL,
                           TASK_PRIORITY_CRITICAL, IMU_PERIOD_US, 200);

    if (mag_available) {
        task_register_periodic("MAG_POLL", task_mag_poll, NULL,
                               TASK_PRIORITY_LOW, MAG_PERIOD_US, 500);
    }

    task_register_event_cb("GYRO_FILTER", task_gyro_filter, should_run_filter,
                                 NULL, TASK_PRIORITY_HIGH, 200);

    // 调试阶段：避免长串口输出，暂不注册 STREAM/STATS

    // 周期输出 DMA 状态与周期统计
    task_register_periodic("DMA_STAT", task_dma_status, NULL,
                           TASK_PRIORITY_IDLE, STATS_PERIOD_US, 0);

    scheduler_config_t cfg = {
        .enable_stats = true,
        .enable_overrun_check = true,
        .cpu_freq_hz = SystemCoreClock,
        .max_tasks = SCHED_TEST_CAPACITY
    };

    scheduler_init(&sched, task_pool, SCHED_TEST_CAPACITY, &cfg);
    int registered = task_register_apply(&sched);
    printf("[sched_test] 已注册任务数: %d\r\n", registered);
}

// ----------------------------------------------------------------------------
// 主入口
// ----------------------------------------------------------------------------
void test_scheduler_run(void)
{
    printf("\r\n========================================\r\n");
    printf("[sched_test] 任务调度器自检 (gyro+acc+mag+filter)\r\n");
    printf("========================================\r\n\r\n");

    init_sensor_pipeline();
    register_tasks();

    printf("[sched_test] 开始循环调度...\r\n");
    printf("[sched_test] 任务间隔: IMU_POLL=%uus, MAG_POLL=%uus, GYRO_FILTER=事件就绪, DMA_STAT=%uus\r\n",
           IMU_PERIOD_US, MAG_PERIOD_US, STATS_PERIOD_US);
    while (1) {
        scheduler_run(&sched);
    }
}
