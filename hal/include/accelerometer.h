#ifndef ACCELOMETER_H
#define ACCELOMETER_H


#include "audioMIxer.h"
#include <stdatomic.h>
// Initialize accelerometer input module.
//  - runFlag: shared atomic flag; when set to 0, the accel thread will exit.
//  - bass, snare, hihat: pointers to the drum wave data to play on hits.
//
// Call this once after AudioMixer_readWaveFileIntoMemory() and AudioMixer_init().
void accelometer_init(atomic_int *runFlag,
                      wavedata_t *bass,
                      wavedata_t *snare,
                      wavedata_t *hihat);

// Join the accel thread and close SPI FD.
// Call this during shutdown after setting *runFlag to 0.
void accelometer_stop(void);

#endif