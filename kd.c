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
#define max(a, b) ((a) > (b) ? (a) : (b))

void stampa()
{
    printf("\ntake --> accetta una comanda\n");
    printf("show --> mostra le comande accettate\n");
    printf("ready --> imposta lo stato della comanda\n\n");
}


int main(int argc, char* argv[])
{
    int ret,len,sd;
    int bytes_needed;
    uint32_t real_len;
    char buffer[BUFFER_SIZE],bufferCommand[BUFFER_SIZE];
    uint16_t port = (uint16_t)strtol(argv[1],NULL,10);
    struct sockaddr_in srv_addr;
    char *type="K\0";
    char token[5];
    char input_string[BUFFER_SIZE];
    FILE *fp;

    fd_set master;
    fd_set read_fds;
    int fdmax;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
    sd = socket(AF_INET,SOCK_STREAM,0);
    memset(&srv_addr,0,sizeof(srv_addr));
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_port=htons(port);
    inet_pton(AF_INET,"127.0.0.1",&srv_addr.sin_addr); 

    ret = connect(sd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    //real_len=htons(2);
    ret = send(sd, type, strlen(type)+1, 0);
    if(ret < 0)
    {
        printf("errore \n");
        exit(-1);
    }
    if(ret==0)
    {
        printf("impossibile connettersi al server\n");
        exit(-1);
    }

    FD_SET(sd, &master);
    FD_SET(0, &master);
    fdmax = max(sd, 0);

    stampa();

    for(;;)
    {
        read_fds = master;
        ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
        if(ret<0){
            perror("ERRORE SELECT:");
            exit(1);
        }
        int i;
        for(i=0; i<= fdmax; i++){
            if(FD_ISSET(i, &read_fds)) {
                if(i == 0) {
                    fgets(input_string, 100, stdin);
                    sscanf(input_string,"%s",token);
                    //invio token

                    strcpy(bufferCommand,input_string);
                    len = strlen(bufferCommand) +1;
                    real_len=htons(len);
                    ret=send(sd,(void*)&real_len,sizeof(uint16_t),0);
                    ret=send(sd,(void*)bufferCommand,len,0);

                    printf("token: %s\n",bufferCommand);


                    if(strcmp(token,"take")==0) // mette ad uno la prima comanda
                    {
                        /*codice comanda*/
                        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        buffer[bytes_needed-1]='\0';
                        printf("%s ",buffer);

                        /*id del tavolo*/
                        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        buffer[bytes_needed-1]='\0';
                        printf("-%s\n",buffer);
                    
                        /*portate della comanda*/
                        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        buffer[bytes_needed-1]='\0';
                        printf("%s\n",buffer);

                    }
                    else if(strcmp(token,"show")==0)
                    {
                        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        buffer[bytes_needed-1]='\0';
                        printf("comanda:%s\n",buffer);

                        ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(sd,(void*)buffer,bytes_needed,0);
                        buffer[bytes_needed-1]='\0';
                        printf("%s\n",buffer);
                    }
                    else if(strcmp(token,"ready")==0)
                    {
                        
                        printf("COMANDA IN SERVIZIO\n");
                    }
                }
                else /*gestisco la notifica dal server*/
                {
                    ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                    bytes_needed = ntohs(real_len);
                    ret = recv(sd,(void*)buffer,bytes_needed,0);
                    buffer[bytes_needed-1]='\0';
                    printf("%s\n",buffer);
                }
            }
        }
    }
}
