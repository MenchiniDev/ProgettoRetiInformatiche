#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024

#define WEEK_SIZE 7
#define ROOM_SIZE 3
#define TABLE_SIZE 20
#define HOUR_SIZE 24

#define CHOOSE_LEN 5
#define ID_LEN 3
#define ID_BOOK_LEN 12

#define MENU_SIZE 8
#define COMANDA_SIZE 4
#define TAV_SIZE

#define MAX_DEVICE 10


struct prenotazione 
{
    char cognome[20];
    int nPersone;
    char data[9];
    int HH;
};

/*la funzione cerca la prenotazione richiesta, la schedula e lo fa modificando il file delle prenotazioni (?)*/

int findPosto(struct prenotazione p)
{

    FILE *fp;
    fp = fopen("FindPrenotazioni.txt", "r");

    if (fp == NULL) {
    printf("Errore nell'apertura del file");
    exit(1);
    }




    fclose(fp);

   
}

int main()
{
    int ret, sd, new_sd, len,bytes_needed,cl;
    pid_t pid;
    struct sockaddr_in cl_addr, cl2_addr;
    int i;
    char buffer[1024],bufferCommand[1024];

    sd = socket(AF_INET,SOCK_STREAM,0);
    memset(&cl_addr,0,sizeof(cl_addr));
    cl_addr.sin_family=AF_INET;
    cl_addr.sin_port=htons(4242); 
    inet_pton(AF_INET,"127.0.0.1",&cl_addr.sin_addr); 

    ret = bind(sd,(struct sockaddr*)&cl_addr,sizeof(cl_addr));
    if(ret<0)
    {
        printf("non si è riusciti a bindare il socket all'indirizzo client\n");
        exit(-1);
    }
    ret = listen(sd,10);
    if(ret < 0)
    {
        printf("non si è riusciti ad ascoltare il socket all'indirizzo client");
        exit(-1);
    }else
    {
        printf("il server sta ascoltando! \n");
    }

    while(1){
    new_sd = accept(sd,(struct sockaddr*)&cl2_addr,&len);
    pid = fork();
    if(pid==0)
    {

        FILE *fp;
        uint32_t real_len;
        uint32_t lenght;
        close(sd);

        //prima ricezione per il token
        ret = recv(new_sd,(void*)&real_len,sizeof(uint16_t),0);
        bytes_needed = ntohs(real_len);
        ret = recv(new_sd,(void*)bufferCommand,bytes_needed,0);
        printf("comando ricevuto: [%s]\n",bufferCommand);

        if(strcmp("find",bufferCommand)==0)
        {

        struct prenotazione p;
        //seconda ricezione per tutti i dati

        ret = recv(new_sd,(void*)&real_len,sizeof(uint32_t),0);
        bytes_needed = ntohs(real_len);

        ret = recv(new_sd,(void*)buffer,bytes_needed,0);

        //printf("lunghezza buffer: %d\n",bytes_needed);
        //printf("dati ricevuti: %d\n",ret);
        //printf("buffer: %s\n",buffer);

        sscanf(buffer, "%s %s %d %s %d" ,bufferCommand, p.cognome, &p.nPersone, p.data, &p.HH);
        //printf("prenotazione registrata!\n");
        printf(" %s %d %s %d\n" , p.cognome, p.nPersone, p.data, p.HH);

        fp = fopen("FindPrenotazioni.txt", "a");
        if (fp == NULL) {
            printf("Errore nell'apertura del file.\n");
            exit(1);
        }

        fprintf(fp, buffer);
        fclose(fp);

        /*analisi dei posti per individuare una possibile prenotazione*/
        ret = findPosto(p);

        strcpy(bufferCommand,"qui andranno i posti");
        len = strlen(bufferCommand) +1;
        real_len=htons(len);
        ret=send(new_sd,(void*)&real_len,sizeof(uint32_t),0);
        ret =send(new_sd,(void*)bufferCommand,len,0);

        printf("trasferimento verso il client avvenuto\n");

        close(new_sd);
        exit(0);
        }
    }
    close(new_sd);
    if(ret < 0)
    {
        printf("non si è riusciti ad ascoltare il socket all'indirizzo client\n");
        exit(-1);
    }
    }

}