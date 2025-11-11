#include "bsp_spi.h"
#include "stm32f4xx_hal_spi.h"


extern void Error_Handler(void);
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;


//开启SPI1的RX—DMA模式
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
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; // ≈20M
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }

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

  /* 不再配置 TX DMA */
}
