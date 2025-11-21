/**
 * @file    test_spi_hardware.c
 * @brief   SPI硬件底层诊断实现
 */

#include "test_spi_hardware.h"
#include "bsp_pins.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>

extern SPI_HandleTypeDef hspi1;

/**
 * @brief 读取SPI1关键寄存器
 */
static void print_spi1_registers(void)
{
    printf("\r\n[SPI1 寄存器状态]\r\n");
    printf("  CR1 = 0x%04X\r\n", (unsigned int)SPI1->CR1);
    printf("  CR2 = 0x%04X\r\n", (unsigned int)SPI1->CR2);
    printf("  SR  = 0x%04X\r\n", (unsigned int)SPI1->SR);
    
    // 解析CR1
    printf("\r\n[CR1 配置]\r\n");
    printf("  SPE (使能): %s\r\n", (SPI1->CR1 & SPI_CR1_SPE) ? "是" : "否");
    printf("  CPOL (时钟极性): %s\r\n", (SPI1->CR1 & SPI_CR1_CPOL) ? "高(1)" : "低(0)");
    printf("  CPHA (时钟相位): %s\r\n", (SPI1->CR1 & SPI_CR1_CPHA) ? "第2边沿(1)" : "第1边沿(0)");
    printf("  MSTR (主从): %s\r\n", (SPI1->CR1 & SPI_CR1_MSTR) ? "主机" : "从机");
    printf("  波特率分频: %d\r\n", 2 << (((SPI1->CR1 >> 3) & 0x07)));
    
    // 解析SR
    printf("\r\n[SR 状态]\r\n");
    printf("  TXE (发送空): %s\r\n", (SPI1->SR & SPI_SR_TXE) ? "是" : "否");
    printf("  RXNE (接收非空): %s\r\n", (SPI1->SR & SPI_SR_RXNE) ? "是" : "否");
    printf("  BSY (忙): %s\r\n", (SPI1->SR & SPI_SR_BSY) ? "是" : "否");
}

/**
 * @brief 读取GPIO配置
 */
static void print_gpio_config(GPIO_TypeDef *port, uint16_t pin, const char *name)
{
    // 找到引脚编号
    uint8_t pin_num = 0;
    for (int i = 0; i < 16; i++) {
        if (pin & (1 << i)) {
            pin_num = i;
            break;
        }
    }
    
    uint32_t moder = (port->MODER >> (pin_num * 2)) & 0x03;
    uint32_t otyper = (port->OTYPER >> pin_num) & 0x01;
    uint32_t ospeedr = (port->OSPEEDR >> (pin_num * 2)) & 0x03;
    uint32_t pupdr = (port->PUPDR >> (pin_num * 2)) & 0x03;
    uint32_t afr = (port->AFR[pin_num / 8] >> ((pin_num % 8) * 4)) & 0x0F;
    uint32_t idr = (port->IDR >> pin_num) & 0x01;
    uint32_t odr = (port->ODR >> pin_num) & 0x01;
    
    const char *mode_str[] = {"输入", "输出", "复用", "模拟"};
    const char *otype_str[] = {"推挽", "开漏"};
    const char *speed_str[] = {"低速", "中速", "高速", "超高速"};
    const char *pupd_str[] = {"无", "上拉", "下拉", "保留"};
    
    printf("\r\n[%s 配置]\r\n", name);
    printf("  模式: %s\r\n", mode_str[moder]);
    if (moder == 1) {
        printf("  输出类型: %s\r\n", otype_str[otyper]);
    }
    if (moder == 2) {
        printf("  复用功能: AF%d\r\n", (int)afr);
    }
    printf("  速度: %s\r\n", speed_str[ospeedr]);
    printf("  上拉/下拉: %s\r\n", pupd_str[pupdr]);
    printf("  当前电平: %s (%d)\r\n", idr ? "高(3.3V)" : "低(0V)", (int)idr);
    if (moder == 1) {
        printf("  输出值: %s\r\n", odr ? "高" : "低");
    }
}

/**
 * @brief SPI1硬件诊断
 */
void test_spi1_hardware_diagnosis(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("     SPI1 硬件底层诊断\r\n");
    printf("========================================\r\n");
    
    // 1. 检查时钟
    printf("\r\n[步骤1] 检查时钟使能状态\r\n");
    printf("  SPI1 时钟: %s\r\n", (__HAL_RCC_SPI1_IS_CLK_ENABLED()) ? "已使能 ✓" : "未使能 ✗");
    printf("  GPIOA 时钟: %s\r\n", (__HAL_RCC_GPIOA_IS_CLK_ENABLED()) ? "已使能 ✓" : "未使能 ✗");
    printf("  GPIOC 时钟: %s\r\n", (__HAL_RCC_GPIOC_IS_CLK_ENABLED()) ? "已使能 ✓" : "未使能 ✗");
    printf("  DMA2 时钟: %s\r\n", (__HAL_RCC_DMA2_IS_CLK_ENABLED()) ? "已使能 ✓" : "未使能 ✗");
    
    // 2. SPI寄存器
    printf("\r\n[步骤2] SPI1 寄存器配置\r\n");
    print_spi1_registers();
    
    // 3. GPIO配置
    printf("\r\n[步骤3] GPIO 引脚配置\r\n");
    print_gpio_config(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN, "PA5 (SCK)");
    print_gpio_config(ICM42688P_SPI1_GPIO_PORT, ICM42688P_MISO_PIN, "PA6 (MISO)");
    print_gpio_config(ICM42688P_SPI1_GPIO_PORT, ICM42688P_MOSI_PIN, "PA7 (MOSI)");
    print_gpio_config(ICM42688P_CS_GPIO_PORT, ICM42688P_CS_PIN, "PC2 (CS)");
    
    // 4. 测试CS控制
    printf("\r\n[步骤4] 测试CS片选控制\r\n");
    printf("  设置CS为高电平...\r\n");
    ICM42688P_CS_HIGH();
    HAL_Delay(10);
    printf("  PC2当前电平: %s\r\n", 
           HAL_GPIO_ReadPin(ICM42688P_CS_GPIO_PORT, ICM42688P_CS_PIN) ? "高(3.3V) ✓" : "低(0V) ✗");
    
    printf("  设置CS为低电平...\r\n");
    ICM42688P_CS_LOW();
    HAL_Delay(10);
    printf("  PC2当前电平: %s\r\n", 
           HAL_GPIO_ReadPin(ICM42688P_CS_GPIO_PORT, ICM42688P_CS_PIN) ? "高(3.3V) ✗" : "低(0V) ✓");
    
    ICM42688P_CS_HIGH();
    
    // 5. 测试MISO引脚
    printf("\r\n[步骤5] 测试MISO引脚状态\r\n");
    printf("  PA6 (MISO) 当前电平: %s\r\n",
           HAL_GPIO_ReadPin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_MISO_PIN) ? "高" : "低");
    printf("  注意: MISO应该连接到传感器的SDO引脚\r\n");
    
    // 6. 简单回环测试
    printf("\r\n[步骤6] SPI回环测试（需要短接MOSI-MISO）\r\n");
    printf("  提示: 如果短接PA7和PA6，应该能读回发送的数据\r\n");
    printf("  发送: 0x55, 接收: ");
    
    ICM42688P_CS_LOW();
    uint8_t tx = 0x55;
    uint8_t rx = 0x00;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 100);
    ICM42688P_CS_HIGH();
    
    printf("0x%02X %s\r\n", rx, (rx == 0x55) ? "✓ (回环成功)" : (rx == 0xFF) ? "✗ (读取全1)" : "✗");
    
    printf("\r\n========================================\r\n");
    printf("诊断完成\r\n");
    printf("========================================\r\n\r\n");
}

/**
 * @brief 测试ICM42688P的GPIO引脚
 */
void test_icm42688p_gpio_pins(void)
{
    printf("\r\n========================================\r\n");
    printf("     ICM42688P GPIO 引脚测试\r\n");
    printf("========================================\r\n");
    
    printf("\r\n[测试1] CS引脚切换测试（观察示波器/逻辑分析仪）\r\n");
    for (int i = 0; i < 5; i++) {
        printf("  切换#%d: 高->低->高\r\n", i+1);
        ICM42688P_CS_HIGH();
        HAL_Delay(100);
        ICM42688P_CS_LOW();
        HAL_Delay(100);
        ICM42688P_CS_HIGH();
        HAL_Delay(100);
    }
    
    printf("\r\n[测试2] 中断引脚状态\r\n");
    print_gpio_config(ICM42688P_INT_GPIO_PORT, ICM42688P_INT_PIN, "PC3 (INT)");
    
    printf("\r\n========================================\r\n\r\n");
}

/**
 * @brief 位操作方式测试SPI（绕过HAL）
 */
void test_spi1_bitbang(void)
{
    printf("\r\n========================================\r\n");
    printf("     SPI 位操作测试\r\n");
    printf("========================================\r\n");
    
    printf("\r\n使用GPIO位操作模拟SPI读取WHO_AM_I...\r\n");
    
    // 临时切换引脚为GPIO模式
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // SCK和MOSI为输出
    GPIO_InitStruct.Pin = ICM42688P_SCK_PIN | ICM42688P_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ICM42688P_SPI1_GPIO_PORT, &GPIO_InitStruct);
    
    // MISO为输入
    GPIO_InitStruct.Pin = ICM42688P_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ICM42688P_SPI1_GPIO_PORT, &GPIO_InitStruct);
    
    // 设置初始状态（模式3: CPOL=1）
    HAL_GPIO_WritePin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN, GPIO_PIN_SET);  // 时钟空闲为高
    ICM42688P_CS_HIGH();
    HAL_Delay(1);
    
    // 开始传输
    ICM42688P_CS_LOW();
    HAL_Delay(1);
    
    // 发送读命令: 0xF5 (0x75 | 0x80)
    uint8_t cmd = 0xF5;
    for (int i = 7; i >= 0; i--) {
        // 设置MOSI
        HAL_GPIO_WritePin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_MOSI_PIN, 
                          (cmd & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        
        // 下降沿（模式3在第2边沿采样）
        HAL_GPIO_WritePin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);
        
        // 上升沿
        HAL_GPIO_WritePin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    
    // 读取数据
    uint8_t data = 0;
    for (int i = 7; i >= 0; i--) {
        // 下降沿
        HAL_GPIO_WritePin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);
        
        // 上升沿并采样
        HAL_GPIO_WritePin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_SCK_PIN, GPIO_PIN_SET);
        if (HAL_GPIO_ReadPin(ICM42688P_SPI1_GPIO_PORT, ICM42688P_MISO_PIN)) {
            data |= (1 << i);
        }
        HAL_Delay(1);
    }
    
    ICM42688P_CS_HIGH();
    
    printf("  位操作读取结果: WHO_AM_I = 0x%02X\r\n", data);
    
    if (data == 0xFF || data == 0x00) {
        printf("  ✗ 读取失败！可能原因:\r\n");
        printf("    - MISO线未连接或接错\r\n");
        printf("    - 传感器未上电\r\n");
        printf("    - 传感器VDD/VDDIO电压错误\r\n");
    } else if (data == 0x47) {
        printf("  ✓ ICM42688P检测成功！\r\n");
        printf("  说明HAL库SPI配置可能有问题\r\n");
    } else {
        printf("  ? 读取到数据但不是0x47\r\n");
        printf("  可能是时序或模式问题\r\n");
    }
    
    // 恢复SPI复用功能
    GPIO_InitStruct.Pin = ICM42688P_SCK_PIN | ICM42688P_MISO_PIN | ICM42688P_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(ICM42688P_SPI1_GPIO_PORT, &GPIO_InitStruct);
    
    printf("\r\n========================================\r\n\r\n");
}

