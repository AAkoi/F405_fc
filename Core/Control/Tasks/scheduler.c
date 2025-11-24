/**
 * @file    scheduler.c
 * @brief   飞控任务调度器实现
 */

#include "scheduler.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// 内部辅助函数
// ============================================================================

/**
 * @brief 根据任务名查找任务索引
 * @param sched 调度器实例
 * @param name 任务名称
 * @return 任务索引，未找到返回-1
 */
static int find_task_index(task_scheduler_fc_t *sched, const char *name)
{
    if (!sched || !name) return -1;
    for (uint8_t i = 0; i < sched->task_count; i++) {
        if (sched->tasks[i].name && strcmp(sched->tasks[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief CPU周期数转换为微秒
 * @param cycles CPU周期数
 * @param cpu_freq_hz CPU频率（Hz）
 * @return 微秒数
 */
static inline uint32_t cycles_to_us(uint32_t cycles, uint32_t cpu_freq_hz)
{
    return (cycles / (cpu_freq_hz / 1000000));
}

// ============================================================================
// 任务注册函数
// ============================================================================

/**
 * @brief 注册周期性任务
 */
bool scheduler_register_periodic(task_scheduler_fc_t *sched,
                                 const char *name,
                                 task_cb_t callback,
                                 void *user_data,
                                 task_priority_t priority,
                                 uint32_t period_us,
                                 uint32_t max_exec_us)
{
    // 参数检查
    if (!sched || !name || !callback || period_us == 0) {
        return false;
    }
    if (sched->task_count >= sched->capacity) {
        return false;  // 任务数量已满
    }

    // 分配任务槽
    task_entry_t *task = &sched->tasks[sched->task_count++];
    
    // 设置基本信息
    task->name = name;
    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->trigger_mode = TASK_TRIGGER_PERIODIC;
    task->state = TASK_STATE_READY;

    // 计算周期（微秒转为CPU周期数）
    task->period_cycles = clockMicrosToCycles(period_us);
    if (task->period_cycles == 0) {
        task->period_cycles = 1;  // 防止除零错误
    }
    
    // 设置下次运行时间
    uint32_t now = DWT_GetTick();
    task->next_run_time = now + task->period_cycles;

    // 性能监控参数
    task->max_exec_time_us = max_exec_us;
    task->active = true;
    
    // 清空统计信息
    memset(&task->stats, 0, sizeof(task_stats_t));
    
    return true;
}

/**
 * @brief 注册事件触发任务（标志位方式）
 */
bool scheduler_register_event_flag(task_scheduler_fc_t *sched,
                                   const char *name,
                                   task_cb_t callback,
                                   void *user_data,
                                   task_priority_t priority,
                                   volatile bool *event_flag,
                                   uint32_t max_exec_us)
{
    // 参数检查
    if (!sched || !name || !callback || !event_flag) return false;
    if (sched->task_count >= sched->capacity) return false;

    // 分配任务槽
    task_entry_t *task = &sched->tasks[sched->task_count++];
    
    // 设置基本信息
    task->name = name;
    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->trigger_mode = TASK_TRIGGER_EVENT;
    task->state = TASK_STATE_READY;
    
    // 设置事件标志位
    task->event_flag = event_flag;
    task->should_run = NULL;
    
    // 性能监控参数
    task->max_exec_time_us = max_exec_us;
    task->active = true;
    
    // 清空统计信息
    memset(&task->stats, 0, sizeof(task_stats_t));
    
    return true;
}

/**
 * @brief 注册事件触发任务（回调判断方式）
 */
bool scheduler_register_event_callback(task_scheduler_fc_t *sched,
                                       const char *name,
                                       task_cb_t callback,
                                       task_should_run_cb_t should_run,
                                       void *user_data,
                                       task_priority_t priority,
                                       uint32_t max_exec_us)
{
    // 参数检查
    if (!sched || !name || !callback || !should_run) return false;
    if (sched->task_count >= sched->capacity) return false;

    // 分配任务槽
    task_entry_t *task = &sched->tasks[sched->task_count++];
    
    // 设置基本信息
    task->name = name;
    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->trigger_mode = TASK_TRIGGER_EVENT;
    task->state = TASK_STATE_READY;
    
    // 设置判断回调
    task->event_flag = NULL;
    task->should_run = should_run;
    
    // 性能监控参数
    task->max_exec_time_us = max_exec_us;
    task->active = true;
    
    // 清空统计信息
    memset(&task->stats, 0, sizeof(task_stats_t));
    
    return true;
}

// ============================================================================
// 调度器初始化
// ============================================================================

/**
 * @brief 初始化调度器
 */
void scheduler_init(task_scheduler_fc_t *sched,
                    task_entry_t *storage,
                    uint8_t capacity,
                    const scheduler_config_t *config)
{
    if (!sched || !storage) return;

    // 设置任务存储
    sched->tasks = storage;
    sched->capacity = capacity;
    sched->task_count = 0;

    // 应用配置
    if (config) {
        sched->config = *config;  // 使用外部配置
    } else {
        // 使用默认配置
        sched->config.enable_stats = true;
        sched->config.enable_overrun_check = true;
        sched->config.cpu_freq_hz = SystemCoreClock;
        sched->config.max_tasks = capacity;
    }

    // 清空所有任务槽
    for (uint8_t i = 0; i < capacity; i++) {
        memset(&sched->tasks[i], 0, sizeof(task_entry_t));
        sched->tasks[i].state = TASK_STATE_IDLE;
        sched->tasks[i].active = false;
    }

    // 初始化CPU负载统计
    sched->total_cycles = 0;
    sched->idle_cycles = 0;
    sched->cpu_load = 0.0f;
    sched->last_load_update = DWT_GetTick();
}

// ============================================================================
// 任务执行逻辑
// ============================================================================

/**
 * @brief 执行单个任务并记录统计信息
 * @param sched 调度器实例
 * @param task 要执行的任务
 */
static void execute_task(task_scheduler_fc_t *sched, task_entry_t *task)
{
    if (!task || !task->callback || !task->active) return;

    // 记录开始时间
    uint32_t start_time = DWT_GetTick();
    task->state = TASK_STATE_RUNNING;
    
    // 执行任务回调
    task->callback(task->user_data);
    
    // 记录结束时间
    uint32_t end_time = DWT_GetTick();
    task->state = TASK_STATE_READY;

    // 计算执行时间并更新统计
    uint32_t exec_cycles = end_time - start_time;
    if (sched->config.enable_stats) {
        task->stats.exec_count++;
        task->stats.exec_time_us = cycles_to_us(exec_cycles, sched->config.cpu_freq_hz);
        task->stats.exec_time_total_us += task->stats.exec_time_us;
        
        // 更新最大执行时间
        if (task->stats.exec_time_us > task->stats.exec_time_max_us) {
            task->stats.exec_time_max_us = task->stats.exec_time_us;
        }
        
        // 检查是否超过最大允许时间
        if (sched->config.enable_overrun_check && task->max_exec_time_us > 0) {
            if (task->stats.exec_time_us > task->max_exec_time_us) {
                task->stats.overrun_count++;
            }
        }
        
        // 对于周期任务，检查是否超过周期时间
        if (task->trigger_mode == TASK_TRIGGER_PERIODIC) {
            uint32_t period_us = cycles_to_us(task->period_cycles, sched->config.cpu_freq_hz);
            if (task->stats.exec_time_us > period_us) {
                task->stats.overrun_count++;
            }
        }
    }
}

/**
 * @brief 判断任务是否应该运行
 * @param task 任务
 * @param now 当前时间（CPU周期数）
 * @return true=应该运行，false=不应该运行
 */
static bool should_task_run(task_entry_t *task, uint32_t now)
{
    // 检查任务是否激活且未挂起
    if (!task->active || task->state == TASK_STATE_SUSPENDED) {
        return false;
    }
    
    // 根据触发模式判断
    switch (task->trigger_mode) {
        case TASK_TRIGGER_PERIODIC:
            // 周期任务：检查是否到达运行时间
            return ((int32_t)(now - task->next_run_time) >= 0);
            
        case TASK_TRIGGER_EVENT:
            // 事件任务：检查标志位或回调函数
            if (task->event_flag) {
                return (*task->event_flag != 0);
            } else if (task->should_run) {
                return task->should_run(task->user_data);
            }
            return false;
            
        default:
            return false;
    }
}

// ============================================================================
// 调度器主循环
// ============================================================================

/**
 * @brief 调度器主循环
 * @note 按优先级从高到低遍历所有任务，执行到期的任务
 */
void scheduler_run(task_scheduler_fc_t *sched)
{
    if (!sched) return;

    uint32_t now = DWT_GetTick();
    uint32_t loop_start = now;
    bool any_task_ran = false;

    // 按优先级从高到低遍历（CRITICAL -> HIGH -> NORMAL -> LOW -> IDLE）
    for (task_priority_t prio = TASK_PRIORITY_CRITICAL; prio <= TASK_PRIORITY_IDLE; prio++) {
        for (uint8_t i = 0; i < sched->task_count; i++) {
            task_entry_t *task = &sched->tasks[i];
            
            // 跳过不是当前优先级的任务
            if (task->priority != prio) continue;

            // 更新当前时间（因为前面的任务可能执行了一段时间）
            now = DWT_GetTick();
            
            // 判断任务是否应该运行
            if (should_task_run(task, now)) {
                // 执行任务
                execute_task(sched, task);
                any_task_ran = true;

                // 周期任务：更新下次运行时间
                if (task->trigger_mode == TASK_TRIGGER_PERIODIC) {
                    now = DWT_GetTick();
                    task->next_run_time = now + task->period_cycles;
                    
                    // 检测是否错过了周期（任务执行时间过长）
                    if ((int32_t)(now - task->next_run_time) >= 0) {
                        if (sched->config.enable_stats) {
                            task->stats.missed_count++;
                        }
                        // 直接设置下一个周期，避免追赶
                        task->next_run_time = now + task->period_cycles;
                    }
                }

                // 事件任务：清除事件标志位
                if (task->trigger_mode == TASK_TRIGGER_EVENT && task->event_flag) {
                    *task->event_flag = false;  // 自动清零标志位
                }
            }
        }
    }

    // 计算CPU负载
    if (sched->config.enable_stats) {
        uint32_t loop_end = DWT_GetTick();
        uint32_t loop_cycles = loop_end - loop_start;
        
        sched->total_cycles += loop_cycles;
        if (!any_task_ran) {
            sched->idle_cycles += loop_cycles;  // 没有任务运行，累计空闲时间
        }
        
        // 每秒更新一次CPU负载
        uint32_t load_update_interval = sched->config.cpu_freq_hz;  // 1秒
        if ((loop_end - sched->last_load_update) >= load_update_interval) {
            if (sched->total_cycles > 0) {
                sched->cpu_load = 100.0f * (1.0f - (float)sched->idle_cycles / (float)sched->total_cycles);
            }
            // 重置计数器
            sched->total_cycles = 0;
            sched->idle_cycles = 0;
            sched->last_load_update = loop_end;
        }
    }
}

// ============================================================================
// 中断触发
// ============================================================================

/**
 * @brief 在中断中直接触发任务执行
 * @warning 任务会在中断上下文中执行，必须确保任务足够快！
 */
void scheduler_trigger_from_isr(task_scheduler_fc_t *sched, const char *task_name)
{
    if (!sched || !task_name) return;
    
    // 查找任务
    int idx = find_task_index(sched, task_name);
    if (idx < 0) return;
    
    task_entry_t *task = &sched->tasks[idx];
    
    // 在中断上下文中直接执行任务
    if (task->active && task->callback) {
        uint32_t start = DWT_GetTick();
        task->callback(task->user_data);
        uint32_t end = DWT_GetTick();
        
        // 简单的统计（不做复杂计算）
        if (sched->config.enable_stats) {
            task->stats.exec_count++;
            uint32_t exec_us = cycles_to_us(end - start, sched->config.cpu_freq_hz);
            task->stats.exec_time_us = exec_us;
            if (exec_us > task->stats.exec_time_max_us) {
                task->stats.exec_time_max_us = exec_us;
            }
        }
    }
}

// ============================================================================
// 任务管理
// ============================================================================

/**
 * @brief 挂起任务（暂停执行）
 */
bool scheduler_suspend_task(task_scheduler_fc_t *sched, const char *name)
{
    int idx = find_task_index(sched, name);
    if (idx < 0) return false;
    
    sched->tasks[idx].state = TASK_STATE_SUSPENDED;
    return true;
}

/**
 * @brief 恢复任务（继续执行）
 */
bool scheduler_resume_task(task_scheduler_fc_t *sched, const char *name)
{
    int idx = find_task_index(sched, name);
    if (idx < 0) return false;
    
    sched->tasks[idx].state = TASK_STATE_READY;
    
    // 对于周期任务，重置下次运行时间
    if (sched->tasks[idx].trigger_mode == TASK_TRIGGER_PERIODIC) {
        uint32_t now = DWT_GetTick();
        sched->tasks[idx].next_run_time = now + sched->tasks[idx].period_cycles;
    }
    
    return true;
}

// ============================================================================
// 统计信息查询
// ============================================================================

/**
 * @brief 获取任务统计信息
 */
const task_stats_t* scheduler_get_stats(task_scheduler_fc_t *sched, const char *name)
{
    int idx = find_task_index(sched, name);
    if (idx < 0) return NULL;
    
    return &sched->tasks[idx].stats;
}

/**
 * @brief 获取CPU负载
 */
float scheduler_get_cpu_load(task_scheduler_fc_t *sched)
{
    if (!sched) return 0.0f;
    return sched->cpu_load;
}

/**
 * @brief 重置所有任务的统计信息
 */
void scheduler_reset_stats(task_scheduler_fc_t *sched)
{
    if (!sched) return;
    
    // 清空所有任务的统计
    for (uint8_t i = 0; i < sched->task_count; i++) {
        memset(&sched->tasks[i].stats, 0, sizeof(task_stats_t));
    }
    
    // 重置CPU负载统计
    sched->total_cycles = 0;
    sched->idle_cycles = 0;
    sched->cpu_load = 0.0f;
    sched->last_load_update = DWT_GetTick();
}

// ============================================================================
// 统计信息打印
// ============================================================================

/**
 * @brief 打印调度器和所有任务的统计信息
 */
void scheduler_print_stats(task_scheduler_fc_t *sched)
{
    if (!sched) return;

    // 打印调度器总览
    printf("\r\n========== 调度器统计 ==========\r\n");
    printf("CPU 负载: %.1f%%\r\n", sched->cpu_load);
    printf("任务数量: %d/%d\r\n\r\n", sched->task_count, sched->capacity);

    // 打印表头
    printf("%-18s %-8s %-10s %-8s %-8s %-8s %-8s\r\n",
           "任务名", "优先级", "触发模式", "执行次数", "当前(us)", "最大(us)", "超时次数");
    printf("------------------------------------------------------------------------------------\r\n");

    // 打印每个任务的统计信息
    const char *prio_str[] = {"CRITICAL", "HIGH", "NORMAL", "LOW", "IDLE"};
    const char *mode_str[] = {"PERIODIC", "EVENT"};
    
    for (uint8_t i = 0; i < sched->task_count; i++) {
        task_entry_t *t = &sched->tasks[i];
        printf("%-18s %-8s %-10s %-8lu %-8lu %-8lu %-8lu\r\n",
               t->name,                              // 任务名
               prio_str[t->priority],                // 优先级
               mode_str[t->trigger_mode],            // 触发模式
               t->stats.exec_count,                  // 执行次数
               t->stats.exec_time_us,                // 最近一次执行时间
               t->stats.exec_time_max_us,            // 最大执行时间
               t->stats.overrun_count);              // 超时次数
    }
    
    printf("=================================\r\n\r\n");
}
