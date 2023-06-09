#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ROOM_SIZE 3
#define TABLE_SIZE 20
#define BUFFER_SIZE 1024   
#define MAX_LEN 100
#define MAX_DEVICE 6 

/*
    struttura utile per la prenotazione del campo find
*/
struct prenotazione 
{
    char cognome[20];
    int nPersone;
    char data[9];
    int HH;
};


int main(int argc, char* argv[])
{
    int ret,len,sd,fdmax;
    int i=0;
    int bytes_needed;
    uint16_t real_len;
    fd_set read_fds;
    fd_set master;
    char buffer[BUFFER_SIZE];
    uint16_t port;
    //if(argc != 2)
        port = 4242;
    //else
    //    port = (uint16_t)strtol(argv[1],NULL,10);
    char bufferCommand[BUFFER_SIZE];
    char CodicePrenotazione[20];
    char *type = "C\0";
    struct sockaddr_in srv_addr;

    //necessario per il parametro di ingresso
    char input_string[MAX_LEN];
    char token[MAX_LEN];

    printf("\nfind --> ricerca la disponibilità per una prenotazione\n");
    printf("book --> invia una prenotazione\n");
    printf("esc --> termina il client\n\n");

    sd = socket(AF_INET,SOCK_STREAM,0);
    memset(&srv_addr,0,sizeof(srv_addr));
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_port=htons(port);
    inet_pton(AF_INET,"127.0.0.1",&srv_addr.sin_addr); 

    ret = connect(sd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    
    //PRIMA TRASMISSIONE: INVIO C
    real_len=htons(2);
    ret = send(sd, type, strlen(type)+1, 0);
    if(ret < 0)
    {
        printf("impossibile connettersi al server \n");
        exit(-1);
    }
    FD_SET(sd,&master);
    FD_SET(0,&master);
    fdmax=sd;

    //PRENOTAZIONE SU CLIENT    
    for(;;){
        read_fds=master;
        ret=select(fdmax+1,&read_fds,NULL,NULL,NULL);
        if(ret<0){
            perror("ERRORE SELECT\n");
            exit(1);
        }
        for(i=0; i<=fdmax; i++){
            if(FD_ISSET(i,&read_fds)){
               if(i==0){

            if(ret<0){
                perror("Richiesta problematica");
                exit(0);
            }

        fgets(input_string, 100, stdin);
        sscanf(input_string,"%s",token);

        //SECONDA TRASMISSIONE: INVIO TOKEN
        strcpy(bufferCommand,token);
        len = strlen(bufferCommand) +1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret =send(sd,(void*)bufferCommand,len,0);

        char NameBuffer[BUFFER_SIZE];

        if(strcmp(token,"esc")==0)
        {
        strcpy(buffer,token);
        len = strlen(buffer);
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret = send(sd,(void*)buffer,len,0);
        printf("arrivederci!\n");
        close(sd);
         FD_CLR(sd, &master);
         FD_CLR(i,&master);
         exit(0);
        return 0;
        }//FIND
        else if(strcmp(token,"find")==0) 
        {
        char postiDisponibili[200];
        //printf("comando: %s\n",bufferCommand);

        //TERZA TRASMISSIONE: INVIO DATI FIND
        strcpy(buffer,input_string);
        len = strlen(buffer);
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret = send(sd,(void*)buffer,len,0);

        if(ret < len)
        {
            printf("problema nell'invio");
        }
        //QUARTA TRASMISSIONE: RICEZIONE PARAMETRI

        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
        bytes_needed = ntohs(real_len);
        ret = recv(sd,(void*)postiDisponibili,bytes_needed,0);
        printf(postiDisponibili);

        /*salvo il nome per la book*/
        sscanf(buffer,"%s %s",input_string,NameBuffer);
        
        }//BOOK
        else if(strcmp(token,"book")==0)
        {
        strcpy(buffer,input_string);

        //QUINTA TRASMISSIONE: INVIO DATI
        len = strlen(buffer) + 1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret=send(sd,(void*)buffer,len,0);

        //5.1 : invio nome prenotazione
        len = strlen(NameBuffer) + 1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret=send(sd,(void*)NameBuffer,len,0);

        //SESTA RICEZIONE: RICEZIONE PARAMETRI
        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
        bytes_needed = ntohs(real_len);
        ret = recv(sd,(void*)CodicePrenotazione,bytes_needed,0);
        if(strcmp(CodicePrenotazione,"NO")==0)
        {
            printf("tavolo non piu disponibile\n");
        }else
            printf("Prenotazione effettuata! Codice: %s\n",CodicePrenotazione);
        }
        if(ret<=0)
        {
            printf("arrivederci!\n");
        }
        }
        else{ 
        memset(buffer,0,strlen(buffer));
        ret=recv(sd,(void*)&real_len,sizeof(uint8_t),0);
        ret=recv(sd,(void*)buffer,real_len,0);
   
        if(strcmp(buffer,"ce")==0)
        printf("Chiusura di tutti i device \n");
        close(sd);
        FD_CLR(sd, &master);
        FD_CLR(i,&master);
         exit(0);
    }
    }
    }

        }
    }
