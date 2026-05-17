#include "soil_monitoring.h"

uint8_t soil_value = 0;

// ===== 传感器电源控制（防电解腐蚀） =====
// 仅在读取时通电，其余时间断电，探头99%时间不带电

void soil_sensor_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = SOIL_PWR_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(SOIL_PWR_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SOIL_PWR_GPIO_Port, SOIL_PWR_Pin, GPIO_PIN_RESET);
}

static void soil_power_on(void)
{
    HAL_GPIO_WritePin(SOIL_PWR_GPIO_Port, SOIL_PWR_Pin, GPIO_PIN_SET);
    HAL_Delay(50);  // 等50ms让传感器输出稳定
}

static void soil_power_off(void)
{
    HAL_GPIO_WritePin(SOIL_PWR_GPIO_Port, SOIL_PWR_Pin, GPIO_PIN_RESET);
}

uint16_t TS_ADC_Read(void)
{
	//设置指定的ADC的规则组通道，采样时间
    uint16_t adc_value = 0;
    
    // 启动ADC转换
    HAL_ADC_Start(&hadc1);
    
    // 等待转换完成
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1);
    }
    
    HAL_ADC_Stop(&hadc1);
    return adc_value;
}

// ===== 校准参数 —— 运行 test_sensor_range() 后填入实际值！ =====
// 不同传感器模块的分压电阻不同，必须用自己的传感器校准！
#define CAL_AIR_ADC     3967   // 空气中ADC(最干) —— 需校准！
#define CAL_WATER_ADC   1215   // 水中ADC(最湿)   —— 需校准！

// ===== 通用脱落检测：ADC贴到电源轨 = 引脚悬空 =====
// 正常传感器输出不会达到这个极端值
#define FLOAT_HIGH_ADC  4000   // ADC>4000(接近3.3V) → 引脚悬空被上拉
#define FLOAT_LOW_ADC    100   // ADC<100(接近0V)  → 引脚对地短路/悬空下拉

// EMA滤波系数: 越小越平滑但响应越慢
#define EMA_ALPHA       0.15f

// 简易冒泡排序（9个元素，Cortex-M3足够）
static void sort_samples(uint16_t *s, uint8_t n)
{
	for (uint8_t i = 0; i < n - 1; i++)
		for (uint8_t j = i + 1; j < n; j++)
			if (s[i] > s[j]) { uint16_t t = s[i]; s[i] = s[j]; s[j] = t; }
}

uint8_t TS_GetData(void)
{
	// 1. 传感器上电，等待稳定后采集样本
	soil_power_on();
	uint16_t samples[TS_READ_TIMES];
	for (uint8_t i = 0; i < TS_READ_TIMES; i++)
	{
		samples[i] = TS_ADC_Read();
		HAL_Delay(5);
	}
	soil_power_off();  // 读完立即断电，减少电解

	// 2. 中值滤波：排序后取中间值，彻底剔除偶发噪声尖峰
	sort_samples(samples, TS_READ_TIMES);
	uint16_t adc_median = samples[TS_READ_TIMES / 2];

	// 3. 通用脱落/短路检测（基于电源轨，不依赖校准值）
	if (adc_median >= FLOAT_HIGH_ADC || adc_median <= FLOAT_LOW_ADC)
	{
		return 0; // 引脚悬空/短路 → 0%（物理：无介质=无湿度）
	}

	// 4. 线性映射到0-100%
	float span = (float)(CAL_AIR_ADC - CAL_WATER_ADC);
	if (span <= 0) return 0;
	float moisture = (float)(CAL_AIR_ADC - adc_median) / span * 100.0f;
	if (moisture < 0) moisture = 0;
	if (moisture > 100) moisture = 100;

	// 5. EMA滤波平滑输出
	static uint8_t ema_inited = 0;
	static float   ema_value = 0;
	if (!ema_inited)
	{
		ema_value  = moisture;
		ema_inited = 1;
	}
	else
	{
		ema_value = ema_value * (1.0f - EMA_ALPHA) + moisture * EMA_ALPHA;
	}

	return (uint8_t)(ema_value + 0.5f);
}
/*测量传感器实际ADC范围——用于校准(修剪平均，抗噪声)*/
void test_sensor_range(void)
{
    uint16_t samples[50];
    uint32_t sum;
    uint16_t cal_air, cal_water;

    printf("===== 传感器校准开始 =====\r\n");
    printf("请将探头擦拭干净，暴露在空气中...\r\n");
    HAL_Delay(3000);

    // 空气中采集50个样本
    soil_power_on();
    for(int i=0; i<50; i++) {
        samples[i] = TS_ADC_Read();
        HAL_Delay(10);
    }
    soil_power_off();
    // 排序后去掉最高最低各5个，取中间40个平均
    sort_samples(samples, 50);
    sum = 0;
    for(int i=5; i<45; i++) sum += samples[i];
    cal_air = (uint16_t)(sum / 40);

    printf("空气中ADC(修剪平均): %d\r\n", cal_air);
    printf("请将探头完全浸入水中，等待提示...\r\n");
    HAL_Delay(8000);

    // 水中采集50个样本
    soil_power_on();
    for(int i=0; i<50; i++) {
        samples[i] = TS_ADC_Read();
        HAL_Delay(10);
    }
    soil_power_off();
    sort_samples(samples, 50);
    sum = 0;
    for(int i=5; i<45; i++) sum += samples[i];
    cal_water = (uint16_t)(sum / 40);

    printf("水中ADC(修剪平均): %d\r\n", cal_water);
    printf("实际量程: %d(干) ~ %d(湿)\r\n", cal_air, cal_water);
    printf("传感器有效跨度: %d\r\n", cal_air - cal_water);
    printf(">> 将 soil_monitoring.c 中 CAL_AIR_ADC 改为 %d, CAL_WATER_ADC 改为 %d <<\r\n", cal_air, cal_water);
    printf("===== 校准完成 =====\r\n");
}

void soil_moisture_proc(void)
{
    soil_value = TS_GetData();  //TS_GetData已返回uint8_t 0-100
	// 校准步骤：取消下面注释，观察串口输出找到空气/水中的ADC值
	// printf("soil=%d, rawADC=%d\r\n", soil_value, TS_ADC_Read());
}
