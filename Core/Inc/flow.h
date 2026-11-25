#ifndef FLOW_H
#define FLOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 流水灯的"唯一一份运行状态"。
 *        按键、串口都只通过 Flow_* 接口修改它；LED、OLED、串口反馈都只读它。
 */
typedef struct
{
    volatile uint8_t  run;        /* 0 = 暂停，1 = 运行 */
    volatile uint8_t  dir;        /* 0 = 正向，1 = 反向 */
    volatile uint16_t step_ms;    /* 相邻 LED 切换间隔，100 ~ 2000 ms */
    volatile uint8_t  led_idx;    /* 当前点亮的 LED 序号，0 起 */
    volatile uint32_t last_tick;  /* 上一次切换的时刻（HAL_GetTick） */
} FlowState_t;

/** 一份完整设置，串口一包正好解析成这个结构 */
typedef struct
{
    uint8_t  run;
    uint8_t  dir;
    uint16_t step_ms;
} FlowCmd_t;

extern volatile FlowState_t g_flow;

/* 供其它模块置位：谁改了状态谁负责通知 */
extern volatile uint8_t g_oled_dirty;   /* 1 = 显示需要重画 */
extern volatile uint8_t g_fb_pending;   /* 1 = 需要回传一次串口状态 */

void    Flow_Init(void);                /* 上电默认：运行 / 正向 / 300ms */
void    Flow_Apply(const FlowCmd_t *cmd);/* 唯一的状态修改入口（带反馈） */
void    Flow_ToggleDir(void);           /* 按键：正向 <-> 反向 */
void    Flow_ToggleRun(void);           /* 按键：暂停 <-> 继续 */
void    Flow_Task(void);                /* 主循环调用，非阻塞，负责切灯 */
uint8_t Flow_LedNumber(void);           /* 当前 LED 编号，1 ~ LED_NUM */

#ifdef __cplusplus
}
#endif

#endif /* FLOW_H */
