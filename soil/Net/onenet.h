#ifndef __ONENET_H__
#define __ONENET_H__

#include "bsp.h"

extern uint8_t soil_value;

_Bool OneNet_DevLink(void);
void OneNET_Subscribe(void);
void OneNet_SendData(void);
void OneNet_RevPro(unsigned char *cmd);

#endif