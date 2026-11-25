/**
  ******************************************************************************
  * @file    oled_port_hw.c
  * @brief   硬件 I2C 版传输层（I2C1 外设 + HAL）—— 选做二
  ******************************************************************************
  * 与软件版的区别：
  *  - 软件版：CPU 一位一位翻转 GPIO，每字节约 8~9 次函数调用；
  *  - 硬件版：CPU 只把字节塞进 I2C1 数据寄存器，SCL/SDA 波形由外设自动产生，
  *    起始位/停止位/应答位也由硬件处理，CPU 占用低得多。
  *
  * 引脚：I2C1 默认就是 PB6(SCL) / PB7(SDA)，和软件版接法完全一样，不用改线。
  *       两个引脚必须配成"复用开漏（AF_OD）"，并保证有上拉电阻。
  *
  * 技巧：SSD1306 的"控制字节"（0x00 命令 / 0x40 数据）正好对应 HAL 的
  *       MemAddress，所以直接用 HAL_I2C_Mem_Write 就能发出和软件版一模一样的
  *       "START + 地址 + 控制字节 + 数据 + STOP"，不用额外准备缓冲区。
  ******************************************************************************
  */

#include "main.h"
#include "board_config.h"
#include "oled_port.h"

#if (OLED_USE_HW_I2C == 1)

I2C_HandleTypeDef hi2c1;

#define I2C_CMD_TIMEOUT     10u     /* 命令帧短，10ms 足够 */
#define I2C_DATA_TIMEOUT    50u     /* 1024 字节 @400kHz 约 23ms，留余量 */

static void I2C1_Config(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* 硬件 I2C 用 I2C1 的默认脚 PB6(SCL) / PB7(SDA)：
       LEDwen 板上 OLED 接的是 PB4/PB3，那两脚没有 I2C 复用功能，
       想跑这一版必须把屏的两根线换接到 PB6/PB7 */
    {
        GPIO_InitTypeDef g = {0};
        g.Pin   = HW_I2C_SCL_Pin | HW_I2C_SDA_Pin;
        g.Mode  = GPIO_MODE_AF_OD;
        g.Pull  = GPIO_NOPULL;
        g.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &g);
        HAL_GPIO_WritePin(HW_I2C_SCL_GPIO_Port, HW_I2C_SCL_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(HW_I2C_SDA_GPIO_Port, HW_I2C_SDA_Pin, GPIO_PIN_SET);
    }

    hi2c1.Instance             = I2C1;
    hi2c1.Init.ClockSpeed      = 400000u;                   /* 400kHz 快速模式 */
    hi2c1.Init.DutyCycle       = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1     = 0u;                        /* 主机模式不用自己的地址 */
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0u;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
}

/* 总线异常（NACK / 仲裁丢失）后重新初始化一次，避免屏幕从此不再刷新 */
static void I2C1_Recover(void)
{
    (void)HAL_I2C_DeInit(&hi2c1);
    I2C1_Config();
}

static void I2C1_Write(uint8_t ctrl, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    HAL_StatusTypeDef st;

    st = HAL_I2C_Mem_Write(&hi2c1, OLED_I2C_ADDR, (uint16_t)ctrl,
                           I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, timeout);
    if (st != HAL_OK)
    {
        I2C1_Recover();        /* 失败一次不至于让屏幕彻底黑掉 */
    }
}

/* ----------------------- 对外三个动作 ----------------------- */
void OledPort_Init(void)
{
    I2C1_Config();
}

void OledPort_WriteCmds(const uint8_t *cmds, uint16_t len)
{
    I2C1_Write(0x00u, cmds, len, I2C_CMD_TIMEOUT);
}

void OledPort_WriteData(const uint8_t *data, uint16_t len)
{
    I2C1_Write(0x40u, data, len, I2C_DATA_TIMEOUT);
}

#endif /* OLED_USE_HW_I2C == 1 */
