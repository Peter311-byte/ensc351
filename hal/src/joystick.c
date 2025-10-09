#include "joystick.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>



struct joystick j1;

static int read_ch(int fd, int ch, uint32_t speed_hz){
   uint8_t tx[3] = { (uint8_t)(0x06 | ((ch & 0x04) >> 2)),
 (uint8_t)((ch & 0x03) << 6),
 0x00 };

uint8_t rx[3] = { 0 };

struct spi_ioc_transfer tr = {
 .tx_buf = (unsigned long)tx,
 .rx_buf = (unsigned long)rx,
 .len = 3,
 .speed_hz = speed_hz,
 .bits_per_word = 8,
 .cs_change = 0
 };

if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) < 1) return -1;
return ((rx[1] & 0x0F) << 8) | rx[2]; 


}

int read_direction(int z){
int fd = z;
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;
 double vref = 3.300; // ADC reference (same as joystick supply)
// Direction tuning constants
const double DEADZONE = 0.08; // ignore small movements
const double THRESH   = 0.30; // 


if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return
1; }

 if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw");
return 1; }
 if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1)
{ perror("speed"); return 1; }

//  printf("Reading MCP3208 joystick on %s (speed %u Hz, vref %.2fV)\n", dev, speed, vref);
//     printf("Move the joystick — press Ctrl+C to stop.\n");

    static int center0 = -1, center1 = -1;

// for(;;){


        int ch0 = read_ch(fd, 0, speed);  // X-axis
        int ch1 = read_ch(fd, 1, speed);  // Y-axis
        if (ch0 < 0 || ch1 < 0) { perror("spi"); return 1;}

        // double vx = ch0 * vref / 4095.0;
        // double vy = ch1 * vref / 4095.0;

        // Capture joystick center once
        if ((center0 < 0) && ((ch0>2070||ch0<2050)|| (ch1>2050||ch1<2030))){ 
            return 2;
        }else if(center0<0){
            center0 = ch0;
            center1 = ch1;
        }

    //    if(center0<0){
    //     center0 = ch0;
    //     center1 = ch1;
    //    }
        // Normalize to roughly -1..+1 range
        double x = (ch0 - center0) / 2048.0;
        double y = (ch1 - center1) / 2048.0;

        // Apply deadzone
        if (x > -DEADZONE && x < DEADZONE) x = 0.0;
        if (y > -DEADZONE && y < DEADZONE) y = 0.0;

        // Determine direction labels
        const char* horiz =
            (x <= -THRESH) ? "LEFT"  :
            (x >=  THRESH) ? "RIGHT" : "CENTER";

        const char* vert  =
            (y <= -THRESH) ? "DOWN"    :
            (y >=  THRESH) ? "UP"  : "CENTER";

            j1.x = horiz;
            j1.y = vert;

            // if(horiz == "CENTER" && vert == "CENTER"){
            //     j1.center = 1;
            // }

        // printf("RAW X=%4d Y=%4d | Vx=%.3fV Vy=%.3fV | Dir: %-6s %-6s\r",
        //        ch0, ch1, vx, vy, horiz, vert);
        // fflush(stdout);
    
// }

 return 0;

}

const char* getX(void){
    return j1.x;
}

const char* getY(void){
    return j1.y;
}

bool center(void){
    return j1.center;
}