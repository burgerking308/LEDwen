/**
  ******************************************************************************
  * @file    main.c
  * @brief   电控组第三次培训作业：按键 + 串口 控制流水灯，OLED 显示状态
  ******************************************************************************
  * 整体结构（"一份状态 + 三个入口 + 一个出口"）：
  *
  *            按键(Key) ─┐
  *                       ├─> g_flow（唯一状态）─> LED / OLED / 串口反馈
  *            串口(UartPkt)─┘
  *
  * 主循环里没有任何阻塞延时，所以：
  *   - 暂停时按键和串口照常有效；
  *   - 串口连发多包不会丢。
  ******************************************************************************
  */

#include "main.h"
#include "board_config.h"
#include "flow.h"
#include "key.h"
#include "uart_pkt.h"
#include "oled.h"

#include <stdio.h>

UART_HandleTypeDef huart1;

#if (FB_USE_DMA == 1)
DMA_HandleTypeDef hdma_usart1_tx;
#endif

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
#if (FB_USE_DMA == 1)
static void MX_DMA_Init(void);
#endif
static void Ui_Refresh(void);

/* ============================== main ============================== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
#if (FB_USE_DMA == 1)
    MX_DMA_Init();        /* 必须在串口初始化之后链接 DMA */
#endif

    OLED_Init();
    Flow_Init();          /* 上电默认：运行 / 正向 / 300ms，1 号灯点亮 */
    Key_Init();
    UartPkt_Init();

    while (1)
    {
        uint8_t evt = Key_Scan();

        /* ---- 入口 1：按键 ---- */
        if ((evt & KEY_EVT_DIR) != 0u)
        {
            Flow_ToggleDir();
        }
        if ((evt & KEY_EVT_RUN) != 0u)
        {
            Flow_ToggleRun();
        }

        /* ---- 流水灯推进（非阻塞） ---- */
        Flow_Task();

        /* ---- 入口 2：串口（解析 + 应用 + 反馈） ---- */
        UartPkt_Task();

        /* ---- 出口：显示 ---- */
        if (g_oled_dirty != 0u)
        {
            g_oled_dirty = 0u;
            Ui_Refresh();
        }
    }
}

/* ============================== 显示 ============================== */
static void Ui_Refresh(void)
{
    char line[24];

    OLED_ClearBuffer();

    OLED_ShowString(0u, 0u, (g_flow.run != 0u) ? "STATE: RUN" : "STATE: STOP");
    OLED_ShowString(0u, 2u, (g_flow.dir != 0u) ? "DIR: REV" : "DIR: FWD");

    /* 6x8 字库只做了大写字母和数字，所以单位也用大写 MS，看起来最清楚 */
    (void)snprintf(line, sizeof(line), "STEP: %uMS", (unsigned)g_flow.step_ms);
    OLED_ShowString(0u, 4u, line);

    (void)snprintf(line, sizeof(line), "LED: %u", (unsigned)Flow_LedNumber());
    OLED_ShowString(0u, 6u, line);

    OLED_Flush();          /* 整屏推一次，不闪 */
}

/* ============================ 时钟 ============================ */
/* 外部 8MHz 晶振 -> 72MHz。板子上没有晶振的话换成 HSI 版本（见 README） */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef       osc = {0};
    RCC_ClkInitTypeDef       clk = {0};

    osc.OscillatorType  = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState        = RCC_HSE_ON;
    osc.HSEPredivValue  = RCC_HSE_PREDIV_DIV1;
    osc.HSIState        = RCC_HSI_ON;
    osc.PLL.PLLState    = RCC_PLL_ON;
    osc.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL      = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
    {
        Error_Handler();
    }

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;      /* 36MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV1;      /* 72MHz */
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ============================ GPIO ============================ */
static void GPIO_InitPin(GPIO_TypeDef *port, uint16_t pin, uint32_t mode, uint32_t pull, uint32_t speed)
{
    GPIO_InitTypeDef g = {0};

    g.Pin   = pin;
    g.Mode  = mode;
    g.Pull  = pull;
    g.Speed = speed;
    HAL_GPIO_Init(port, &g);
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();

    /* PB3 / PB4 复位后是 JTAG 的 JTDO / NJTRST，关掉 JTAG（保留 SWD 下载）才能当 GPIO 用。
       LEDwen 的 hal_msp.c 里也有这一句，重复调用没有副作用，属于双保险 */
    __HAL_AFIO_REMAP_SWJ_NOJTAG();

    /* LED：先给"熄灭"电平，避免上电瞬间三颗全亮 */
    HAL_GPIO_WritePin(HW_LED1_GPIO_Port, HW_LED1_Pin, LED_OFF_LEVEL);
    HAL_GPIO_WritePin(HW_LED2_GPIO_Port, HW_LED2_Pin, LED_OFF_LEVEL);
    HAL_GPIO_WritePin(HW_LED3_GPIO_Port, HW_LED3_Pin, LED_OFF_LEVEL);

    GPIO_InitPin(HW_LED1_GPIO_Port, HW_LED1_Pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    GPIO_InitPin(HW_LED2_GPIO_Port, HW_LED2_Pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);
    GPIO_InitPin(HW_LED3_GPIO_Port, HW_LED3_Pin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);

    /* 按键：输入 + 内部上拉，按下为低 */
    GPIO_InitPin(HW_KEY_DIR_GPIO_Port, HW_KEY_DIR_Pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);
    GPIO_InitPin(HW_KEY_RUN_GPIO_Port, HW_KEY_RUN_Pin, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);

#if (OLED_USE_HW_I2C == 0)
    /* OLED 软件 I2C：开漏输出（模块自带 4.7k 上拉即可），脚位是板上 SCL=PB4 / SDA=PB3 */
    GPIO_InitPin(HW_OLED_SCL_GPIO_Port, HW_OLED_SCL_Pin, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    GPIO_InitPin(HW_OLED_SDA_GPIO_Port, HW_OLED_SDA_Pin, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    HAL_GPIO_WritePin(HW_OLED_SCL_GPIO_Port, HW_OLED_SCL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(HW_OLED_SDA_GPIO_Port, HW_OLED_SDA_Pin, GPIO_PIN_SET);
#endif
}

/* ============================ 串口 ============================ */
static void MX_USART1_UART_Init(void)
{
    /* PA9 = USART1_TX（复用推挽），PA10 = USART1_RX（浮空/上拉输入） */
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitPin(GPIOA, GPIO_PIN_9, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    GPIO_InitPin(GPIOA, GPIO_PIN_10, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);

    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = UART_BAUD;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    /* 不开中断就收不到任何字节 —— 这一步最容易漏 */
    HAL_NVIC_SetPriority(USART1_IRQn, 1u, 0u);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/* ============================ DMA（选做一） ============================ */
#if (FB_USE_DMA == 1)
/* F103 上 USART1_TX 固定对应 DMA1 的 Channel 4，不需要在 CubeMX 里另行选择 */
static void MX_DMA_Init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_usart1_tx.Instance                 = DMA1_Channel4;
    hdma_usart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;   /* 外设地址固定 */
    hdma_usart1_tx.Init.MemInc              = DMA_MINC_ENABLE;    /* 内存地址递增 */
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode                = DMA_NORMAL;         /* 发完就停，不循环 */
    hdma_usart1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
        Error_Handler();
    }

    /* 把 DMA 挂到串口句柄上：HAL_UART_Transmit_DMA 就靠这根线找到通道 */
    __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);

    /* DMA 传输完成要靠中断通知，不开中断就收不到 TxCplt 回调 */
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 1u, 1u);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
}
#endif /* FB_USE_DMA */

/* ========================== 出错处理 ========================== */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* 卡死在这里通常意味着：时钟没起来 / 串口参数配错 */
    }
}
