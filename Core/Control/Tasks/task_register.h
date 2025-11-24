/**
 * @file    task_register.h
 * @brief   Task registry to stage registrations before scheduler_init.
 */

#ifndef TASK_REGISTER_H
#define TASK_REGISTER_H

#include <stdint.h>
#include <stdbool.h>
#include "scheduler.h"

#ifndef TASK_REGISTER_MAX
#define TASK_REGISTER_MAX 16
#endif

typedef enum {
    TASK_REG_TYPE_EVENT_FLAG,
    TASK_REG_TYPE_EVENT_CB,
    TASK_REG_TYPE_PERIODIC,
} task_reg_type_t;

typedef struct {
    task_reg_type_t     type;
    const char         *name;
    task_cb_t           callback;
    task_should_run_cb_t should_run;   // for EVENT_CB
    void               *user_data;
    task_priority_t     priority;
    volatile bool      *event_flag;    // for EVENT_FLAG
    uint32_t            period_us;     // for PERIODIC
    uint32_t            max_exec_us;
} task_reg_item_t;

void task_register_clear(void);

bool task_register_event_flag(const char *name,
                              task_cb_t callback,
                              void *user_data,
                              task_priority_t priority,
                              volatile bool *event_flag,
                              uint32_t max_exec_us);

bool task_register_event_cb(const char *name,
                            task_cb_t callback,
                            task_should_run_cb_t should_run,
                            void *user_data,
                            task_priority_t priority,
                            uint32_t max_exec_us);

bool task_register_periodic(const char *name,
                            task_cb_t callback,
                            void *user_data,
                            task_priority_t priority,
                            uint32_t period_us,
                            uint32_t max_exec_us);

// Apply staged tasks to scheduler after scheduler_init.
int task_register_apply(task_scheduler_fc_t *sched);

#endif // TASK_REGISTER_H
