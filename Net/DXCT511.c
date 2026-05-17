#include "DXCT511.h"
#include "onenet.h"
#include "base64.h"
#include "hmac_sha1.h"
#include "cJSON.h"
#include "Relay.h"
#include <string.h>
#include <stdio.h>

/*---------------------------------------------------------------------------*/
/* OneNET 连接参数                                                             */
/*---------------------------------------------------------------------------*/
#define PROID       "5S6Nv8BF3z"
#define DEVICE_NAME "d1"
#define ACCESS_KEY  "NDRVVEt3eFY5bzJ3RTFMZnNKa0tWM3BFSDZydFpwZ2E="
#define VER         "2018-10-31"
#define METHOD      "sha1"
#define ET          2073561585UL

/*---------------------------------------------------------------------------*/
/* 全局变量                                                                   */
/*---------------------------------------------------------------------------*/
extern UART_HandleTypeDef huart3;

#define RX_BUF_SIZE  1024

volatile uint8  dxct511_rev_flag = 0;
uint16          dxct511_rev_len  = 0;
unsigned char   dxct511_buf[RX_BUF_SIZE];
unsigned short  dxct511_cnt = 0;

extern uint8_t soil_value;
extern uint8_t SenWaitForAck;

/*===========================================================================*/
/* Token生成（无URL编码——DXCT511模块要求）                                      */
/*===========================================================================*/

static void DXCT511_GenToken(char *token, unsigned short token_len)
{
    char sign_buf[64];
    char hmac_sha1_buf[64];
    char access_key_base64[64];
    char string_for_signature[72];
    size_t olen = 0;

    memset(access_key_base64, 0, sizeof(access_key_base64));
    BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64),
                  &olen, (unsigned char *)ACCESS_KEY, strlen(ACCESS_KEY));

    memset(string_for_signature, 0, sizeof(string_for_signature));
    snprintf(string_for_signature, sizeof(string_for_signature),
             "%lu\n%s\nproducts/%s/devices/%s\n%s",
             ET, METHOD, PROID, DEVICE_NAME, VER);

    memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
    hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
              (unsigned char *)string_for_signature, strlen(string_for_signature),
              (unsigned char *)hmac_sha1_buf);

    olen = 0;
    memset(sign_buf, 0, sizeof(sign_buf));
    BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen,
                  (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

    snprintf(token, token_len,
             "version=%s&res=products/%s/devices/%s&et=%lu&method=%s&sign=%s",
             VER, PROID, DEVICE_NAME, ET, METHOD, sign_buf);
}

/*===========================================================================*/
/* 底层通信 —— 参考 connet_wifi.c 的 ESP8266 DMA+Idle 模式                      */
/*===========================================================================*/

void DXCT511_Clear(void)
{
    memset(dxct511_buf, 0, sizeof(dxct511_buf));
    dxct511_cnt = 0;
    dxct511_rev_flag = 0;
}

// 等待DMA空闲中断
static _Bool DXCT511_WaitRecive(void)
{
    if(dxct511_rev_flag == 1)
    {
        dxct511_rev_flag = 0;
        dxct511_cnt = dxct511_rev_len;
        return DXCT511_REV_OK;
    }
    return DXCT511_REV_WAIT;
}

_Bool DXCT511_SendCmd(char *cmd, char *res)
{
    unsigned char timeOut = 250;

    HAL_UART_Transmit(&huart3, (uint8 *)cmd, strlen((const char *)cmd), 500);

    while(timeOut--)
    {
        if(DXCT511_WaitRecive() == DXCT511_REV_OK)
        {
            if(strstr((const char *)dxct511_buf, res) != NULL)
            {
                DXCT511_Clear();
                return 0;
            }
        }
        HAL_Delay(10);
    }
    return 1;
}

unsigned char *DXCT511_GetData(unsigned short timeOut)
{
    char *ptr = NULL;

    do
    {
        if(DXCT511_WaitRecive() == DXCT511_REV_OK)
        {
            ptr = strstr((char *)dxct511_buf, "+MSUB:");
            if(ptr != NULL)
                return (unsigned char *)ptr;
        }
        HAL_Delay(5);
    } while(timeOut--);

    return NULL;
}

void DXCT511_SendRaw(unsigned char *data, unsigned short len)
{
    DXCT511_Clear();
    HAL_UART_Transmit(&huart3, (uint8 *)data, len, 500);
}

/*===========================================================================*/
/* 模块初始化                                                                 */
/*===========================================================================*/

void DXCT511_Init(void)
{
    uint8 retry = 0;

    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dxct511_buf, sizeof(dxct511_buf));
    DXCT511_Clear();

    printf("DX-CT511 Init...\r\n");

    while(DXCT511_SendCmd("AT\r\n", "OK"))
    {
        retry++;
        printf("AT retry %d, cnt=%d, buf=[%s]\r\n", retry, dxct511_cnt, dxct511_buf);
        HAL_Delay(500);
        if(retry >= 15)
        {
            printf("DX-CT511 Init FAIL\r\n");
            return;
        }
    }
    printf("DX-CT511 AT OK\r\n");
}

/*===========================================================================*/
/* MQTT 连接                                                                  */
/*===========================================================================*/

_Bool DXCT511_MqttConnect(void)
{
    char token_buf[192];
    char cmd_buf[384];

    memset(token_buf, 0, sizeof(token_buf));
    DXCT511_GenToken(token_buf, sizeof(token_buf));
    printf("Token: %s\r\n", token_buf);

    printf("1. Set APN...\r\n");
    if(DXCT511_SendCmd("AT+QICSGP=1,1,\"\",\"\",\"\"\r\n", "OK"))
    {
        printf("APN FAIL\r\n");
        return 0;
    }

    printf("2. Open Network...\r\n");
    if(DXCT511_SendCmd("AT+NETOPEN\r\n", "SUCCESS"))
    {
        printf("NETOPEN FAIL\r\n");
        return 0;
    }

    printf("3. Config MQTT Client...\r\n");
    snprintf(cmd_buf, sizeof(cmd_buf),
             "AT+MCONFIG=\"%s\",\"%s\",\"%s\"\r\n",
             DEVICE_NAME, PROID, token_buf);
    if(DXCT511_SendCmd(cmd_buf, "OK"))
    {
        printf("MCONFIG FAIL\r\n");
        return 0;
    }

    printf("4. Config MQTT Server...\r\n");
    if(DXCT511_SendCmd("AT+MIPSTART=\"mqtts.heclouds.com\",1883,4\r\n", "SUCCESS"))
    {
        printf("MIPSTART FAIL\r\n");
        return 0;
    }

    printf("5. MQTT Connect...\r\n");
    if(DXCT511_SendCmd("AT+MCONNECT=1,300\r\n", "SUCCESS"))
    {
        printf("MCONNECT FAIL\r\n");
        return 0;
    }

    printf("MQTT Connect SUCCESS\r\n");
    return 1;
}

_Bool DXCT511_MqttSubscribe(const int8 *topic, uint8 qos)
{
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+MSUB=\"%s\",%d\r\n", topic, qos);
    if(DXCT511_SendCmd(cmd, "SUCCESS"))
    {
        printf("Subscribe FAIL: %s\r\n", topic);
        return 0;
    }
    printf("Subscribe OK: %s\r\n", topic);
    return 1;
}

/*===========================================================================*/
/* MQTT 发布                                                                  */
/*===========================================================================*/

_Bool DXCT511_MqttPublish(const int8 *topic, const int8 *msg, uint32 len)
{
    char cmd[192];

    // 第1步：发MPUBEX，等 >
    snprintf(cmd, sizeof(cmd), "AT+MPUBEX=\"%s\",0,0,%d\r\n", topic, (int)len);
    if(DXCT511_SendCmd(cmd, ">"))
    {
        printf("MPUBEX no '>'\r\n");
        return 0;
    }

    // 第2步：发纯数据（不加\r\n）
    HAL_UART_Transmit(&huart3, (uint8 *)msg, len, 500);

    // 第3步：等 SUCCESS
    {
        unsigned char t = 100;
        while(t--)
        {
            if(DXCT511_WaitRecive() == DXCT511_REV_OK)
            {
                if(strstr((const char *)dxct511_buf, "SUCCESS") != NULL)
                {
                    DXCT511_Clear();
                    return 1;
                }
                if(strstr((const char *)dxct511_buf, "ERROR") != NULL)
                {
                    printf("MPUBEX ERROR\r\n");
                    return 0;
                }
            }
            HAL_Delay(10);
        }
    }
    printf("MPUBEX Timeout\r\n");
    return 0;
}

void DXCT511_MqttDisconnect(void)
{
    DXCT511_SendCmd("AT+MDISCONNECT\r\n", "OK");
}

void DXCT511_MqttClose(void)
{
    DXCT511_SendCmd("AT+MDISCONNECT\r\n", "OK");
    DXCT511_SendCmd("AT+MIPCLOSE\r\n", "OK");
    DXCT511_SendCmd("AT+NETCLOSE\r\n", "OK");
    HAL_Delay(500);
}

/*===========================================================================*/
/* 传感器数据上报                                                              */
/*===========================================================================*/

static uint16 DXCT511_FillBuf(char *buf)
{
    char text[48];
    memset(text, 0, sizeof(text));

    strcpy(buf, "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{");

    memset(text, 0, sizeof(text));
    sprintf(text, "\"soil_value\":{\"value\":%d},", soil_value);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Bun\":{\"value\":%s},", Bun_state ? "true" : "false");
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Pump_On_Humi\":{\"value\":%d},", pump_on_humi);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Pump_Off_Humi\":{\"value\":%d}", pump_off_humi);
    strcat(buf, text);

    strcat(buf, "}}");
    return strlen(buf);
}

void DXCT511_SendSensorData(void)
{
    char buf[256];
    char topic[64];
    uint16 body_len;

    memset(buf, 0, sizeof(buf));
    body_len = DXCT511_FillBuf(buf);

    snprintf(topic, sizeof(topic), "$sys/%s/%s/thing/property/post", PROID, DEVICE_NAME);

    printf("Publish: %s\r\n", buf);
    if(DXCT511_MqttPublish(topic, buf, body_len))
    {
        if(SenWaitForAck < 255) SenWaitForAck++;
    }
}

/*===========================================================================*/
/* 平台下行处理 —— 参考 onenet.c 的 OneNet_RevPro                               */
/*===========================================================================*/

void DXCT511_RevPro(unsigned char *data)
{
    char *ptr = (char *)data;
    char topic[96];
    char *payload_start = NULL;
    char *payload_end = NULL;
    char payload[256];
    unsigned short payload_len = 0;
    unsigned char qos = 0;
    static unsigned short pkt_id = 0;

    // 解析+MSUB: "+MSUB: <topic>",<len> bytes,"<payload>"
    ptr += 7;                           // 跳过"+MSUB: "
    if(*ptr == '"') ptr++;

    {
        char *t = topic;
        while(*ptr && *ptr != '"' && (t - topic) < (int)(sizeof(topic) - 1))
            *t++ = *ptr++;
        *t = '\0';
    }

    payload_start = strchr(ptr, ',');
    if(payload_start == NULL) return;
    payload_start = strchr(payload_start + 1, '"');
    if(payload_start == NULL) return;
    payload_start++;

    // 找payload结束引号（DMA idle隔离，buf中仅一条+MSUB，strrchr安全）
    payload_end = strrchr(payload_start, '"');
    if(payload_end == NULL) return;

    payload_len = (unsigned short)(payload_end - payload_start);
    if(payload_len >= sizeof(payload)) payload_len = sizeof(payload) - 1;
    memcpy(payload, payload_start, payload_len);
    payload[payload_len] = '\0';

    printf("Topic: %s\r\nPayload: %s\r\n", topic, payload);

    // 根据topic类型处理
    if(strstr(topic, "thing/property/set") != NULL
       && strstr(topic, "reply") == NULL)
    {
        cJSON *raw_json = cJSON_Parse(payload);
        if(raw_json == NULL) return;

        cJSON *params_json = cJSON_GetObjectItem(raw_json, "params");
        char reply_topic[96];
        char reply_msg[128];

        if(params_json)
        {
            cJSON *Bun_json = cJSON_GetObjectItem(params_json, "Bun");
            if(Bun_json != NULL)
            {
                if(Bun_json->type == cJSON_True)
                {
                    Bun_Flag = 1;
                    Bun_Set(Bun_On);
                    Bun_state = 1;
                }
                else if(Bun_json->type == cJSON_False)
                {
                    Bun_Flag = 1;
                    Bun_Set(Bun_Off);
                    Bun_state = 0;
                }
            }

            cJSON *json = cJSON_GetObjectItem(params_json, "Pump_On_Humi");
            if(json != NULL)
            {
                if(json->type == cJSON_Number)
                    pump_on_humi = (uint8)json->valuedouble;
                else if(json->type == cJSON_String)
                    pump_on_humi = (uint8)atoi(json->valuestring);
                if(pump_on_humi < 0)  pump_on_humi = 0;
                if(pump_on_humi > 45) pump_on_humi = 45;
            }

            json = cJSON_GetObjectItem(params_json, "Pump_Off_Humi");
            if(json != NULL)
            {
                if(json->type == cJSON_Number)
                    pump_off_humi = (uint8)json->valuedouble;
                else if(json->type == cJSON_String)
                    pump_off_humi = (uint8)atoi(json->valuestring);
                if(pump_off_humi < 45) pump_off_humi = 45;
                if(pump_off_humi > 100) pump_off_humi = 100;
            }
        }

        cJSON *id_json = cJSON_GetObjectItem(raw_json, "id");
        snprintf(reply_topic, sizeof(reply_topic),
                 "$sys/%s/%s/thing/property/set_reply", PROID, DEVICE_NAME);
        snprintf(reply_msg, sizeof(reply_msg),
                 "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\"}",
                 id_json ? id_json->valuestring : "");
        DXCT511_MqttPublish(reply_topic, reply_msg, strlen(reply_msg));

        cJSON_Delete(raw_json);
    }
    else if(strstr(topic, "thing/property/post/reply") != NULL)
    {
        cJSON *raw_json = cJSON_Parse(payload);
        if(raw_json != NULL)
        {
            cJSON *code_json = cJSON_GetObjectItem(raw_json, "code");
            if(code_json != NULL && code_json->valueint == 200)
            {
                SenWaitForAck = 0;
                printf("Post Reply: success\r\n");
            }
            else
            {
                printf("Post Reply: code=%d\r\n",
                       code_json ? code_json->valueint : -1);
            }
            cJSON_Delete(raw_json);
        }
    }

    // 消费完+MSUB后清空缓冲区
    DXCT511_Clear();
}
