/**
 * @file    task_scheduler_fc.h
 * @brief   飞控任务调度器：支持周期/事件任务，兼容预注册器 task_register。
 */

#ifndef TASK_SCHEDULER_FC_H
#define TASK_SCHEDULER_FC_H

#include <stdint.h>
#include <stdbool.h>
#include "bsp_System.h"

// 优先级（数字越小优先级越高）
typedef enum {
    TASK_PRIORITY_CRITICAL = 0,
    TASK_PRIORITY_HIGH     = 1,
    TASK_PRIORITY_NORMAL   = 2,
    TASK_PRIORITY_LOW      = 3,
    TASK_PRIORITY_IDLE     = 4,
} task_priority_t;

// 触发模式
typedef enum {
    TASK_TRIGGER_PERIODIC,   // 周期触发
    TASK_TRIGGER_EVENT,      // 事件触发（标志位或 should_run 回调）
} task_trigger_mode_t;

// 任务状态
typedef enum {
    TASK_STATE_IDLE = 0,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_SUSPENDED,
} task_state_t;

// 回调类型
typedef void (*task_cb_t)(void *user);
typedef bool (*task_should_run_cb_t)(void *user);

// 统计信息
typedef struct {
    uint32_t exec_count;
    uint32_t exec_time_us;
    uint32_t exec_time_max_us;
    uint32_t exec_time_total_us;
    uint32_t overrun_count;
    uint32_t missed_count;
} task_stats_t;

// 任务控制块
typedef struct {
    const char          *name;
    task_cb_t            callback;
    void                *user_data;

    task_priority_t      priority;
    task_trigger_mode_t  trigger_mode;
    task_state_t         state;

    // 周期任务
    uint32_t             period_cycles;
    uint32_t             next_run_time;

    // 事件任务
    task_should_run_cb_t should_run;
    volatile bool       *event_flag;

    task_stats_t         stats;
    uint32_t             max_exec_time_us;

    bool                 active;
} task_entry_t;

// 调度器配置
typedef struct {
    bool     enable_stats;
    bool     enable_overrun_check;
    uint32_t cpu_freq_hz;
    uint32_t max_tasks;
} scheduler_config_t;

// 调度器控制块
typedef struct {
    task_entry_t       *tasks;
    uint8_t             task_count;
    uint8_t             capacity;
    scheduler_config_t  config;

    uint32_t            total_cycles;
    uint32_t            idle_cycles;
    float               cpu_load;
    uint32_t            last_load_update;
} task_scheduler_fc_t;

void scheduler_init(task_scheduler_fc_t *sched,
                    task_entry_t *storage,
                    uint8_t capacity,
                    const scheduler_config_t *config);

bool scheduler_register_periodic(task_scheduler_fc_t *sched,
                                 const char *name,
                                 task_cb_t callback,
                                 void *user_data,
                                 task_priority_t priority,
                                 uint32_t period_us,
                                 uint32_t max_exec_us);

bool scheduler_register_event_flag(task_scheduler_fc_t *sched,
                                   const char *name,
                                   task_cb_t callback,
                                   void *user_data,
                                   task_priority_t priority,
                                   volatile bool *event_flag,
                                   uint32_t max_exec_us);

bool scheduler_register_event_callback(task_scheduler_fc_t *sched,
                                       const char *name,
                                       task_cb_t callback,
                                       task_should_run_cb_t should_run,
                                       void *user_data,
                                       task_priority_t priority,
                                       uint32_t max_exec_us);

void scheduler_run(task_scheduler_fc_t *sched);
void scheduler_trigger_from_isr(task_scheduler_fc_t *sched, const char *task_name);

bool scheduler_suspend_task(task_scheduler_fc_t *sched, const char *name);
bool scheduler_resume_task(task_scheduler_fc_t *sched, const char *name);

const task_stats_t* scheduler_get_stats(task_scheduler_fc_t *sched, const char *name);
void scheduler_print_stats(task_scheduler_fc_t *sched);
float scheduler_get_cpu_load(task_scheduler_fc_t *sched);
void scheduler_reset_stats(task_scheduler_fc_t *sched);

#endif // TASK_SCHEDULER_FC_H
