#include "test_pwm_servo.h"
#include "bsp_pwm.h"
#include "bsp_uart.h"
#include "elrs_crsf_port.h"
#include <string.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <ctype.h>

// 舵机典型 50Hz，1~2ms 脉宽；占空比=脉宽/周期。
#define SERVO_PWM_HZ       50U
#define SERVO_PULSE_MIN_MS 1.0f
#define SERVO_PULSE_MID_MS 1.5f
#define SERVO_PULSE_MAX_MS 2.0f
#define SERVO_SWEEP_STEPS  20U

static float pulse_ms_to_duty(float pulse_ms)
{
    const float period_ms = 1000.0f / (float)SERVO_PWM_HZ; // e.g. 20ms
    float duty = pulse_ms / period_ms;
    if (duty < 0.0f) duty = 0.0f;
    if (duty > 1.0f) duty = 1.0f;
    return duty;
}

// 简易 UART1 命令行（可选择通道 CH1..CH4）
//   sel  <1-4>        选择通道
//   us   <1000-2000>  设置所选通道脉宽(微秒)
//   duty <0-100>      设置所选通道占空比(%)
//   list              打印通道映射与当前选择
//   help              打印帮助

static volatile uint8_t cli_buf[64];
static volatile uint8_t cli_len = 0;
static volatile uint8_t cli_ready = 0;
static uint8_t g_sel = 3; // 默认 CH3

static void uart1_cli_byte(uint8_t b)
{
    if (b == '\r' || b == '\n') {
        if (cli_len > 0) { cli_ready = 1; }
        return;
    }
    if (cli_len < sizeof(cli_buf) - 1) {
        cli_buf[cli_len++] = b;
        cli_buf[cli_len] = 0; // zero-terminate
    } else {
        cli_len = 0; // overflow, reset
    }
}

static void servo_cli_process(void)
{
    if (!cli_ready) return;
    cli_ready = 0;

    // copy buffer
    char line[64];
    uint8_t n = cli_len; if (n >= sizeof(line)) n = sizeof(line)-1;
    for (uint8_t i=0;i<n;i++) line[i] = (char)cli_buf[i];
    line[n] = 0; cli_len = 0;

    // tokenize
    char cmd[16];
    char arg1[16];
    char arg2[16];
    int cnt = sscanf(line, "%15s %15s %15s", cmd, arg1, arg2);
    if (cnt <= 0) return;

    // sanitize cmd: to lower, strip trailing non-alnum
    for (size_t i = 0; cmd[i]; ++i) {
        unsigned char c = (unsigned char)cmd[i];
        if (!(isalnum(c) || c == '_' || c == '-')) { cmd[i] = 0; break; }
        cmd[i] = (char)tolower(c);
    }

    if (strcmp(cmd, "help") == 0) {
        printf("[servo_cli] cmds: sel <1-4> | us <1000-2000> | duty <0-100> | list | status\r\n");
        return;
    }

    if (strcmp(cmd, "list") == 0) {
        printf("[servo_cli] map: CH1=PC6 TIM8_CH1, CH2=PC7 TIM8_CH2, CH3=PC8 TIM3_CH3, CH4=PB1 TIM3_CH4; sel=CH%u\r\n", g_sel);
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        bsp_pwm_debug_dump((bsp_pwm_channel_t)(g_sel-1));
        return;
    }

    if (strcmp(cmd, "sel") == 0 && cnt >= 2) {
        int ch = atoi(arg1);
        if (ch >= 1 && ch <= (int)BSP_PWM_CH_MAX) {
            if (bsp_pwm_init_single((bsp_pwm_channel_t)(ch-1), SERVO_PWM_HZ)) {
                g_sel = (uint8_t)ch;
                // 居中
                float duty_mid = pulse_ms_to_duty(SERVO_PULSE_MID_MS);
                bsp_pwm_write((bsp_pwm_channel_t)(g_sel-1), duty_mid);
                printf("[servo_cli] selected CH%u, center=1.5ms\r\n", g_sel);
            } else {
                printf("[servo_cli] select CH%u failed\r\n", ch);
            }
        } else {
            printf("[servo_cli] invalid ch\r\n");
        }
        return;
    }

    if (strcmp(cmd, "us") == 0 && cnt >= 2) {
        int us = atoi(arg1);
        // convert us to duty
        float duty = pulse_ms_to_duty(us / 1000.0f);
        bsp_pwm_write((bsp_pwm_channel_t)(g_sel-1), duty);
        printf("[servo_cli] CH%u = %d us (duty=%.3f)\r\n", g_sel, us, duty);
        return;
    }

    if (strcmp(cmd, "duty") == 0 && cnt >= 2) {
        float pct = (float)atof(arg1);
        float duty = pct / 100.0f;
        bsp_pwm_write((bsp_pwm_channel_t)(g_sel-1), duty);
        printf("[servo_cli] CH%u duty = %.2f%%\r\n", g_sel, pct);
        return;
    }

    printf("[servo_cli] unknown cmd. try 'help'\r\n");
}

void test_pwm_servo_run(void)
{
    printf("[servo_test] init PWM %u Hz (channel selectable)\r\n", SERVO_PWM_HZ);
    if (!bsp_pwm_init_single((bsp_pwm_channel_t)(g_sel-1), SERVO_PWM_HZ)) {
        printf("[servo_test] init failed\r\n");
        return;
    }

    const float duty_mid = pulse_ms_to_duty(SERVO_PULSE_MID_MS);

    // 默认选中通道居中
    bsp_pwm_write((bsp_pwm_channel_t)(g_sel-1), duty_mid);
    printf("[servo_test] CH%u center duty=%.3f (1.5ms)\r\n", g_sel, duty_mid);
    HAL_Delay(1000);

    // 禁用 CRSF 占用 UART，注册 UART1 CLI 处理
    ELRS_CRSF_Disable();
    ELRS_RegisterHostUart1Handler(uart1_cli_byte);

    printf("[servo_test] UART1 CLI ready. cmds: help | list | sel/us/duty ...\r\n");
    printf("[servo_test] mapping: CH1=PC6, CH2=PC7, CH3=PC8, CH4=PB1\r\n");

    // 仅响应串口命令（CH3），不扫动
    while (1) {
        for (uint8_t i=0;i<20;i++) { servo_cli_process(); HAL_Delay(10);} // ~200ms slice
    }
}
