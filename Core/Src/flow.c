/**
  ******************************************************************************
  * @file    flow.c
  * @brief   流水灯状态机：一份状态 + 非阻塞切换 + 统一的 LED 输出
  ******************************************************************************
  * 关键点：这里没有任何 HAL_Delay()。
  *         暂停是通过"不进入切换分支"实现的，不是把 CPU 卡住，
  *         所以暂停期间按键和串口依然有效（对应作业要求 1.3）。
  ******************************************************************************
  */

#include "board_config.h"
#include "flow.h"

volatile FlowState_t g_flow;
volatile uint8_t     g_oled_dirty = 1u;
volatile uint8_t     g_fb_pending = 0u;

/* ---------------- LED 输出：全工程只有这一个函数点灯 ---------------- */
/* 引脚按 LEDwen 板：LED0=PA0 / LED1=PA4 / LED2=PB0，对应内部编号 1 / 2 / 3 */
static void LED_Show(uint8_t idx)
{
    HAL_GPIO_WritePin(HW_LED1_GPIO_Port, HW_LED1_Pin, (idx == 0u) ? LED_ON_LEVEL : LED_OFF_LEVEL);
    HAL_GPIO_WritePin(HW_LED2_GPIO_Port, HW_LED2_Pin, (idx == 1u) ? LED_ON_LEVEL : LED_OFF_LEVEL);
#if (LED_NUM >= 3)
    HAL_GPIO_WritePin(HW_LED3_GPIO_Port, HW_LED3_Pin, (idx == 2u) ? LED_ON_LEVEL : LED_OFF_LEVEL);
#endif
}

/* ---------------- 所有"改了状态"的地方都要走这里 ---------------- */
static void Flow_Notify(void)
{
    g_oled_dirty = 1u;
    g_fb_pending = 1u;
}

void Flow_Init(void)
{
    g_flow.run       = 1u;
    g_flow.dir       = 0u;
    g_flow.step_ms   = DEFAULT_STEP_MS;
    g_flow.led_idx   = 0u;
    g_flow.last_tick = HAL_GetTick();
    LED_Show(g_flow.led_idx);
    Flow_Notify();
}

void Flow_Apply(const FlowCmd_t *cmd)
{
    g_flow.run     = (cmd->run != 0u) ? 1u : 0u;
    g_flow.dir     = (cmd->dir != 0u) ? 1u : 0u;
    g_flow.step_ms = cmd->step_ms;
    /* 重新计时：否则改完参数可能立刻跳一格，看起来像"没停住" */
    g_flow.last_tick = HAL_GetTick();
    Flow_Notify();
}

void Flow_ToggleDir(void)
{
    g_flow.dir = (uint8_t)(g_flow.dir ^ 1u);
    g_flow.last_tick = HAL_GetTick();
    Flow_Notify();
}

void Flow_ToggleRun(void)
{
    g_flow.run = (uint8_t)(g_flow.run ^ 1u);
    g_flow.last_tick = HAL_GetTick();
    Flow_Notify();
}

void Flow_Task(void)
{
    uint32_t now;

    if (g_flow.run == 0u)
    {
        return;                 /* 暂停：保留当前 LED，直接不动 */
    }

    now = HAL_GetTick();
    if ((now - g_flow.last_tick) < (uint32_t)g_flow.step_ms)
    {
        return;
    }

    g_flow.last_tick = now;
    if (g_flow.dir == 0u)
    {
        g_flow.led_idx = (uint8_t)((g_flow.led_idx + 1u) % LED_NUM);
    }
    else
    {
        g_flow.led_idx = (uint8_t)((g_flow.led_idx + LED_NUM - 1u) % LED_NUM);
    }

    LED_Show(g_flow.led_idx);
    g_oled_dirty = 1u;          /* 灯号变了，OLED 要同步 */
}

uint8_t Flow_LedNumber(void)
{
    return (uint8_t)(g_flow.led_idx + 1u);
}
