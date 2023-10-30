#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "socket_list.h"
#include "reliable.h"

SocketList socketList = NULL;
#define MAX_BUFFER_LEN 512
struct SocketNode *error_node;


void * client_handle(void *data){
        
    struct sockaddr *addr=malloc(sizeof(struct sockaddr *));
    memcpy(addr,data,sizeof(struct sockaddr*));
    
    // Creazione di un nuovo nodo con una socket pronta per l'uso
    struct SocketNode *thread_socket_node = crea_socket_node();
    int thread_sockfd = ottieni_socket(thread_socket_node);   // Ottenere la socket da un nodo       
    aggiungi_nodo(&socketList, thread_socket_node);    // Aggiunta del nodo alla fine della lista
    
    //codifico pacchetto con informazioni del server
    DataPacket *server_synack_pkg = malloc(sizeof(DataPacket));
    server_synack_pkg->port_number  =ntohs(thread_socket_node->addr.sin_port);
    server_synack_pkg->error_port  =ntohs(error_node->addr.sin_port);
    printfclr(1,"->thread_socket fd:%d\tport:%d\tip:%s\n",thread_socket_node->sockfd,ntohs(thread_socket_node->addr.sin_port),inet_ntoa(thread_socket_node->addr.sin_addr)); // stampo fd della threa socket
    strcpy(server_synack_pkg->ip_addr,inet_ntoa(thread_socket_node->addr.sin_addr));
    
    char *first_responce=composeDataPacket(server_synack_pkg);
    char *rcv_buffer=malloc(sizeof(char)*MAX_BUFFER_LEN); //buffer per immagazzinare ciò che arriva
    
    char ipstr[INET6_ADDRSTRLEN];
    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
        inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;
        inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipstr, sizeof(ipstr));
    }
    printfclr(1,"->Indirizzo IP del client: %s\n", ipstr);
    printf("thread:%p\n",addr);

    int snd_byte = send_r(thread_sockfd,first_responce,strlen(first_responce),addr,sizeof(struct sockaddr));
    if(snd_byte==-1){ 
        perror("error sending threa dinfo to client");
        exit(1);
    }else{
        int rcv_byte  = rcvfrom_r(thread_sockfd,rcv_buffer,MAX_BUFFER_LEN, 0,0);
        if (rcv_byte==-1){
            perror("rcv ack from client error");
            exit(1);
        }else{
            if(strcmp(rcv_buffer,"ACK")==0){
                int snd_byte = send_r(thread_sockfd,"ACK",strlen("ACK"),addr,sizeof(struct sockaddr));
                if(snd_byte==-1){
                    perror("error sending to client new thread info");
                    exit(1);
                }

                //connessione con il client stabilita, inizia la fase di richiesta utente, resto in attesa
                while(1){
                    char get_request[] = "get";
                    char put_request[] = "put";
                    bzero(rcv_buffer,strlen(rcv_buffer));
                    int rcv_byte  = rcvfrom_r(thread_sockfd,rcv_buffer,MAX_BUFFER_LEN,0,0);
                    if (rcv_byte==-1){
                        printfclr(1,"-->%s:", strerror(errno));
                        printf("socket_fd non valido o è stato chiuso\n");
                        pthread_exit(NULL);
                    }else{
                        //se mi arriva un messaggio qui dal client è una richiesta, la elaboro:
                        if(strcmp(rcv_buffer,"list")==0)
                        { /*CASO LIST RISPOSTA*/                           
                            
                            list_request_from_client(rcv_buffer,thread_sockfd,addr,sizeof(struct sockaddr));

                        }
                        else if (strncmp(rcv_buffer, get_request, 3)==0)
                        {
                            get_request_from_client(rcv_buffer,thread_sockfd,addr,sizeof(struct sockaddr));
                        }
                        else if (strncmp(rcv_buffer, put_request, 3)==0)
                        {
                            
                            put_request_from_client(rcv_buffer,thread_sockfd,addr,sizeof(struct sockaddr));
                        }
                        else{
                            int snd_byte = send_r(thread_sockfd,"comando sconosciuto",strlen("comando sconosciuto"),addr,sizeof(struct sockaddr));
                            if(snd_byte==-1){
                                perror("error sending to client new thread info");
                                exit(1);
                            }
                            continue;
                        }
                    }
                }//while()
            }
        }
    }   
}


void *error_handle(struct SocketNode error_private_socket){
    // thread dedicato all'arrivo di chiusure inaspettate da parte dei client
    // stampo fd della error socket
    printfclr(3,"->error_socket fd:%d\tport:%d\tip:%s\n",error_node->sockfd,ntohs(error_node->addr.sin_port),inet_ntoa(error_node->addr.sin_addr)); 

    int sock_fd_error = ottieni_socket(&error_private_socket);
    while(1){

        char *buffer_error=malloc(sizeof(char)*20);
        int rcv_error=rcvfrom_r(sock_fd_error,buffer_error,20,0,0);
        printfclr(1,"%s\n",buffer_error);
        if(rcv_error==-1){
            perror("error in rcv error from client");
            exit(1);
        }
        printfclr(1,"->Chiusura del client inaspettata. Chiudo socket di appertenenza sulla porta: %d\n",atoi(buffer_error));
       
        int socket_da_cestinare=atoi(buffer_error);
        //close(socket_da_cestinare);
        elimina_nodo(&socketList,socket_da_cestinare);
        bzero(buffer_error,strlen(buffer_error));
        free(buffer_error);
        
    }

}

int main(int argc, char**argv){

    printfclr(3,"->Benvenuto nel programma.\n->Per avviare un client usa:");
    printfclr(0,"./client <number port\n");
    get_ip();

    // Creazione di un nuovo nodo con una socket pronta per l'uso
    struct SocketNode *main_socket_node = crea_main_socket_node();
    int sockfd_main = ottieni_socket(main_socket_node);   // Ottenere la socket da un nodo       
    aggiungi_nodo(&socketList, main_socket_node);    // Aggiunta del nodo alla fine della lista  
    printfclr(3,"->main_socket fd:%d\tport:%d\tip:%s\n",main_socket_node->sockfd,ntohs(main_socket_node->addr.sin_port),inet_ntoa(main_socket_node->addr.sin_addr)); // stampo fd della main socket
    
    /*INIZZIALIZZO thread ERRORE*/
    error_node = crea_socket_node();
    pthread_t error_tid;
    if(pthread_create(&error_tid,NULL,(void*)error_handle,(void*)error_node)==-1){
        perror("Error in creating thread ");
    }

    char *buffer=malloc(sizeof(char)*3);
    void *addr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
    while(1){
    
        
        bzero(addr,sizeof(struct sockaddr *));
        socklen_t len = sizeof(struct sockaddr);
        main_socket_node->addr.sin_port = htons(55555);
        bzero(buffer,sizeof(char)*3);
        printfclr(3,"->Pronto per una nuova connesione\n");
        int bytes_received = rcvfrom_r(sockfd_main, buffer, 3, addr, &len);
        if (bytes_received == -1) {
            perror("Errore nella rcvfrom_r");
            continue; // Continua a rimanere in ascolto per altri pacchetti
        }
        printfclr(3,"->Ricevuto:%s.Nuovo thread in creazione\n",buffer);
     
        pthread_t tid;
        if(pthread_create(&tid,NULL,(void*)client_handle,(void *)addr)==-1){
            perror("Error in creating thread ");
            continue;
        }
        printf("%d-%p\n",bytes_received,addr);
        
    }   
    stampa_coda(socketList);
    libera_lista(socketList);

    return 0;
}