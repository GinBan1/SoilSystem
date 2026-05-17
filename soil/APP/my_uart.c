#include "my_uart.h"
//ptintf重定向
int fputc(int ch,FILE *f)
{
	HAL_UART_Transmit(&huart1,(uint8_t*)&ch,1,10);//超时从HAL_MAX_DELAY改为10ms，防止死等
	return ch;
}
