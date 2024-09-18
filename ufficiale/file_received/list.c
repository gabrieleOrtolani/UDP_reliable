#include "socket_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include "reliable.h"

#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"
#define MAX_SEGMENT_SIZE 512
/*-------------------------------funzioni per comporre i pacchetti--------------------------------------*/
char* composeDataPacket(const DataPacket *packet) {
    char *buffer = (char *)malloc(310);  // Allocazione di un buffer sufficientemente grande
    if (buffer != NULL) {
        sprintf(buffer, "(%s-%d-%d)", packet->ip_addr, packet->port_number,packet->error_port);
       // printf("\n%d \n",packet->error_port);
    }
    return buffer;
}
int parseDataPacket(const char *buffer, DataPacket *packet) {
    // Assicurati che il buffer abbia il formato corretto
    if (sscanf(buffer, "(%15[^-]-%d-%d)", packet->ip_addr, &(packet->port_number),&(packet->error_port)) == 3) {
        return 1;  // Successo
    } else {
        return 0;  // Formato non valido
    }
}
/*-------------------------------------------------------------------------------------------*/

/*----------------------------funzioni gestione lista socket--------------------------------*/
// Funzione per creare una nuova struct SocketNode con una socket pronta per l'uso
struct SocketNode *crea_socket_node() {
    struct SocketNode *newNode = (struct SocketNode *)malloc(sizeof(struct SocketNode));
    if (newNode == NULL) {
        perror("Errore nell'allocazione di memoria per SocketNode");
        exit(EXIT_FAILURE);
    }

    // Crea la socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Errore nella creazione della socket");
        free(newNode);
        exit(EXIT_FAILURE);
    }

    // Imposta l'indirizzo a zero (puoi personalizzarlo secondo le tue esigenze)
    memset(&newNode->addr, 0, sizeof(newNode->addr));
    newNode->addr.sin_family = AF_INET;
    newNode->addr.sin_addr.s_addr =INADDR_ANY;
    newNode->addr.sin_port = htons(0); // Assegna una porta libera

    // Associa la socket all'indirizzo
    if (bind(sockfd, (struct sockaddr *)&newNode->addr, sizeof(newNode->addr)) == -1) {
        perror("Errore nella bind");
        close(sockfd);
        free(newNode);
        exit(EXIT_FAILURE);
    }

    newNode->sockfd = sockfd;
    newNode->addrlen = sizeof(newNode->addr);
    newNode->next = NULL;

    // Ottieni la porta assegnata dal sistema operativo
    if (getsockname(sockfd, (struct sockaddr *)&newNode->addr, &newNode->addrlen) == -1) {
        perror("Errore nell'ottenere la porta");
        close(sockfd);
        free(newNode);
        exit(EXIT_FAILURE);
    }

    return newNode; // Restituisce un puntatore alla nuova struct SocketNode con la porta assegnata
}

struct SocketNode *crea_main_socket_node() {
    struct SocketNode *newNode = (struct SocketNode *)malloc(sizeof(struct SocketNode));
    if (newNode == NULL) {
        perror("Errore nell'allocazione di memoria per SocketNode");
        exit(EXIT_FAILURE);
    }

    // Crea la socket UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Errore nella creazione della socket");
        free(newNode);
        exit(EXIT_FAILURE);
    }

    // Imposta l'indirizzo a zero (puoi personalizzarlo secondo le tue esigenze)
    memset(&newNode->addr, 0, sizeof(newNode->addr));
    newNode->addr.sin_family = AF_INET;
    newNode->addr.sin_addr.s_addr =INADDR_ANY;
    newNode->addr.sin_port = htons(55555); // Assegna una porta libera

    // Associa la socket all'indirizzo
    if (bind(sockfd, (struct sockaddr *)&newNode->addr, sizeof(newNode->addr)) == -1) {
        perror("Errore nella bind");
        close(sockfd);
        free(newNode);
        exit(EXIT_FAILURE);
    }

    newNode->sockfd = sockfd;
    newNode->addrlen = sizeof(newNode->addr);
    newNode->next = NULL;
/*
    // Ottieni la porta assegnata dal sistema operativo
    if (getsock