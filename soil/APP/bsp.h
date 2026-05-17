#ifndef __BSP_H__
#define __BSP_H__

#include "main.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "adc.h"
#include "usart.h"
#include "scheduler.h"
#include "Mqtt.h"
#include "connet_wifi.h"
#include "cJSON.h"
#include "base64.h"
#include "hmac_sha1.h"
#include "onenet.h"
#include "Delay.h"
#include "key.h"
#include "soil_monitoring.h"
#include "my_uart.h"
#include "oled.h"
#include "Relay.h"
#include "DXCT511.h"

extern uint8_t soil_value;
extern volatile uint8_t mqtt_cmd_Receive;//MQTT命令接收标志
extern volatile uint8_t led_cmd_value ;//LED命令标志值

#endif
