#include "main.h"
#include "bsp_System.h"
#include "bsp_IO.h"

int main(void)
{
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();

  while (1)
  {
  }
}




