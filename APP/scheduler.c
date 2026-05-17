#include "scheduler.h"
/*结构体*/
typedef struct
{
    void (*task_func)(void);
    uint32_t rate_ms;//运行周期
    uint32_t last_run;//上次运行时间
}task_t;

uint8_t task_num;//发送间隔变量
unsigned char timCount;//发送间隔变量（由TIM2中断置1）

/*存储结构体和结构体名称*/
static task_t scheduler_task[] =
{
	{oled_task,           20,   0},
	{soil_moisture_proc,  1000, 0},	// 湿度传感器采集，1秒一次
	{Bun_AutoCheck,       1000, 0},	// 自动灌溉判断，1秒一次
};

/*结构体初始化*/
void scheduler_init()
{
    task_num = sizeof(scheduler_task) / sizeof(task_t);//数据大小/结构体大小
}
/*调度器运行函数*/
void scheduler_run()
{
    for(uint8_t i=0;i<task_num;i++)
    {
        uint32_t now_time = HAL_GetTick();
        if(now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_run)
        {
            scheduler_task[i].last_run = now_time;//更新时间
            scheduler_task[i].task_func();
        }
    }
}
