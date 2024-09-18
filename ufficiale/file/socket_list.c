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
    if (getsockname(sockfd, (struct sockaddr *)&newNode->addr, &newNode->addrlen) == -1) {
        perror("Errore nell'ottenere la porta");
        close(sockfd);
        free(newNode);
        exit(EXIT_FAILURE);
    }
*/
    return newNode; // Restituisce un puntatore alla nuova struct SocketNode con la porta assegnata
}
// Restituisci il puntatore alla socket da un nodo
int ottieni_socket(struct SocketNode *node) {
    return node->sockfd;
}

// Funzione per aggiungere un nodo alla fine della lista
void aggiungi_nodo(SocketList *list, struct SocketNode *newNode) {
    if (*list == NULL) {
        *list = newNode;
    } else {
        struct SocketNode *current = *list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }
}

// Funzione per cercare un nodo con una determinata socket nella lista
struct SocketNode *trova_nodo(SocketList list, int sockfd) {
    struct SocketNode *current = list;
    while (current != NULL) {
        if (current->sockfd == sockfd) {
            return current;
        }
        current = current->next;
    }
    return NULL; // Nodo non trovato
}

void elimina_nodo(SocketList *list, int sockfd) {
    struct SocketNode *current = *list;
    struct SocketNode *prev = NULL;
    
    while (current != NULL) {
        if (ntohs(current->addr.sin_port )== sockfd) {
            if (prev == NULL) {
                *list = current->next;
            } else {
                prev->next = current->next;
            }
            close(current->sockfd); // Chiudi la socket prima di liberare la memoria
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}
void stampa_coda(SocketList list) {
    struct SocketNode *current = list;
    printf("Printing the entire list\n");
    while (current != NULL) {
        printf("Socket Descriptor: %d\t", current->sockfd);
        printf("Indirizzo IP: %s\t", inet_ntoa(current->addr.sin_addr));
        printf("Porta: %d\n\n", ntohs(current->addr.sin_port));
        current = current->next;
    }
}
// Funzione per liberare la memoria allocata per la lista
void libera_lista(SocketList list) {
    while (list != NULL) {
        struct SocketNode *temp = list;
        list = list->next;
        close(temp->sockfd); // Chiudi la socket prima di liberare la memoria
        free(temp);
    }
}
/*-------------------------------------------------------------------------------------------*/

/*-----------------------------------funzioni directory--------------------------------------*/
// Funzione per elencare i file in una cartella
int elencoNomiFileInCartella(const char *path, char **buffer) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) {
        perror("Errore nell'apertura della cartella");
        exit(1);
    }

    size_t buffer_size = 0;
    size_t current_length = 0;
    char *current_entry;
    int file=0;
    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG) { // Verifica se è un file
            current_entry = entry->d_name;
            current_length = strlen(current_entry);
            file++;
            // Calcola la nuova dimensione del buffer
            buffer_size += current_length + 1; // +1 per il separatore (newline o null-terminator)

            // Alloca più memoria per il buffer
            *buffer = (char *)realloc(*buffer, buffer_size);

            // Copia il nome del file nel buffer
            strcat(*buffer, current_entry);
            strcat(*buffer, "\n");
        }
    }
    if(file==0){
        return 0;
    }
    closedir(dp);
    return 1;
}

// Funzione per verificare se un file è presente in una cartella
int fileExistsInDirectory(const char *folderPath, const char *fileName) {
    struct dirent *entry;
    DIR *dp = opendir(folderPath);

    if (dp == NULL) {
        perror("Errore nell'apertura della directory");
        return -1;
    }

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG && strcmp(entry->d_name, fileName) == 0) {
            closedir(dp);
            return 1;  // Il file esiste nella cartella
        }
    }

    closedir(dp);
    return 0;  // Il file non esiste nella cartella
}
/*-------------------------------------------------------------------------------------------*/
/*--------------------------------funzione stampa colorata---------------------------------*/
void printfclr(int color, const char *format, ...) {
    switch (color) {
        case 1:
            printf("\033[31m"); // Rosso
            break;
        case 2:
            printf("\x1b[34m"); // Blu
            break;
        case 3:
            printf("\033[32m"); // Ciano
            break;
        default:
            printf("\033[0m");  // Reset al colore predefinito
            break;
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\033[0m");  // Assicura che il colore torni al predefinito alla fine
}

/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------help function------------------------------------------*/
void help_function(){
    printf("\n\nBENVENUTO NELLA PAGINA DI AIUTO\n\n");
    printf("Di seguito saranno elencati i comandi disponibili.\n");
    printfclr(2,"help                            fornisce in quasiasi momento un manuale per l'uso\n");
    printfclr(2,"list                            ritorna la lista dei file contenuti nella cartella corrente del server\n");
    printfclr(2,"put [local file] [remote path]\ttrasferisce il file da [local path] a [remote path]\n");
    printfclr(2,"get [remote path] [local path]\ttrasferisce il file da [remote path] a [local path]\n");
    printfclr(2,"fin                             esce dal programma in maniera sicura\n");
    printf("\n\nEND\n\n");

}
/*-------------------------------------------------------------------------------------------*/
/*----------------------------request function-----------------------------------------------*/
int checkAndCreateFolder(char *folderName) {
    struct stat st;

    // Verifica se la cartella "file" esiste nel percorso corrente
    if (stat(folderName, &st) == 0) {
        // La cartella esiste già
        if (S_ISDIR(st.st_mode)) {
            return 1;
        } else {
            return -1;
        }
    } else {
        // La cartella non esiste, quindi la creiamo
        if (mkdir(folderName, 0755) == 0) {
            return 0;
        } else {
            return -1;
        }
    }
}


int send_file(const char *nome_file, size_t segment_size,int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen) {
    char percorso_completo[256];  // Assumiamo una dimensione massima di 256 caratteri per il percorso
    snprintf(percorso_completo, sizeof(percorso_completo), "./file/%s", nome_file);
    FILE *file = fopen(percorso_completo, "rb");
    if (file == NULL) {
        perror("Errore nell'apertura del file");
        return 0;
    }

    
    size_t bytes_read;



    // Leggi il nome e la dimensione del file
    struct FileHeader header;
    strncpy(header.nome, nome_file, sizeof(header.nome));
    header.dimensione = 0;
    char *buffer=malloc(sizeof(char)*header.dimensione);
    fseek(file, 0L, SEEK_END);
    header.dimensione = ftell(file);
    fseek(file, 0L, SEEK_SET);
    // Invia l'intestazione del file
    ssize_t header_sent = send_r(sockfd, &header, sizeof(header), dest_addr, addrlen);
    if (header_sent == -1) {
        perror("Errore nell'invio dell'intestazione del file");
        fclose(file);
        return 0;
    }
    printf(" ");
    while ((bytes_read = fread(buffer, 1, segment_size, file)) > 0) {
        ssize_t bytes_sent = send_r(sockfd, buffer, bytes_read, dest_addr, addrlen);
        if (bytes_sent == -1) {
            perror("Errore nell'invio del segmento");
            break;
        }
    }

    // Invia un segmento vuoto per segnalare la fine del file
    bzero(buffer,strlen(buffer));
    ssize_t bytes_sent = send_r(sockfd, buffer, 0, dest_addr, addrlen);
    if (bytes_sent == -1) {
        perror("Errore nell'invio del segmento di chiusura");
    }

    fclose(file);
    //free(buffer);
    return 1;
}

int receive_and_write_file(size_t segment_size, int sockfd) {
    
    ssize_t bytes_received;

    // Ricevi l'intestazione del file
    struct FileHeader header;
    bytes_received = rcvfrom_r(sockfd, &header, sizeof(header), 0, 0);
    if (bytes_received == -1) {
        perror("Errore nella ricezione dell'intestazione del file");
        close(sockfd);
        return 0;
    }else if (strcmp(header.nome, "999")==0)
    {
        //printf("il file non esiste\n");
        return 2;
    }else if(strcmp(header.nome, "666")==0){
        //printf("il file name mancante\n");
        return 3;
    }else{
    
        if(checkAndCreateFolder("./file_received") == -1){
            perror("impossibile aprire o creare cartella");
        }
        char percorso_completo[256];  // Assumiamo una dimensione massima di 256 caratteri per il percorso
        snprintf(percorso_completo, sizeof(percorso_completo), "./file_received/%s", header.nome);

        FILE *file = fopen(percorso_completo, "wb");
        if (file == NULL) {
            perror("Errore nell'apertura del file");
            close(sockfd);
            return 0;
        }
        char *buffer = malloc(sizeof(char)*header.dimensione);
        int byte_total=0;
        
        while (byte_total<header.dimensione) {
            bytes_received = rcvfrom_r(sockfd, buffer, segment_size, 0, 0);
            
            if (bytes_received == -1) {
                perror("Errore nella ricezione del segmento");
                break;
            } else if (bytes_received == 0) {
                // Ricevuto EOF
                break;
            }
            fwrite(buffer, 1, bytes_received, file);
            byte_total+=bytes_received;
            mostraBarra((byte_total*100)/(header.dimensione),100);
            //printf("%d\n",byte_total); debug
        }
        //printf("%d-%d\n",header.dimensione,byte_total); debug
        buffer[byte_total]='\0';
        fclose(file);
        free(buffer);
        return 1;
    }
}

/*funzione che usa il server per una get_request_from_client*/
int get_request_from_client(char *request_buffer,int sockfd,const struct sockaddr *dest_addr, socklen_t addrlen){
    
    if(checkAndCreateFolder("./file") == -1){
        perror("impossibile aprire o creare cartella");
    }

    char *spazio = strchr(request_buffer, ' ');
    if (spazio != NULL) {
        // Trovato uno spazio, avanziamo di un carattere per ottenere la parte successiva
        spazio++; // avanziamo al carattere successivo al primo spazio
        char filename[sizeof(request_buffer) - (spazio - request_buffer)]; // Alloca una nuova stringa
        strcpy(filename, spazio); // Copia la parte successiva nella nuova stringa
        if(fileExistsInDirectory("./file",filename)==1){
            //il file richiesto esiste, provvedo ad  inviarlo
            printfclr(2,"-->chiamo send_file per inviare %s\n",filename);
            int file_send_ok =send_file(filename,8192,sockfd,dest_addr,addrlen);
            if(file_send_ok==-1){
                perror("errore nella send file");
                exit(1);
            }else{
                printfclr(2,"-->id:%d-file inviato\n",sockfd);
                return 1;
            }
        }else{
        //il file non esiste, avviso il client
            // Leggi il nome e la dimensione del file
            struct FileHeader header;
            char file_not_exist_code[3]="999";
            strncpy(header.nome,file_not_exist_code , sizeof(3));
            header.dimensione = 0;
            printfclr(1,"-->id:%d-il client cerca un file che non esiste\n",sockfd);
            int snd_byte = send_r(sockfd, &header, sizeof(header), dest_addr,addrlen);
                if(snd_byte==-1){
                perror("error sending file name error, 999 code");
                exit(1);
            }

        }
    }else{
        //il file name non inserito, avviso il client
        struct FileHeader header;
        char file_name_missed_code[3]="666";
        strncpy(header.nome,file_name_missed_code, sizeof(3));
        header.dimensione = 0;
        printfclr(1,"-->id:%d-il client ha sbagliato a digitare il comando\n",sockfd);
        int snd_byte = send_r(sockfd, &header, sizeof(header), dest_addr,addrlen);
            if(snd_byte==-1){
            perror("file name mancante from client, 666 code");
            exit(1);
        }
    }
    return 1;
}

/*funzoine che usa il server per una list_request_from_client*/
int list_request_from_client(char *request_buffer,int sockfd,const struct sockaddr *dest_addr, socklen_t addrlen){
   
    if(checkAndCreateFolder("./file") == -1){
        perror("impossibile aprire o creare cartella");
    }

    char *fileNamesBuffer  = malloc(sizeof(char)*256);
    if(elencoNomiFileInCartella("./file", &fileNamesBuffer)==0){
        //caso cartella vuota
        int snd_byte = send_r(sockfd,"-->empty folder\n",strlen("cartella vuota\n"), dest_addr,addrlen);
        if(snd_byte==-1){
            perror("error sending list buffer to client");
            exit(1);
        }
    }else{
        //caso cartella non vuota
        int snd_byte = send_r(sockfd,fileNamesBuffer,strlen(fileNamesBuffer), dest_addr,addrlen);
        
        if(snd_byte==-1){
            perror("error sending list buffer to client");
            return 1;
        }
        bzero(fileNamesBuffer,strlen(fileNamesBuffer));
    }
    return 1;
}

/*funzione che usa il server per una put_request_from_client*/
int put_request_from_client(char *request_buffer,int sockfd,const struct sockaddr *dest_addr, socklen_t addrlen){
/*put request va gestita cosi: il server legge il comando, dice al client che è pronto, si aspetta dal client la dimensione del file,
crea il file della dimensione giusta, manda un ack, e si mette in attesa per scrivere nel file fino a completamento.*/
    
    //1. controllo se esiste la cartella dove mettere il file
    if(checkAndCreateFolder("./file") == -1){
        perror("impossibile aprire o creare cartella");
    }
    // Cerca il primo spazio nel buffer
    char* spacePtr = strchr(request_buffer, ' ');
    if (spacePtr == NULL) {
        perror("comando errato");  
    }
    size_t length = strlen(spacePtr + 1);
    char* filename = (char*)malloc(length + 1);
    if (filename == NULL) {
        perror("errore malloc");
    }
    strcpy(filename, spacePtr + 1);
    printfclr(2,"-->id:%d file in arrivo %s\n",sockfd, filename);
        //2. controllo se il file richiesto esiste nella cartella
    if(fileExistsInDirectory("./file_received",filename)==1){
        //2.1 se esiste elimino il file da ./file e mi metto in recv_file  

        //printfclr(2,"il file esiste già lo elimino\n");
        char comando_elimina_file[128];
        snprintf(comando_elimina_file,sizeof(comando_elimina_file),"rm ./file_received/%s",filename);
        printfclr(0,"-->eseguo: %s\n",comando_elimina_file);
        system(comando_elimina_file);
        int snd_byte = send_r(sockfd,"ACK",strlen("ACK"), dest_addr,addrlen);
        if(snd_byte==-1){
            perror("error sending ack for put");
            exit(1);
        }

        int recv_file_ok = receive_and_write_file(8192,sockfd);
        if(recv_file_ok==0){
            perror("error in recv file from client");
        }


    }else{
        //2.2 se non esiste mi metto recv_file
        //printf("il file non esiste lo ricevo\n");

        int snd_byte = send_r(sockfd,"ACK",strlen("ACK"), dest_addr,addrlen);
        if(snd_byte==-1){
            perror("error sending ack for put");
            exit(1);
        }    

        int recv_file_ok = receive_and_write_file(512,sockfd);
        if(recv_file_ok==0){
            perror("error in recv file from client");
        
        }  
    }
}

void get_ip(){
        char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Errore durante la lettura del nome host");
        
    }

    struct hostent *host_info = gethostbyname(hostname);

    if (host_info == NULL) {
        perror("Errore durante la risoluzione del nome host");
        
    }

    struct in_addr **addr_list = (struct in_addr **)host_info->h_addr_list;
    for (int i = 0; addr_list[i] != NULL; i++) {
        printf("Indirizzo IP: %s\n", inet_ntoa(*addr_list[i]));
    }

}

void mostraBarra(int progresso, int lunghezzaBarra) {
    int i;
    printf("[");
    for (i = 0; i < lunghezzaBarra; i++) {
        if (i < progresso)
            printf("*");
        else
            printf(" ");
    }
    printf("] %d%%\r", (progresso * 100) / lunghezzaBarra);
    fflush(stdout);
}