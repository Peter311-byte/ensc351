#ifndef HAL_JOYSTICK_H

#define HAL_JOYSTICK_H

#include<stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

typedef enum {
    DIR_CENTER,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
} Direction;

bool isCenter(void);

Direction getX(void);

Direction getY(void);

static int read_ch(int fd, int ch, uint32_t speed_hz);
int read_direction(int z);

struct joystick {
    bool center;
    Direction x;
    Direction y;
};



#endif