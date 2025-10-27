#include <stdio.h>
#include<stdbool.h>
#include<stdint.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/spi/spidev.h>
#include<pthread.h>
#include "sampler.h"


int main(){

    sampler_init();

    sleep(3);

    sampler_cleanup();
  
}