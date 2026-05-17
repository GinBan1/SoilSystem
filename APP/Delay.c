#include "Delay.h"

Delay_t my_delay;

// 初始化延时
void Delay_Start(Delay_t *delay)
{
    delay->start_time = HAL_GetTick();
    delay->is_running = 1;
}

// 检查延时是否完成
uint8_t Delay_Check(Delay_t *delay, uint32_t delay_time_ms)
{
    if (!delay->is_running) return 0;
    if (HAL_GetTick() - delay->start_time >= delay_time_ms)
    {
        delay->is_running = 0;
        return 1;
    }
    return 0;
}