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

int verbose = 0;
int datadisplay = 1;
int blocking = 1;
int reliable =0; // se 1 usa reliable

int error(const char *msg);
int min(int a, int b);
int max(int a, int b);
int send_r(int socket_fd, char* buffer, long size, struct sockaddr *server_addr, socklen_t server_len);
int rcvfrom_r(int socket_fd, void *buffer, size_t length, struct sockaddr *client_addr_0 , socklen_t* client_len);


int error(const char *msg) {
    if(verbose) perror(msg);
    return -1;
}

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int rcvfrom_r(int socket_fd, void *buffer, size_t length, struct sockaddr *client_addr_0, socklen_t* client_len_0){
    if (reliable){
        int rcv_byte=recvfrom(socket_fd,buffer,length,0,client_addr_0,client_len_0);
        if(rcv_byte==-1){
            perror("error in recvfrom");
        }
        printf("notrel\n");
        return rcv_byte;
    }
    FILE *file_log = fopen("pippo.txt","a");
    Packet recv_packet, ack_packet;
    int   gap, loss, totcount = 0, randn = 0, ack2send = 0, endack = -1;
    int  expected_seq_n = 0;
    char *ptr = buffer, freec = '0';
    int  bytes_end = 0;

    
    size_t count  = 0;
    fd_set  rfds;

    struct timeval  tv, *tvptr;


    int buffMSS =((int)ceil((float)length/(float)MSS))+1;

    int *buffvect = malloc(sizeof(int)*buffMSS);
    for(int i=0;i<buffMSS;i++){buffvect[i]=1;}

    if (blocking==1){
        tvptr = NULL;
    }else tvptr = &tv;
    FD_ZERO(&rfds);


    struct sockaddr* client_addr = malloc(sizeof(struct sockaddr));
    socklen_t* client_len = malloc(sizeof(socklen_t)); 
    *client_len = sizeof(struct sockaddr);



    if (client_addr_0 != NULL || client_len_0 != NULL){
        *client_addr = *client_addr_0;
        *client_len = *client_len_0;
    }
    //printf("qualche cosa prima while\n");
    while (1) {
        FD_SET(socket_fd, &rfds);

        tv.tv_sec = 0;
        tv.tv_usec = RTT_mics*2;

        //printf("while 1, %d\n", socket_fd);
        //fflush(stdout);
        if (select(socket_fd+1, &rfds, NULL , NULL , NULL) < 0){
            error("select error:");
        };
        
        count = 0;


        while(FD_ISSET(socket_fd, &rfds)){
            
            
            if (recvfrom(socket_fd, &recv_packet, sizeof(Packet), 0, client_addr, client_len)==-1){
                return error("error:");
            }
            //printf("data = %s\n",client_addr.sa_family);
                  
            if (atoi(recv_packet.seq_n) != expected_seq_n && atoi(recv_packet.seq_n) < buffMSS+1){
                if(verbose){
                    printf("Unexpected seq #%d - ", atoi(recv_packet.seq_n));
                    fflush(stdout);
                }
                gap = 1;
            }else if (atoi(recv_packet.seq_n) == expected_seq_n && atoi(recv_packet.seq_n)< buffMSS+1){
                if(verbose){
                    printf("Accept seq #%d arrived - ", atoi(recv_packet.seq_n));
                    fflush(stdout);
                }
                expected_seq_n  = atoi(recv_packet.seq_n)+1;
                gap = 0;
            }

            ptr = (char*)buffer;
            ptr += ((atoi(recv_packet.seq_n)) * MSS);
            //if(verbose){printf("ptr = %p\n",ptr);}

            if(atoi(recv_packet.seq_n) < buffMSS ){

                if( buffvect[atoi(recv_packet.seq_n)] == 1 ){              

                    if(verbose){printf("Saving...\n");}

                    if(recv_packet.code -'0' == ENDSEQ){
                        //printf("last bytes :%s",recv_packet.ack_n);
                        bytes_end = atoi(recv_packet.ack_n);
                        endack = atoi(recv_packet.seq_n)+1;
                        memcpy(ptr,&(recv_packet.data),atoi(recv_packet.ack_n));
                        if(verbose){printf("Endseq = %d\n", endack);}
                        count++;
                    }else{
                        memcpy(ptr,&recv_packet.data,MSS); 
                        count++;
                    }
                    buffvect[atoi(recv_packet.seq_n)] = -1;

                }else{
                        if(verbose){printf("Already saved...\n");}
                        count++;
                        gap = 0;
                }
            }else{
                if(verbose){printf("Not saving...\n");}
                count++;
                gap = 0;
            }
                    
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            select(socket_fd+1, &(rfds), NULL , NULL , &tv);
            fflush(stdout);
        }

        if(datadisplay)(printf("%ld\n",count));

        for(int i = 0; i<buffMSS; i++){
            if (buffvect[i] != -1){
                expected_seq_n = i;
                break;
            }
        }
        
        if(verbose){printf("Recived %zu packets: last seq is #%d (expcseq = %d, ack2send %d)...\n", count ,atoi(recv_packet.seq_n),expected_seq_n,ack2send);fflush(stdout);}

        fflush(stdout);

        if(count == 0 && blocking == 0){
            if(verbose){printf("quitting...\n");fflush(stdout);};
            return (max(expected_seq_n-1,0) * MSS) + bytes_end;
        }else if(count>0) {
            randn = rand();
        }
       
        if(randn%100>100){
            if (count>0) {loss = rand()%count;}else loss =0;

            expected_seq_n -= loss;
            ack2send = expected_seq_n;
            if(verbose){printf("SIM LOSS, sending ack #%d\n", ack2send);fflush(stdout);};
            ptr-= loss*MSS;
        }
        else {
            ack2send = expected_seq_n;
            if(verbose){printf("OK, sending ack #%d gap=%d\n",ack2send, gap);fflush(stdout);};
        }


        //printf("%zu\n",count);

        sprintf(ack_packet.ack_n,"%d",ack2send);
        ack_packet.code = ACK + '0';
        sendto(socket_fd, &ack_packet, sizeof(Packet), 0, client_addr, *client_len);

        if(endack == ack2send && gap == 0 ){
            break;
        }
        expected_seq_n = ack2send;
    }
    if(verbose)(printf("Quitting: %d bytes corretly recived, byts end: %d\n",((expected_seq_n-1) * MSS) + bytes_end, bytes_end));

    if (client_addr_0 != NULL || client_len_0 != NULL){
        *client_addr_0 =  *client_addr;
        *client_len_0 = *client_len ; 
    }


    //usleep(RTT_mics);
    return ((expected_seq_n-1) * MSS) + bytes_end;
}

int send_r( int socket_fd, char* buffer, long size, struct sockaddr *server_addr, socklen_t server_len){
        if (reliable){
        int rcv_byte=sendto(socket_fd,buffer,size,0,server_addr,server_len);
        if(rcv_byte==-1){
            perror("error in recvfrom");
        }
        printf("notrel\n");
        return rcv_byte;
    }
    Packet send_packet, recv_packet;
    int cwnd = 1, ok = 1;
    int count = 0, totcount = 0, endbytes = 0, lastcwnd;
    int expected_ack_number = cwnd;
    int seq2send = 0;
    char* ptr = buffer;
    struct timeval  tv;
    fd_set  rfds;
    tv.tv_sec = 0;
    tv.tv_usec = RTT_mics*100;

    FD_ZERO(&rfds);

    if (size==0) return 0;

    while (1) {

        fflush(stdout);
        count =0;
        //printf("%p %p %d\n",ptr,buffer + (int)size, ptr > buffer + (int)size);

        if (ptr < buffer + (int)size){
            for(int i = 0 ; i< cwnd; i++){
                if (rand()%100>90){
                    if(buffer+size <= ptr) break;

                    memset(&send_packet.data,(int)'\0',MSS); 
                    //if(verbose){printf("ptr = %p\n",ptr);}

                    if(buffer+size-ptr > MSS){
                        send_packet.code = '0';
                        memcpy(&send_packet.data,ptr,MSS);
                        ptr+=MSS; 
                    }else{     
                        send_packet.code = ENDSEQ +'0';
                        sprintf(send_packet.ack_n,"%ld",buffer+size-ptr);
                        endbytes = buffer+size-ptr;
                        memcpy(&send_packet.data,ptr,buffer+size-ptr);
                        ptr+=MSS; 
                    }
                    sprintf (send_packet.seq_n, "%d",seq2send);
                    sendto(socket_fd, &send_packet, sizeof(Packet), 0, server_addr, server_len);
                    seq2send++;
                }else{
                    if(verbose)(printf("SIM-LOSS: Not sending seq #%d\n",seq2send));
                    seq2send++;
                    ptr+=MSS;
                }
                count++;
            }          
        }
        // Receive ACK
        FD_SET(socket_fd, &rfds);
        select(socket_fd+1, &rfds, NULL , NULL , &tv);

        if(FD_ISSET(socket_fd, &rfds)){
            if (recvfrom(socket_fd, &recv_packet, sizeof(Packet), 0, server_addr, &server_len)==-1){
                return error("error:");
            }

            ok = 1;
            //correct acking, evrything went well
            if (atoi(recv_packet.ack_n) == expected_ack_number){          
                cwnd  += cwnd;
                totcount = (atoi(recv_packet.ack_n)*MSS);

                fflush(stdout);
                if (atoi(recv_packet.ack_n) >= ceil(((float)size)/((float)MSS))  && recv_packet.code - '0' == ACK ) break;
                if(verbose)(printf("OK ACK since %d\tFrom #%d sending %d seqs ...\n", atoi(recv_packet.ack_n)-1,atoi(recv_packet.ack_n), cwnd));
                expected_ack_number = min(expected_ack_number + cwnd, ceil(((float)size)/((float)MSS)));
            }
            else{
                if (atoi(recv_packet.ack_n) < expected_ack_number){ //loss detecteed
                    //sshtresh = cwnd/2;
                    //cwnd = sshtresh + 3;
                    ptr = &(buffer[atoi(recv_packet.ack_n) * MSS]);
                    lastcwnd = max(2,ceil(((float)cwnd)/(float)2));
                    cwnd = 1;
                    seq2send = atoi(recv_packet.ack_n);

                    if (atoi(recv_packet.ack_n) >= ceil(((float)size)/((float)MSS))  && recv_packet.code - '0' == ACK ) break;

                    if(verbose)(printf("Wrong ACK #%d ...\tExpecting to be %d, sending from #%d %dseqs...\n",atoi(recv_packet.ack_n), expected_ack_number,seq2send, cwnd));
                    expected_ack_number = min(seq2send + cwnd, ceil( ((float)size)/MSS));

                }else if (atoi(recv_packet.ack_n) > expected_ack_number){

                    totcount = (atoi(recv_packet.ack_n)*MSS);

                    if (atoi(recv_packet.ack_n) >= ceil(((float)size)/(float)MSS) && recv_packet.code - '0' == ACK ) break;
                    
                    ptr = &(buffer[atoi(recv_packet.ack_n) * MSS]);
                    cwnd = lastcwnd ;
                    seq2send = atoi(recv_packet.ack_n);
                    if(verbose)(printf("GAP ACK #%d ...\tExpecting to be %d, sending from #%d %dseqs...\n",atoi(recv_packet.ack_n), expected_ack_number,seq2send, cwnd));
                    expected_ack_number = min(seq2send + cwnd, (int)(size/MSS)+1);

                }
            }
        }else{
            if(verbose)(printf("Failed to recive ACK #%d, quitting...", expected_ack_number));
            return -1;
        }
    }
    if(verbose)(printf("END ACK #%d ...\tExpecting to be %d\n",atoi(recv_packet.ack_n), expected_ack_number));
    if(verbose)(printf("Quitting: %ld bytes corretly sent\n",size));


    return totcount - MSS + endbytes;
}

