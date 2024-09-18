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

#define MAX_BUFFER_SIZE 512
// Funzione di gestione dei segnali
int pippo; //porta comunicazione thread_server<->client
struct sockaddr_in server_addr;
int sockfd;
int error_port;

int variable = 0;

void timer_handler(int signo) {
    if (variable == 0) {
        printfclr(1,"->The server has left the network\n");
        exit(1);
    }
}
// Funzione di gestione del segnale SIGTERM
void gestione_sigterm(int segnale) {
    char tosend[16];
    
    sprintf(tosend,"%d", pippo);
    printfclr(1,"\n->gestisco il segnale ctrl+C.  Chiusura socket %s\n",tosend);
    
    server_addr.sin_port = htons(error_port);
    signal(SIGALRM, timer_handler); // Imposta il gestore per il segnale di allarme
    alarm(5); // Imposta un timer di 5 secondi
    int snd_byte=send_r(sockfd,tosend,16, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(snd_byte==-1){
        perror("error in sendind signal to server");
        exit(1);
    }
    alarm(0);// cancello il timer
    //close(socket);
    exit(1); // Terminazione pulita
}
int main(int argc, char **argv) {
    if(argc<2){
        printfclr(1,"Usage: ");
        printfclr(0,"./client <server_ip_addr>\n");
        exit(1);
    }
    printfclr(3,"Benvenuto nel programma UPDR. Puoi digitare help in qualsiasi momento\n");
    socklen_t server_len;

    // Creazione della socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Errore nella creazione della socket");
        exit(EXIT_FAILURE);
    }

    // Impostazione dell'indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(55555); // Sostituisci con la porta del server
    server_addr.sin_addr.s_addr = inet_addr(argv[1]); // Indirizzo IP del server

    // Messaggio da inviare
    const char *message = "SYN";
    
    // Invio del messaggio al server
    if (send_r(sockfd, message, strlen(message), (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Errore nell'invio del messaggio");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    /*da questo momento in poi controllo SIGTERM*/

    struct sigaction sa;
    sa.sa_handler = gestione_sigterm;
    // Imposta il gestore di segnali per SIGTERM
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Errore nell'impostazione del gestore di segnali");
        exit(EXIT_FAILURE);
    }
    
   
    /*fine controllo segnale*/
    char rcv_buffer[MAX_BUFFER_SIZE];
   // printf("Messaggio inviato al server: %s\n", message);
   int rcv_byte  = rcvfrom_r(sockfd,rcv_buffer,MAX_BUFFER_SIZE,NULL,NULL);
   if (rcv_byte==-1){
        perror("rcv new server info error");
        exit(1);
   }else{
        //parte la vera connessione
        DataPacket *synack_pkg = malloc(sizeof(DataPacket));
        parseDataPacket(rcv_buffer, synack_pkg);
        server_addr.sin_port = htons(synack_pkg->port_number); // Sostituisci con la porta del server
        error_port=synack_pkg->error_port;
        pippo=synack_pkg->port_number;
        printfclr(3,"->error_socket\tport: %d\n",synack_pkg->error_port);
        char *snd_buffer = "ACK";
        if (send_r(sockfd, snd_buffer, strlen(snd_buffer), (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Errore nell'invio del messaggio");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        bzero(rcv_buffer,sizeof(rcv_buffer));
        int rcv_byte  = rcvfrom_r(sockfd,rcv_buffer,MAX_BUFFER_SIZE,NULL,NULL);
        if (rcv_byte==-1){
            perror("rcv new server info error");
            exit(1);
        }else{
            if(strcmp(rcv_buffer,"ACK")==0){
                while(1){
                    server_addr.sin_port = htons(synack_pkg->port_number);
                    // connessione avviata con successo, adesso posso fare una richiesta utente al server
                    //inserisco la richiesta
                    char comando[256]; // Una variabile chiamata "comando" per contenere l'input dell'utente
                    char get_request[] = "get";
                    char put_request[] = "put";
insert_command:
                    memset(comando, 0, sizeof(comando));
                    printf("\x1b[33mudpr>>\x1b[0m "); /*scrivo arancio la richiesta del comando*/
                    if (fgets(comando, sizeof(comando), stdin) != NULL) {
                    // Rimuovi il carattere di newline (\n) dalla fine dell'input se presente
                    size_t inputLength = strlen(comando);
                    if (inputLength > 0 && comando[inputLength - 1] == '\n') {
                        comando[inputLength - 1] = '\0';
                    }else{
                        perror("reading input error");
                    }
                    //analizzo il comando
                    if (strcmp(comando,"list")==0){ /*CASO LIST*/
                         //se la richiesta è LIST leggo e stampo a video la risposta
                         printf("mando list\n");
                        if (send_r(sockfd, comando, strlen(comando), (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
                            perror("Errore nell'invio del messaggio");
                            close(sockfd);
                            exit(EXIT_FAILURE);
                        }else{
                        // attendo la risposta dal server per listare a schermo i files
                            bzero(rcv_buffer,strlen(rcv_buffer));
                            char *listfiles = malloc(sizeof(char)*256);
                            int rcv_byte  = rcvfrom_r(sockfd,listfiles,512,NULL,NULL);
                            if (rcv_byte==-1){
                                perror("rcv list file error");
                                exit(1);
                            }else{
                                bzero(comando,strlen(comando));
                                printf("%s\n",listfiles);
                                fflush(stdout);
                                continue;
                            }
                        }                   
                    }else if (strncmp(comando,put_request,3)==0){ /*CASO PUT*/
                        //codice per implementare put file
                        //se la richiesta è PUT mando e attendo la risposta che il file è stato ricevuto
                        //controllo memoria
                        //put()
                        char *spazio = strchr(comando, ' ');
                        if (spazio != NULL) {
                            // Trovato uno spazio, avanziamo di un carattere per ottenere la parte successiva
                            spazio++; // avanziamo al carattere successivo al primo spazio
                            char filename[sizeof(comando) - (spazio - comando)]; // Alloca una nuova stringa
                            strcpy(filename, spazio); // Copia la parte successiva nella nuova stringa
                            if(fileExistsInDirectory("./file",filename)==1){
                                //codice per mandare il file
                                int snd_byte = send_r(sockfd, comando, strlen(comando), (struct sockaddr *)&server_addr, sizeof(server_addr));
                                if(snd_byte==-1){
                                    perror("->errror in sending put filename");
                                    continue;
                                }else{
                                    char rcv_buffer[3];
                                    int rcv_byte  = rcvfrom_r(sockfd,rcv_buffer,sizeof(rcv_buffer),NULL,NULL);
                                    if (rcv_byte==-1){
                                        perror("rcv list file error");
                                        exit(1);
                                    }else{
                                        //controllo se mi è arrivato l'ACK
                                        //printfclr(2,"mi è arrivato %s\n",rcv_buffer);
                                        if(strcmp(rcv_buffer,"ACK")==0){
                                            int file_send_ok =send_file(filename,4096,sockfd,(struct sockaddr *)&server_addr, sizeof(server_addr));
                                            if(file_send_ok==-1){
                                                perror("errore nell'invio del file");
                                                exit(1);
                                            }else{
                                                printfclr(2,"-->file inviato\n\n");
                                                bzero(comando,strlen(comando));
                                                fflush(stdin);                                               
                                            }
                                        }
                                    }
                                }
                            bzero(comando,strlen(comando));
                            fflush(stdin);
                            }else{
                                printfclr(1,"file name mancante nella cartella ./file\n");
                                goto insert_command;
                            }
                            
                        }else{
                            //caso in cui scrivo solo put
                            printfclr(1,"comando non valido\n");
                            goto insert_command;

                        }

                    }else if (strncmp(comando, get_request, 3)==0) /*CASO GEGT*/
                    {
                       //codice per implementare la richiesta get file
                       //se la richiesta è GET mando e aspetto la risposta
                        if (send_r(sockfd, comando, strlen(comando), (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
                            perror("Errore nell'invio del messaggio");
                            close(sockfd);
                            exit(EXIT_FAILURE);
                        }else{
                            int recv_file_ok = receive_and_write_file(4096,sockfd);
                            if (recv_file_ok==0){
                                perror("error in receiving file");
                                goto insert_command;
                            }else if (recv_file_ok==2)
                            {
                                printfclr(1,"il file richiesto non esiste nel server\n");
                                goto insert_command;
                            }else if(recv_file_ok==3){
                                 printfclr(1,"file name mancante\n");
                                goto insert_command;
                            } else{
                                printfclr(2,"\n-->file ricevuto\n\n");
                            }
                        }
                    }else if (strcmp(comando,"fin")==0) /*CASO FIN*/
                    {
                       //codice per implementare l'uscita dalla connessione con il server
                       // se la richiesta é FIN mando e aspetto FIN_ACK per uscire

                    }else if (strcmp(comando,"help")==0)
                    {
                        help_function();
                    
                    }
                    else{
                        printf("comando non valido, riprova\n");
                        goto insert_command;
                    }

                    }    
                }
            }

        }
        //fai la free delle strutture pacchetto
   }

    // Chiudi la socket
    close(sockfd);

    return 0;
}
