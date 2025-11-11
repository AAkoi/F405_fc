#include "stm32f4xx.h"

//启动dwt初始化开始计时
void cycleCounterInit(void);
//获取当前时刻
uint32_t DWT_GetTick(void);
//us转周期
uint32_t clockMicrosToCycles(uint32_t micros);

void SystemClock_Config(void);