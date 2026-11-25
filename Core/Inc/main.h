#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "board_config.h"

void Error_Handler(void);

extern UART_HandleTypeDef huart1;

#if (FB_USE_DMA == 1)
extern DMA_HandleTypeDef hdma_usart1_tx;      /* 串口反馈用的 DMA 通道 */
#endif

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
