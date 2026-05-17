#include "key.h"

uint8_t key_num;
uint8_t key_Flag;//按键标志位
/*按键标志位检测函数，传入掩码，返回1或0*/
uint8_t key_CheckNum(uint8_t Flag)
{
    if(key_Flag & Flag) //从按键的所有标志位中按掩码取出的某一位是否是1
    {
        if(Flag != KEY_HOLD)//除KEY_HOLD外的标志位，读取后清除
        {
            key_Flag &= ~Flag;//清除标志位
        }
        return 1;
    }
    return 0;
}
/*获取按键状态*/
uint8_t key_Getstate(void)
{
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_3) == GPIO_PIN_RESET)
    {
        return KEY_PRESSED;
    }
    return KEY_UNRELESSED;
}
/*定时器中断调用，1ms进入一次*/
void key_Tick(void)
{
    static uint8_t count;
    static uint8_t PrevState;//先前状态
    static uint8_t CurrentState;//当前状态

    count++;
    if(count >= 20)
    {
        count = 0;
        PrevState = CurrentState;
        CurrentState = key_Getstate();
        if(CurrentState == KEY_PRESSED)
        {
            if(PrevState == KEY_UNRELESSED)
            {
                //HOLD = 1;
                key_Flag |= KEY_HOLD;//0x01

            }
            else
            {
                //HOLD = 0;
                key_Flag &= ~KEY_HOLD;//清除保持标志
            }
        }
        if(CurrentState == KEY_PRESSED && PrevState == KEY_UNRELESSED)
        {
            //DOWN = 1;
            key_Flag |= KEY_DOWN;//0x02
        }
        if(CurrentState == KEY_UNRELESSED && PrevState == KEY_PRESSED)
        {
            //UP = 1;
            key_Flag |= KEY_UP;//0x04
        }
    }
}