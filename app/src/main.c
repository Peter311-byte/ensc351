#include "led.h"
#include "joystick.h"
#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>
#include <stdlib.h>

/// global variables for main/////////
static bool exitLoop = 1;
bool running = 1;
int state = 0;
Direction chosenDirection;
Direction oppDirection;
long long best_time = -1;
int won;
/////////////////////////////////////

void set_LEDGREEN(void){
  led_setGreenBrightness(1);
  usleep(100000);  
  led_setGreenBrightness(0);
  usleep(100000);  


}

void set_LEDRED(void){
  led_setRedBrightness(1);
  usleep(100000);  
  led_setRedBrightness(0);
  usleep(100000);  

}

static inline long long now_ms_monotonic(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static inline void sleep_ms(int ms) {
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
  nanosleep(&ts, NULL);
}


static int wait_for_center_stable(int fd, int needed, int poll_ms, int timeout_ms) {
  int consec = 0;
  long long t0 = now_ms_monotonic();
  while ((now_ms_monotonic() - t0) < timeout_ms) {
    read_direction(fd);             
    if (isCenter() == 1) consec++;  
    else consec = 0;
    if (consec >= needed) return 1; 
    sleep_ms(poll_ms);              
  }
  return 0; 
}

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
      printf("You did it! \n"); // move these two lines later//
      state = 3;
      exitLoop = 0;
      won = 1;
      continue;
    } else if (y == oppDirection){
      printf("Wrong Direction!\n");
      state = 3;
      exitLoop = 0;
      won = 0;
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
  won = -1;

   int r = rand() % 2;
   chosenDirection = (r == 0) ? DIR_UP: DIR_DOWN;
   printf("Get ready...\n");
   
  
//regular center checks

  sleep(2); 
  if (!wait_for_center_stable(fd, 4, 20, 1000)) {
    printf("Please leave the joystick in the middle.\n");
    int z = wait_for_center_stable(fd,4,20,3000);
    if(!z){
      printf("Too soon!\n");
      exitLoop = 0;
      running = 1;
      state = 1;
      continue;
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

    led_setGreenBrightness(0);
    led_setRedBrightness(0);

  if(state!=2){
    if(best_time == -1 && won == 1){
      best_time = reaction_time;
    }

    if(won == 1){
      set_LEDGREEN();
    }else{
      set_LEDRED();
    }

    if(reaction_time<best_time && won == 1){
      best_time = reaction_time;
      printf("New Best time!\n");
    }else if(won == 1){
      printf("Your time is %lld ms\n", reaction_time);
      printf("Current best time is %lld ms\n", best_time);

    }
       
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

    led_setGreenBrightness(0);
    led_setRedBrightness(0);
  
   if(state!=2){
    if(best_time == -1 && won == 1){
      best_time = reaction_time;
    }

     if(won == 1){
      set_LEDGREEN();
    }else{
      set_LEDRED();
    }

    if(reaction_time<best_time && won == 1){
      best_time = reaction_time;
      printf("New Best time!\n");
    } else if(won == 1){
      printf("Your time is %lld ms\n", reaction_time);
      printf("Current best time is %lld ms\n", best_time);

    }
      
  }
    
}
break;

 }
}

}