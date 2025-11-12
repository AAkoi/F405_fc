#include "bsp_IO.h"
#include "bsp_pins.h"

/* EXTI中断号定义 */
#define ICM42688P_INT_EXTI_IRQn EXTI3_IRQn
void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* 配置 ICM42688P 中断引脚 PC3 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  GPIO_InitStruct.Pin = ICM42688P_INT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // 下降沿触发中断
  GPIO_InitStruct.Pull = GPIO_PULLUP;           // 上拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ICM42688P_INT_GPIO_PORT, &GPIO_InitStruct);
  
  /* 配置EXTI中断优先级并使能 */
  HAL_NVIC_SetPriority(ICM42688P_INT_EXTI_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(ICM42688P_INT_EXTI_IRQn);

  /* 配置 ICM42688P CS 引脚 PC2 为推挽输出，默认拉高不选中 */
  GPIO_InitStruct.Pin = ICM42688P_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ICM42688P_CS_GPIO_PORT, &GPIO_InitStruct);
  ICM42688P_CS_HIGH();

  /* USER CODE END MX_GPIO_Init_2 */
}

/**
 * @brief EXTI3 中断服务函数（PC3）
 */
void EXTI3_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
}
