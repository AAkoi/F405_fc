#include "main.h"
#include "bsp_System.h"
#include "bsp_IO.h"
#include "bsp_spi.h"
#include "bsp_iic.h"
#include "bsp_uart.h"
#include "icm42688p.h"
#include "bmp280.h"
#include "elrs_crsf_port.h"

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  BSP_UART_Init();
  ELRS_CRSF_InitOnUART1();

  // 传感器初始化（基于各自的 Lib 封装）
  icm42688p_init_driver();
  bmp280_init_driver();

  while (1)
  {
    HAL_Delay(1);
  }
}




