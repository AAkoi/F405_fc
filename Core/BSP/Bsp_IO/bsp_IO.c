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
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* 配置ICM42688P中断引脚 PC3 */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  GPIO_InitStruct.Pin = ICM42688P_INT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // 下降沿触发中断
  GPIO_InitStruct.Pull = GPIO_PULLUP;           // 上拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ICM42688P_INT_GPIO_PORT, &GPIO_InitStruct);
  
  /* 配置EXTI中断优先级并使能 */
  HAL_NVIC_SetPriority(ICM42688P_INT_EXTI_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(ICM42688P_INT_EXTI_IRQn);

  /* USER CODE END MX_GPIO_Init_2 */
}

/**
 * @brief EXTI3 中断服务函数（PC3）
 */
void EXTI3_IRQHandler(void)
{
  HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
}

/**
 * @brief GPIO外部中断回调函数
 * @param GPIO_Pin 触发中断的引脚
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == ICM42688P_INT_PIN)
  {
    /* ICM42688P数据就绪中断 */
    // 在这里添加你的中断处理代码
    // 例如：设置标志位，在主循环中读取数据
    // icm42688p_data_ready_flag = 1;
  }
}