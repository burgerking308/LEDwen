/**
  ******************************************************************************
  * @file    board_config.h
  * @brief   全部引脚与可调参数集中在这里 —— 已按 LEDwen 工程（STM32F103C8T6）配置
  ******************************************************************************
  * 引脚来源：LEDwen.ioc 里的 User Label
  *
  *   板子丝印        引脚     代码里的名字              用途
  *   LED0           PA0   ->  HW_LED1_Pin             第 1 颗灯
  *   LED1           PA4   ->  HW_LED2_Pin             第 2 颗灯
  *   LED2           PB0   ->  HW_LED3_Pin             第 3 颗灯
  *   KEY0           PA8   ->  HW_KEY_DIR_Pin          切换正向 / 反向
  *   KEY1           PB10  ->  HW_KEY_RUN_Pin          暂停 / 继续
  *   SCL            PB4   ->  HW_OLED_SCL_Pin         软件 I2C 时钟
  *   SDA            PB3   ->  HW_OLED_SDA_Pin         软件 I2C 数据
  *   USART1_TX      PA9   ->  固定，不用改
  *   USART1_RX      PA10  ->  固定，不用改
  *
  * 为什么名字统一加 HW_ 前缀：
  *   CubeMX 生成的 main.h 里已经有 LED0_Pin / LED1_Pin / KEY0_Pin / SCL_Pin 这些宏，
  *   若这里再用同名宏，会重定义冲突，而且 LED 序号还会错位（main.h 的 LED1 是第 2 颗灯）。
  *   加前缀彻底绕开，也不影响你在 CubeMX 里继续用原来的标签。
  *
  * 注意 PB3 / PB4 的坑：
  *   这两脚复位后默认是 JTAG 的 JTDO / NJTRST，必须关掉 JTAG（保留 SWD）才能当 GPIO 用。
  *   LEDwen 的 stm32f1xx_hal_msp.c 里已经有 __HAL_AFIO_REMAP_SWJ_NOJTAG()，
  *   本工程的 MX_GPIO_Init() 里也补了一句，双保险，不影响下载调试。
  ******************************************************************************
  */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* ============================ LED ============================ */
#define LED_NUM             3u

/* 0 = 高电平点亮（LED 阳极接 IO、阴极经电阻接 GND）—— LEDwen 板现在是这一种
 * 1 = 低电平点亮（LED 阳极经电阻接 3V3，阴极接 IO） */
#define LED_ACTIVE_LOW      0

/* 第 1 颗：板上丝印 LED0 */
#define HW_LED1_Pin         GPIO_PIN_0
#define HW_LED1_GPIO_Port   GPIOA

/* 第 2 颗：板上丝印 LED1 */
#define HW_LED2_Pin         GPIO_PIN_4
#define HW_LED2_GPIO_Port   GPIOA

/* 第 3 颗：板上丝印 LED2 */
#define HW_LED3_Pin         GPIO_PIN_0
#define HW_LED3_GPIO_Port   GPIOB

#if LED_ACTIVE_LOW
#define LED_OFF_LEVEL       GPIO_PIN_SET
#define LED_ON_LEVEL        GPIO_PIN_RESET
#else
#define LED_OFF_LEVEL       GPIO_PIN_RESET
#define LED_ON_LEVEL        GPIO_PIN_SET
#endif

/* ============================ 按键 =========================== */
/* 内部上拉，按下为低电平（按键一端接 IO，一端接 GND） */
#define HW_KEY_DIR_Pin      GPIO_PIN_8      /* KEY0：切换正向 / 反向 */
#define HW_KEY_DIR_GPIO_Port GPIOA

#define HW_KEY_RUN_Pin      GPIO_PIN_10     /* KEY1：暂停 / 继续 */
#define HW_KEY_RUN_GPIO_Port GPIOB

#define KEY_DEBOUNCE_MS     20u

/* ====================== OLED（软件 I2C） ====================== */
/* 板上 SCL = PB4，SDA = PB3，配成开漏输出 + 上拉（模块自带 4.7k） */
#define HW_OLED_SCL_Pin     GPIO_PIN_4
#define HW_OLED_SCL_GPIO_Port GPIOB

#define HW_OLED_SDA_Pin     GPIO_PIN_3
#define HW_OLED_SDA_GPIO_Port GPIOB

/* ================ 硬件 I2C（选做二）用的引脚 ==================
 * 重点：PB3 / PB4 在 F103 上只有 JTAG / SPI1 复用功能，没有 I2C！
 *       所以想跑"硬件 I2C"版，必须把 OLED 的两根线挪到 I2C1 的默认脚
 *       PB6(SCL) / PB7(SDA)（这两脚在 LEDwen 里是空闲的），
 *       或者改用 PB8/PB9（需要引脚重映射）。
 *       软件 I2C 版不受影响，仍用板上的 PB4/PB3。 */
#define HW_I2C_SCL_Pin      GPIO_PIN_6
#define HW_I2C_SCL_GPIO_Port GPIOB

#define HW_I2C_SDA_Pin      GPIO_PIN_7
#define HW_I2C_SDA_GPIO_Port GPIOB

/* ======================== 串口（USART1） ====================== */
#define UART_BAUD           115200u

/* ========================= 业务参数 ========================== */
#define STEP_MIN_MS         100u
#define STEP_MAX_MS         2000u
#define DEFAULT_STEP_MS     300u

/* ======================== 选做开关 =========================== */
#define FB_USE_DMA          1       /* 1 = 串口反馈走 DMA；0 = 走中断 */
#define OLED_USE_HW_I2C     0       /* 1 = 硬件 I2C（需把线挪到 PB6/PB7）；0 = 软件 I2C */

#define OLED_I2C_ADDR       0x78u   /* 0x3C << 1 */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_CONFIG_H */
