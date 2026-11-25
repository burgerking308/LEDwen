/**
  ******************************************************************************
  * @file    uart_pkt.c
  * @brief   串口数据包的接收（中断）与解析（主循环）
  ******************************************************************************
  * 分工原则（对应作业"程序要求 2"）：
  *   - HAL_UART_RxCpltCallback 里只做两件事：收字节、凑够一帧就交给主循环；
  *   - 校验、解析、改状态、回传全部放在 UartPkt_Task()（主循环上下文）。
  ******************************************************************************
  */

#include "main.h"
#include "board_config.h"
#include "uart_pkt.h"
#include <stdio.h>
#include <string.h>

#define TX_BUF_SIZE     64u

/* ------------------------- 中断侧 ------------------------- */
static uint8_t rx_byte;                 /* HAL_UART_Receive_IT 的落脚点 */

static volatile struct
{
    Pkt_t   pkt;                        /* 正在拼的包 */
    uint8_t idx;
    uint8_t active;                     /* 1 = 已收到帧头，正在拼 */
} s_rx;

static volatile Pkt_t   s_frame;        /* 凑满一帧后放这里 */
static volatile uint8_t s_frame_ready;

/* ------------------------- 主循环侧 ------------------------- */
volatile uint16_t g_pkt_ok;
volatile uint16_t g_pkt_err;

static volatile uint8_t s_tx_busy;      /* 上一包还在发 */
static char s_tx_buf[TX_BUF_SIZE];      /* 必须全局：中断发送是异步的 */

/* ======================= 接收回调 ======================= */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint8_t b;

    if (huart->Instance != USART1)
    {
        return;
    }

    b = rx_byte;

    if (s_rx.active == 0u)
    {
        /* 找帧头，不是 0xAA 的字节直接扔掉 */
        if (b == PKT_HEAD)
        {
            s_rx.pkt.raw[0] = b;
            s_rx.idx = 1u;
            s_rx.active = 1u;
        }
    }
    else
    {
        s_rx.pkt.raw[s_rx.idx] = b;
        s_rx.idx++;
        if (s_rx.idx >= PKT_LEN)
        {
            if (s_frame_ready == 0u)             /* 主循环还没取走才覆盖 */
            {
                uint8_t i;
                for (i = 0u; i < PKT_LEN; i++)
                {
                    s_frame.raw[i] = s_rx.pkt.raw[i];
                }
                s_frame_ready = 1u;
            }
            s_rx.active = 0u;
            s_rx.idx = 0u;
        }
    }

    /* 关键：单字节接收是一次性的，必须重新挂一次，否则只收到一包 */
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1u);
}

/* 溢出/噪声/校验错误时 HAL 会中止接收，这里把它重新挂上，否则串口就"死了" */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
    {
        return;
    }

    __HAL_UART_CLEAR_OREFLAG(huart);
    s_rx.active = 0u;
    s_rx.idx = 0u;
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1u);
}

/* ======================= 发送完成回调 ======================= */
/* 中断发送与 DMA 发送共用同一个完成回调，所以业务代码完全不用改 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        s_tx_busy = 0u;
    }
}

/* ======================= 解码（纯函数，便于单测） ======================= */
PktStatus_t UartPkt_Decode(const Pkt_t *pkt, FlowCmd_t *cmd)
{
    uint16_t sum;
    uint16_t step;

    if (pkt->f.head != PKT_HEAD)
    {
        return PKT_ERR_HEAD;
    }

    /* CHECK = (0xAA + RUN + DIR + STEP_L + STEP_H) % 256，只留低 8 位 */
    sum = (uint16_t)pkt->raw[0] + (uint16_t)pkt->raw[1] + (uint16_t)pkt->raw[2]
        + (uint16_t)pkt->raw[3] + (uint16_t)pkt->raw[4];
    if ((uint8_t)(sum & 0x00FFu) != pkt->f.check)
    {
        return PKT_ERR_CHECK;
    }

    if ((pkt->f.run > 1u) || (pkt->f.dir > 1u))
    {
        return PKT_ERR_FLAG;
    }

    /* 低字节在前，高字节在后 */
    step = (uint16_t)(((uint16_t)pkt->f.step_h << 8) | pkt->f.step_l);
    if ((step < STEP_MIN_MS) || (step > STEP_MAX_MS))
    {
        return PKT_ERR_STEP;
    }

    cmd->run     = pkt->f.run;
    cmd->dir     = pkt->f.dir;
    cmd->step_ms = step;
    return PKT_OK;
}

/* ======================= 反馈发送 ======================= */
/* 把 s_tx_buf 的前 n 个字节用中断方式发出去 */
static void Uart_SendBuf(uint16_t n)
{
    if ((s_tx_busy != 0u) || (n == 0u) || (n >= TX_BUF_SIZE))
    {
        return;                                  /* 忙就先不发，下一轮再来 */
    }

    s_tx_busy = 1u;
#if (FB_USE_DMA == 1)
    /* 选做一：DMA 版。CPU 只发起一次搬运，字节由 DMA1 通道 4 送到串口数据寄存器；
       发完由 DMA 传输完成中断 + 串口的 TC 中断一起收尾，最后进 TxCpltCallback */
    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)s_tx_buf, n) != HAL_OK)
#else
    /* 基础版：中断发送，每发一个字节进一次中断（作业默认要求） */
    if (HAL_UART_Transmit_IT(&huart1, (uint8_t *)s_tx_buf, n) != HAL_OK)
#endif
    {
        s_tx_busy = 0u;                          /* 没发出去就别把发送锁死 */
    }
}

static void Uart_SendStatus(void)
{
    int n = snprintf(s_tx_buf, TX_BUF_SIZE, "STATE=%s,DIR=%s,STEP=%u,LED=%u\r\n",
                     (g_flow.run != 0u) ? "RUN" : "STOP",
                     (g_flow.dir != 0u) ? "REV" : "FWD",
                     (unsigned)g_flow.step_ms,
                     (unsigned)Flow_LedNumber());

    if (n > 0)
    {
        Uart_SendBuf((uint16_t)n);
    }
}

/* 出错反馈：便于在串口助手上看出"是没收到还是被丢弃了" */
static void Uart_SendError(PktStatus_t st)
{
    static const char *const msg[] =
    {
        "", "ERR:HEAD\r\n", "ERR:CHECK\r\n", "ERR:FLAG\r\n", "ERR:STEP\r\n"
    };

    if ((uint8_t)st < (uint8_t)(sizeof(msg) / sizeof(msg[0])))
    {
        (void)strncpy(s_tx_buf, msg[(uint8_t)st], TX_BUF_SIZE - 1u);
        s_tx_buf[TX_BUF_SIZE - 1u] = '\0';
        Uart_SendBuf((uint16_t)strlen(s_tx_buf));
    }
}

/* ======================= 初始化 ======================= */
void UartPkt_Init(void)
{
    s_rx.active      = 0u;
    s_rx.idx         = 0u;
    s_frame_ready    = 0u;
    s_tx_busy        = 0u;
    g_pkt_ok         = 0u;
    g_pkt_err        = 0u;

    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1u);
}

/* ======================= 主循环任务 ======================= */
void UartPkt_Task(void)
{
    /* 1) 有新帧：先解析，状态改了会给 g_fb_pending 置位 */
    if ((s_frame_ready != 0u) && (s_tx_busy == 0u))
    {
        Pkt_t     pkt;
        PktStatus_t st;
        FlowCmd_t cmd;
        uint8_t   i;

        for (i = 0u; i < PKT_LEN; i++)
        {
            pkt.raw[i] = s_frame.raw[i];
        }
        s_frame_ready = 0u;

        st = UartPkt_Decode(&pkt, &cmd);
        if (st == PKT_OK)
        {
            g_pkt_ok++;
            Flow_Apply(&cmd);        /* 校验通过才动状态 */
        }
        else
        {
            g_pkt_err++;
            Uart_SendError(st);      /* 非法帧：不改状态，只回错误码 */
        }
    }

    /* 2) 待发反馈：串口设置生效后 / 按键改了状态后，都在这里统一回传 */
    if ((g_fb_pending != 0u) && (s_tx_busy == 0u))
    {
        g_fb_pending = 0u;
        Uart_SendStatus();
    }
}
