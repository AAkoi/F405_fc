#include "task_scheduler_fc.h"
#include <string.h>
#include <stdio.h>

// 查找任务索引
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

// cycles 转 us
static inline uint32_t cycles_to_us(uint32_t cycles, uint32_t cpu_freq_hz)
{
    return (cycles / (cpu_freq_hz / 1000000));
}

void scheduler_init(task_scheduler_fc_t *sched,
                    task_entry_t *storage,
                    uint8_t capacity,
                    const scheduler_config_t *config)
{
    if (!sched || !storage) return;

    sched->tasks = storage;
    sched->capacity = capacity;
    sched->task_count = 0;

    if (config) {
        sched->config = *config;
    } else {
        sched->config.enable_stats = true;
        sched->config.enable_overrun_check = true;
        sched->config.cpu_freq_hz = SystemCoreClock;
        sched->config.max_tasks = capacity;
    }

    for (uint8_t i = 0; i < capacity; i++) {
        memset(&sched->tasks[i], 0, sizeof(task_entry_t));
        sched->tasks[i].state = TASK_STATE_IDLE;
        sched->tasks[i].active = false;
    }

    sched->total_cycles = 0;
    sched->idle_cycles = 0;
    sched->cpu_load = 0.0f;
    sched->last_load_update = DWT_GetTick();
}

bool scheduler_register_periodic(task_scheduler_fc_t *sched,
                                 const char *name,
                                 task_cb_t callback,
                                 void *user_data,
                                 task_priority_t priority,
                                 uint32_t period_us,
                                 uint32_t max_exec_us)
{
    if (!sched || !name || !callback || period_us == 0) {
        return false;
    }
    if (sched->task_count >= sched->capacity) {
        return false;
    }

    task_entry_t *task = &sched->tasks[sched->task_count++];
    task->name = name;
    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->trigger_mode = TASK_TRIGGER_PERIODIC;
    task->state = TASK_STATE_READY;

    task->period_cycles = clockMicrosToCycles(period_us);
    if (task->period_cycles == 0) {
        task->period_cycles = 1;
    }
    uint32_t now = DWT_GetTick();
    task->next_run_time = now + task->period_cycles;

    task->max_exec_time_us = max_exec_us;
    task->active = true;
    memset(&task->stats, 0, sizeof(task_stats_t));
    return true;
}

bool scheduler_register_event_flag(task_scheduler_fc_t *sched,
                                   const char *name,
                                   task_cb_t callback,
                                   void *user_data,
                                   task_priority_t priority,
                                   volatile bool *event_flag,
                                   uint32_t max_exec_us)
{
    if (!sched || !name || !callback || !event_flag) return false;
    if (sched->task_count >= sched->capacity) return false;

    task_entry_t *task = &sched->tasks[sched->task_count++];
    task->name = name;
    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->trigger_mode = TASK_TRIGGER_EVENT;
    task->state = TASK_STATE_READY;
    task->event_flag = event_flag;
    task->should_run = NULL;
    task->max_exec_time_us = max_exec_us;
    task->active = true;
    memset(&task->stats, 0, sizeof(task_stats_t));
    return true;
}

bool scheduler_register_event_callback(task_scheduler_fc_t *sched,
                                       const char *name,
                                       task_cb_t callback,
                                       task_should_run_cb_t should_run,
                                       void *user_data,
                                       task_priority_t priority,
                                       uint32_t max_exec_us)
{
    if (!sched || !name || !callback || !should_run) return false;
    if (sched->task_count >= sched->capacity) return false;

    task_entry_t *task = &sched->tasks[sched->task_count++];
    task->name = name;
    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->trigger_mode = TASK_TRIGGER_EVENT;
    task->state = TASK_STATE_READY;
    task->event_flag = NULL;
    task->should_run = should_run;
    task->max_exec_time_us = max_exec_us;
    task->active = true;
    memset(&task->stats, 0, sizeof(task_stats_t));
    return true;
}

static void execute_task(task_scheduler_fc_t *sched, task_entry_t *task)
{
    if (!task || !task->callback || !task->active) return;

    uint32_t start_time = DWT_GetTick();
    task->state = TASK_STATE_RUNNING;
    task->callback(task->user_data);
    uint32_t end_time = DWT_GetTick();
    task->state = TASK_STATE_READY;

    uint32_t exec_cycles = end_time - start_time;
    if (sched->config.enable_stats) {
        task->stats.exec_count++;
        task->stats.exec_time_us = cycles_to_us(exec_cycles, sched->config.cpu_freq_hz);
        task->stats.exec_time_total_us += task->stats.exec_time_us;
        if (task->stats.exec_time_us > task->stats.exec_time_max_us) {
            task->stats.exec_time_max_us = task->stats.exec_time_us;
        }
        if (sched->config.enable_overrun_check && task->max_exec_time_us > 0) {
            if (task->stats.exec_time_us > task->max_exec_time_us) {
                task->stats.overrun_count++;
            }
        }
        if (task->trigger_mode == TASK_TRIGGER_PERIODIC) {
            uint32_t period_us = cycles_to_us(task->period_cycles, sched->config.cpu_freq_hz);
            if (task->stats.exec_time_us > period_us) {
                task->stats.overrun_count++;
            }
        }
    }
}

static bool should_task_run(task_entry_t *task, uint32_t now)
{
    if (!task->active || task->state == TASK_STATE_SUSPENDED) {
        return false;
    }
    switch (task->trigger_mode) {
        case TASK_TRIGGER_PERIODIC:
            return ((int32_t)(now - task->next_run_time) >= 0);
        case TASK_TRIGGER_EVENT:
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

void scheduler_run(task_scheduler_fc_t *sched)
{
    if (!sched) return;

    uint32_t now = DWT_GetTick();
    uint32_t loop_start = now;
    bool any_task_ran = false;

    for (task_priority_t prio = TASK_PRIORITY_CRITICAL; prio <= TASK_PRIORITY_IDLE; prio++) {
        for (uint8_t i = 0; i < sched->task_count; i++) {
            task_entry_t *task = &sched->tasks[i];
            if (task->priority != prio) continue;

            now = DWT_GetTick();
            if (should_task_run(task, now)) {
                execute_task(sched, task);
                any_task_ran = true;

                if (task->trigger_mode == TASK_TRIGGER_PERIODIC) {
                    now = DWT_GetTick();
                    task->next_run_time = now + task->period_cycles;
                    if ((int32_t)(now - task->next_run_time) >= 0) {
                        if (sched->config.enable_stats) {
                            task->stats.missed_count++;
                        }
                        task->next_run_time = now + task->period_cycles;
                    }
                }

                if (task->trigger_mode == TASK_TRIGGER_EVENT && task->event_flag) {
                    *task->event_flag = false;
                }
            }
        }
    }

    if (sched->config.enable_stats) {
        uint32_t loop_end = DWT_GetTick();
        uint32_t loop_cycles = loop_end - loop_start;
        sched->total_cycles += loop_cycles;
        if (!any_task_ran) {
            sched->idle_cycles += loop_cycles;
        }
        uint32_t load_update_interval = sched->config.cpu_freq_hz; // 1s
        if ((loop_end - sched->last_load_update) >= load_update_interval) {
            if (sched->total_cycles > 0) {
                sched->cpu_load = 100.0f * (1.0f - (float)sched->idle_cycles / (float)sched->total_cycles);
            }
            sched->total_cycles = 0;
            sched->idle_cycles = 0;
            sched->last_load_update = loop_end;
        }
    }
}

void scheduler_trigger_from_isr(task_scheduler_fc_t *sched, const char *task_name)
{
    if (!sched || !task_name) return;
    int idx = find_task_index(sched, task_name);
    if (idx < 0) return;
    task_entry_t *task = &sched->tasks[idx];
    if (task->active && task->callback) {
        uint32_t start = DWT_GetTick();
        task->callback(task->user_data);
        uint32_t end = DWT_GetTick();
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

bool scheduler_suspend_task(task_scheduler_fc_t *sched, const char *name)
{
    int idx = find_task_index(sched, name);
    if (idx < 0) return false;
    sched->tasks[idx].state = TASK_STATE_SUSPENDED;
    return true;
}

bool scheduler_resume_task(task_scheduler_fc_t *sched, const char *name)
{
    int idx = find_task_index(sched, name);
    if (idx < 0) return false;
    sched->tasks[idx].state = TASK_STATE_READY;
    if (sched->tasks[idx].trigger_mode == TASK_TRIGGER_PERIODIC) {
        uint32_t now = DWT_GetTick();
        sched->tasks[idx].next_run_time = now + sched->tasks[idx].period_cycles;
    }
    return true;
}

const task_stats_t* scheduler_get_stats(task_scheduler_fc_t *sched, const char *name)
{
    int idx = find_task_index(sched, name);
    if (idx < 0) return NULL;
    return &sched->tasks[idx].stats;
}

float scheduler_get_cpu_load(task_scheduler_fc_t *sched)
{
    if (!sched) return 0.0f;
    return sched->cpu_load;
}

void scheduler_reset_stats(task_scheduler_fc_t *sched)
{
    if (!sched) return;
    for (uint8_t i = 0; i < sched->task_count; i++) {
        memset(&sched->tasks[i].stats, 0, sizeof(task_stats_t));
    }
    sched->total_cycles = 0;
    sched->idle_cycles = 0;
    sched->cpu_load = 0.0f;
    sched->last_load_update = DWT_GetTick();
}

void scheduler_print_stats(task_scheduler_fc_t *sched)
{
    if (!sched) return;

    printf("\r\n========== 调度器统计 ==========\r\n");
    printf("CPU 负载: %.1f%%\r\n", sched->cpu_load);
    printf("任务数量: %d/%d\r\n\r\n", sched->task_count, sched->capacity);

    printf("%-18s %-8s %-10s %-8s %-8s %-8s %-8s\r\n",
           "任务名", "优先级", "模式", "执行次数", "当前(us)", "最大(us)", "超时次数");
    printf("------------------------------------------------------------------------------------\r\n");

    const char *prio_str[] = {"CRITICAL", "HIGH", "NORMAL", "LOW", "IDLE"};
    const char *mode_str[] = {"PERIODIC", "EVENT"};
    for (uint8_t i = 0; i < sched->task_count; i++) {
        task_entry_t *t = &sched->tasks[i];
        printf("%-18s %-8s %-10s %-8lu %-8lu %-8lu %-8lu\r\n",
               t->name,
               prio_str[t->priority],
               mode_str[t->trigger_mode],
               t->stats.exec_count,
               t->stats.exec_time_us,
               t->stats.exec_time_max_us,
               t->stats.overrun_count);
    }
    printf("=================================\r\n\r\n");
}
