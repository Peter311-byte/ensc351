#ifndef _UDP_H_
#define _UDP_H_

#include <stdatomic.h>
#include "audioMIxer.h"
#include "accelerometer.h"

// Initialize the UDP interface on the given port
void udp_init(unsigned short port);

// Clean up UDP interface
void udp_cleanup(void);

// Set the global BPM (tempo)
void udp_set_tempo(atomic_int *BPM, int newTempo);

// Set the beat mode/state
void udp_set_mode(atomic_int *state, int newMode);

// Set the volume (0-100)
void udp_set_volume(int newVolume);

// Play a specific sound
void udp_play_sound(wavedata_t *sound);

// Shut down the program
void udp_shutdown(atomic_int *app_running);

// Attach the app_running flag so UDP "shutdown" and loop control work
void udp_set_app_running(atomic_int *app_running);

// Attach sound pointers so "play bass/snare/hihat" works
void udp_set_sounds(wavedata_t *bass, wavedata_t *snare, wavedata_t *hihat);

#endif
