#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <sys/select.h>

#define MSS 4096
#define ENDSEQ 7
#define BEGSEQ 6
#define ACK 3
#define RTT_mics 500

// Dichiarazioni di funzioni, strutture, variabili globali, ecc.



typedef struct {
    char seq_n[16];
    char ack_n[16];
    char code;
    char data[MSS];
} Packet;


int error(const char *msg);
int min(int a, int b);
int max(int a, int b);
int send_r(int socket_fd, char* buffer, size_t size, struct sockaddr *server_addr, socklen_t server_len);
int rcvfrom_r(int socket_fd, void *buffer, size_t length, struct sockaddr *client_addr_0, socklen_t* client_len);

