#include <stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/spi/spidev.h>
#include<pthread.h>
#include "sampler.h"

//////////////////////////////////////////////////////
pthread_t sampler;
const char* dev = "/dev/spidev0.0";
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;
double vref = 3.300;
bool averageIntialized = 0;
double a = 0.0;
int fd;
double* current_arr;
double* history_arr;

//////////////////////////////////////////////////


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

void sampler_moveCurrentDataHistory(void){
    
    for(int j = 0; j<1000; ++j){
        history_arr[j] = current_arr[j];
    }

    memset(current_arr,0,1000*sizeof(double));

}

void* light_sampler(void* arg){
    int i = 0;
    for(;;){
        int ch0 = read_ch(fd,0,speed);
        double voltage_R10k = ch0*(vref/4095.0);
        if(i<1000){
            current_arr[i] = voltage_R10k;
            i = i+1;
        }else{
           sampler_moveCurrentDataHistory();
        }
    }

}


void sampler_init(void){
    fd = open(dev, O_RDWR);

    if (fd < 0) { perror("open"); return 1; }

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return
    1; }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw");
    return 1; }

    if(ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1)
    {perror("speed"); return 1; }

    current_arr = (double*)malloc(1000*sizeof(double));

    pthread_create(&sampler,NULL,light_sampler,NULL);
}


void sampler_cleanup(void){
    close(fd);
    pthread_join(sampler,NULL);
}