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
#include <arpa/inet.h>
#include <pthread.h>
#include "../chatCommon.h"

#define MAXCLIENTCOUNT 3 + 3

typedef struct TClientList {
    int sockNum;
    struct TClientList* prev;
    struct TClientList* next;
    char ip[16];
    char nickname[31];
} TClientList;

TClientList *NewClient(int sockfd, char* ip) {
    TClientList *newClient = (TClientList *)malloc( sizeof(TClientList) );
    newClient->sockNum = sockfd;
    newClient->prev = NULL;
    newClient->next = NULL;
    strncpy(newClient->ip, ip, 16);
    strncpy(newClient->nickname, "NULL", 5);
    return newClient;
}


int server_sockfd = 0, client_sockfd = 0, clientCount = 0;
TClientList *clientList, *currClient;

void handleExit(int sig) {
    TClientList *tmp;
    // close all sockets including server_sockfd
    while (clientList != NULL) {
        printf("\nClose socketfd: %d\n", clientList->sockNum);
        close(clientList->sockNum);
        tmp = clientList;
        clientList = clientList->next;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void sendToAll(TClientList *sender, char tmp_buffer[]) {
    TClientList *tmpC = clientList->next;
    printf("%s\n", tmp_buffer);
    while (tmpC != NULL) {
        if (sender->sockNum != tmpC->sockNum) { // to all clients except for sender
            printf("Send to sockfd %d: \"%s\" \n", tmpC->sockNum, tmp_buffer);
            send(tmpC->sockNum, tmp_buffer, LENGTH_SEND, 0);
        }
        tmpC = tmpC->next;
    }
}

void client_handler(TClientList *theClient) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    
    // Obtaining nickname:
    bool correctNickname = FALSE;
    do
    {
        printf("expecting client nickname...\n");
        if (recv(theClient->sockNum, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
            printf("ERROR: %s incorrect name. Disconnecting... \n", theClient->ip);
            //leave_flag = 1;
        } else {
            strncpy(theClient->nickname, nickname, LENGTH_NAME);
            printf("%s(%s)(%d) joines conversation.\n", theClient->nickname, theClient->ip, theClient->sockNum);
            sprintf(send_buffer, "%s joines conversation.", theClient->nickname);
            sendToAll(theClient, send_buffer);
            correctNickname = TRUE;
        }
    }
    while ((!correctNickname) && !leave_flag);
    
    // Conversation loop
    while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(theClient->sockNum, recv_buffer, LENGTH_MSG, 0); //waits until message received
        if (receive > 0) {
            if (strlen(recv_buffer) == 0) {
                continue;
            }
            sprintf(send_buffer, "%s：%s", theClient->nickname, recv_buffer);
        } else if (receive == 0 || strcmp(recv_buffer, "exit") == 0) {
            printf("%s(%s)(%d) leaves conversation.\n", theClient->nickname, theClient->ip, theClient->sockNum);
            sprintf(send_buffer, "%s leaves conversation.", theClient->nickname);
            leave_flag = 1;
        } else {
            perror("Error: -1\n" );
            leave_flag = 1;
        }
        sendToAll(theClient, send_buffer);   //(except_for: , message: )
    }
    
    // Remove Node
    close(theClient->sockNum);
    if (theClient == currClient) { //if last client leaves
        currClient = theClient->prev;
        currClient->next = NULL;
    }
    else
    {
        theClient->prev->next = theClient->next;
        if (theClient->next != NULL)
            theClient->next->prev = theClient->prev;
    }
    clientCount--;
    free(theClient);
}

int main()
{
    signal(SIGINT, handleExit); 
    server_sockfd = socket(AF_INET , SOCK_STREAM , 0);  //creates an endpoint for communication and returns a file descriptor that refers to that endpoint
    if (server_sockfd == -1) {
        error("Socket error");
        exit(EXIT_FAILURE);
    }
    // Set socket information
    struct sockaddr_in serverInfo, clientInfo;
    int s_addrlen = sizeof(serverInfo);
    int c_addrlen = sizeof(clientInfo);
    memset(&serverInfo, 0, s_addrlen);
    memset(&clientInfo, 0, c_addrlen);
    serverInfo.sin_family = PF_INET;
    
    int portCorrect = 0;
    char *IP = (char*)malloc(sizeof(char));
    do
    {
        int portNum = 0;
        printf ("Enter server IP: ");
        scanf (" %s", IP);
        serverInfo.sin_addr.s_addr = inet_addr(IP);
        printf ("Enter server port: ");
        scanf (" %d", &portNum);
        serverInfo.sin_port = htons(portNum);
        //bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
        if (bind(server_sockfd, (struct sockaddr *)&serverInfo, s_addrlen) != -1){
            portCorrect = 1;
            free(IP);
        }
        else
        {
            perror("Binding error");
            printf("Try again\n");
        }
    }
    while (!portCorrect);
    
    int backlog = 5;    //"maximum length to which the queue of pending connections for sockfd may grow.  If a connection request arrives when the queue is full, the client may receive an error with an indication of ECONNREFUSED or, if the underlying protocol supports retransmission, the request may be ignored so that a later reattempt at connection succeeds."

    if (listen(server_sockfd, backlog) == -1)   //
    {
        perror("Listen error");
    }
    
    // Print Server IP
    int tmp = getsockname(server_sockfd, (struct sockaddr*) &serverInfo, (socklen_t*) &s_addrlen);   //returns the local port number that was assigned.
    printf("Start Server on: %s:%d\n", inet_ntoa(serverInfo.sin_addr), ntohs(serverInfo.sin_port));
    
    // Initial linked list for clients
    clientList = NewClient(server_sockfd, inet_ntoa(serverInfo.sin_addr));
    currClient = clientList;
    printf("Server is up and running\n");
    //Handling new clients:
    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &clientInfo, (socklen_t*) &c_addrlen);    //waits until new client sends connect(...)
        if (client_sockfd != -1 && clientCount < MAXCLIENTCOUNT)
        {
            // Print Client IP
            getpeername(client_sockfd, (struct sockaddr*) &clientInfo, (socklen_t*) &c_addrlen);   //fill client_info port obtain the identity of the client
            printf("Client %s:%d comes in.\n", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
            
            
            clientCount++;
            TClientList *newClient = NewClient(client_sockfd, inet_ntoa(clientInfo.sin_addr));
            newClient->prev = currClient;
            currClient->next = newClient;
            currClient = newClient;
            
            pthread_t tmp;
            if (pthread_create(&tmp, NULL, (void *)client_handler, (TClientList *)newClient) != 0) {
                error("Thread error!\n");
            }
        }
        else if (client_sockfd != -1)   //if client connection succeeded but exceeds max num
        {
            send(client_sockfd, "No room for you bud :(", LENGTH_SEND, 0);
            printf ("User declined\n");
        }
        else
        {
            printf("Client error: %s:%d", inet_ntoa(clientInfo.sin_addr), ntohs(clientInfo.sin_port));
            perror("");
        }
    }
    
    return 0;
}
