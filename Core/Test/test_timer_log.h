/**
 * @file    test_timer_log.h
 * @brief   使用 TIM2 打印计数值，便于验证定时器与系统时钟
 */

#ifndef TEST_TIMER_LOG_H
#define TEST_TIMER_LOG_H

/**
 * @brief 初始化 TIM2 为 1MHz 计数，周期性打印计数差值（通过串口 printf）
 */
void test_timer_log_run(void);

#endif // TEST_TIMER_LOG_H
