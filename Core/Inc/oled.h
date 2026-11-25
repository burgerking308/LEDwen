#ifndef OLED_H
#define OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* SSD1306 128x64，软件 I2C，引脚见 board_config.h */

void OLED_Init(void);                               /* 上电初始化 + 清屏 */
void OLED_ClearBuffer(void);                        /* 只清内存，不上屏 */
void OLED_ShowString(uint8_t x, uint8_t page, const char *str);  /* x: 列 0~127, page: 行 0~7 */
void OLED_Flush(void);                              /* 把内存里的画面整屏推给屏幕 */

#ifdef __cplusplus
}
#endif

#endif /* OLED_H */
