#ifndef KEY_H
#define KEY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Key_Scan 返回值按位组合，一个 1ms 里两个键同时按下也能都收到 */
#define KEY_EVT_DIR     0x01u   /* KEY1：切换正向 / 反向 */
#define KEY_EVT_RUN     0x02u   /* KEY2：暂停 / 继续 */

void    Key_Init(void);
uint8_t Key_Scan(void);         /* 主循环反复调用即可，内部自带 20ms 消抖 */

#ifdef __cplusplus
}
#endif

#endif /* KEY_H */
