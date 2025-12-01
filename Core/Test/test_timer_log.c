#include "test_timer_log.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdbool.h>

static TIM_HandleTypeDef htim2;

static uint32_t get_tim_clk(TIM_TypeDef *instance)
{
    uint32_t pclk;
    if (instance == TIM1 || instance == TIM8 || instance == TIM9 || instance == TIM10 || instance == TIM11) {
        pclk = HAL_RCC_GetPCLK2Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_CFGR_PPRE2_DIV1) {
            pclk *= 2U;
        }
    } else {
        pclk = HAL_RCC_GetPCLK1Freq();
        if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
            pclk *= 2U;
        }
    }
    return pclk;
}

static bool tim2_init_1mhz(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    const uint32_t tim_clk = get_tim_clk(TIM2);
    uint32_t prescaler = tim_clk / 1000000U; // 1MHz
    if (prescaler == 0U) prescaler = 1U;

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = prescaler - 1U;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 0xFFFFFFFFU;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        return false;
    }
    return (HAL_TIM_Base_Start(&htim2) == HAL_OK);
}

void test_timer_log_run(void)
{
    if (!tim2_init_1mhz()) {
        printf("[timer_log] TIM2 init failed\r\n");
        return;
    }

    printf("[timer_log] TIM2 running at 1MHz ticks\r\n");
    uint32_t last = __HAL_TIM_GET_COUNTER(&htim2);

    while (1) {
        HAL_Delay(500);
        uint32_t now = __HAL_TIM_GET_COUNTER(&htim2);
        uint32_t delta = now - last; // 32-bit wrap handled by unsigned arithmetic
        last = now;
        printf("[timer_log] cnt=%lu delta_us=%lu\r\n", (unsigned long)now, (unsigned long)delta);
    }
}
