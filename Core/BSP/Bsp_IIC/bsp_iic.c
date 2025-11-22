#include "bsp_iic.h"
#include "bsp_pins.h"

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
extern void Error_Handler(void);

// I2C IT完成标志位
volatile uint8_t i2c1_it_flag = 0;

/**
 * @brief I2C1初始化函数
 */
void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;  // 400kHz
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        // 初始化错误处理
        Error_Handler();
    }

    // 使能I2C1中断（事件与错误）
    HAL_NVIC_SetPriority(I2C1_EV_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_EV_IRQn);
    HAL_NVIC_SetPriority(I2C1_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C1_ER_IRQn);
}

/**
 * @brief I2C2初始化函数
 */
void MX_I2C2_Init(void)
{
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 400000;  // 400kHz
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        // 初始化错误处理
        Error_Handler();
    }

    // 使能I2C2中断（事件与错误）
    HAL_NVIC_SetPriority(I2C2_EV_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C2_EV_IRQn);
    HAL_NVIC_SetPriority(I2C2_ER_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(I2C2_ER_IRQn);
}

/* MSP GPIO 配置迁移至 BSP：配置 I2C1 引脚 PB6/PB7 */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C1_CLK_ENABLE();

        GPIO_InitStruct.Pin = BMP280_IIC1_SCL | BMP280_IIC1_SDA;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(BMP280_IIC1_GPIO_PORT, &GPIO_InitStruct);
    }
    else if (hi2c->Instance == I2C2)
    {
        GPIO_InitTypeDef GPIO_InitStruct = {0};

        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C2_CLK_ENABLE();

        GPIO_InitStruct.Pin = HMC5883l_IIC2_SCL | HMC5883l_IIC2_SDA;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(HMC5883l_IIC2_GPIO_PORT, &GPIO_InitStruct);
    }
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        __HAL_RCC_I2C1_CLK_DISABLE();
        HAL_GPIO_DeInit(BMP280_IIC1_GPIO_PORT, BMP280_IIC1_SCL | BMP280_IIC1_SDA);
    }
}

/**
 * @brief 读取单个寄存器
 * @param dev_addr 设备地址（7位地址）
 * @param reg 寄存器地址
 * @return 寄存器值
 */
uint8_t bsp_i2c_read_reg(uint8_t dev_addr, uint8_t reg)
{
    uint8_t value = 0;
    
    // 发送寄存器地址
    HAL_I2C_Master_Transmit(&hi2c1, dev_addr << 1, &reg, 1, HAL_MAX_DELAY);
    
    // 读取数据
    HAL_I2C_Master_Receive(&hi2c1, dev_addr << 1, &value, 1, HAL_MAX_DELAY);
    
    return value;
}

/**
 * @brief 写入单个寄存器
 * @param dev_addr 设备地址（7位地址）
 * @param reg 寄存器地址
 * @param value 要写入的值
 */
void bsp_i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    
    HAL_I2C_Master_Transmit(&hi2c1, dev_addr << 1, data, 2, HAL_MAX_DELAY);
}

/**
 * @brief 连续读取多个寄存器（IT中断方式）
 * @param dev_addr 设备地址（7位地址）
 * @param reg 起始寄存器地址
 * @param buffer 数据缓冲区
 * @param len 读取长度
 */
void bsp_i2c_read_burst(uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    // 清除IT完成标志
    i2c1_it_flag = 0;
    
    // 发送寄存器地址
    HAL_I2C_Master_Transmit(&hi2c1, dev_addr << 1, &reg, 1, HAL_MAX_DELAY);
    
    // 使用IT中断读取数据
    HAL_I2C_Master_Receive_IT(&hi2c1, dev_addr << 1, buffer, len);

    // 等待中断完成（带超时，保证上层同步使用安全）
    uint32_t start = HAL_GetTick();
    const uint32_t timeout_ms = 20; // 读取6字节足够，预留裕量
    while (i2c1_it_flag == 0) {
        if ((HAL_GetTick() - start) > timeout_ms) {
            break; // 超时退出，避免死等
        }
    }
}

/**
 * @brief I2C IT接收完成回调函数
 * @param hi2c I2C句柄
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        // IT接收完成，设置标志位
        i2c1_it_flag = 1;
    }
}

/**
 * @brief I2C 异常回调（设置标志，便于上层判断）
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        // 置为非零但非1的值表示错误
        i2c1_it_flag = 0xFF;
    }
}

/**
 * @brief I2C1 event interrupt handler
 */
void I2C1_EV_IRQHandler(void)
{
    HAL_I2C_EV_IRQHandler(&hi2c1);
}

/**
 * @brief I2C1 error interrupt handler
 */
void I2C1_ER_IRQHandler(void)
{
    HAL_I2C_ER_IRQHandler(&hi2c1);
}

/**
 * @brief 获取I2C状态
 * @return I2C状态
 */
i2c_state_t bsp_i2c_get_state(void)
{
    HAL_I2C_StateTypeDef hal_state = HAL_I2C_GetState(&hi2c1);
    
    switch (hal_state) {
        case HAL_I2C_STATE_READY:
            return i2c1_it_flag == 1 ? I2C_STATE_DONE : I2C_STATE_IDLE;
        case HAL_I2C_STATE_BUSY:
        case HAL_I2C_STATE_BUSY_TX:
        case HAL_I2C_STATE_BUSY_RX:
            return I2C_STATE_BUSY;
        case HAL_I2C_STATE_ERROR:
        case HAL_I2C_STATE_ABORT:
        case HAL_I2C_STATE_TIMEOUT:
            return I2C_STATE_ERROR;
        default:
            return I2C_STATE_IDLE;
    }
}

/**
 * @brief 检查I2C是否忙碌
 * @return true=忙碌, false=空闲
 */
bool bsp_i2c_is_busy(void)
{
    i2c_state_t state = bsp_i2c_get_state();
    return (state == I2C_STATE_BUSY);
}

/**
 * @brief 非阻塞写寄存器（启动传输后立即返回）
 * @param dev_addr 设备地址（7位）
 * @param reg 寄存器地址
 * @param value 要写入的值
 * @return true=启动成功, false=总线忙
 */
bool bsp_i2c_write_reg_start(uint8_t dev_addr, uint8_t reg, uint8_t value)
{
    if (bsp_i2c_is_busy()) {
        return false;
    }
    
    // 清除标志
    i2c1_it_flag = 0;
    
    // 准备数据
    static uint8_t tx_data[2];
    tx_data[0] = reg;
    tx_data[1] = value;
    
    // 启动IT传输
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_IT(&hi2c1, dev_addr << 1, tx_data, 2);
    
    return (status == HAL_OK);
}

/**
 * @brief 非阻塞连续读取（启动传输后立即返回）
 * @param dev_addr 设备地址（7位）
 * @param reg 起始寄存器地址
 * @param buffer 数据缓冲区
 * @param len 读取长度
 * @return true=启动成功, false=总线忙
 */
bool bsp_i2c_read_burst_start(uint8_t dev_addr, uint8_t reg, uint8_t *buffer, uint16_t len)
{
    if (bsp_i2c_is_busy()) {
        return false;
    }
    
    // 清除标志
    i2c1_it_flag = 0;
    
    // 先发送寄存器地址（阻塞，很快）
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, dev_addr << 1, &reg, 1, 100);
    if (status != HAL_OK) {
        return false;
    }
    
    // 启动IT接收
    status = HAL_I2C_Master_Receive_IT(&hi2c1, dev_addr << 1, buffer, len);
    
    return (status == HAL_OK);
}

/**
 * @brief I2C IT发送完成回调
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        i2c1_it_flag = 1;
    }
}
