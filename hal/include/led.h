#ifndef HAL_LED_H

#define HAL_LED_H

#include<stdbool.h>

void led_initiate(void);

void led_setGreenBrightness(bool brightness);

void led_setRedBrightness(bool brightness);

void led_heartbeat(void);



#endif