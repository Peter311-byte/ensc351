#include "audioMIxer.h"
#include "joystick.h"
#include "accelerometer.h"
#include "rotaryencoder.h"
#include "periodTimer.h"
#include "udp.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>

#define BASS_DRUM_FILE "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define HIHAT_FILE     "wave-files/100053__menegass__gui-drum-cc.wav"
#define SNARE_FILE     "wave-files/100059__menegass__gui-drum-snare-soft.wav"

#define UDP_PORT 12345      // choose any port; match it in your Node server

// Global shared state
atomic_int BPM;          // tempo
atomic_int state;        // 0 = rock, 1 = custom, 2 = none
atomic_int app_running;  // controls whole app lifetime
atomic_int running;      // controls input threads (joystick/encoder/accel)

wavedata_t bassDrum, hiHat, Snare;

// Threads
static pthread_t beat_generate;
static pthread_t displayThread;
static volatile sig_atomic_t sigint_flag = 0;

// ---------- Beat Generator Thread ----------

static void* beat_generator(void* arg)
{
    (void)arg;

    while (atomic_load(&app_running)) {
        int bpm = atomic_load(&BPM);
        if (bpm < 40) bpm = 40;      // clamp just in case
        if (bpm > 300) bpm = 300;

        double halfBeatSec = 60.0 / bpm / 2.0;
        int halfBeatUsec = (int)(halfBeatSec * 1000000.0);

        int mode = atomic_load(&state);

        if (mode == 0) {
            // Standard rock beat (1–4.5)
            AudioMixer_queueSound(&bassDrum);  // 1: bass + hihat
            AudioMixer_queueSound(&hiHat);
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&hiHat);     // 1.5
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&Snare);     // 2: snare + hihat
            AudioMixer_queueSound(&hiHat);
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&hiHat);     // 2.5
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&bassDrum);  // 3: bass + hihat
            AudioMixer_queueSound(&hiHat);
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&hiHat);     // 3.5
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&Snare);     // 4: snare + hihat
            AudioMixer_queueSound(&hiHat);
            usleep(halfBeatUsec);

            AudioMixer_queueSound(&hiHat);     // 4.5
            usleep(halfBeatUsec);

        } else if (mode == 1) {
            
    AudioMixer_queueSound(&bassDrum);
    AudioMixer_queueSound(&hiHat);
    usleep(halfBeatUsec);
    usleep(halfBeatUsec);
    AudioMixer_queueSound(&hiHat);
    usleep(halfBeatUsec);
    usleep(halfBeatUsec);
    AudioMixer_queueSound(&Snare);
    AudioMixer_queueSound(&hiHat);
    usleep(halfBeatUsec);
    AudioMixer_queueSound(&bassDrum);
    usleep(halfBeatUsec);
    AudioMixer_queueSound(&bassDrum);
    AudioMixer_queueSound(&hiHat);
    usleep(halfBeatUsec);
    usleep(halfBeatUsec);

        } else {
            // mode 2 = none (off)
            usleep(1000);
        }
    }

    return NULL;
}

// ---------- Display Thread (Section 4.3 text output) ----------

static void* display_thread(void* arg)
{
    (void)arg;

    Period_statistics_t audioStats;
    Period_statistics_t accelStats;

    while (atomic_load(&app_running)) {
        sleep(1);   // once per second

        int mode = atomic_load(&state);   // M0, M1, ...
        int tempo = atomic_load(&BPM);    // bpm
        int volume = AudioMixer_getVolume();

        Period_getStatisticsAndClear(PERIOD_EVENT_AUDIO_REFILL, &audioStats);
        Period_getStatisticsAndClear(PERIOD_EVENT_ACCEL_SAMPLE, &accelStats);

        printf("M%d %dbpm vol:%d "
               "Audio[%.3f, %.3f] avg %.3f/%d "
               "Accel[%.3f, %.3f] avg %.3f/%d\n",
               mode, tempo, volume,
               audioStats.minPeriodInMs,
               audioStats.maxPeriodInMs,
               audioStats.avgPeriodInMs,
               audioStats.numSamples,
               accelStats.minPeriodInMs,
               accelStats.maxPeriodInMs,
               accelStats.avgPeriodInMs,
               accelStats.numSamples);
        fflush(stdout);
    }
    return NULL;
}

// ---------- Signal handler ----------

static void handle_sigint(int sig)
{
    (void)sig;
    sigint_flag = 1;
}

// ---------- main() ----------

int main(void)
{

    printf("Welcome to BeatBox simulator. You can exit through the UDP connection. Note: Pressing Ctrl C will not work.\n");
    signal(SIGINT, handle_sigint);

    // Period timer for stats
    Period_init();

    // Shared state init
    atomic_init(&BPM, 120);
    atomic_init(&state, 0);         // start in rock mode
    atomic_init(&running, 1);       // input threads running
    atomic_init(&app_running, 1);   // whole app running

    // Audio + wave files
    AudioMixer_readWaveFileIntoMemory(BASS_DRUM_FILE, &bassDrum);
    AudioMixer_readWaveFileIntoMemory(HIHAT_FILE,     &hiHat);
    AudioMixer_readWaveFileIntoMemory(SNARE_FILE,     &Snare);
    AudioMixer_init();

    // GPIO / rotary encoder
    const char *chip = "/dev/gpiochip2";
    unsigned A   = 7;   // GPIO16
    unsigned B   = 8;   // GPIO17
    unsigned switch_encoder = 16;
    encoder_init(chip, A, B, switch_encoder, &BPM, &state, &running);

    // Joystick (ADC) + accelerometer
    joystick_init(&running);
    accelometer_init(&running, &bassDrum, &Snare, &hiHat);

    // UDP: attach shared state and start server
    udp_set_app_running(&app_running);
    udp_set_sounds(&bassDrum, &Snare, &hiHat);
    udp_set_tempo(&BPM, atomic_load(&BPM));
    udp_set_mode(&state, atomic_load(&state));
    udp_init(UDP_PORT);

    // Threads: beat + display
    pthread_create(&beat_generate, NULL, beat_generator, NULL);
    pthread_create(&displayThread, NULL, display_thread, NULL);

    // Main thread just waits until app_running becomes 0
    while (1) {
        if(sigint_flag){
            atomic_store(&app_running,0);
            break;
        }

        if(!atomic_load(&app_running)){
            break;
        }
        sleep(1);
    }

    // Begin shutdown
    atomic_store(&running, 0);   // stop joystick / encoder / accel threads

    pthread_join(beat_generate, NULL);
    pthread_join(displayThread, NULL);

    udp_cleanup();

    accelometer_stop();
    joystick_cleanup();
    encoder_stop();

    AudioMixer_cleanup();
    AudioMixer_freeWaveFileData(&bassDrum);
    AudioMixer_freeWaveFileData(&hiHat);
    AudioMixer_freeWaveFileData(&Snare);

    Period_cleanup();

    return 0;
}
