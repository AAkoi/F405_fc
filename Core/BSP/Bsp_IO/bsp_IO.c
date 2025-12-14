#include "bsp_IO.h"
#include "bsp_pins.h"

static IRQn_Type exti_irqn_from_pin(uint16_t pin)
{
  switch (pin) {
    case GPIO_PIN_0:  return EXTI0_IRQn;
    case GPIO_PIN_1:  return EXTI1_IRQn;
    case GPIO_PIN_2:  return EXTI2_IRQn;
    case GPIO_PIN_3:  return EXTI3_IRQn;
    case GPIO_PIN_4:  return EXTI4_IRQn;
    case GPIO_PIN_5:
    case GPIO_PIN_6:
    case GPIO_PIN_7:
    case GPIO_PIN_8:
    case GPIO_PIN_9:
      return EXTI9_5_IRQn;
    default:
      return EXTI15_10_IRQn;
  }
}

static void exti_enable_for_pin(uint16_t pin, uint32_t preempt_prio, uint32_t sub_prio)
{
  IRQn_Type irqn = exti_irqn_from_pin(pin);
  HAL_NVIC_SetPriority(irqn, preempt_prio, sub_prio);
  HAL_NVIC_EnableIRQ(irqn);
}

void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* 配置 ICM42688P 中断引脚（由 bsp_pins.h 指定） */
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  GPIO_InitStruct.Pin = ICM42688P_INT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;  // 下降沿触发中断
  GPIO_InitStruct.Pull = GPIO_PULLUP;           // 上拉
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ICM42688P_INT_GPIO_PORT, &GPIO_InitStruct);
  
  /* 配置EXTI中断优先级并使能 */
  exti_enable_for_pin(ICM42688P_INT_PIN, 1, 0);

  /* 配置 HMC5883L DRDY 中断引脚（由 bsp_pins.h 指定） */
  GPIO_InitStruct.Pin = HMC5883l_INT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING; // DRDY 为高电平有效
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;      // 线缆断开时下拉，避免浮空误中断
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(HMC5883l_INT_GPIO_PORT, &GPIO_InitStruct);

  exti_enable_for_pin(HMC5883l_INT_PIN, 2, 0);

  /* 配置 ICM42688P CS 引脚为推挽输出，默认拉高不选中 */
  GPIO_InitStruct.Pin = ICM42688P_CS_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ICM42688P_CS_GPIO_PORT, &GPIO_InitStruct);
  ICM42688P_CS_HIGH();

  /* USER CODE END MX_GPIO_Init_2 */
}

/**
 * @brief EXTI0 中断服务函数
 */
void EXTI0_IRQHandler(void)
{
  if (ICM42688P_INT_PIN == GPIO_PIN_0) {
    HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
  }
  if (HMC5883l_INT_PIN == GPIO_PIN_0) {
    HAL_GPIO_EXTI_IRQHandler(HMC5883l_INT_PIN);
  }
}

/**
 * @brief EXTI3 中断服务函数
 */
void EXTI3_IRQHandler(void)
{
  if (ICM42688P_INT_PIN == GPIO_PIN_3) {
    HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
  }
  if (HMC5883l_INT_PIN == GPIO_PIN_3) {
    HAL_GPIO_EXTI_IRQHandler(HMC5883l_INT_PIN);
  }
}

/**
 * @brief EXTI4 中断服务函数
 */
void EXTI4_IRQHandler(void)
{
  if (ICM42688P_INT_PIN == GPIO_PIN_4) {
    HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
  }
  if (HMC5883l_INT_PIN == GPIO_PIN_4) {
    HAL_GPIO_EXTI_IRQHandler(HMC5883l_INT_PIN);
  }
}

/**
 * @brief EXTI9_5 中断服务函数
 */
void EXTI9_5_IRQHandler(void)
{
  if (ICM42688P_INT_PIN & (GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9)) {
    HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
  }
  if (HMC5883l_INT_PIN & (GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9)) {
    HAL_GPIO_EXTI_IRQHandler(HMC5883l_INT_PIN);
  }
}

/**
 * @brief EXTI15_10 中断服务函数
 */
void EXTI15_10_IRQHandler(void)
{
  if (ICM42688P_INT_PIN & (GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15)) {
    HAL_GPIO_EXTI_IRQHandler(ICM42688P_INT_PIN);
  }
  if (HMC5883l_INT_PIN & (GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15)) {
    HAL_GPIO_EXTI_IRQHandler(HMC5883l_INT_PIN);
  }
}
