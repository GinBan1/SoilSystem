#ifndef __KEY_H__
#define __KEY_H__

#include "bsp.h"

#define KEY_PRESSED  1
#define KEY_UNRELESSED 0

/*标志位掩码*/
#define KEY_HOLD  0x01 //按键按下保持标志
#define KEY_DOWN  0x02 //按键按下标志
#define KEY_UP    0x04 //按键释放标志

uint8_t key_CheckNum(uint8_t Flag);

#endif

