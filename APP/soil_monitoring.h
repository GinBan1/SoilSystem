#ifndef SOIL_MOISTURING_H
#define SOIL_MOISTURING_H

#include "bsp.h"
#include "math.h"

#define TS_READ_TIMES	    9   //土壤湿度ADC循环读取次数(奇数，用于中值滤波)

// 传感器电源控制引脚 —— 传感器VCC接PA5，仅在读取时通电，防止电解腐蚀
#define SOIL_PWR_Pin        GPIO_PIN_5
#define SOIL_PWR_GPIO_Port  GPIOA

uint8_t TS_GetData(void);
void soil_moisture_proc(void);
void test_sensor_range(void);
void soil_sensor_init(void);    // 初始化传感器电源引脚

#endif /* __ADC_H */
