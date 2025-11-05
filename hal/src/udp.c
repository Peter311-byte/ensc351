#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>
#include<stdbool.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<string.h>
#include<signal.h>
#include "udp.h"
#include "sampler.h"


// Beagle board is the UDP server
// My computer is the host. 
// beagle board will recieve commands from the host and process them and send it back to the host

#define PORT 12345

int sockfd;
pthread_t udp_thread;
static bool running = true;


void handle_command(char* userInput, int sockfd, struct sockaddr_in client_address, socklen_t size){
    static char previous_input [128] = "";
    char reply [1024];

if (userInput[0] == '\0' || strcmp(userInput, "\n") == 0 || strcmp(userInput, "\r\n") == 0) {
        // Repeat the previous command if we have one
        if (previous_input[0] != '\0') {
            int sockfd1 = sockfd;
            struct sockaddr_in c_addr = client_address;
            socklen_t len = size;
            handle_command(previous_input, sockfd1, c_addr, len);
            return;
        } else {
            snprintf(reply, sizeof(reply), "No previous command.\n");
            sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr*)&client_address, size);
            return;
        }
    }

    if(strcmp(userInput,"help\n") == 0  || strcmp(userInput,"?\n") == 0){
        snprintf(reply,sizeof(reply),"Accepted Commands:\n" 
        "count: Get the total number of samples taken \n"
        "length: Get the number of samples taken in the last second \n"
        "dips: Get the number of dips in the previously completed second \n"
        "history: Get all the samples in the previously completed second \n"
        "stop: End the UDP connection \n"
        "<enter>: Repeat the previous command \n");
    } else if (strcmp(userInput,"stop\n") == 0 ){
        running = false;                           // stop UDP loop
        snprintf(reply, sizeof(reply), "Program terminating.\n");
        sendto(sockfd, reply, strlen(reply), 0,
           (struct sockaddr*)&client_address, size);
        if (sockfd >= 0) shutdown(sockfd, SHUT_RDWR);
        raise(SIGINT);                       
    } else if (strcmp(userInput,"length\n") == 0){
        int size_history = sampler_getHistorySize();
        snprintf(reply,sizeof(reply),"Number of samples taken in the last second: %d\n",size_history);

    }else if (strcmp(userInput,"count\n") == 0){
        int total_samples = getTotalNumberofSamples();
        snprintf(reply,sizeof(reply),"Number of total samples taken: %d\n",total_samples);
    }else if (strcmp(userInput,"history\n") == 0){
    int n = 0;
    double* samples = sampler_getHistory(&n); 

    char reply[1500];  
    reply[0] = '\0';
    int count = 0;
    for (int i = 0; i < n; ++i) {
        char num[32];
        snprintf(num, sizeof(num), "%.3f,", samples[i]);
        strcat(reply, num);
        count++;

        // Send after every 10 samples or near 1400 bytes
        if (count == 10 || strlen(reply) > 1400) {
            strcat(reply, "\n");
            sendto(sockfd, reply, strlen(reply), 0,
                   (struct sockaddr*)&client_address, size);
            reply[0] = '\0';
            count = 0;
        }
    }

    // Send remaining samples (if any)
    if (strlen(reply) > 0) {
        strcat(reply, "\n");
        sendto(sockfd, reply, strlen(reply), 0,
               (struct sockaddr*)&client_address, size);
    }

    free(samples);   // <--- free the heap memory allocated in sampler_getHistory()
    return;
 }else if(strcmp(userInput,"dips\n") == 0){
    int num_dips = getTotalNumberofDips();
    snprintf(reply,sizeof(reply),"Number of dips in the last second: %d\n",num_dips);

 }

 
        snprintf(previous_input,sizeof(userInput),userInput);

     int s = sendto(sockfd,reply,strlen(reply),0,(struct sockaddr*)&client_address,size);

    return;

}


void*udp_thread_listen(void*arg){
    struct sockaddr_in server_addr,client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[2048];
    sockfd = socket(AF_INET,SOCK_DGRAM,0);

     if (sockfd < 0) {
        perror("socket");
        pthread_exit(NULL);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(bind(sockfd,(struct sockaddr*)&server_addr, sizeof(server_addr))<0){
        perror("bind");
        close(sockfd);
        pthread_exit(NULL);
    }

    while(running){
        int n = recvfrom(sockfd, buffer, sizeof(buffer)-1,0,(struct sockaddr*)&client_addr,&client_len);

        if(n<0){
            perror("recvfrom");
            break;
        } 

        if(n>0){
        buffer[n] ='\0';
        handle_command(buffer,sockfd,client_addr,client_len);

        }

        
    }


}

void udp_init(void){
pthread_create(&udp_thread,NULL,udp_thread_listen,NULL);

    
}

void udp_cleanup(void){
   running = false;

    // Unblock recvfrom() in the UDP thread
    if (sockfd >= 0) {
        shutdown(sockfd, SHUT_RDWR);   // immediately interrupts recvfrom()
    }

    // Wait for thread to finish cleanly
    pthread_join(udp_thread, NULL);

    // Close socket after thread ends
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}


