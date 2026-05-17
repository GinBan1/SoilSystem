#ifndef __DXCT511_H__
#define __DXCT511_H__

#include "bsp.h"
#include "Common.h"

#define DXCT511_REV_OK    0
#define DXCT511_REV_WAIT  1

extern volatile uint8  dxct511_rev_flag;
extern uint16          dxct511_rev_len;
extern unsigned char   dxct511_buf[1024];

void     DXCT511_Clear(void);
_Bool    DXCT511_SendCmd(char *cmd, char *res);
void     DXCT511_SendRaw(unsigned char *data, unsigned short len);
unsigned char *DXCT511_GetData(unsigned short timeOut);

_Bool   DXCT511_MqttConnect(void);
_Bool   DXCT511_MqttSubscribe(const int8 *topic, uint8 qos);
_Bool   DXCT511_MqttPublish(const int8 *topic, const int8 *msg, uint32 len);
void    DXCT511_MqttDisconnect(void);
void    DXCT511_MqttClose(void);
_Bool   DXCT511_CheckStatus(void);
void    DXCT511_Init(void);

void    DXCT511_RevPro(unsigned char *data);
void    DXCT511_SendSensorData(void);

#endif
