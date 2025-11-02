#ifndef _BLINKER_H_
#define _BLINKER_H_

#include <stdatomic.h>

void Blinker_init(const char *chip, unsigned led_off, atomic_int *hz);

void Blinker_stop(void);

#endif
