#include "connet_wifi.h"

#define ESP8266_WIFI_INFO		"AT+CWJAP=\"88\",\"12345678\"\r\n"

extern UART_HandleTypeDef huart2;

volatile uint8_t esp8266_rev_flag = 0;  // 接收完成标志
uint16_t esp8266_rev_len = 0;           // 接收到的数据长度
unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0;
extern Delay_t my_delay;
//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;
	esp8266_rev_flag = 0;  //同步清除标志位，防止残留标志干扰下次接收

}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{


    if(esp8266_rev_flag == 1)  //接收完成
    {
        esp8266_rev_flag = 0;
        esp8266_cnt = esp8266_rev_len;  // 更新接收计数
        return REV_OK;	//返回接收完成标志位
    }
	return REV_WAIT;								//返回接收未完成标志

}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{

	unsigned char timeOut = 200;

    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen((const char *)cmd), 500);

	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存

				return 0;
			}
		}

		HAL_Delay(10);
	}
	return 1;

}

//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];

	ESP8266_Clear();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到’>’时可以发送数据
	{
		HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 500);
	}

}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;

	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索"IPD"头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				//printf("\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;

			}
		}

		HAL_Delay(5);													//延时等待
	} while(timeOut--);

	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：
//==========================================================
void ESP8266_Init(void)
{
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, esp8266_buf, sizeof(esp8266_buf));//启动DMA空闲中断接收

	ESP8266_Clear();

	OLED_Clear();
	my_oledprintf(0,"1.AT...");
	while(ESP8266_SendCmd("AT\r\n", "OK"))
		HAL_Delay(500);

	my_oledprintf(2,"2.CWMODE...");
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
		HAL_Delay(500);


	my_oledprintf(4,"3.AT+CWDHCP...");
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
		HAL_Delay(500);


	my_oledprintf(6,"4.CWJAP...");
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
		HAL_Delay(500);


	OLED_Clear();
	my_oledprintf(0,"ESP8266 Init OK");


}
