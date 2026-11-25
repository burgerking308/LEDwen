/**
  ******************************************************************************
  * @file    oled_port_sw.c
  * @brief   软件 I2C 版传输层（GPIO 位翻转）—— 必做项
  ******************************************************************************
  * 时序要点：
  *  - SCL/SDA 必须配成"开漏输出 + 上拉电阻"，高电平靠上拉，不能推挽拉高，
  *    否则从机想拉低时会打架。
  *  - 每位之间的延时别太短（72MHz 下约 1us 比较稳），太快会花屏或黑屏。
  ******************************************************************************
  */

#include "board_config.h"
#include "oled_port.h"

#if (OLED_USE_HW_I2C == 0)

static void OLED_Dly(void)
{
    volatile uint16_t i;
    for (i = 0u; i < 12u; i++)
    {
        __NOP();
    }
}

static void SCL(uint8_t v)
{
    HAL_GPIO_WritePin(HW_OLED_SCL_GPIO_Port, HW_OLED_SCL_Pin, (v != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void SDA(uint8_t v)
{
    HAL_GPIO_WritePin(HW_OLED_SDA_GPIO_Port, HW_OLED_SDA_Pin, (v != 0u) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void I2C_Start(void)
{
    SDA(1u); OLED_Dly();
    SCL(1u); OLED_Dly();
    SDA(0u); OLED_Dly();
    SCL(0u); OLED_Dly();
}

static void I2C_Stop(void)
{
    SDA(0u); OLED_Dly();
    SCL(1u); OLED_Dly();
    SDA(1u); OLED_Dly();
}

/* 发一个字节，第 9 个时钟让从机应答（这里不检查 ACK） */
static void I2C_WriteByte(uint8_t b)
{
    uint8_t i;

    for (i = 0u; i < 8u; i++)
    {
        SDA((b & 0x80u) ? 1u : 0u);
        b = (uint8_t)(b << 1);
        OLED_Dly();
        SCL(1u); OLED_Dly();
        SCL(0u); OLED_Dly();
    }

    SDA(1u); OLED_Dly();
    SCL(1u); OLED_Dly();
    SCL(0u); OLED_Dly();
}

static void I2C_Write(uint8_t ctrl, const uint8_t *data, uint16_t len)
{
    uint16_t i;

    I2C_Start();
    I2C_WriteByte(OLED_I2C_ADDR);
    I2C_WriteByte(ctrl);
    for (i = 0u; i < len; i++)
    {
        I2C_WriteByte(data[i]);
    }
    I2C_Stop();
}

/* ----------------------- 对外三个动作 ----------------------- */
void OledPort_Init(void)
{
    GPIO_InitTypeDef g = {0};

    g.Pin   = HW_OLED_SCL_Pin;
    g.Mode  = GPIO_MODE_OUTPUT_OD;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(HW_OLED_SCL_GPIO_Port, &g);

    g.Pin   = HW_OLED_SDA_Pin;
    HAL_GPIO_Init(HW_OLED_SDA_GPIO_Port, &g);

    HAL_GPIO_WritePin(HW_OLED_SCL_GPIO_Port, HW_OLED_SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HW_OLED_SDA_GPIO_Port, HW_OLED_SDA_Pin, GPIO_PIN_SET);
}

void OledPort_WriteCmds(const uint8_t *cmds, uint16_t len)
{
    I2C_Write(0x00u, cmds, len);
}

void OledPort_WriteData(const uint8_t *data, uint16_t len)
{
    I2C_Write(0x40u, data, len);
}

#endif /* OLED_USE_HW_I2C == 0 */
