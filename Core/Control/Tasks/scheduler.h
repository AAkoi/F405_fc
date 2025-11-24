/**
 * @file    scheduler.h
 * @brief   飞控专用任务调度器 - 支持周期性任务和事件触发任务
 * @note    基于 DWT 周期计数器实现精确时间管理
 */

#ifndef SCHEDULERH
#define SCHEDULERH

#include <stdint.h>
#include <stdbool.h>
#include "bsp_System.h"

// ============================================================================
// 任务优先级（数字越小优先级越高）
// ============================================================================
typedef enum {
    TASK_PRIORITY_CRITICAL = 0,  // 关键任务：IMU数据处理、姿态融合
    TASK_PRIORITY_HIGH     = 1,  // 高优先级：PID控制、电机输出
    TASK_PRIORITY_NORMAL   = 2,  // 普通优先级：遥控器接收、传感器读取
    TASK_PRIORITY_LOW      = 3,  // 低优先级：气压计、磁力计
    TASK_PRIORITY_IDLE     = 4,  // 空闲任务：LED、日志输出
} task_priority_t;

// ============================================================================
// 任务触发模式
// ============================================================================
typedef enum {
    TASK_TRIGGER_PERIODIC,   // 周期性触发（固定时间间隔）
    TASK_TRIGGER_EVENT,      // 事件触发（标志位或回调判断）
} task_trigger_mode_t;

// ============================================================================
// 任务状态
// ============================================================================
typedef enum {
    TASK_STATE_IDLE = 0,     // 空闲
    TASK_STATE_READY,        // 就绪（等待执行）
    TASK_STATE_RUNNING,      // 运行中
    TASK_STATE_SUSPENDED,    // 挂起
} task_state_t;

// ============================================================================
// 回调函数类型
// ============================================================================
typedef void (*task_cb_t)(void *user);                // 任务执行回调
typedef bool (*task_should_run_cb_t)(void *user);     // 任务判断回调（返回true表示应该执行）

// ============================================================================
// 任务统计信息
// ============================================================================
typedef struct {
    uint32_t exec_count;           // 执行次数
    uint32_t exec_time_us;         // 最近一次执行时间（微秒）
    uint32_t exec_time_max_us;     // 最大执行时间（微秒）
    uint32_t exec_time_total_us;   // 累计执行时间（微秒）
    uint32_t overrun_count;        // 超时次数（执行时间超过允许值）
    uint32_t missed_count;         // 错过次数（未能按时执行）
} task_stats_t;

// ============================================================================
// 任务控制块（Task Control Block）
// ============================================================================
typedef struct {
    // 基本信息
    const char          *name;          // 任务名称
    task_cb_t            callback;      // 任务回调函数
    void                *user_data;     // 用户数据指针

    // 调度参数
    task_priority_t      priority;      // 任务优先级
    task_trigger_mode_t  trigger_mode;  // 触发模式
    task_state_t         state;         // 任务状态

    // 周期性任务参数
    uint32_t             period_cycles;  // 周期（CPU时钟周期数）
    uint32_t             next_run_time;  // 下次运行时间

    // 事件触发任务参数
    task_should_run_cb_t should_run;    // 判断是否应该运行的回调函数
    volatile bool       *event_flag;    // 事件标志位指针

    // 性能监控
    task_stats_t         stats;         // 统计信息
    uint32_t             max_exec_time_us;  // 最大允许执行时间（微秒，0=不限制）

    // 控制标志
    bool                 active;        // 任务是否激活
} task_entry_t;

// ============================================================================
// 调度器配置
// ============================================================================
typedef struct {
    bool     enable_stats;          // 是否启用统计功能
    bool     enable_overrun_check;  // 是否检查任务超时
    uint32_t cpu_freq_hz;           // CPU频率（Hz）
    uint32_t max_tasks;             // 最大任务数量
} scheduler_config_t;

// ============================================================================
// 调度器控制块
// ============================================================================
typedef struct {
    task_entry_t       *tasks;          // 任务数组
    uint8_t             task_count;     // 当前任务数量
    uint8_t             capacity;       // 任务容量
    scheduler_config_t  config;         // 配置参数

    // CPU负载统计
    uint32_t            total_cycles;       // 总周期数
    uint32_t            idle_cycles;        // 空闲周期数
    float               cpu_load;           // CPU负载（0-100%）
    uint32_t            last_load_update;   // 上次负载更新时间
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

#endif 
