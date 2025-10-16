#include "led.h"
#include "joystick.h"
#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<string.h>
#include <stdlib.h>

// Name: Prasanna Loganathan
//Student Number: 301576977

/// ============================================================================
/// Global state (game control + timing)
/// ============================================================================
static bool exitLoop = 1;      // inner loop control for joystick_task
bool running = 1;              // top-level run flag
int state = 0;                 // FSM state: 0=welcome,1=reset,2=exit,3=round
Direction chosenDirection;     // required direction for this round (UP/DOWN)
Direction oppDirection;        // opposite of chosenDirection
long long best_time = -1;      // best reaction time (ms), -1 means unset
int won;                       // -1: unset, 0: miss, 1: hit
/////////////////////////////////////

/// ============================================================================
/// LED feedback helpers
/// - set_LEDGREEN: quick green blink
/// - set_LEDRED:   quick red blink
/// ============================================================================
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

/// ============================================================================
/// Timing helpers
/// - now_ms_monotonic(): monotonic ms (stable for intervals)
/// - sleep_ms():         nanosleep wrapper
/// ============================================================================
static inline long long now_ms_monotonic(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static inline void sleep_ms(int ms) {
  struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
  nanosleep(&ts, NULL);
}

/// ============================================================================
/// Input stability helpers
/// - wait_for_center_stable: require joystick centered for N consecutive polls
/// - wait_for_exitPress_stable: require stable LEFT/RIGHT press for exit
/// ============================================================================
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

static int wait_for_exitPress_stable (int fd, int needed, int poll_ms, int timeout_ms, Direction dir){
  int consec = 0;
  long long t0 = now_ms_monotonic();
   while ((now_ms_monotonic() - t0) < timeout_ms) {
    read_direction(fd);             
    if (getX() == dir) consec++;  
    else consec = 0;
    if (consec >= needed) return 1; 
    sleep_ms(poll_ms);              
  }
  return 0;
}

/// ============================================================================
/// Wall-clock helper (used by your code for intervals; monotonic is preferred
/// for elapsed timing, but this function is kept unchanged by request)
/// ============================================================================
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

/// ============================================================================
/// LED intro sequence (alternating red/green)
/// ============================================================================
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

/// ============================================================================
/// SPI device open/close
/// ============================================================================
int openFile(void){
  int fd = open("/dev/spidev0.0", O_RDWR);
  if (fd < 0) { perror("open"); return -1; }
  
  return fd;
}

void closeFile(int x){
  close(x);
}

/// ============================================================================
/// Core input loop
/// - Reads joystick continuously
/// - Implements 5s inactivity timeout
/// - Handles early press (Too soon), LEFT/RIGHT exit, and trial decision
/// NOTE: State transitions are performed here by writing global 'state'
/// ============================================================================
void joystick_task(int fd){
   long long start = getTimeInMs(); 
  while(exitLoop == 1){
  
   int r  = read_direction(fd);
   Direction x = getX();
   Direction y = getY();
   
       // Reset inactivity timer on any movement (X or Y not centered)
    if(x != DIR_CENTER || y != DIR_CENTER){
      start = getTimeInMs(); // reset timer whenever user moves
    }

    // Exit after 5s of no activity
    if(getTimeInMs() - start >= 5000){
      printf("No input within 5 seconds. Exiting game.\n");
      running = 0;
      state = 2;        // move to existing exit state
      exitLoop = 0;
      break;
    }

  // Early press handling (stick not centered at launch)
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

  // LEFT/RIGHT = exit path (with stability check)
  if((x == DIR_LEFT || x == DIR_RIGHT) && (y != DIR_DOWN && y!= DIR_UP)){
   if(wait_for_exitPress_stable (fd,4,20,1000,x)){
    state = 2;
    exitLoop =0;
    continue; 
   }
     
  }

  // Ensure we transition to the trial state (3) once
  if(state!=3){
    state =3;
    exitLoop = 0;
    continue;
  }
   
  // Trial decision: correct vs opposite direction
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

 // Polling rate for this loop
 usleep(20000);
}
}

/// ============================================================================
/// Main FSM:
/// 0) Welcome + intro LEDs → run joystick_task()
/// 1) Reset back to 0
/// 2) Exit (turn LEDs off, close device)
/// 3) One round: pick UP/DOWN, center checks, run timed trial,
///    compute + print reaction + best time
/// ============================================================================
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
    led_setRedBrightness(0);
    led_setGreenBrightness(0);
    closeFile(fd);
    return 0;
    break;
  
  case 3:
  // Begin a new round (clear LEDs, reset result)
  led_setGreenBrightness(0);
  led_setRedBrightness(0);
  won = -1;

   // Randomly choose required direction (UP/DOWN)
   int r = rand() % 2;
   chosenDirection = (r == 0) ? DIR_UP: DIR_DOWN;
   printf("Get ready...\n");
   
  // Regular center checks (ensure user releases the stick)
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
  
   // --------------------- Trial: UP branch ---------------------
   if(chosenDirection == DIR_UP){
    oppDirection = DIR_DOWN;
    led_setRedBrightness(0);
    led_setGreenBrightness(1);
    printf("Press UP now!\n");
    long long timeBefore = getTimeInMs();   // start reaction timer
    exitLoop = 1;
    joystick_task(fd);                      // waits until decision / timeout
    long long timeAfter = getTimeInMs();    // end reaction timer
    long long reaction_time = timeAfter - timeBefore;

    // Turn LEDs off after the trial
    led_setGreenBrightness(0);
    led_setRedBrightness(0);

  // If we didn't exit, evaluate and print times
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
    
   // --------------------- Trial: DOWN branch ---------------------
   }else if(chosenDirection == DIR_DOWN){
    oppDirection = DIR_UP;
    led_setGreenBrightness(0);
    led_setRedBrightness(1);
    printf("Press DOWN now!\n");
    long long timeBefore = getTimeInMs();   // start reaction timer
    exitLoop = 1;
    joystick_task(fd);                      // waits until decision / timeout
    long long timeAfter = getTimeInMs();    // end reaction timer
    long long reaction_time = timeAfter - timeBefore;

    // Turn LEDs off after the trial
    led_setGreenBrightness(0);
    led_setRedBrightness(0);
  
   // If we didn't exit, evaluate and print times
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
