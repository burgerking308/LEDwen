/**
  ******************************************************************************
  * @file    oled.c
  * @brief   SSD1306 128x64 的"显示层"：显存、字库、整屏刷新
  ******************************************************************************
  * 这一层与硬件无关：不管底层是软件 I2C 还是硬件 I2C，画法都一样，
  * 只通过 oled_port.h 的三个函数把字节送出去（选做二就是换掉这三个函数的实现）。
  *
  * 设计要点：
  *  1) 先在内存里画（s_buf[8][128]），再整屏推一次 —— 不会出现"清屏闪一下"。
  *  2) 6x8 小字库（数字 + 大写字母 + 少量符号），字库在 oled_font.c。
  ******************************************************************************
  */

#include "board_config.h"
#include "oled.h"
#include "oled_port.h"
#include "oled_font.h"
#include <string.h>

#define OLED_WIDTH      128u
#define OLED_PAGES      8u

static uint8_t s_buf[OLED_PAGES][OLED_WIDTH];

/* ----------------------- 初始化 ----------------------- */
void OLED_Init(void)
{
    static const uint8_t seq[] =
    {
        0xAEu,              /* 关显示 */
        0xD5u, 0x80u,       /* 时钟分频 */
        0xA8u, 0x3Fu,       /* 复用率 1/64 */
        0xD3u, 0x00u,       /* 显示偏移 0 */
        0x40u,              /* 起始行 0 */
        0x8Du, 0x14u,       /* 打开内部电荷泵 */
        0x20u, 0x00u,       /* 水平寻址模式 */
        0xA1u,              /* 段重映射（左右方向） */
        0xC8u,              /* COM 扫描反向（上下方向） */
        0xDAu, 0x12u,       /* COM 引脚配置 */
        0x81u, 0xCFu,       /* 对比度 */
        0xD9u, 0xF1u,       /* 预充电周期 */
        0xDBu, 0x40u,       /* VCOMH */
        0xA4u,              /* 显示跟随内存 */
        0xA6u,              /* 正常显示（非反色） */
        0x2Eu,              /* 关闭滚动 */
        0xAFu               /* 开显示 */
    };

    OledPort_Init();        /* 总线准备好了才能发命令 */
    OledPort_WriteCmds(seq, (uint16_t)(sizeof(seq) / sizeof(seq[0])));

    OLED_ClearBuffer();
    OLED_Flush();
}

/* ----------------------- 画内存 ----------------------- */
void OLED_ClearBuffer(void)
{
    (void)memset(s_buf, 0x00, sizeof(s_buf));
}

void OLED_ShowString(uint8_t x, uint8_t page, const char *str)
{
    if (page >= OLED_PAGES)
    {
        return;
    }

    while ((*str != '\0') && ((uint16_t)x + OLED_FONT_W <= OLED_WIDTH))
    {
        uint16_t i;
        uint8_t  found = 0u;

        for (i = 0u; i < OLED_FontCount; i++)
        {
            if (OLED_FontTable[i].ch == *str)
            {
                (void)memcpy(&s_buf[page][x], OLED_FontTable[i].col, OLED_FONT_W);
                found = 1u;
                break;
            }
        }
        if (found == 0u)                      /* 字库里没有的字，显示为空白 */
        {
            (void)memset(&s_buf[page][x], 0x00, OLED_FONT_W);
        }

        x = (uint8_t)(x + OLED_FONT_W);
        str++;
    }
}

/* ----------------------- 上屏 ----------------------- */
void OLED_Flush(void)
{
    static const uint8_t window[] =
    {
        0x21u, 0x00u, 0x7Fu,        /* 列地址 0~127 */
        0x22u, 0x00u, 0x07u         /* 页地址 0~7 */
    };

    OledPort_WriteCmds(window, (uint16_t)(sizeof(window) / sizeof(window[0])));
    OledPort_WriteData(&s_buf[0][0], (uint16_t)(OLED_WIDTH * OLED_PAGES));
}
