#include "hal/led.h"
#include <stdio.h>
#include <stdlib.h>


#define LED_GREEN_BRIGHT "/sys/class/leds/ACT/brightness"
#define LED_TRIGGER "/sys/class/leds/ACT/trigger"
#define LED_RED_BRIGHT "/sys/class/leds/PWR/brightness"


void led_setGreenBrightness(bool brightness){

    FILE*file =  fopen(LED_GREEN_BRIGHT,"w");
     if (!file) return; 

    fprintf(file, "%d", brightness ? 1 : 0);
    
    fclose(file);
}

void led_setRedBrightness(bool brightness){
    FILE*file =  fopen(LED_RED_BRIGHT,"w");
     if (!file) return; 

    fprintf(file, "%d", brightness ? 1 : 0);
    
    fclose(file);
}

void led_heartbeat(void){
    FILE*triggerfile = fopen(LED_TRIGGER,"w");
    fprintf(triggerfile,"heartbeat");
    fclose(triggerfile);
}