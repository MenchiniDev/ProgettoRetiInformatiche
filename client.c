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
    int bytes_needed;
    char proposta[ROOM_SIZE*TABLE_SIZE][BUFFER_SIZE];
    uint32_t real_len;
    char buffer[BUFFER_SIZE];
    char bufferCommand[BUFFER_SIZE];
    char CodicePrenotazione[20];
    int bufferlen;
    char *type = "C\0";
    int i=0;
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
    srv_addr.sin_port=htons(4242);
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


    //PRENOTAZIONE SU CLIENT    
    for(;;){
        fgets(input_string, 100, stdin);
        sscanf(input_string,"%s",token);

        //SECONDA TRASMISSIONE: INVIO TOKEN
        strcpy(bufferCommand,token);
        len = strlen(bufferCommand) +1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
        ret =send(sd,(void*)bufferCommand,len,0);

        if(strcmp(token,"esc")==0)
        {
        printf("arrivederci!\n");
        return;
        }//FIND
        else if(strcmp(token,"find")==0) 
        {
        char postiDisponibili[200];
        //printf("comando: %s\n",bufferCommand);

        //TERZA TRASMISSIONE: INVIO DATI FIND
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
        //QUARTA TRASMISSIONE: RICEZIONE PARAMETRI

        ret = recv(sd,(void*)&real_len,sizeof(uint32_t),0);
        bytes_needed = ntohs(real_len);
        ret = recv(sd,(void*)postiDisponibili,bytes_needed,0);
        printf(postiDisponibili);
        
        }//BOOK
        else if(strcmp(token,"book")==0)
        {
        strcpy(buffer,input_string);
        //QUINTA TRASMISSIONE: INVIO DATI
        len = strlen(buffer) + 1;
        real_len=htons(len);
        ret=send(sd,(void*)&real_len,sizeof(uint32_t),0);
        ret=send(sd,(void*)buffer,len,0);

        //printf("dati inviati:%s lunghezza:[%d]\n",buffer,len);

        //SESTA RICEZIONE: RICEZIONE PARAMETRI
        ret = recv(sd,(void*)&real_len,sizeof(uint32_t),0);
        bytes_needed = ntohs(real_len);
        ret = recv(sd,(void*)CodicePrenotazione,bytes_needed,0);
        printf("Prenotazione effettuata! Codice: %s\n",CodicePrenotazione);

        }
    }
}