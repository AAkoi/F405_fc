#include "bsp_spi.h"
#include "stm32f4xx_hal_spi.h"
#include "bsp_pins.h"
#include <stdio.h>


extern void Error_Handler(void);
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;


/**
 * @brief 初始化SPI1 (用于ICM42688P IMU传感器)
 * @note  ICM42688P要求: SPI模式3 (CPOL=1, CPHA=1), MSB先行
 *        最大SPI时钟: 24MHz (推荐10MHz以确保稳定性)
 *        CS片选: 低电平有效，软件控制
 */
void MX_SPI1_Init(void)
{
  /* Enable clocks */
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* SPI1 parameter configuration */
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;       // 双线全双工 (可收发)
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;        // CPOL=1 (ICM42688P要求)
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;             // CPHA=1 (ICM42688P要求)
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8; // 改为8分频(约10MHz)，更稳定
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  
  HAL_StatusTypeDef status = HAL_SPI_Init(&hspi1);
  if (status != HAL_OK)
  {
    printf("[ERROR] HAL_SPI_Init failed with status: %d\r\n", status);
    Error_Handler();
  }
  
  // 手动使能SPI（确保SPI被启用）
  __HAL_SPI_ENABLE(&hspi1);
  
  printf("[SPI1] 初始化完成 - SPE=%d\r\n", (SPI1->CR1 & SPI_CR1_SPE) ? 1 : 0);

  /* --- Only enable RX DMA --- */
  hdma_spi1_rx.Instance = DMA2_Stream0;
  hdma_spi1_rx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_spi1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi1_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi1_rx.Init.Mode = DMA_NORMAL;
  hdma_spi1_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_spi1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma_spi1_rx) != HAL_OK)
  {
    Error_Handler();
  }

  /* Link only RX DMA to SPI1 */
  __HAL_LINKDMA(&hspi1, hdmarx, hdma_spi1_rx);

  /* NVIC configuration for RX DMA interrupt */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

  /* --- TX DMA: 提供SPI主机时钟 (HAL_SPI_Receive_DMA会同时启动TX DMA) --- */
  hdma_spi1_tx.Instance = DMA2_Stream3;
  hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
  hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_spi1_tx.Init.Mode = DMA_NORMAL;
  hdma_spi1_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

  if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
  {
    Error_Handler();
  }

  __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);

  /* NVIC configuration for TX DMA interrupt */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 1, 1);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

/* MSP GPIO 配置迁移至 BSP：配置 SPI1 引脚 PA5/PA6/PA7 */
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{
  if (hspi->Instance == SPI1)
  {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = ICM42688P_SCK_PIN | ICM42688P_MISO_PIN | ICM42688P_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(ICM42688P_SPI1_GPIO_PORT, &GPIO_InitStruct);
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if (hspi->Instance == SPI1)
  {
    HAL_GPIO_DeInit(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN | ICM42688P_MISO_PIN | ICM42688P_MOSI_PIN);
  }
}
