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
//beagle board will recieve commands from the host and process them and send it back to the host

#define PORT 12345

int sockfd;
pthread_t udp_thread;
static bool running = true;


void handle_command(char* userInput, int sockfd, struct sockaddr_in client_address, socklen_t size){
    char* previous_input [sizeof(userInput)];
    char reply [1024];
    if(strcmp(userInput,"HELP\n") == 0  || strcmp(userInput,"?\n") == 0){
        snprintf(reply,sizeof(reply),"Accepted Commands:\n" 
        "Count: Get the total number of samples taken \n"
        "Length: Get the number of samples taken in the last second \n"
        "Dips: Get the number of dips in the previously completed second \n"
        "History: Get all the samples in the previously completed second \n"
        "Stop: End the UDP connection \n"
        "<enter>: Repeat the previous command \n");
    } else if (strcmp(userInput,"STOP\n") == 0 ){
        running = false;                           // stop UDP loop
        snprintf(reply, sizeof(reply), "Program terminating.\n");
        sendto(sockfd, reply, strlen(reply), 0,
           (struct sockaddr*)&client_address, size);
        if (sockfd >= 0) shutdown(sockfd, SHUT_RDWR);
        raise(SIGINT);                       
    } else if (strcmp(userInput,"\n") == 0){
        int sockfd1 = sockfd;
        struct sockaddr_in c_addr = client_address;
        socklen_t len = size;
        handle_command(previous_input,sockfd1,c_addr,len);
        return;
    } else if (strcmp(userInput,"Length\n") == 0){
        int size_history = sampler_getHistorySize();
        snprintf(reply,sizeof(reply),"Number of samples taken in the last second: %d\n",size_history);

    }


    int s = sendto(sockfd,reply,strlen(reply),0,(struct sockaddr*)&client_address,size);

    if(strcmp(userInput,"\n") == 1){
        snprintf(previous_input,sizeof(userInput),userInput);
    }
    

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

        printf("[UDP] Received %d bytes: %s\n", n, buffer);

        if(n<0){
            perror("recvfrom");
            break;
        }

        printf("[UDP] Listening on %d. Try: netcat -u <host-ip> %d\n", PORT, PORT); 

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


