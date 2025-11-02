#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdatomic.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/spi/spidev.h>
#include<pthread.h>
#include<time.h>
#include <gpiod.h>
#include "rotaryencoder.h"

#define MIN_HZ 0
#define MAX_HZ 500
#define STEP_HZ 1

static pthread_t th;
static atomic_int running = 0;
static struct gpiod_line_request *rq = NULL;
unsigned offs[2];

static void sleep_ms(int ms){
    struct timespec ts = {ms/1000, (ms%1000) *1000000L};
    nanosleep(&ts, NULL);
}

static inline int clamp(int x){
    if (x < MIN_HZ) return MIN_HZ;
    if (x > MAX_HZ) return MAX_HZ;
    return x;
}

void* loop(void*arg){
     atomic_int *target_hz = (atomic_int *)arg;

    int A = gpiod_line_request_get_value(rq,offs[0]);
    int B = gpiod_line_request_get_value(rq,offs[1]);
    int prev = ((A & 1) << 1) | (B & 1);
    static const signed char step_map[16] =
      { 0,-1,+1, 0, +1, 0, 0,-1, -1, 0, 0,+1, 0,+1,-1, 0 };

    int accum = 0;
    running = 1;
        while (atomic_load(&running)){
        int a = gpiod_line_request_get_value(rq, offs[0]);
        int b = gpiod_line_request_get_value(rq, offs[1]);
        if (a < 0 || b < 0) break;

        int curr = ((a & 1) << 1) | (b & 1);
        int delta = step_map[(prev << 2) | curr];
        if (delta){
            accum += delta;             // ±1 per half-step
            if (accum >= +4){           // one detent CW
                int v = atomic_load(target_hz) + STEP_HZ;
                atomic_store(target_hz, clamp(v));
                accum = 0;
            } else if (accum <= -4){    // one detent CCW
                int v = atomic_load(target_hz) - STEP_HZ;
                atomic_store(target_hz, clamp(v));
                accum = 0;
            }
        }
        prev = curr;
        sleep_ms(1);
    }
    return NULL;

}

void encoder_init(const char* chip, unsigned a_off, unsigned b_off, atomic_int *target_hz){
    struct gpiod_chip *c = gpiod_chip_open(chip);

    struct gpiod_line_settings *ls = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(ls, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_bias(ls, GPIOD_LINE_BIAS_PULL_UP);

    struct gpiod_line_config *lc = gpiod_line_config_new();
    offs[0] = a_off;
    offs[1] = b_off;
    gpiod_line_config_add_line_settings(lc, offs, 2, ls);

    struct gpiod_request_config *rc = gpiod_request_config_new();
    gpiod_request_config_set_consumer(rc, "encoder");

    rq = gpiod_chip_request_lines(c, rc, lc);
    if (!rq){ perror("chip_request_lines"); return; }

    if (!rq){ perror("chip_request_lines"); return; }
    
    gpiod_request_config_free(rc);
    gpiod_line_config_free(lc);
    gpiod_line_settings_free(ls);

    pthread_create(&th, NULL, loop, target_hz);

    



}

void encoder_stop(void){
    if (!rq) return;
    atomic_store(&running, 0);
    pthread_join(th, NULL);
    gpiod_line_request_release(rq);
    rq = NULL;
}