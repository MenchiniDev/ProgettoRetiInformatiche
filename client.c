#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LEN_REPLY 6
#define MAX_LEN 100

struct prenotazione 
{
    char cognome[20];
    int nPersone;
    char data[9];
    int HH;
};


int main(int argc, char* argv[])
{
    int ret,len,sd;
    uint32_t real_len;
    char buffer[1024];
    int bufferlen;
    char str[100];
    int i=0;
    struct sockaddr_in srv_addr;

    //necessario per il parametro di ingresso
    char input_string[MAX_LEN];
    char token[100];

    printf("\nfind --> ricerca la disponibilità per una prenotazione\n");
    printf("book --> invia una prenotazione\n");
    printf("esc --> termina il client\n\n");

    sd = socket(AF_INET,SOCK_STREAM,0);
    memset(&srv_addr,0,sizeof(srv_addr));
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_port=htons(4242);
    inet_pton(AF_INET,"127.0.0.1",&srv_addr.sin_addr); 

    ret = connect(sd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    if(ret < 0)
    {
        printf("impossibile connettersi al server \n");
        exit(-1);
    }

    //PRENOTAZIONE SU CLIENT    
    for(;;){
    fgets(input_string, 100, stdin);

    sscanf(input_string,"%s",token);

    if(strcmp(token,"esc")==0)
    {
        printf("arrivederci!\n");
        return;
    }
    else if(strcmp(token,"find")==0) //COMANDO FIND: TRASMISSIONE PARAMETRI
    {
        int bytes_needed;
        //uint32_t lenght2;
        char bufferCommand[1024];
        char postiDisponibili[200];

        //trasferiamo primo: find
        strcpy(bufferCommand,token);
        len = strlen(bufferCommand) +1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret =send(sd,(void*)bufferCommand,len,0);

        printf("comando: %s\n",bufferCommand);

        strcpy(buffer,input_string);
        len = strlen(buffer);
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint32_t),0);
        ret = send(sd,(void*)buffer,len,0);

        printf("dati: %s\n",buffer);

        
        if(ret < len)
        {
            printf("problema nell'invio");
        }
        //COMANDO FIND: RICEZIONE PARAMETRI

        ret = recv(sd,(void*)&real_len,sizeof(uint32_t),0);
        bytes_needed = ntohs(real_len);
        ret = recv(sd,(void*)postiDisponibili,bytes_needed,0);
        printf("risposta del server: %s\n",postiDisponibili);
    }
    else if(strcmp(token,"book")) //COMANDO BOOK: TRASMISSIONE PARAMETRI
    {
        strcpy(buffer,input_string);
        len = strlen(input_string) +1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
    }
    }
}