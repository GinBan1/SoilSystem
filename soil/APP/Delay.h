#ifndef __DELAY_H__
#define __DELAY_H__

#include "bsp.h"
#include "main.h"

typedef struct {
    uint32_t start_time;
    uint8_t is_running;
} Delay_t;

void Delay_Start(Delay_t *delay);
uint8_t Delay_Check(Delay_t *delay, uint32_t delay_time_ms);



#endif

