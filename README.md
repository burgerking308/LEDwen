# 电控组第三次培训作业 
参考了电控组三次培训所用资料以及豆包的帮助
STATE=RUN,DIR=FWD,STEP=300,LED=1
- `STATE=RUN`：现在是**在跑**还是**停着**。RUN = 在跑，STOP = 停了
- `DIR=FWD`：**往哪个方向走**。FWD = 1→2→3 这个方向，REV = 反过来
- `STEP=300`：**多久换一次灯**，单位毫秒，300 就是每 300 毫秒换一颗
- `LED=1`：**现在亮的是第几颗灯**。就是肉眼数的第 1、2、3 颗灯
```c
static void Uart_SendStatus(void)
{
    int n = snprintf(s_tx_buf, TX_BUF_SIZE, "STATE=%s,DIR=%s,STEP=%u,LED=%u\r\n",
                     (g_flow.run != 0u) ? "RUN" : "STOP",
                     (g_flow.dir != 0u) ? "REV" : "FWD",
                     (unsigned)g_flow.step_ms,
                     (unsigned)Flow_LedNumber());  
```
从当前的 “总状态单子”（`g_flow`）里，把运行状态、方向、间隔、第几颗灯各取出来，按固定顺序拼成一句话，然后通过串口发出去。
**郝博文**
**20263001713**
