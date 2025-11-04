#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<string.h>
#include "udp.h"
#include "sampler.h"

#define PORT 12345

static bool running = true;

void*udp_thread_listen(void*arg){
    int sockfd;
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

        buffer[n] ='\0';
        printf("[UDP] Received: %s\n", buffer);
    }


}

void udp_init(int argc, char*argv[]){
pthread_t udp_thread;
pthread_create(&udp_thread,NULL,udp_thread_listen,NULL);

    
}

