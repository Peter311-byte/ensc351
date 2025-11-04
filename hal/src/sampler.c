#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdatomic.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<linux/spi/spidev.h>
#include<pthread.h>
#include<time.h>
#include "sampler.h"


// GLOBAL VARIABLES for this c file///

const char* dev = "/dev/spidev0.0";
uint8_t mode = 0; // SPI mode 0
uint8_t bits = 8;
uint32_t speed = 250000;

pthread_t light_sampler;
pthread_mutex_t mutex;
bool running;

int i = 0;

int length_history_arr = 0;

bool averageIntialized = 0;
double a = 0.0;

double vref = 3.3000;

int fd = 0;

double* current_arr;
double* history_arr;


/////////////////////////////////////



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

int openFile(void){
    fd = open(dev, O_RDWR);
    if (fd < 0) { perror("open"); return 1; }
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) { perror("mode"); return
    1; }

    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) { perror("bpw");
    return 1; }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1)
    { perror("speed"); return 1; }
}

void sampler_moveCurrentDataHistory(void){

    double*temp = history_arr;
    history_arr = current_arr;
    current_arr = temp;
    memset(current_arr,0.0,1000*sizeof(current_arr[0]));

    // printf("Updated history_arr!\n");

    //    for (int i=0; i<5; i++){

    //        printf("%.3f", current_arr[i]);

    //    } // remove later
   

}

void*sampler(void* arg){

    time_t start = time(NULL);
    while(running == true){
        int ch0 = read_ch(fd, 0, speed);  // ch0 value
        double voltage_R10K = ch0*(vref/4095.0);

        if(difftime(time(NULL),start)>=1.0){
    //     for (int i=0; i<5; i++){
    //        printf("%.3f ", current_arr[i]);

    //    } // remove later
            sampler_moveCurrentDataHistory();
            pthread_mutex_lock(&mutex);
            length_history_arr = i;
            pthread_mutex_unlock(&mutex);
            i = 0;
            start = time(NULL);
        }else{
            if(i<1000){
                current_arr[i] = voltage_R10K;
                i=i+1;
            }

        }

         if(averageIntialized == 0){
            a = voltage_R10K;
            averageIntialized = 1;
            
        }else{
            a = a + ((0.001)*(voltage_R10K - a));
        }



        // printf("Light Intensity (voltage) = %.3f\n", voltage_R10K);

        usleep(1000);
        
    }
}

double* sampler_getHistory(int *size){
    pthread_mutex_lock(&mutex);
    int actualsize = length_history_arr;
    double* copy_history_arr = (double*)calloc(actualsize,sizeof(double)); // need to figure out where to free this!

    for(int i = 0; i<actualsize; ++i){
        copy_history_arr[i] = history_arr[i];
    }

    *size = length_history_arr;
    pthread_mutex_unlock(&mutex);

    return copy_history_arr;

}

int sampler_getHistorySize(void){
    pthread_mutex_lock(&mutex);
    int n = length_history_arr;
    pthread_mutex_unlock(&mutex);
    return n;

}


double sampler_getAverageReading(void){
    return a;
}

void sampler_init(void){

    int x = openFile(); // open file for the spi readings
    if(x!=0){
        return;
    }

    running = true;
    current_arr = (double*)malloc(1000*sizeof(double));
    history_arr = (double*)malloc(1000*sizeof(double));
    pthread_mutex_init(&mutex,NULL);
    pthread_create(&light_sampler, NULL, sampler, NULL);

}


void sampler_cleanup(void){
    running = false;
    pthread_join(light_sampler, NULL);
    pthread_mutex_destroy(&mutex);
    free(current_arr);
    free(history_arr);
    
    close(fd);
    
}