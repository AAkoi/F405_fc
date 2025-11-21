/**
 * @file    test_dma.c
 * @brief   DMA功能测试实现
 */

#include "test_dma.h"
#include "icm42688p.h"
#include "icm42688p_lib.h"
#include <stdio.h>

extern icm42688p_dev_t icm;
extern volatile uint8_t spi1_dma_flag;

/**
 * @brief 测试DMA传输可靠性
 */
void test_dma_reliability(void)
{
    printf("\r\n========================================\r\n");
    printf("     DMA传输可靠性测试\r\n");
    printf("========================================\r\n");
    
#ifdef ICM_USE_DMA
    printf("当前模式: DMA\r\n\r\n");
    
    int error_count = 0;
    int timeout_count = 0;
    int y_zero_count = 0;
    
    for (int i = 0; i < 20; i++) {
        icm42688p_gyro_data_t  gd;
        icm42688p_accel_data_t ad;
        icm42688p_temp_data_t  td;
        
        // 记录DMA标志状态
        uint8_t flag_before = spi1_dma_flag;
        
        if (icm42688p_read_all(&icm, &gd, &ad, &td)) {
            // 检查Y轴是否为0（常见的DMA错误）
            if (ad.y == 0 && gd.y == 0) {
                y_zero_count++;
                printf("#%d: [警告] Y轴全0 - Acc(%d,%d,%d) Gyro(%d,%d,%d)\r\n",
                       i+1, ad.x, ad.y, ad.z, gd.x, gd.y, gd.z);
            } else {
                printf("#%d: OK - Acc(%d,%d,%d) Gyro(%d,%d,%d)\r\n",
                       i+1, ad.x, ad.y, ad.z, gd.x, gd.y, gd.z);
            }
        } else {
            error_count++;
            printf("#%d: [错误] 读取失败\r\n", i+1);
        }
        
        HAL_Delay(50);
    }
    
    printf("\r\n测试结果:\r\n");
    printf("  读取错误: %d/20\r\n", error_count);
    printf("  Y轴全0: %d/20\r\n", y_zero_count);
    
    if (error_count > 0 || y_zero_count > 5) {
        printf("\r\n[建议] DMA模式不稳定，建议使用轮询模式\r\n");
        printf("  修改方法：注释掉CMakeLists.txt中的ICM_USE_DMA\r\n");
    } else {
        printf("\r\n[结果] DMA模式工作正常！\r\n");
    }
    
    // 关键：DMA测试完成后，强制清理SPI状态
    printf("  正在清理SPI状态...\r\n");
    extern SPI_HandleTypeDef hspi1;
    
    // 停止可能残留的DMA
    HAL_SPI_DMAStop(&hspi1);
    
    // 等待BSY清除
    uint32_t timeout = HAL_GetTick() + 100;
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY) && HAL_GetTick() < timeout);
    
    // 强制复位状态
    hspi1.State = HAL_SPI_STATE_READY;
    
    // 清除所有标志
    __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
    if (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE)) {
        volatile uint8_t dummy = *(__IO uint8_t *)&hspi1.Instance->DR;
        (void)dummy;
    }
    
    printf("  SPI状态已清理: State=%d\r\n", hspi1.State);
    
#else
    printf("当前模式: 轮询\r\n");
    printf("轮询模式无需测试，已验证可靠\r\n");
#endif
    
    printf("========================================\r\n\r\n");
}

