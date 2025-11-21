/**
 * @file    test_dma.h
 * @brief   DMA功能测试
 */

#ifndef TEST_DMA_H
#define TEST_DMA_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 测试DMA传输是否正常工作
 * @note  读取10次传感器数据，检查是否有错误
 */
void test_dma_reliability(void);

#endif // TEST_DMA_H

