# ICM42688P 6-Axis IMU Library

基于 Betaflight 实现的 ICM42688P 陀螺仪/加速度计驱动库

## 目录

- [简介](#简介)
- [硬件特性](#硬件特性)
- [文件说明](#文件说明)
- [快速开始](#快速开始)
- [配置说明](#配置说明)
- [使用方法](#使用方法)
- [寄存器说明](#寄存器说明)
- [Betaflight 中的使用](#betaflight-中的使用)

---

## 简介

ICM42688P 是 TDK InvenSense 推出的高性能 6 轴 MEMS 运动追踪设备，集成了：
- 3 轴陀螺仪（±2000 dps 量程）
- 3 轴加速度计（±16g 量程）
- 温度传感器
- SPI 接口（最高 24 MHz）

本库提供了完整的驱动实现，支持：
- ✅ 设备检测和初始化
- ✅ 可配置的输出数据率（ODR）：1 kHz - 8 kHz
- ✅ 可编程抗混叠滤波器（AAF）
- ✅ 中断驱动的数据读取
- ✅ 陀螺仪和加速度计校准
- ✅ 外部时钟输入（CLKIN）
- ✅ 低延迟 UI 滤波器

---

## 硬件特性

### 陀螺仪
- **量程**: ±250, ±500, ±1000, ±2000 dps（默认 ±2000）
- **灵敏度**: 16.4 LSB/dps (±2000 dps)
- **ODR**: 最高 8 kHz
- **AAF 截止频率**: 258 Hz, 536 Hz, 997 Hz, 1962 Hz

### 加速度计
- **量程**: ±2g, ±4g, ±8g, ±16g（默认 ±16g）
- **灵敏度**: 2048 LSB/g (±16g)
- **ODR**: 最高 8 kHz
- **AAF 截止频率**: 258 Hz, 536 Hz, 997 Hz, 1962 Hz

### 通信接口
- **SPI**: 最高 24 MHz
- **中断引脚**: INT1（可配置为推挽或开漏）
- **外部时钟**: 支持 32 kHz CLKIN

---

## 文件说明

```
icm42688p_lib.h         - 库头文件（寄存器定义、数据结构、函数声明）
icm42688p_lib.c         - 库实现文件
icm42688p_example.c     - 使用示例代码
ICM42688P_README.md     - 本文档
```

---

## 快速开始

### 1. 包含头文件

```c
#include "icm42688p_lib.h"
```

### 2. 实现 SPI 通信函数

你需要实现以下 4 个函数：

```c
uint8_t my_spi_read_reg(uint8_t reg);
void my_spi_write_reg(uint8_t reg, uint8_t value);
void my_spi_read_burst(uint8_t reg, uint8_t *buffer, uint16_t len);
void my_delay_ms(uint32_t ms);
```

### 3. 初始化设备

```c
icm42688p_dev_t imu;

// 清零结构体
memset(&imu, 0, sizeof(imu));

// 设置 SPI 函数指针
imu.spi_read_reg = my_spi_read_reg;
imu.spi_write_reg = my_spi_write_reg;
imu.spi_read_burst = my_spi_read_burst;
imu.delay_ms = my_delay_ms;

// 配置参数
imu.config.gyro_fsr = 0;                        // ±2000 dps
imu.config.accel_fsr = 0;                       // ±16g
imu.config.gyro_odr = ICM42688P_ODR_1KHZ;      // 1 kHz
imu.config.accel_odr = ICM42688P_ODR_1KHZ;     // 1 kHz
imu.config.gyro_aaf = ICM42688P_AAF_258HZ;     // 258 Hz AAF
imu.config.accel_aaf = ICM42688P_AAF_258HZ;    // 258 Hz AAF
imu.config.enable_gyro = true;
imu.config.enable_accel = true;
imu.config.enable_temp = true;
imu.config.use_ext_clk = false;

// 初始化
if (!icm42688p_init(&imu)) {
    printf("初始化失败!\n");
    return;
}
```

### 4. 读取数据

```c
icm42688p_gyro_data_t gyro;
icm42688p_accel_data_t accel;
icm42688p_temp_data_t temp;

// 读取所有传感器数据
if (icm42688p_read_all(&imu, &gyro, &accel, &temp)) {
    printf("陀螺仪: X=%d Y=%d Z=%d\n", gyro.x, gyro.y, gyro.z);
    printf("加速度计: X=%d Y=%d Z=%d\n", accel.x, accel.y, accel.z);
    printf("温度: %.2f°C\n", temp.celsius);
}
```

---

## 配置说明

### 陀螺仪量程配置

| 配置值 | 量程 | 灵敏度 (LSB/dps) |
|--------|------|------------------|
| 0 | ±2000 dps | 16.4 |
| 1 | ±1000 dps | 32.8 |
| 2 | ±500 dps | 65.5 |
| 3 | ±250 dps | 131.0 |

### 加速度计量程配置

| 配置值 | 量程 | 灵敏度 (LSB/g) |
|--------|------|----------------|
| 0 | ±16g | 2048 |
| 1 | ±8g | 4096 |
| 2 | ±4g | 8192 |
| 3 | ±2g | 16384 |

### 输出数据率（ODR）

| 宏定义 | 频率 |
|--------|------|
| `ICM42688P_ODR_8KHZ` | 8 kHz |
| `ICM42688P_ODR_4KHZ` | 4 kHz |
| `ICM42688P_ODR_2KHZ` | 2 kHz |
| `ICM42688P_ODR_1KHZ` | 1 kHz |
| `ICM42688P_ODR_500HZ` | 500 Hz |
| `ICM42688P_ODR_200HZ` | 200 Hz |
| `ICM42688P_ODR_100HZ` | 100 Hz |

### 抗混叠滤波器（AAF）

| 配置 | 截止频率 | 适用场景 |
|------|----------|----------|
| `ICM42688P_AAF_258HZ` | 258 Hz | 低噪声，1 kHz ODR |
| `ICM42688P_AAF_536HZ` | 536 Hz | 平衡，2 kHz ODR |
| `ICM42688P_AAF_997HZ` | 997 Hz | 宽带宽，4 kHz ODR |
| `ICM42688P_AAF_1962HZ` | 1962 Hz | 最大带宽，8 kHz ODR |

---

## 使用方法

### 基本读取

```c
icm42688p_gyro_data_t gyro;

if (icm42688p_read_gyro(&imu, &gyro)) {
    // 转换为物理单位（dps）
    float gyro_x_dps = gyro.x / imu.gyro_scale;
    float gyro_y_dps = gyro.y / imu.gyro_scale;
    float gyro_z_dps = gyro.z / imu.gyro_scale;
}
```

### 高速模式（8 kHz）

```c
imu.config.gyro_odr = ICM42688P_ODR_8KHZ;
imu.config.accel_odr = ICM42688P_ODR_8KHZ;
imu.config.gyro_aaf = ICM42688P_AAF_1962HZ;  // 宽带宽滤波器
imu.config.accel_aaf = ICM42688P_AAF_997HZ;

icm42688p_init(&imu);
```

### 中断驱动读取

```c
// 配置中断引脚
icm42688p_config_interrupt(&imu, 
                           ICM42688P_INT1_MODE_PULSED,
                           ICM42688P_INT1_POLARITY_HIGH,
                           ICM42688P_INT1_DRIVE_PP);

// 使能数据就绪中断
icm42688p_enable_data_ready_interrupt(&imu, true);

// 在中断服务程序中读取数据
void EXTI_IRQHandler(void) {
    icm42688p_gyro_data_t gyro;
    icm42688p_read_gyro(&imu, &gyro);
    // 处理数据...
}
```

### 校准

```c
// 陀螺仪校准（设备静止）
printf("保持设备静止...\n");
if (icm42688p_calibrate_gyro(&imu, 1000)) {
    printf("陀螺仪校准完成!\n");
}

// 加速度计校准（设备水平放置，Z 轴向上）
printf("将设备水平放置...\n");
delay_ms(2000);
if (icm42688p_calibrate_accel(&imu, 1000)) {
    printf("加速度计校准完成!\n");
}
```

---

## 寄存器说明

### 关键寄存器（User Bank 0）

| 地址 | 名称 | 功能 |
|------|------|------|
| 0x11 | DEVICE_CONFIG | 设备配置（软复位） |
| 0x14 | INT_CONFIG | 中断引脚配置 |
| 0x1F-0x2A | ACCEL/GYRO_DATA | 传感器数据寄存器 |
| 0x4D | INTF_CONFIG1 | 接口配置（AFSR、CLKIN） |
| 0x4E | PWR_MGMT0 | 电源管理 |
| 0x4F | GYRO_CONFIG0 | 陀螺仪配置（FSR、ODR） |
| 0x50 | ACCEL_CONFIG0 | 加速度计配置（FSR、ODR） |
| 0x52 | GYRO_ACCEL_CONFIG0 | UI 滤波器配置 |
| 0x63 | INT_CONFIG0 | 中断清除配置 |
| 0x64 | INT_CONFIG1 | 中断时序配置 |
| 0x65 | INT_SOURCE0 | 中断源使能 |
| 0x75 | WHO_AM_I | 设备 ID（0x47） |
| 0x76 | BANK_SEL | 寄存器组选择 |

### User Bank 1（AAF 配置）

| 地址 | 名称 | 功能 |
|------|------|------|
| 0x0C | GYRO_CONFIG_STATIC3 | 陀螺仪 AAF DELT |
| 0x0D | GYRO_CONFIG_STATIC4 | 陀螺仪 AAF DELTSQR[7:0] |
| 0x0E | GYRO_CONFIG_STATIC5 | 陀螺仪 AAF DELTSQR[15:8] & BITSHIFT |
| 0x7B | INTF_CONFIG5 | PIN9 功能配置（CLKIN） |

### User Bank 2（AAF 配置）

| 地址 | 名称 | 功能 |
|------|------|------|
| 0x03 | ACCEL_CONFIG_STATIC2 | 加速度计 AAF DELT |
| 0x04 | ACCEL_CONFIG_STATIC3 | 加速度计 AAF DELTSQR[7:0] |
| 0x05 | ACCEL_CONFIG_STATIC4 | 加速度计 AAF DELTSQR[15:8] & BITSHIFT |

---

## Betaflight 中的使用

### 初始化流程

在 Betaflight 中，ICM42688P 的初始化流程如下：

```c
// 1. 检测设备 (gyro_init.c)
uint8_t icm426xxSpiDetect(const extDevice_t *dev)
{
    delay(1);
    icm426xxSoftReset(dev);
    spiWriteReg(dev, ICM426XX_RA_PWR_MGMT0, 0x00);
    
    // 读取 WHO_AM_I
    uint8_t whoAmI = spiReadRegMsk(dev, MPU_RA_WHO_AM_I);
    if (whoAmI == ICM42688P_WHO_AM_I_CONST) {
        return ICM_42688P_SPI;
    }
    return MPU_NONE;
}

// 2. 初始化陀螺仪 (accgyro_spi_icm426xx.c)
void icm426xxGyroInit(gyroDev_t *gyro)
{
    // 设置 SPI 时钟
    spiSetClkDivisor(dev, spiCalculateDivider(24000000));
    
    // 关闭陀螺仪和加速度计
    turnGyroAccOff(dev);
    
    // 配置陀螺仪 AAF
    setUserBank(dev, ICM426XX_BANK_SELECT1);
    aafConfig_t aafConfig = getGyroAafConfig(gyroModel, gyroConfig()->gyro_hardware_lpf);
    spiWriteReg(dev, ICM426XX_RA_GYRO_CONFIG_STATIC3, aafConfig.delt);
    spiWriteReg(dev, ICM426XX_RA_GYRO_CONFIG_STATIC4, aafConfig.deltSqr & 0xFF);
    spiWriteReg(dev, ICM426XX_RA_GYRO_CONFIG_STATIC5, (aafConfig.deltSqr >> 8) | (aafConfig.bitshift << 4));
    
    // 配置加速度计 AAF
    setUserBank(dev, ICM426XX_BANK_SELECT2);
    aafConfig = getGyroAafConfig(gyroModel, AAF_CONFIG_258HZ);
    spiWriteReg(dev, ICM426XX_RA_ACCEL_CONFIG_STATIC2, aafConfig.delt << 1);
    spiWriteReg(dev, ICM426XX_RA_ACCEL_CONFIG_STATIC3, aafConfig.deltSqr & 0xFF);
    spiWriteReg(dev, ICM426XX_RA_ACCEL_CONFIG_STATIC4, (aafConfig.deltSqr >> 8) | (aafConfig.bitshift << 4));
    
    // 配置 UI 滤波器
    setUserBank(dev, ICM426XX_BANK_SELECT0);
    spiWriteReg(dev, ICM426XX_RA_GYRO_ACCEL_CONFIG0, 
                ICM426XX_ACCEL_UI_FILT_BW_LOW_LATENCY | ICM426XX_GYRO_UI_FILT_BW_LOW_LATENCY);
    
    // 配置中断
    spiWriteReg(dev, ICM426XX_RA_INT_CONFIG, 
                ICM426XX_INT1_MODE_PULSED | ICM426XX_INT1_DRIVE_CIRCUIT_PP | ICM426XX_INT1_POLARITY_ACTIVE_HIGH);
    spiWriteReg(dev, ICM426XX_RA_INT_CONFIG0, ICM426XX_UI_DRDY_INT_CLEAR_ON_SBR);
    spiWriteReg(dev, ICM426XX_RA_INT_SOURCE0, ICM426XX_UI_DRDY_INT1_EN_ENABLED);
    
    // 禁用 AFSR（防止陀螺仪输出停滞）
    uint8_t intfConfig1Value = spiReadRegMsk(dev, ICM426XX_INTF_CONFIG1);
    intfConfig1Value &= ~ICM426XX_INTF_CONFIG1_AFSR_MASK;
    intfConfig1Value |= ICM426XX_INTF_CONFIG1_AFSR_DISABLE;
    spiWriteReg(dev, ICM426XX_INTF_CONFIG1, intfConfig1Value);
    
    // 开启陀螺仪和加速度计
    turnGyroAccOn(dev);
    
    // 配置 ODR 和 FSR
    spiWriteReg(dev, ICM426XX_RA_GYRO_CONFIG0, (0 << 5) | (odrConfig & 0x0F));
    spiWriteReg(dev, ICM426XX_RA_ACCEL_CONFIG0, (0 << 5) | (odrConfig & 0x0F));
}
```

### 数据读取

```c
// 通过 SPI DMA 读取数据 (accgyro_mpu.c)
bool mpuGyroReadSPI(gyroDev_t *gyro)
{
    // 使用 DMA 读取 6 字节陀螺仪数据
    int16_t *gyroData = (int16_t *)gyro->dev.rxBuf;
    
    // 数据格式：X_H, X_L, Y_H, Y_L, Z_H, Z_L
    gyro->gyroADCRaw[X] = gyroData[0];
    gyro->gyroADCRaw[Y] = gyroData[1];
    gyro->gyroADCRaw[Z] = gyroData[2];
    
    return true;
}
```

### DMA 配置

在 Betaflight 中，ICM42688P 使用 **SPI DMA Normal 模式**读取数据：

```c
// SPI DMA 配置 (bus_spi_stdperiph.c)
DMA_InitTypeDef dmaInitRx;
dmaInitRx.DMA_Channel = bus->dmaRx->channel;
dmaInitRx.DMA_DIR = DMA_DIR_PeripheralToMemory;
dmaInitRx.DMA_Mode = DMA_Mode_Normal;  // 单次传输模式
dmaInitRx.DMA_PeripheralBaseAddr = (uint32_t)&SPI->DR;
dmaInitRx.DMA_Priority = DMA_Priority_Medium;
dmaInitRx.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
dmaInitRx.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
```

### 关键特性

1. **AFSR 禁用**: 防止陀螺仪输出停滞（ArduPilot 发现的问题）
2. **低延迟滤波器**: UI 滤波器配置为低延迟模式
3. **中断驱动**: 使用数据就绪中断触发读取
4. **SPI DMA**: 使用 DMA 进行高效数据传输
5. **外部时钟**: 支持 32 kHz 外部时钟输入（可选）

---

## 常见问题

### Q: 如何选择 AAF 截止频率？

**A**: 根据 ODR 选择：
- 1 kHz ODR → 258 Hz AAF
- 2 kHz ODR → 536 Hz AAF
- 4 kHz ODR → 997 Hz AAF
- 8 kHz ODR → 1962 Hz AAF

### Q: 为什么需要禁用 AFSR？

**A**: AFSR（Async FIFO Sample Rate）可能导致陀螺仪输出停滞。Betaflight 和 ArduPilot 都禁用了此功能。

### Q: 温度传感器如何转换？

**A**: 温度（°C）= (TEMP_DATA / 132.48) + 25

### Q: 如何实现高速读取（8 kHz）？

**A**: 
1. 设置 ODR 为 8 kHz
2. 使用宽带宽 AAF（1962 Hz）
3. 使用 SPI DMA 读取
4. 使用中断驱动模式

### Q: STM32F405 有什么限制？

**A**: STM32F405 的 CCM SRAM（0x10000000）不支持 DMA，数据缓冲区必须放在普通 SRAM 中。

---

## 参考资料

- [ICM-42688-P 数据手册](https://invensense.tdk.com/products/motion-tracking/6-axis/icm-42688-p/)
- [Betaflight 源代码](https://github.com/betaflight/betaflight)
- [ArduPilot ICM42688P 驱动](https://github.com/ArduPilot/ardupilot/pull/25332)

---

## 许可证

本库基于 Betaflight 实现，遵循 GPL-3.0 许可证。

---

## 作者

基于 Betaflight 项目改编
- 原作者: Dominic Clifton
- 改编: 2025
