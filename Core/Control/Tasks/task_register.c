#include "task_register.h"
#include <string.h>

static task_reg_item_t registry[TASK_REGISTER_MAX];
static uint8_t reg_count = 0;

void task_register_clear(void)
{
    memset(registry, 0, sizeof(registry));
    reg_count = 0;
}

static bool push_item(const task_reg_item_t *item)
{
    if (reg_count >= TASK_REGISTER_MAX) {
        return false;
    }
    registry[reg_count++] = *item;
    return true;
}

bool task_register_event_flag(const char *name,
                              task_cb_t callback,
                              void *user_data,
                              task_priority_t priority,
                              volatile bool *event_flag,
                              uint32_t max_exec_us)
{
    if (!name || !callback || !event_flag) return false;
    task_reg_item_t it = {
        .type = TASK_REG_TYPE_EVENT_FLAG,
        .name = name,
        .callback = callback,
        .should_run = NULL,
        .user_data = user_data,
        .priority = priority,
        .event_flag = event_flag,
        .period_us = 0,
        .max_exec_us = max_exec_us,
    };
    return push_item(&it);
}

bool task_register_event_cb(const char *name,
                            task_cb_t callback,
                            task_should_run_cb_t should_run,
                            void *user_data,
                            task_priority_t priority,
                            uint32_t max_exec_us)
{
    if (!name || !callback || !should_run) return false;
    task_reg_item_t it = {
        .type = TASK_REG_TYPE_EVENT_CB,
        .name = name,
        .callback = callback,
        .should_run = should_run,
        .user_data = user_data,
        .priority = priority,
        .event_flag = NULL,
        .period_us = 0,
        .max_exec_us = max_exec_us,
    };
    return push_item(&it);
}

bool task_register_periodic(const char *name,
                            task_cb_t callback,
                            void *user_data,
                            task_priority_t priority,
                            uint32_t period_us,
                            uint32_t max_exec_us)
{
    if (!name || !callback || period_us == 0) return false;
    task_reg_item_t it = {
        .type = TASK_REG_TYPE_PERIODIC,
        .name = name,
        .callback = callback,
        .should_run = NULL,
        .user_data = user_data,
        .priority = priority,
        .event_flag = NULL,
        .period_us = period_us,
        .max_exec_us = max_exec_us,
    };
    return push_item(&it);
}

int task_register_apply(task_scheduler_fc_t *sched)
{
    if (!sched) return 0;
    int ok = 0;
    for (uint8_t i = 0; i < reg_count; i++) {
        task_reg_item_t *it = &registry[i];
        bool res = false;
        switch (it->type) {
            case TASK_REG_TYPE_EVENT_FLAG:
                res = scheduler_register_event_flag(sched, it->name, it->callback, it->user_data,
                                                    it->priority, it->event_flag, it->max_exec_us);
                break;
            case TASK_REG_TYPE_EVENT_CB:
                res = scheduler_register_event_callback(sched, it->name, it->callback, it->should_run,
                                                        it->user_data, it->priority, it->max_exec_us);
                break;
            case TASK_REG_TYPE_PERIODIC:
                res = scheduler_register_periodic(sched, it->name, it->callback, it->user_data,
                                                  it->priority, it->period_us, it->max_exec_us);
                break;
            default:
                break;
        }
        if (res) ok++;
    }
    return ok;
}
