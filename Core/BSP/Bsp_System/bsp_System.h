#include "stm32f4xx.h"

//启动dwt初始化开始计时
void cycleCounterInit(void);
//获取当前时刻
uint32_t DWT_GetTick(void);
//us转周期
uint32_t clockMicrosToCycles(uint32_t micros);

void SystemClock_Config(void);

/* Cortex-M4 Processor Interruption and Exception Handlers */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* STM32F4xx Peripheral Interrupt Handlers */
void DMA2_Stream0_IRQHandler(void);