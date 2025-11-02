#include "blinker.h"
#include <gpiod.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

static pthread_t th;
static atomic_int running = 0;
static struct gpiod_line_request *rq = NULL;
static unsigned led_off_global = 0;

static void uswait(long usec){
    struct timespec ts = { usec/1000000, (long)(usec%1000000)*1000L };
    nanosleep(&ts, NULL);
}

static void *loop(void *arg){
    atomic_int *hz = (atomic_int *)arg;
    running = 1;
    while (atomic_load(&running)){
        int f = atomic_load(hz);
        if (f <= 0){
            gpiod_line_request_set_value(rq, led_off_global, 0);
            uswait(50*1000);
            continue;
        }
        long period_us = 1000000L / f;
        long half = period_us / 2;
        gpiod_line_request_set_value(rq, led_off_global, 1);
        uswait(half);
        gpiod_line_request_set_value(rq, led_off_global, 0);
        uswait(half);
    }
    return NULL;
}

void Blinker_init(const char *chip, unsigned led_off, atomic_int *hz){
    led_off_global = led_off;

    struct gpiod_chip *c = gpiod_chip_open(chip);
    if (!c){ perror("gpiod_chip_open"); return; }

    struct gpiod_line_settings *ls = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(ls, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(ls, 0);

    struct gpiod_line_config *lc = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(lc, &led_off, 1, ls);

    struct gpiod_request_config *rc = gpiod_request_config_new();
    gpiod_request_config_set_consumer(rc, "blinker");

    rq = gpiod_chip_request_lines(c, rc, lc);
    if (!rq){ perror("chip_request_lines"); return; }

    pthread_create(&th, NULL, loop, hz);

    gpiod_request_config_free(rc);
    gpiod_line_config_free(lc);
    gpiod_line_settings_free(ls);
}

void Blinker_stop(void){
    if (!rq) return;
    atomic_store(&running, 0);
    pthread_join(th, NULL);
    gpiod_line_request_release(rq);
    rq = NULL;
}
