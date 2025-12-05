#include "bsp_pwm.h"
#include "bsp_pins.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

#define PWM_TICK_HZ 1000000U // 1MHz 计数分辨率

typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t channel;
} pwm_map_t;

static TIM_HandleTypeDef htim3;
static TIM_HandleTypeDef htim8;
static pwm_map_t pwm_map[BSP_PWM_CH_MAX] = {
    { NULL, TIM_CHANNEL_1 }, // PC6  TIM8_CH1
    { NULL, TIM_CHANNEL_2 }, // PC7  TIM8_CH2
    { NULL, TIM_CHANNEL_3 }, // PC8  TIM3_CH3
    { NULL, TIM_CHANNEL_4 }, // PB1  TIM3_CH4
};

static bool pwm_ready = false;

static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

static uint32_t timer_get_clock(TIM_TypeDef *instance)
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

static void pwm_gpio_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    // TIM3_CH3 - PC8, AF2
    GPIO_InitStruct.Pin = timer3_ch3;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(timer_port, &GPIO_InitStruct);

    // TIM3_CH4 - PB1, AF2
    GPIO_InitStruct.Pin = timer3_ch4;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
    HAL_GPIO_Init(timer_port1, &GPIO_InitStruct);

    // TIM8_CH1/CH2 - PC6/PC7, AF3
    GPIO_InitStruct.Pin = timer8_ch1 | timer8_ch2;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(timer_port, &GPIO_InitStruct);
}

static bool pwm_setup_timer(TIM_HandleTypeDef *htim, TIM_TypeDef *instance, uint32_t pwm_hz)
{
    uint32_t tim_clk = timer_get_clock(instance);
    if (tim_clk == 0U || pwm_hz == 0U) {
        return false;
    }

    htim->Instance = instance;
    uint32_t prescaler = tim_clk / PWM_TICK_HZ;
    if (prescaler == 0U) prescaler = 1U;

    uint32_t period = PWM_TICK_HZ / pwm_hz;
    if (period < 2U) period = 2U;

    htim->Init.Prescaler = prescaler - 1U;
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = period - 1U;
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter = 0;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    return (HAL_TIM_PWM_Init(htim) == HAL_OK);
}

static bool pwm_config_and_start(TIM_HandleTypeDef *htim, uint32_t channel)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, channel) != HAL_OK) {
        return false;
    }
    if (HAL_TIM_PWM_Start(htim, channel) != HAL_OK) {
        return false;
    }
    return true;
}

bool bsp_pwm_init(uint32_t pwm_hz)
{
    if (pwm_hz == 0U) {
        pwm_hz = BSP_PWM_DEFAULT_HZ;
    }

    pwm_gpio_init();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM8_CLK_ENABLE();

    bool ok3 = pwm_setup_timer(&htim3, TIM3, pwm_hz);
    if (ok3) {
        ok3 &= pwm_config_and_start(&htim3, TIM_CHANNEL_3);
        ok3 &= pwm_config_and_start(&htim3, TIM_CHANNEL_4);
    }

    bool ok8 = pwm_setup_timer(&htim8, TIM8, pwm_hz);
    if (ok8) {
        ok8 &= pwm_config_and_start(&htim8, TIM_CHANNEL_1);
        ok8 &= pwm_config_and_start(&htim8, TIM_CHANNEL_2);
    }

    pwm_map[BSP_PWM_CH1].htim = &htim8; // PC6 TIM8_CH1
    pwm_map[BSP_PWM_CH2].htim = &htim8; // PC7 TIM8_CH2
    pwm_map[BSP_PWM_CH3].htim = &htim3; // PC8 TIM3_CH3
    pwm_map[BSP_PWM_CH4].htim = &htim3; // PB1 TIM3_CH4

    pwm_ready = ok3 && ok8;
    printf("[bsp_pwm] init %lu Hz: %s\r\n", (unsigned long)pwm_hz,
           pwm_ready ? "OK" : "FAIL");
    return pwm_ready;
}

bool bsp_pwm_init_single(bsp_pwm_channel_t ch, uint32_t pwm_hz)
{
    if (pwm_hz == 0U) {
        pwm_hz = BSP_PWM_DEFAULT_HZ;
    }

    pwm_gpio_init();

    bool ok = false;
    if (ch == BSP_PWM_CH3 || ch == BSP_PWM_CH4) {
        __HAL_RCC_TIM3_CLK_ENABLE();
        if (pwm_setup_timer(&htim3, TIM3, pwm_hz)) {
            uint32_t channel = (ch == BSP_PWM_CH3) ? TIM_CHANNEL_3 : TIM_CHANNEL_4;
            ok = pwm_config_and_start(&htim3, channel);
            pwm_map[ch].htim = &htim3;
        }
    } else if (ch == BSP_PWM_CH1 || ch == BSP_PWM_CH2) {
        __HAL_RCC_TIM8_CLK_ENABLE();
        if (pwm_setup_timer(&htim8, TIM8, pwm_hz)) {
            uint32_t channel = (ch == BSP_PWM_CH1) ? TIM_CHANNEL_1 : TIM_CHANNEL_2;
            ok = pwm_config_and_start(&htim8, channel);
            pwm_map[ch].htim = &htim8;
        }
    }

    pwm_ready = ok;
    printf("[bsp_pwm] single init ch%u %lu Hz: %s\r\n", (unsigned)(ch+1), (unsigned long)pwm_hz, ok?"OK":"FAIL");
    return ok;
}
void bsp_pwm_write(bsp_pwm_channel_t ch, float duty)
{
    if (!pwm_ready || ch >= BSP_PWM_CH_MAX) {
        return;
    }
    TIM_HandleTypeDef *htim = pwm_map[ch].htim;
    if (!htim) {
        return;
    }
    float v = clampf(duty, 0.0f, 1.0f);
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
    uint32_t pulse = (uint32_t)((float)(arr + 1U) * v + 0.5f);
    if (pulse > arr) pulse = arr;
    __HAL_TIM_SET_COMPARE(htim, pwm_map[ch].channel, pulse);
}

void bsp_pwm_write_all(const float duty[BSP_PWM_CH_MAX])
{
    if (!pwm_ready || !duty) {
        return;
    }
    for (uint8_t i = 0; i < BSP_PWM_CH_MAX; i++) {
        bsp_pwm_write((bsp_pwm_channel_t)i, duty[i]);
    }
}

bool bsp_pwm_ready(void)
{
    return pwm_ready;
}

void bsp_pwm_stop_all(void)
{
    float zero[BSP_PWM_CH_MAX] = {0};
    bsp_pwm_write_all(zero);
}

void bsp_pwm_debug_dump(bsp_pwm_channel_t ch)
{
    if (ch >= BSP_PWM_CH_MAX) return;
    TIM_HandleTypeDef *htim = pwm_map[ch].htim;
    if (!htim) {
        printf("[bsp_pwm] ch%u not inited\r\n", (unsigned)(ch+1));
        return;
    }
    uint32_t psc = htim->Instance->PSC;
    uint32_t arr = htim->Instance->ARR;
    uint32_t ccr = 0;
    switch (pwm_map[ch].channel) {
        case TIM_CHANNEL_1: ccr = htim->Instance->CCR1; break;
        case TIM_CHANNEL_2: ccr = htim->Instance->CCR2; break;
        case TIM_CHANNEL_3: ccr = htim->Instance->CCR3; break;
        case TIM_CHANNEL_4: ccr = htim->Instance->CCR4; break;
        default: break;
    }
    unsigned tim_no = (htim->Instance==TIM1)?1:(htim->Instance==TIM2)?2:(htim->Instance==TIM3)?3:(htim->Instance==TIM4)?4:(htim->Instance==TIM5)?5:(htim->Instance==TIM8)?8:0;
    unsigned ch_no  = (pwm_map[ch].channel==TIM_CHANNEL_1)?1:(pwm_map[ch].channel==TIM_CHANNEL_2)?2:(pwm_map[ch].channel==TIM_CHANNEL_3)?3:4;
    printf("[bsp_pwm] ch%u TIM%u CH%u PSC=%u ARR=%u CCR=%u duty=%.3f\r\n",
           (unsigned)(ch+1), tim_no, ch_no,
           (unsigned)psc, (unsigned)arr, (unsigned)ccr,
           (arr? ((float)ccr/(float)(arr+1)) : 0.0f));
}
