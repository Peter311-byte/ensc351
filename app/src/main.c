#include "led.h"
#include "joystick.h"
#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>
#include <stdlib.h>

/// global variables for main
static bool exitLoop = 1;
bool running = 1;
int state = 0;
Direction chosenDirection;
Direction oppDirection;
long long best_time = -1;

///////////////////////////

static long long getTimeInMs(void)
{
 struct timespec spec;
 clock_gettime(CLOCK_REALTIME, &spec);
 long long seconds = spec.tv_sec;
 long long nanoSeconds = spec.tv_nsec;
 long long milliSeconds = seconds * 1000
 + nanoSeconds / 1000000;
 return milliSeconds;
}


void set_led(void){
 struct timespec delay = {0,250000000};
  for(int i = 0;i<3;++i){
  led_setRedBrightness(0);
  led_setGreenBrightness(1);
  nanosleep(&delay,(struct timespec*) NULL);
  led_setGreenBrightness(0);
  led_setRedBrightness(1);
  nanosleep(&delay,(struct timespec*) NULL);
}
led_setRedBrightness(0);
led_setGreenBrightness(0);
}

int openFile(void){
  int fd = open("/dev/spidev0.0", O_RDWR);
  if (fd < 0) { perror("open"); return -1; }
  
  return fd;
}

void closeFile(int x){
  close(x);
}

void joystick_task(int fd){
  while(exitLoop == 1){
   int r  = read_direction(fd);
   Direction x = getX();
   Direction y = getY();
  if(r==2){ // if they are holding up at the very beginning of the launch
    printf("Please leave the joystick in the middle.\n");
    sleep(3);
    int y = read_direction(fd);
      if(y == 2){
        printf("Too soon!\n");
        exitLoop = 0;
        running = 1;
        state = 1;
        continue;
      }
  }

  if((x == DIR_LEFT || x == DIR_RIGHT) && (y != DIR_DOWN && y!= DIR_UP)){
    state = 2;
    exitLoop =0;
    continue;  
  }
  if(state!=3){
    state =3;
    exitLoop = 0;
    continue;
  }
   

  if(state == 3){
    if(y == chosenDirection){
      printf("You did it! \n"); // move these two lines later
      led_setGreenBrightness(1);//
      state = 3;
      exitLoop = 0;
      continue;
    } else if (y == oppDirection){
      printf("Wrong Direction!\n");
      led_setRedBrightness(1);
      state = 3;
      exitLoop = 0;
      continue;

    }
  }

 usleep(20000);
}
}

int main(void){
srand(time(NULL));
int fd = openFile();
while(running == 1){
  switch(state){
  case 0:
    printf("Welcome!\n");
    printf("When the LEDs light up, press the joystick in that direction!(Press left or right to exit)\n");
    set_led();
    joystick_task(fd);
    break;
  case 1:
    state = 0;
    exitLoop = 1;
    break;
  
  case 2:
    printf("Exiting game.\n");
    running = 0;
    closeFile(fd);
    return 0;
    break;
  
  case 3:
  led_setGreenBrightness(0);
  led_setRedBrightness(0);

   int r = rand() % 2;
   chosenDirection = (r == 0) ? DIR_UP: DIR_DOWN;
   printf("Get ready...\n");
   
  
//regular center checks
  sleep(1); // give time to reset
  read_direction(fd);
  if(isCenter() != 1){//check if joystick is center
    printf("Please leave the joystick in the middle.\n");
    sleep(3);
    read_direction(fd); //if not wait for center
    if(isCenter() !=  1){ 
      printf("Too soon!\n");
        exitLoop = 0;
        running = 1;
        state = 1;
        break;
    }  
  }


   if(chosenDirection == DIR_UP){
    oppDirection = DIR_DOWN;
    led_setRedBrightness(0);
    led_setGreenBrightness(1);
    printf("Press UP now!\n");
    long long timeBefore = getTimeInMs();
    exitLoop = 1;
    joystick_task(fd);
    long long timeAfter = getTimeInMs();
    long long reaction_time = timeAfter - timeBefore;


  if(state!=2){
    if(best_time == -1){
      best_time = reaction_time;
    }

    if(reaction_time<best_time){
      best_time = reaction_time;
      printf("New Best time!");
    }
      printf("Your time is %lld ms\n", reaction_time);
      printf("Current best time is %lld ms\n", best_time);
  }
    
    
    
   }else if(chosenDirection == DIR_DOWN){
    oppDirection = DIR_UP;
    led_setGreenBrightness(0);
    led_setRedBrightness(1);
    printf("Press DOWN now!\n");
    long long timeBefore = getTimeInMs();
    exitLoop = 1;
    joystick_task(fd);
    long long timeAfter = getTimeInMs();
    long long reaction_time = timeAfter - timeBefore;

  
   if(state!=2){
    if(best_time == -1){
      best_time = reaction_time;
    }

    if(reaction_time<best_time){
      best_time = reaction_time;
      printf("New Best time!\n");
    }
      printf("Your time is %lld ms\n", reaction_time);
      printf("Current best time is %lld ms\n", best_time);
  }
    
}



break;

}
}

}