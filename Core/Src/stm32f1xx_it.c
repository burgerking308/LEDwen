/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   中断入口：SysTick 与 USART1
  ******************************************************************************
  * 说明：如果你用的是 CubeMX 生成的工程，直接覆盖同名的 stm32f1xx_it.c 即可。
  *       只要保证 SysTick_Handler 里有 HAL_IncTick()、USART1_IRQHandler 里
  *       调用 HAL_UART_IRQHandler(&huart1)，作业的串口收发就能跑起来。
  ******************************************************************************
  */

#include "main.h"

extern UART_HandleTypeDef huart1;

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/* HAL 的时基：HAL_GetTick / HAL_Delay 都靠它 */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* 串口中断：收字节、接收完成、发送完成、错误回调都在 HAL 内部转发 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

#if (FB_USE_DMA == 1)
extern DMA_HandleTypeDef hdma_usart1_tx;

/* 选做一：DMA 传输完成中断走这里。DMA 发完还要靠 USART1 的 TC 中断收尾，
   所以上面那个 USART1_IRQHandler 也必须保留，不能关 */
void DMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}
#endif /* FB_USE_DMA */
