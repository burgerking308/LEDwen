/**
  ******************************************************************************
  * @file    oled_port.h
  * @brief   OLED 的"传输层"接口：只管怎么把字节送到屏幕
  ******************************************************************************
  * oled.c 负责画（显存、字库、整屏推图），本文件定义它需要的三个动作。
  * 具体实现有两份，由 board_config.h 的 OLED_USE_HW_I2C 二选一编译：
  *   - oled_port_sw.c ：软件 I2C（GPIO 位翻转）—— 必做项
  *   - oled_port_hw.c ：硬件 I2C（I2C1 外设 + HAL）—— 选做项
  * 显示内容、字号、刷新逻辑完全共用，两者效果一致。
  ******************************************************************************
  */

#ifndef OLED_PORT_H
#define OLED_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 初始化总线（软件版配 GPIO、硬件版配 I2C1 外设） */
void OledPort_Init(void);

/* 发命令：SSD1306 的控制字节 0x00 */
void OledPort_WriteCmds(const uint8_t *cmds, uint16_t len);

/* 发显存数据：SSD1306 的控制字节 0x40 */
void OledPort_WriteData(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* OLED_PORT_H */
