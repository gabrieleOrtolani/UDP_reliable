#ifndef SOCKET_LIST_H
#define SOCKET_LIST_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"

typedef struct {
    char ip_addr[16];   // Massimo 15 caratteri per un indirizzo IPv4 (es. "192.168.1.1")
    int port_number;    // Numero di porta (intero)
    int error_port;
} DataPacket;

// Definizione della struttura del nodo
struct SocketNode {
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addrlen;
    struct SocketNode *next; // Puntatore al prossimo nodo
};
typedef struct SocketNode *SocketList;
// Definizione della struttura client
struct Client {
    struct sockaddr_in client_addr;
    socklen_t client_len;
};

// Struttura per l'intestazione del file
struct FileHeader {
    char nome[256];  // Nome del file (massimo 255 caratteri)
    size_t dimensione;  // Dimensione del file in byte
};


// Funzione per creare una nuova struct SocketNode con una socket pronta per l'uso
struct SocketNode *crea_socket_node();

struct SocketNode *crea_main_socket_node();

// Restituisci il puntatore alla socket da un nodo
int ottieni_socket(struct SocketNode *node);


// Funzione per aggiungere un nodo alla fine della lista
void aggiungi_nodo(SocketList *list, struct SocketNode *newNode);

// Funzione per cercare un nodo con una determinata socket nella lista
struct SocketNode *trova_nodo(SocketList list, int sockfd);

// Funzione per eliminare un nodo dalla lista
void elimina_nodo(SocketList *list, int sockfd);

// Funzione per liberare la memoria allocata per la lista
void libera_lista(SocketList list);

// stampo intera lista
void stampa_coda(SocketList list);

// Funzione per elencare i file in una cartella
int elencoNomiFileInCartella(const char *path, char **buffer);

//funzione che ritorna se un file è presente o meno in una cartella
int fileExistsInDirectory(const char *folderPath, const char *fileName);

char* composeDataPacket(const DataPacket *packet);

int parseDataPacket(const char *buffer, DataPacket *packet);

void printfclr(int color, const char *format, ...);

void help_function();

int checkAndCreateFolder(char *folderName);

int get_request_from_client(char *request_buffer,int sockfd,const struct sockaddr *dest_addr, socklen_t addrlen);

int list_request_from_client(char *request_buffer,int sockfd,const struct sockaddr *dest_addr, socklen_t addrlen);

int put_request_from_client(char *request_buffer,int sockfd,const struct sockaddr *dest_addr, socklen_t addrlen);

int receive_and_write_file(size_t segment_size, int sockfd);

int send_file(const char *nome_file, size_t segment_size,int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen);

void mostraBarra(int progresso, int lunghezzaBarra);

void get_ip();

#endif
