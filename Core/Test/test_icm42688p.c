/**
 * @file    test_icm42688p.c
 * @brief   精简版 ICM42688P 测试（写轮询，读DMA）
 */

#include "test_icm42688p.h"
#include "icm42688p.h"
#include "icm42688p_lib.h"
#include "attitude.h"
#include <stdio.h>
#include <stdlib.h>

extern icm42688p_dev_t icm;

/* 简单完整测试：直接进入姿态输出循环 */
void test_icm42688p_full(void)
{
    printf("\r\nICM42688P 测试（DMA读/轮询写）\r\n");
    test_icm42688p_euler_angles();  // 不返回
}

/* 姿态输出循环 */
void test_icm42688p_euler_angles(void)
{
    icm42688p_gyro_data_t  gd;
    icm42688p_accel_data_t ad;
    icm42688p_temp_data_t  td;

    while (1)
    {
        if (icm42688p_read_all(&icm, &gd, &ad, &td)) {
            Euler_angles e = Attitude_Update(ad.x, ad.y, ad.z, gd.x, gd.y, gd.z);
            // 避免浮点printf未开启：用整数*10输出一位小数
            int r10 = (int)(e.roll * 10.0f);
            int p10 = (int)(e.pitch * 10.0f);
            int y10 = (int)(e.yaw * 10.0f);
            printf("Euler(deg): roll=%d.%d pitch=%d.%d yaw=%d.%d\r\n",
                   r10 / 10, abs(r10 % 10),
                   p10 / 10, abs(p10 % 10),
                   y10 / 10, abs(y10 % 10));
        }
        HAL_Delay(50); // 20 Hz
    }
}

/* 原始数据输出（保留简单版本） */
void test_icm42688p_raw_data(void)
{
    printf("\r\nICM42688P 原始数据\r\n");
    while (1)
    {
        icm42688p_gyro_data_t  gd;
        icm42688p_accel_data_t ad;
        icm42688p_temp_data_t  td;

        if (icm42688p_read_all(&icm, &gd, &ad, &td)) {
            printf("ACC:%6d %6d %6d | GYR:%6d %6d %6d | T:%d\r\n",
                   ad.x, ad.y, ad.z, gd.x, gd.y, gd.z, (int)td.celsius);
        }
        HAL_Delay(100);
    }
}
