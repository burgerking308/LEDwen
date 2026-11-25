/**
  ******************************************************************************
  * @file    key.c
  * @brief   按键消抖（时间窗口法，不需要定时器，也不用 HAL_Delay 阻塞）
  ******************************************************************************
  * 原理：
  *   每次扫描记录"原始电平"。只要原始电平一变，就把变化时刻充值。
  *   只有电平连续稳定超过 KEY_DEBOUNCE_MS 之后，才承认它变成了新的稳定电平。
  *   只在"稳定电平由 1 变成 0"（按下沿）那一刻返回 1，一次按下只触发一次。
  ******************************************************************************
  */

#include "board_config.h"
#include "key.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    uint8_t       raw_last;     /* 上一次采样值，1 = 松开 */
    uint8_t       stable;       /* 消抖后的稳定值，1 = 松开 */
    uint32_t      t_change;     /* 原始电平最后一次变化的时刻 */
} Key_t;

#define KEY_COUNT   2u

static Key_t s_key[KEY_COUNT];

static void Key_Bind(Key_t *k, GPIO_TypeDef *port, uint16_t pin)
{
    k->port      = port;
    k->pin       = pin;
    k->raw_last  = 1u;
    k->stable    = 1u;
    k->t_change  = HAL_GetTick();
}

void Key_Init(void)
{
    uint8_t i;

    /* 引脚按 LEDwen 板：KEY0=PA8（方向）、KEY1=PB10（暂停/继续） */
    Key_Bind(&s_key[0], HW_KEY_DIR_GPIO_Port, HW_KEY_DIR_Pin);
    Key_Bind(&s_key[1], HW_KEY_RUN_GPIO_Port, HW_KEY_RUN_Pin);

    /* 用当前实际电平初始化，避免上电瞬间被误判成"按下" */
    for (i = 0u; i < KEY_COUNT; i++)
    {
        uint8_t lv = (HAL_GPIO_ReadPin(s_key[i].port, s_key[i].pin) == GPIO_PIN_SET) ? 1u : 0u;
        s_key[i].raw_last = lv;
        s_key[i].stable   = lv;
        s_key[i].t_change = HAL_GetTick();
    }
}

/* 返回 1 表示这一帧检测到一次有效按下 */
static uint8_t Key_Pressed(Key_t *k)
{
    uint8_t  raw = (HAL_GPIO_ReadPin(k->port, k->pin) == GPIO_PIN_SET) ? 1u : 0u;
    uint32_t now = HAL_GetTick();

    if (raw != k->raw_last)
    {
        k->raw_last = raw;
        k->t_change = now;                  /* 还在抖，重新计窗口 */
        return 0u;
    }

    if ((now - k->t_change) < KEY_DEBOUNCE_MS)
    {
        return 0u;                          /* 窗口未满，不认 */
    }

    if (raw != k->stable)
    {
        k->stable = raw;                    /* 承认新电平 */
        return (raw == 0u) ? 1u : 0u;       /* 只在按下沿报一次 */
    }

    return 0u;
}

uint8_t Key_Scan(void)
{
    uint8_t evt = 0u;

    if (Key_Pressed(&s_key[0]) != 0u)
    {
        evt |= KEY_EVT_DIR;
    }
    if (Key_Pressed(&s_key[1]) != 0u)
    {
        evt |= KEY_EVT_RUN;
    }

    return evt;
}
