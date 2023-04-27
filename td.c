#include <arpa/inet.h> /*td.c 27/04/2023*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define ROOM_SIZE 3
#define TABLE_SIZE 20
#define BUFFER_SIZE 1024   
#define MAX_LEN 100
#define MAX_DEVICE 6

struct Comanda
{
    char idPrenotazione[10];  
    int comandaCodice;
    int quantita;
    int stato;
};


void stampa()
{
    printf("***************************** BENVENUTO *****************************\n");
    printf("Digita un comando:\n\n");
    printf("1) help --> mostra i dettagli dei comandi\n");
    printf("2) menu --> mostra il menu dei piatti\n");
    printf("3) comanda --> invia una comanda\n");
    printf("4) conto --> chiede il conto\n\n");
    return;
}

int main()
{
    int ret,len,sd;
    int bytes_needed;
    uint32_t real_len;
    char buffer[BUFFER_SIZE],bufferCommand[BUFFER_SIZE];
    int bufferlen;
    char *type = "T\0";
    int cmd_new=1;
    struct sockaddr_in srv_addr;

    //necessario per il parametro di ingresso
    char input_string[MAX_LEN];
    char token[MAX_LEN];

    fd_set master;
    fd_set read_fds;
    int fdmax;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

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
    //fdmax = listener;
    fdmax = max(sd, 0);
    int i;
    char ok='s';
    int help =0;
    
    //INVIO T E SECONDO TIPO
    real_len=htons(2);
    ret = send(sd, type, strlen(type)+1, 0);
    ret = send(sd, (void*) &cmd_new, sizeof(uint16_t),0); 
    if(ret < 0){
        perror("Errore in fase di connessione: \n");
        exit(1);
    }
    if(!help) 
        printf("inserisci il codice di prenotazione\n");

    FD_SET(sd, &master);
    FD_SET(0, &master);
    fdmax = max(sd, 0);
    for(;;)
    {
        read_fds = master;
        ret = select(fdmax+1,&read_fds,NULL,NULL,NULL);
        if(ret < 0)
        {
            perror("ERRORE SELECT:");
            exit(1);
        }
        for(i=0;i<=fdmax;i++)
        {
            if(FD_ISSET(i,&read_fds))
            {
                if(i==0)
                {
                    
                    help++;
                    //printf("inserisci il codice di prenotazione\n");
                    struct Comanda c;
                    char prenoCode[20];

                    //validazione codice prenotazione
                    //invio codice
                    int aut = 0;
                    if(ok=='s'){
                    while(aut==0){
                    fgets(prenoCode, 100, stdin);
                    len = strlen(prenoCode) +1;
                    real_len=htons(len);
                    ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
                    ret =send(sd,(void*)prenoCode,len,0);

                    //ricezione risposta
                    ok = 'n';
                    recv(sd, &ok, sizeof(char), 0);
                    if(ok=='s')
                    {
                            stampa();
                            ok = 'n';
                            aut = 1;
                            strcpy(c.idPrenotazione,prenoCode);   
                    }else
                    {
                            printf("codice non valido!\n");
                    }
                    }
                    }

                    fgets(input_string, 100, stdin);
                    sscanf(input_string,"%s",token);

                    //invio token
                    strcpy(bufferCommand,input_string);
                    len = strlen(bufferCommand) +1;
                    real_len=htons(len);
                    ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
                    ret =send(sd,(void*)bufferCommand,len,0);
                    //printf("token: %s\n",token);

                    if(strcmp(token,"help")==0)
                    {
                        printf("-280 giorni al carnevale\n");
                    }
                    else if(strcmp(token,"menu")==0)
                    {
                    //ricezione menu
                        ret = recv(sd,(void*)&real_len,sizeof(uint32_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        printf("%s\n",buffer);
                    }
                    else if(strcmp(token,"comanda")==0)
                    {        
                     // invio tutta la comanda
                        strcpy(bufferCommand,input_string);
                        len = strlen(bufferCommand) +1;
                        real_len=htons(len);
                        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
                        ret =send(sd,(void*)bufferCommand,len,0);

                        //invio codice di prenotazione per gestire la comanda    
                        strcpy(bufferCommand,prenoCode);
                        len = strlen(bufferCommand) +1;
                        real_len=htons(len);
                        ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
                        ret =send(sd,(void*)bufferCommand,len,0);

                        char new_buffer[3];
                        ret = recv(sd, (void*)&real_len,sizeof(uint16_t), 0);
                        len = ntohs(real_len);
                        ret = recv(sd, (void*)new_buffer,3, 0);
                        if(strcmp(new_buffer,"OK\0")==0){
                        printf("\nCOMANDA RICEVUTA\n");
                        }
                        //printf("COMANDA RICEVUTA a nome di: %s\n",prenoCode);
                    }
                    else if(strcmp(token,"conto")==0)
                    {
                        int conto = 0;
                        ret = recv(sd,(void*)&real_len,sizeof(uint32_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        printf("conto:%s\n",buffer);

                        /*ricevo il costo*/
                        ret = recv(sd,&conto,sizeof(int),0);
                        printf("totale: %d\n",conto);

                        /*ricevo il conto e lo stampo a video*/
                    }
                }else{
                    char buffer_take[1024];
                    ret = recv(i, (void*)&real_len, sizeof(uint16_t), 0);
                    // Conversione in formato 'host'
                    len = ntohs(real_len); 
                    // Ricezione del messaggio
                    ret = recv(i, (void*)buffer_take, len, 0); 
                    printf("%s\n", buffer_take);
                }
            }
        }


    }
}