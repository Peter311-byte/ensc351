#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

typedef struct{
    int numSamples;
    short *pData;


}wavedata_t;

#define AUDIOMIXER_MAX_VOLUME 100

void AudioMixer_init(void);
void AudioMixer_cleanup(void);

void AudioMixer_readWaveFileIntoMemory(char *fileName, wavedata_t *pSound);
void AudioMixer_freeWaveFileData(wavedata_t *pSound);

// Queue up another sound bite to play as soon as possible.
void AudioMixer_queueSound(wavedata_t *pSound);


int AudioMixer_getVolume();

void AudioMixer_setVolume(int newVolume);
#endif