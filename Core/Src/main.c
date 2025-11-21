#include <stdbool.h>
#include <stdio.h>
#include "main.h"
#include "bsp_System.h"
#include "bsp_IO.h"
#include "bsp_spi.h"
#include "bsp_iic.h"
#include "bsp_uart.h"
#include "icm42688p.h"
#include "attitude.h"
#include "bmp280.h"
#include "elrs_crsf_port.h"

// 测试代码
#include "test_icm42688p.h"
#include "test_bmp280.h"
#include "test_spi_hardware.h"
#include "test_dma.h"

// BMP280测试函数（保留兼容性）
void bmp280_test_loop(void)
{
  printf("\r\n========================================\r\n");
  printf("       BMP280 气压传感器测试\r\n");
  printf("========================================\r\n\r\n");
  
  // 设置海平面气压（用于计算高度）
  // 标准大气压: 101325 Pa
  // 可根据当地天气调整，查询网站：https://www.weather.com.cn
  bmp280_set_sea_level_pressure_pa(101325.0f);
  printf("海平面气压设置为: 101325 Pa (标准大气压)\r\n");
  printf("如需精确高度，请根据当地实际气压调整\r\n\r\n");
  
  HAL_Delay(1000);
  
  printf("开始连续读取数据...\r\n");
  printf("------------------------------------------------------------\r\n");
  printf("  次数  |  温度(°C)  |  气压(Pa)  |  高度(m)  \r\n");
  printf("------------------------------------------------------------\r\n");
  
  uint32_t count = 0;
  uint32_t fail_count = 0;
  
  while (1)
  {
    float temperature = 0.0f;
    int32_t pressure = 0;
    float altitude = 0.0f;
    
    // 读取所有数据
    if (bmp280_get_all(&temperature, &pressure, &altitude))
    {
      // 成功读取
      fail_count = 0;
      count++;
      
      // 格式化输出（使用整数避免浮点数printf问题）
      int temp_int = (int)temperature;
      int temp_frac = (int)((temperature - temp_int) * 100);
      if (temp_frac < 0) temp_frac = -temp_frac;
      
      int alt_int = (int)altitude;
      int alt_frac = (int)((altitude - alt_int) * 100);
      if (alt_frac < 0) alt_frac = -alt_frac;
      
      printf("  %4d  | %3d.%02d °C | %6d Pa | %4d.%02d m\r\n",
             (int)count,
             temp_int, temp_frac,
             (int)pressure,
             alt_int, alt_frac);
      
      // 每10次输出一些额外信息
      if (count % 10 == 0) {
        printf("------------------------------------------------------------\r\n");
        printf("  统计: 已读取 %d 次 | 气压: %.2f hPa (百帕)\r\n",
               (int)count, (float)pressure / 100.0f);
        printf("------------------------------------------------------------\r\n");
      }
    }
    else
    {
      // 读取失败
      fail_count++;
      printf("  [错误] 读取失败 (连续失败: %d 次)\r\n", (int)fail_count);
      
      if (fail_count >= 5) {
        printf("\r\n[严重错误] 连续读取失败！\r\n");
        printf("可能原因:\r\n");
        printf("  1. I2C连接断开\r\n");
        printf("  2. 传感器未正确初始化\r\n");
        printf("  3. 传感器故障\r\n");
        printf("\r\n正在尝试重新初始化...\r\n");
        
        bmp280_init_driver();
        fail_count = 0;
        HAL_Delay(1000);
      }
    }
    
    HAL_Delay(500); // 每0.5秒读取一次
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  BSP_UART_Init();

  printf("\r\n\r\n");
  printf("****************************************************\r\n");
  printf("*          STM32 传感器测试程序                   *\r\n");
  printf("****************************************************\r\n");
  printf("系统初始化完成\r\n\r\n");

  /* ============================================
   * 选择测试模式：注释掉不需要的测试
   * ============================================ */
  
  // 测试模式0：SPI硬件诊断（0xFF问题专用）
  printf("\r\n[阶段1] SPI硬件诊断（初始化前）\r\n");
  test_spi1_hardware_diagnosis();
  
  HAL_Delay(2000);
  
  printf("\r\n========================================\r\n");
  printf("     尝试修复：手动使能SPI\r\n");
  printf("========================================\r\n");
  __HAL_SPI_ENABLE(&hspi1);
  printf("已执行 __HAL_SPI_ENABLE()\r\n");
  printf("SPI1->CR1 SPE位: %s\r\n", (SPI1->CR1 & SPI_CR1_SPE) ? "已使能 ✓" : "未使能 ✗");
  
  HAL_Delay(1000);
  
  // 再次测试
  printf("\r\n[阶段2] SPI硬件诊断（使能SPI后）\r\n");
  test_spi1_hardware_diagnosis();
  
  HAL_Delay(2000);
  
  // 尝试位操作SPI
  test_spi1_bitbang();
  HAL_Delay(2000);
  
  // 测试模式1：ICM42688P IMU传感器 + 姿态解算
  printf("正在初始化 ICM42688P...\r\n");
  printf("  [模式] 写轮询 / 读DMA\r\n");
  icm42688p_init_driver();
  Attitude_Init();
  HAL_Delay(500);
  
  // DMA可靠性测试（仅在DMA模式下）
  test_dma_reliability();
  
#ifdef ICM_USE_DMA
  // DMA模式：在测试之间加更长延时，确保SPI完全恢复
  printf("等待SPI恢复...\r\n");
  HAL_Delay(3000);
#else
  HAL_Delay(1000);
#endif
  
  test_icm42688p_full();  // 完整测试流程
  
  /* 测试模式2：BMP280气压传感器
  printf("正在初始化 BMP280...\r\n");
  bmp280_init_driver();
  HAL_Delay(500);
  test_bmp280_full();
  */
  
  /* 测试模式3：ICM42688P原始数据（不进行姿态解算）
  printf("正在初始化 ICM42688P...\r\n");
  icm42688p_init_driver();
  HAL_Delay(500);
  test_icm42688p_raw_data();
  */

  // Should never reach here
  while (1) {
  }
}
