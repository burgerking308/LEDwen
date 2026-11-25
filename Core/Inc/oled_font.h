/* 自动生成，勿手改；字形在 docs/gen_font.py 的 ART 里维护 */
#ifndef OLED_FONT_H
#define OLED_FONT_H

#include <stdint.h>

#define OLED_FONT_W     6u   /* 每个字符占 6 列（5 列字形 + 1 列间距） */
#define OLED_FONT_PAGES 1u   /* 每个字符占 1 个 page（8 行） */

typedef struct
{
    char    ch;
    uint8_t col[6];              /* 一列一字节，bit0 = 最上面一行 */
} OledGlyph_t;

extern const OledGlyph_t OLED_FontTable[];
extern const uint16_t    OLED_FontCount;

#endif /* OLED_FONT_H */
