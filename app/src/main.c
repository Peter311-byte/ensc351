
#include "audioMIxer.h"
#include "rotaryencoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdatomic.h>
#include <unistd.h>
#include <pthread.h>

#define bass_drum "wave-files/100051__menegass__gui-drum-bd-hard.wav"
#define hihat "wave-files/100053__menegass__gui-drum-cc.wav"
#define snare "wave-files/100059__menegass__gui-drum-snare-soft.wav"
atomic_int BPM;
atomic_int state;  // state = 0: rock beat
                   // state = 1: custom beat
                   // state = 2: nothing
atomic_int app_running = 1;
atomic_int running = 1;
 wavedata_t bassDrum,hiHat,Snare;
   static pthread_t beat_generate;

void* beat_generator(void* arg){
double halfBeatSec = 60.0 / atomic_load(&BPM)/ 2.0;
int halfBeatUsec = (int)(halfBeatSec * 1000000.0);

while(1){
    if(atomic_load(&state) == 0){
    printf("%d", atomic_load(&BPM));
    AudioMixer_queueSound(&bassDrum);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec); // change to bpm formula
    AudioMixer_queueSound(&hiHat);
    usleep(halfBeatUsec);
    AudioMixer_queueSound(&Snare);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec);
     AudioMixer_queueSound(&bassDrum);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec);
      AudioMixer_queueSound(&Snare);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec);
     AudioMixer_queueSound(&hiHat);
     usleep(halfBeatUsec);

    }else if (atomic_load(&state) == 1){
        printf("%d", atomic_load(&BPM));
        AudioMixer_queueSound(&bassDrum);
        usleep(halfBeatUsec); // change to bpm formul

    }else{
        usleep(1000);
    }
}

return NULL;
}

void handle_sigint(int sig){
    (void)sig;
    atomic_store(&app_running,0);
}
int main(){
    signal(SIGINT,handle_sigint);
    const char *chip = "/dev/gpiochip2";
    unsigned A   =  7;   // GPIO16
    unsigned B   =  8;   // GPIO17
    unsigned switch_encoder = 16;   // switch_encoder gpio
    atomic_init(&BPM, 100);
    atomic_init(&state,0);




    encoder_init(chip, A, B, switch_encoder, &BPM, &state, &running);
    AudioMixer_readWaveFileIntoMemory(bass_drum, &bassDrum);
    AudioMixer_readWaveFileIntoMemory(hihat,&hiHat);
    AudioMixer_readWaveFileIntoMemory(snare,&Snare);
    AudioMixer_init();
    pthread_create(&beat_generate,NULL,beat_generator,NULL);


    while(atomic_load(&app_running)){
        sleep(1);
    }

    atomic_store(&running,0);
    pthread_join(beat_generate,NULL);
    encoder_stop();
    AudioMixer_cleanup();
    AudioMixer_freeWaveFileData(&bassDrum);
    AudioMixer_freeWaveFileData(&hiHat);
    AudioMixer_freeWaveFileData(&Snare);

    return 0;




}