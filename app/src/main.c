#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>
#include "sampler.h"
#include "rotaryencoder.h"
#include "blinker.h"
#include "udp.h"
#include "periodTimer.h"

static volatile sig_atomic_t stop_flag = 0;
static void on_sigint(int _){ (void)_; stop_flag = 1; }

// ------------------------------------------------------------
// Print one status report to the terminal (once per second)
// ------------------------------------------------------------
static void print_status(int led_hz)
{
    // Move last second’s data to history (required once per second)
    sampler_moveCurrentDataHistory();

    // Fetch data
    int n = sampler_getHistorySize();
    int dips = getTotalNumberofDips();
    double avgV = sampler_getAverageReading();

    // Collect timing jitter stats
    Period_statistics_t stats;
    Period_getStatisticsAndClear(PERIOD_EVENT_SAMPLE_LIGHT, &stats);

    // -------- Line 1: Summary --------
    printf("#Smpl/s = %4d    Flash @ %3dHz   avg = %.3fV    dips = %3d    "
           "Smpl ms[%6.3f, %6.3f] avg %6.3f/%d\n",
           n, led_hz, avgV, dips,
           stats.minPeriodInMs, stats.maxPeriodInMs,
           stats.avgPeriodInMs, stats.numSamples);

    // -------- Line 2: Show 10 evenly spaced samples --------
    int hist_size;
    double *history = sampler_getHistory(&hist_size);

    if (hist_size > 0) {
        int show = (hist_size < 10) ? hist_size : 10;
        for (int i = 0; i < show; i++) {
            int idx = (int)((double)i * (hist_size - 1) / (show - 1));
            printf("%3d:%.3f   ", idx, history[idx]);
        }
        printf("\n");
    } else {
        printf("(no samples)\n");
    }

    free(history);
}
// ------------------------------------------------------------

int main()
{
    const char *chip = "/dev/gpiochip2";
    unsigned A   =  7;   // GPIO16
    unsigned B   =  8;   // GPIO17
    unsigned LED = 16;   // LED pin

    atomic_int blink_hz;
    atomic_init(&blink_hz, 10);  // start at 10 Hz

    signal(SIGINT, on_sigint);

    // Initialize subsystems
    sampler_init();
    encoder_init(chip, A, B, &blink_hz);
    Blinker_init(chip, LED, &blink_hz);
    udp_init();

    // Period timer init
    Period_init();

    // Main reporting loop
    while (!stop_flag) {
        sleep(1);
        print_status(atomic_load(&blink_hz));
    }

    // Clean shutdown
    udp_cleanup();
    Blinker_stop();
    encoder_stop();
    sampler_cleanup();
    Period_cleanup();
    return 0;
}
