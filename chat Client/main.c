//
//  main.c
//  chat Client
//
//  Created by Ilya Yelagov on 13/03/2019.
//  Copyright © 2019 Ilya Yelagov. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <locale.h>
#include "../chatCommon.h"

void str_overwrite_stdout() {
    printf("\r%s", "You: ");
    fflush(stdout);
}

typedef struct ClientNode {
    int data;
    struct ClientNode* prev;
    struct ClientNode* link;
    char ip[16];
    char name[31];
} ClientList;

ClientList *newNode(int sockfd, char* ip) {
    ClientList *np = (ClientList *)malloc( sizeof(ClientList) );
    np->data = sockfd;
    np->prev = NULL;
    np->link = NULL;
    strncpy(np->ip, ip, 16);
    strncpy(np->name, "NULL", 5);
    return np;
}


// VARS:
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char nickname[LENGTH_NAME] = {};

////////////////////////////////
void HandleTermination(int sig) {
    flag = 1;
}

void ReceiveMessage() {
    char receivedMessage[LENGTH_SEND] = {};
    while (1) {
        int received = recv(sockfd, receivedMessage, LENGTH_SEND, 0);
        if (received > 0) {
            printf("\r%s\n", receivedMessage);
            str_overwrite_stdout();
        } else if (received == 0) { //if server terminated
            printf("Server disconnected\n");
            HandleTermination(2);
            break;
        }
        sleep(PAUSE);
    }
}

void SendMessage() {
    char message[LENGTH_MSG] = {};
    int size = 0;
    while (1) {
        str_overwrite_stdout();
        fflush(stdin);
        if (fgets(message, LENGTH_MSG, stdin) && strlen(message) > 0)
        {
            FixString(message, LENGTH_MSG);
            if (strlen(message) == 0) {
                str_overwrite_stdout();
            }
            else
            {
                //send(sockfd, &size, sizeof(int), 0);
                send(sockfd, message, LENGTH_MSG, 0);
                if (strcmp(message, "exit") == 0) {
                    break;
                }
            }
        }
        sleep(PAUSE);
    }
    HandleTermination(2);
}

int main()
{
    signal(SIGINT, HandleTermination);
    //char *locale = setlocale(LC_ALL, )
    // Socket information
    struct sockaddr_in serverInfo, clientInfo;
    int s_addrlen = sizeof(serverInfo);
    int c_addrlen = sizeof(clientInfo);
    memset(&serverInfo, 0, s_addrlen);
    memset(&clientInfo, 0, c_addrlen);
    serverInfo.sin_family = PF_INET;
    //Connection:
    int connectionSucc = 0;
    char *IP = malloc(sizeof(char));
    do
    {
        serverInfo.sin_family = PF_INET;
        
        int tmpsockfd = socket(AF_INET , SOCK_STREAM , 0);
        if (tmpsockfd == -1) {
            perror("Socket error");
        }
        printf ("Enter server IP address: ");
        fflush(stdin);
        scanf(" %s", IP);
        FixString(IP, strlen(IP));
        serverInfo.sin_addr.s_addr = inet_addr(IP);
        int portNum = 0;
        printf ("Enter server port: ");
        scanf (" %d", &portNum);
        serverInfo.sin_port = htons(portNum);
        // Connect to Server
        if (connect(tmpsockfd, (struct sockaddr *)&serverInfo, s_addrlen) == -1) {
            perror("Connection error");
            printf("Try another server IP or port\n");
            memset(&serverInfo, 0, s_addrlen);
            close(tmpsockfd);
        }
        else
        {
            sockfd = tmpsockfd;
            connectionSucc = 1;
            free (IP);
        }
    }
    while (!connectionSucc);
    
    bool loginFine = FALSE;
    do
    {
        printf("Enter login: ");
        scanf(" %s", nickname);
        if (strlen(nickname) > 0)
        {
            FixString(nickname, LENGTH_NAME);
            if (strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
                printf("\nEnter valid name. Try again\n");
            }
            else
            {
                loginFine = TRUE;
            }
        }
    }
    while (!loginFine);
    
    getsockname(sockfd, (struct sockaddr*) &clientInfo, (socklen_t*) &c_addrlen);   //fills clientInfo
    getpeername(sockfd, (struct sockaddr*) &serverInfo, (socklen_t*) &s_addrlen);   //fills serverInfo->sin_addr "returns the local IP address and local port number assigned to the connection by the kernel."
    

    printf("Connecting to Server: %s:%d\n", inet_ntoa(serverInfo.sin_addr), ntohs(serverInfo.sin_port));
    printf("::::%s:%d(%d)\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port), clientInfo.sin_port);
    
    if (send(sockfd, nickname, LENGTH_NAME, 0) == -1)
    {
        error ("Server connection error");
    }
    
    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *) SendMessage, NULL) != 0) {
        printf ("Thread error: SendMessage\n");
        exit(EXIT_FAILURE);
    }
    
    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *) ReceiveMessage, NULL) != 0) {
        printf ("Thread error: ReceiveMessage\n");
        exit(EXIT_FAILURE);
    }
    
    //Main loop:
    while (1) {
        if(flag) {
            printf("\nExiting...\n");
            break;
        }
        sleep(PAUSE);
    }
    
    close(sockfd);
    return 0;
}
