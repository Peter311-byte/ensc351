#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>

#include "sampler.h"        //sampler_init/cleanup
#include "rotaryencoder.h"  // encoder_init/encoder_stop (libgpiod v2)
#include "blinker.h"        // Blinker_init/Blinker_stop (software PWM)

static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int _){ (void)_; stop_flag = 1; }

int main()
{
    // Defaults that match your board:
    const char *chip = "/dev/gpiochip2";
    unsigned A   =  7;   // GPIO16
    unsigned B   =  8;   // GPIO17
    unsigned LED = 16;  // your LED line

    // Shared blink frequency (Hz), updated by encoder thread
    atomic_int blink_hz;
    atomic_init(&blink_hz, 10);  // start at 2 Hz

    signal(SIGINT, on_sigint);

    // 1) Start light sampler
    sampler_init();

    // 2) Start rotary encoder → updates blink_hz
    encoder_init(chip, A, B, &blink_hz);

    // 3) Start LED blinker (software PWM) using blink_hz
    Blinker_init(chip, LED, &blink_hz);

    printf("Running: chip=%s  A=%u  B=%u  LED=%u  (Ctrl+C to stop)\n",
           chip, A, B, LED);

    // 4) Main loop: once per second show current blink rate
    while (!stop_flag) {
        printf("Blink @ %d Hz\n", atomic_load(&blink_hz));
        sleep(1);
    }

    // 5) Clean shutdown (reverse init order)
    Blinker_stop();
    encoder_stop();
    sampler_cleanup();

    puts("Bye!");
    return 0;
}
