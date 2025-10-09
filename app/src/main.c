#include "hal/led.h"
#include "hal/joystick.h"
#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>

int main(void){

// led_setBrightness(0);
// led_setBrightness(1);
// led_heartbeat();

struct timespec delay = {0,250000000};
printf("Welcome!\n");
printf("When the LEDs light up, press the joystick in that direction!(Press left or right to exit)\n");
bool exit = 1;
int fd = open("/dev/spidev0.0", O_RDWR);
if (fd < 0) { perror("open"); return 1; }

while(exit == 1){

int x  = read_direction(fd);

// if(x == -1){
//     printf("Please leave the joycon in the middle.");
//     continue;
// }
if( strcmp(getX(),"LEFT") == 0|| strcmp(getX(), "RIGHT") == 0){
    printf("Exiting game.");
    exit =0;
    continue;
}

for(int i = 0;i<3;++i){
led_setRedBrightness(0);
led_setGreenBrightness(1);
nanosleep(&delay,(struct timespec*) NULL);
led_setGreenBrightness(0);
led_setRedBrightness(1);
nanosleep(&delay,(struct timespec*) NULL);
}
led_setRedBrightness(0);

 usleep(20000);

}
close(fd);
return 0;
}