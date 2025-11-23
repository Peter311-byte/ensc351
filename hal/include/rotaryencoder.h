#ifndef _ROTARY_ENCODER_H_
#define _ROTARY_ENCODER_H_

#include <stdatomic.h>

void encoder_init(const char* chip, unsigned a_off, unsigned b_off, unsigned c_off, atomic_int *target_bpm, atomic_int* beat_state, atomic_int* runState);

void encoder_stop(void);







#endif