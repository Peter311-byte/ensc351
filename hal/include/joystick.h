#ifndef HAL_JOYSTICK_H

#define HAL_JOYSTICK_H
#include<stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

bool isCenter(void);

const char* getX(void);

const char* getY(void);

static int read_ch(int fd, int ch, uint32_t speed_hz);
int read_direction(int z);

struct joystick {
    bool center;
    const char* x;
    const char* y;
};

#endif