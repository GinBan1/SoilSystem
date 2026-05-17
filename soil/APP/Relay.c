#include "Relay.h"

uint8_t Bun_state = 0;			// 水泵状态：0=关 1=开
uint8_t Bun_Flag  = 0;			// 手动指令标志：1=收到过App手动开关命令
uint8_t pump_on_humi  = 30;		// 开启水泵湿度阈值（默认30%）
uint8_t pump_off_humi = 70;		// 关闭水泵湿度阈值（默认70%）

static uint8_t auto_lock = 0;	// 手动命令锁定计数：收到手动命令后若干周期内跳过自动控制

void Bun_init(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
}

void Bun_Set(uint8_t state)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, state == Bun_On ? GPIO_PIN_SET : GPIO_PIN_RESET);
	Bun_state = state;
}

//==========================================================
//	函数名称：	Bun_AutoCheck
//	函数功能：	根据土壤湿度和阈值自动控制水泵
//				湿度 < pump_on_humi  → 开启水泵
//				湿度 > pump_off_humi → 关闭水泵
//				中间地带保持当前状态（避免频繁开关）
//	调用周期：	建议 1 秒调用一次
//==========================================================
void Bun_AutoCheck(void)
{
	extern uint8_t soil_value;

	// 手动命令优先：收到App手动开关后，锁定5个周期（5秒）不自动干预
	// 不清除Bun_Flag，由主循环consume（force_upload后清零）
	if(Bun_Flag != 0)
	{
		auto_lock = 5;
		return;
	}

	if(auto_lock > 0)
	{
		auto_lock--;
		return;
	}

	// 阈值安全检查：开启值必须小于关闭值，否则不动作
	if(pump_on_humi >= pump_off_humi)
		return;

	// 自动灌溉逻辑（滞回控制）
	if(soil_value < pump_on_humi && Bun_state == Bun_Off)
	{
		Bun_Set(Bun_On);
	}
	else if(soil_value > pump_off_humi && Bun_state == Bun_On)
	{
		Bun_Set(Bun_Off);
	}
	// 中间地带（pump_on_humi <= soil_value <= pump_off_humi）：保持当前状态
}
