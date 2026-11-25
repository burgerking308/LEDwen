#ifndef UART_PKT_H
#define UART_PKT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "flow.h"

/* 数据包：AA RUN DIR STEP_L STEP_H CHECK，共 6 字节 */
#define PKT_HEAD        0xAAu
#define PKT_LEN         6u

/**
 * @brief 数据包结构体（作业要求"自行定义结构体接收数据包"）。
 *        union 的作用：既能按字段读，又能按字节填 ——
 *        中断里按 raw[] 逐字节往里塞，解析时按 f.xxx 读，不用做位运算。
 */
typedef union
{
    struct
    {
        uint8_t head;
        uint8_t run;
        uint8_t dir;
        uint8_t step_l;      /* 间隔低字节 */
        uint8_t step_h;      /* 间隔高字节 */
        uint8_t check;
    } f;
    uint8_t raw[PKT_LEN];
} Pkt_t;

typedef enum
{
    PKT_OK = 0,
    PKT_ERR_HEAD,
    PKT_ERR_CHECK,
    PKT_ERR_FLAG,
    PKT_ERR_STEP
} PktStatus_t;

extern volatile uint16_t g_pkt_ok;      /* 累计收到的合法包数，调试用 */
extern volatile uint16_t g_pkt_err;     /* 累计丢弃的非法包数 */

void        UartPkt_Init(void);          /* 启动接收中断 */
void        UartPkt_Task(void);          /* 主循环调用：解析 + 应用 + 反馈 */
PktStatus_t UartPkt_Decode(const Pkt_t *pkt, FlowCmd_t *cmd);   /* 纯解码，不动作 */

#ifdef __cplusplus
}
#endif

#endif /* UART_PKT_H */
