#ifndef __RELAY_H__
#define __RELAY_H__

#include "bsp.h"

#define Bun_On  1
#define Bun_Off 0

void Bun_Set(uint8_t state);
void Bun_init(void);
void Bun_AutoCheck(void);			// 自动灌溉检查（按阈值控制水泵）

extern uint8_t Bun_state;
extern uint8_t Bun_Flag;			// 手动指令标志：1=收到过手动命令
extern uint8_t pump_on_humi;		// 开启水泵湿度阈值（低于开启）
extern uint8_t pump_off_humi;		// 关闭水泵湿度阈值（高于关闭）

#endif
