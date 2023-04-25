#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define MAX_TAVOLI 30

#define WEEK_SIZE 7
#define ROOM_SIZE 3
#define TABLE_SIZE 20
#define HOUR_SIZE 24

#define CHOOSE_LEN 5
#define ID_LEN 3
#define ID_BOOK_LEN 12

#define MENU_SIZE 15
#define COMANDA_SIZE 4
#define TAV_SIZE 20

#define MAX_DEVICE 10

struct settimana{
    int numeroGiorno;
    struct sala{
        char nome_sala[6];           
        struct tavolo{
            char nome_tavolo[4];
            char vicinanza[20];
            int nPosti;
                struct status{
                    int orario;
                    int stato;
                }status[HOUR_SIZE];
        }tav[TABLE_SIZE];   
    }sala[ROOM_SIZE];
}sett[WEEK_SIZE];

struct tavPrenotati
{
    char nome_tavolo[4];
    char vicinanza[20];
    int nPosti;
    char nome_sala[6];
};

struct menu
{
    char codice[2];
    char pietanza[200];
    int costo;
}Menu[MENU_SIZE];

struct prenotazione 
{
    char cognome[20];
    int nPersone;
    char data[9];
    int orario;
};

struct comanda /*non so se mantenerla*/
{
    char idPrenotazione[20];
    char pietanza[200];
    int quantita;
    int stato; // 0->appena arrivata, 1->in gestione 2->in servizio
};

int main()
{
    int ret, sd, new_sd, len,bytes_needed,cl;
    int listener;
    int prenIndex;
    struct sockaddr_in cl_addr, cl2_addr;
    int i;
    char buffer[1024],bufferCommand[1024],buftype[2];
    char type;
    FILE *fp,*fptr;
    uint32_t real_len;
    uint32_t lenght;
    int num=0;
    char num1[6];
    char num2[3];
    char num3[15];
    int num4=0;
    int num5=0;
    int num6=0;
    int convStringa;
    struct tavPrenotati tabInviati[MAX_TAVOLI];
    char Data[9]; //necessario per lavorare su p.data
    char stringa[3][2]; //stringa[0] contiene gg stringa[1] contiene mm stringa[2] contiene aa
    struct comanda c;
    struct menu m[MENU_SIZE];
    int numComanda=0;

    time_t rawtime;
    struct tm * timeinfo;

    struct prenotazione p;
    struct settimana sett[WEEK_SIZE];
    struct tavolo tav[TAV_SIZE];
    //device d[MAX_DEVICE];
    //device_2 d_2[MAX_DEVICE];
    //int nn;
    fd_set master;
    fd_set read_fds;
    int fdmax;

    int b = 1;


    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    fp = fopen("Menu2.txt","r");
    if(fp == NULL)
    {
    printf("errore nell'apertura del file\n");
        exit(1);
    }
    char line[200];
    char codice[2];
    char pietanza[40];
    int costo;
    int ii=0;
    char* token = NULL;
    char a,a1;
    while (fgets(line, sizeof(line), fp)) { 
        memset(buffer,0,sizeof(buffer));
        token = strtok(line, "-");
        strncpy(m[ii].codice,token,2);
        m[ii].codice[2]='\0';
        //printf("codice:%s\n",m[ii].codice);
        a = m[ii].codice[0];
        a1 = m[ii].codice[1];
        token = strtok(NULL, "-"); 
        strcpy(m[ii].pietanza,token);
        token = strtok(NULL, "-"); 
        m[ii].costo = atoi(token) /*- 48*/;
        //printf("%d\n",m[ii].costo);
        ii++;
    }/*inizializzazione del menu completata*/


    if ((fptr = fopen("prenotazioniTotali.txt","r")) == NULL){
       printf("Error! opening file");
       exit(1);
    }
    int prova;int j;int k;int g;
    for(prova=0;prova<WEEK_SIZE; prova++){
       fscanf(fptr,"%d", &num);
        sett[prova].numeroGiorno = num; 
        //printf("%d %d\n",num,sett[prova].numeroGiorno);
        for( j = 0; j<ROOM_SIZE; j++){
            fscanf(fptr,"%s", num1);
            strcpy(sett[prova].sala[j].nome_sala,num1);

            for(k = 0; k<TABLE_SIZE; k++){
                fscanf(fptr,"%s", num2);
                strcpy(sett[prova].sala[j].tav[k].nome_tavolo,num2);
                fscanf(fptr,"%s", num3);
                strcpy(sett[prova].sala[j].tav[k].vicinanza,num3);
                //printf(" %s", sett[i].sala[j].tav[k].vicinanza);
                fscanf(fptr,"%d", &num4);
                sett[prova].sala[j].tav[k].nPosti =num4 ;
                //printf(" nPosti:%d\n",sett[i].sala[j].tav[k].nPosti);
                for(g=0; g<HOUR_SIZE;g++){
                fscanf(fptr,"%d", &num5);
                    sett[prova].sala[j].tav[k].status[g].orario = num5;
                    //printf("%d ",sett[i].sala[j].tav[k].status[g].orario);
                    fscanf(fptr,"%d", &num6);
                    sett[prova].sala[j].tav[k].status[g].stato = num6 ;
                    //printf("%d\n",sett[i].sala[j].tav[k].status[g].stato);    
                }
            }
        }
    }




    /*popolazione della comanda e delle prenotazioni gia avvenute*/


    listener = socket(AF_INET,SOCK_STREAM,0);
    memset(&cl_addr,0,sizeof(cl_addr));
    cl_addr.sin_family=AF_INET;
    cl_addr.sin_port=htons(4242); 
    inet_pton(AF_INET,"127.0.0.1",&cl_addr.sin_addr); 

    ret = bind(listener,(struct sockaddr*)&cl_addr,sizeof(cl_addr));
    ret = listen(listener,10);
    if(ret<0)
    {
        perror("errore in fase di bind: \n");
        exit(-1);
    }else

    FD_SET(listener,&master);
    FD_SET(0,&master);
    fdmax = listener; // come da slide 


    while(1){
        read_fds = master;
        ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
        if(ret<0){
            perror("ERRORE SELECT");
            exit(1);
        }
        for(i = 0; i <= fdmax ; i++)
        {
            if(FD_ISSET(i,&read_fds))
            {                   
                if(i==0)
                {
                    /*stampe che devo fare lato server*/
                    printf("sono qui\n");
                }
                if(i==listener)
                {
                    fflush(stdout);
                    len = sizeof(cl_addr);
                    socklen_t len_p = sizeof(struct sockaddr);
                    new_sd = accept(listener, (struct sockaddr *) &cl_addr, &len_p);

                    FD_SET(new_sd, &master);
                    if(new_sd > fdmax)
                    {
                        fdmax = new_sd;
                    }
                }
                else
                {
                    //PRIMA RICEZIONE C
                    ret = recv(new_sd, (void*)buftype, 2, 0);
                    sscanf(buftype, "%c", &type);

                    printf("tipo: %c\n", type);

                    if(type=='C')
                    {
                    //SECONDA RICEZIONE: TOKEN
                    ret = recv(new_sd,(void*)&real_len,sizeof(uint16_t),0);
                    bytes_needed = ntohs(real_len);
                    ret = recv(new_sd,(void*)bufferCommand,bytes_needed,0);
                
                    if(ret == 0){
                        printf("CHIUSURA rilevata!\n");
                        fflush(stdout);
                        close(i);
                        FD_CLR(i, &master);
                        break;
                    }
                    if(ret < 0 )
                    {
                        printf("ERRORE\n");
                        fflush(stdout);
                        close(i);
                        FD_CLR(i, &master);
                        break;
                    }

                    char bufferPrenotazioni[1024];
                    
                    //TERZA RICEZIONE: DATI
                    ret = recv(new_sd,(void*)&real_len,sizeof(uint32_t),0);
                    bytes_needed = ntohs(real_len);

                    ret = recv(new_sd,(void*)buffer,bytes_needed,0);

                    printf("dati ricevuti:%s",buffer);

                    //FIND
                    if(strcmp("find",bufferCommand)==0)
                    {
                        char nome[20],dataP[20],nometav[6],sala[6],vicinanza[6];
                        int orario,indice;
                        char PrenEffettuate[30];
                        int i = 0;
                        char prenotabili[1024]; //buffer di concatenazione
                        sscanf(buffer, "%s %s %d %s %d" ,bufferCommand, p.cognome, &p.nPersone, p.data, &p.orario);


                        strcpy(Data,p.data); //Data è un array di appoggio per fare il confronto dopo
                        char* token = strtok(p.data, "-");
                        convStringa=atoi(token); //converte stringa intero
                        while (token != NULL) {
                        strcpy(stringa[i],token);
                        token = strtok(NULL, "-");
                        //printf("stringa: %s\n",stringa[i]);
                        i++;
                        }


                        //printf("convStringa: %d\n",convStringa);

                        fp = fopen("Prenotazioni.txt", "r");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }


                        char line[200];
                        while (fgets(line, sizeof(line), fp)) {
                            token = strtok(line, " "); 
                            int i=0;
                            while (token != NULL) {
                            //printf("%s\n", token);  // stampa il singolo parametro
                            if(i==0)
                                strcpy(nome,token);
                            if(i==1)
                                strcpy(dataP,token);
                            if(i==2)
                                orario=atoi(token);
                            if(i==3)
                                indice=atoi(token);
                            if(i==4)
                                strcpy(nometav,token);
                            if(i==5)
                                strcpy(sala,token);
                            if(i==6)
                                strcpy(vicinanza,token);
                            token = strtok(NULL, " "); 
                            i++;
                            }
                            //printf("nel file abbiamo: %s %s %d %d %s %s %s\n",nome,dataP,orario,indice,nometav,sala,vicinanza);

                        //printf("orari e date: %d %d %s %s\n",orario,p.orario,dataP,Data);
                        if(strcmp(dataP,Data)==0)
                        {
                            //printf("here\n");
                            if(orario==p.orario)
                            {
                            int ii;int j; int k,g;
                            for(ii =0; ii<WEEK_SIZE;ii++){
                                if(convStringa%7 == sett[ii].numeroGiorno){
                                for(j=0;j<ROOM_SIZE;j++)
                                    {
                                        if(strcmp(sett[ii].sala[j].nome_sala,sala)==0)
                                        {
                                            for(k = 0; k<TABLE_SIZE; k++){
                                                if(strcmp(nometav,sett[ii].sala[j].tav[k].nome_tavolo)==0){
                                                    for(g=0; g<HOUR_SIZE;g++){
                                                        if(sett[ii].sala[j].tav[k].status[g].orario == p.orario){
                                                            if(sett[ii].sala[j].tav[k].status[g].stato == 0){
                                                                sett[ii].sala[j].tav[k].status[g].stato=1;
                                                                //printf("%s %s %s %d prenotato\n",sett[ii].sala[j].tav[k].nome_tavolo,sett[ii].sala[j].nome_sala,sett[ii].sala[j].tav[k].vicinanza,sett[ii].sala[j].tav[k].status[g].stato);
                                                                }
                                                                }
                                                            }         
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                                if(nome==NULL)
                                    break;
                                }

                                fclose(fp);

                                FILE *fp;
                                fp = fopen("prenotazioniTotali.txt", "r");

                        if (fp == NULL) {
                        printf("Errore nell'apertura del file");
                        exit(1);
                        }
                        //printf("il giorno che cerchiamo è il: %d\n",convStringa);

                        int ii;int j;int k; int g;int count;
                        for(ii = 0; ii<WEEK_SIZE;ii++){
                        count = 0;             
                        if(convStringa%7 == sett[ii].numeroGiorno){
                            memset(buffer, 0, sizeof(buffer));
                            memset(prenotabili,0,sizeof(prenotabili));
                        for(j = 0; j<ROOM_SIZE; j++){
                            for(k = 0; k<TABLE_SIZE; k++){
                                //printf("nPosti: %d\n",sett[ii].sala[j].tav[k].nPosti);
                                if(sett[ii].sala[j].tav[k].nPosti >= p.nPersone){
                                    for(g=0; g<HOUR_SIZE;g++){
                                        if(sett[ii].sala[j].tav[k].status[g].orario == p.orario){
                                            //printf("%s %s %s %d\n", sett[ii].sala[j].tav[k].nome_tavolo, sett[ii].sala[j].nome_sala, sett[ii].sala[j].tav[k].vicinanza,sett[ii].sala[j].tav[0].status[g].stato);
                                            if(sett[ii].sala[j].tav[k].status[g].stato == 0){
                                                
                                                //printf("%s %s %s\n", sett[ii].sala[j].tav[k].nome_tavolo, sett[ii].sala[j].nome_sala, sett[ii].sala[j].tav[k].vicinanza);
                                                count++;
                                                sprintf(buffer,"%d) %s %s %s\n",count, sett[ii].sala[j].tav[k].nome_tavolo, sett[ii].sala[j].nome_sala, sett[ii].sala[j].tav[k].vicinanza);
                                                strcat(prenotabili,buffer);
                                                strcpy(tabInviati[count].nome_sala,sett[ii].sala[j].nome_sala);
                                                strcpy(tabInviati[count].nome_tavolo,sett[ii].sala[j].tav[k].nome_tavolo);
                                                strcpy(tabInviati[count].vicinanza,sett[ii].sala[j].tav[k].vicinanza);
                                                //printf("tavolo disponibile: %s\n",tabInviati[count].nome_tavolo);
                                                //printf("tavolo salvato: %s\n",tabInviati[count].nome_tavolo);
                                                }
                                            }
                                        }                  
                                    }          
                                }
                            }
                        }
                    }//ho creato un vettore indicizzato con count con tutti i tavoli su cui è possibile scegliere
                    fclose(fp);

                    // QUARTA TRASMISSIONE: INVIO PARAMETRI
                    len = strlen(prenotabili) +1;
                    //printf("lunghezza buffer: %d\n",len); 
                    real_len=htons(len);
                    ret=send(new_sd,(void*)&real_len,sizeof(uint32_t),0);
                    ret=send(new_sd,(void*)prenotabili,len,0);
                    //BOOK  
                    }else if(strcmp("book",bufferCommand)==0)
                    {
                        char PrenotazioneCode[1024];
                        char bufferFile[1024];
                        char BufferOrarioPren[1024];
                        sscanf(buffer, "%s %d" ,bufferCommand, &prenIndex);
                        printf("ossia il tavolo: %d %s %s %s\n", prenIndex,tabInviati[prenIndex].nome_tavolo, tabInviati[prenIndex].nome_sala,tabInviati[prenIndex].vicinanza);
                        
                        
                        fp = fopen("Prenotazioni.txt", "a");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }
                        sprintf(bufferFile,"\n%s %s %d %d %s %s %s",p.cognome,Data,p.orario,prenIndex,tabInviati[prenIndex].nome_tavolo, tabInviati[prenIndex].nome_sala,tabInviati[prenIndex].vicinanza);
                        fprintf(fp,bufferFile);
                        fclose(fp);

                        printf("prenotazione effettuata! Invio codice in corso\n");

                        fp = fopen("PrenotazioniCodici.txt", "a");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }
                        //BufferOrarioPren contiene il codice prenotazione

                        sprintf(BufferOrarioPren,"%s%d%s",p.cognome,p.nPersone,tabInviati[prenIndex].nome_tavolo);
                        fprintf(fp,"%s ",PrenotazioneCode);

                        len = strlen(BufferOrarioPren)+1;
                        real_len = htons(len);
                        ret = send(i, (void*) &real_len, sizeof(uint32_t),0);
                        ret = send(i, (void*) BufferOrarioPren, len, 0);

                        //printf("mando il codice: %s\n",BufferOrarioPren);

                        fprintf(fp,"%s\n",BufferOrarioPren);
                        fclose(fp);

                        /*time(&rawtime);
                        timeinfo = localtime(&rawtime);
                
                        //sprintf(buffer, "%02d:%02d:%02d","20","30","30");
                        //timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec );
                        strcpy(buffer,"orario da implementare");
                        fprintf(fp,"%s\n",buffer);

                        sprintf(BufferOrarioPren,"%s %s" , buffer , BufferOrarioPren);*/
                        }
                    }
                    if(type=='T')
                    {
                        /*
                          l'intero serve per evitare di mandare sempre il
                          codice di prenotazione
                        */
                        if(b==1){
                        //ricezione codice prenotazione
                        char prenoCode[20];
                        FILE *fp1;
                        int aut = 0;
                        while(aut == 0){
                        ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(i,(void*)prenoCode,bytes_needed,0);
                        prenoCode[bytes_needed-2]='\0';

                        printf("codice giunto:%s.\n",prenoCode);

                        fp1 = fopen("PrenotazioniCodici.txt","r");
                        if(fp1==NULL)
                        {
                            printf("errore nell'apertura del file\n");
                            exit(1);
                        }
                        char linea[20];
                        char ok = 'n';

                        char code[8];
                        while (fgets(linea, sizeof(linea), fp1))
                        {   
                            sscanf(linea,"%s",code);
                            code[strlen(code)]='\0';
                            //printf("confronto con:%s.\n",code);

                            if(strcmp(code,prenoCode)==0){
                                b = 1;
                                ok = 's';
                                aut = 1;
                                strcpy(c.idPrenotazione,prenoCode);
                                c.idPrenotazione[8]='\0';
                                break;
                                //printf("codice trovato!\n");
                            }
                        }
                        
                        /*invio la risposta*/
                        send(i, &ok, sizeof(char), 0);

                        if(b==0)
                        {
                            printf("prenotazione non trovata!\n");
                            exit(1);
                        }
                        }
                        b=0;
                        }
                        //ricezione token
                        ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(i,(void*)buffer,bytes_needed,0);
                        bufferCommand[bytes_needed-1]='\0';
                        sscanf(buffer,"%s",bufferCommand);
                        

                    if(ret == 0){
                        printf("chiusura td rilevata!\n");
                        fflush(stdout);
                        close(i);
                        FD_CLR(i, &master);
                        break;
                    }
                    if(ret < 0 )
                    {
                        printf("ERRORE\n");
                        fflush(stdout);
                        close(i);
                        FD_CLR(i, &master);
                        break;
                    }

                        printf("bufferCommand:%s %d\n",buffer,ret);
                        if(strcmp(bufferCommand,"menu")==0){
                            FILE *fp;
                            fp = fopen("Menu2.txt","r");
                            if(fp == NULL)
                            {
                                printf("errore nell'apertura del file\n");
                                exit(1);
                            }
                            char bufConcatenazione[BUFFER_SIZE];
                            char line[200];
                            char codice[2];
                            char pietanza[40];
                            int costo;

                            int ii=0;
                            char* token = NULL;
                            char a,b;
                            while (fgets(line, sizeof(line), fp)) { 
                                        memset(buffer,0,sizeof(buffer));

                                        token = strtok(line, "-");
                                        strncpy(m[ii].codice,token,2);
                                        m[ii].codice[2]='\0';
                                        a = m[ii].codice[0];
                                        b = m[ii].codice[1];
                                        token = strtok(NULL, "-"); 
                                        strcpy(m[ii].pietanza,token);
                                        token = strtok(NULL, "-"); 
                                        m[ii].costo = *(token) - 48;

                                        sprintf(buffer,"%c%c - %s %d\n",a,b,m[ii].pietanza,m[ii].costo);

                                        //printf("menu: codice: %s,pietanza:%s,costo:%s",m[ii].codice,m[ii].pietanza,m[ii].costo);
                                        
                                        strcat(bufConcatenazione,buffer); 
                                        ii++;
                            }
                            len = strlen(bufConcatenazione)+1;
                            real_len = htons(len);
                            ret = send(i, (void*) &real_len, sizeof(uint32_t),0);
                            ret = send(i, (void*) bufConcatenazione, len, 0);
                            printf("menu inviato!\n");
                        }
                        if(strcmp(bufferCommand,"comanda")==0)
                        {
                            char comanda[BUFFER_SIZE];
                            char PrenotaId[BUFFER_SIZE];
                            char parziale[100];
                            char comandeFile[BUFFER_SIZE];
                            char idComanda[100];
                            //ricevo la comanda tutta
                            ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                            bytes_needed = ntohs(real_len);
                            ret = recv(i,(void*)comanda,bytes_needed,0);
                            comanda[bytes_needed-2]='\0';

                            //ricevo l'id prenotazione per la comanda in questione
                            ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                            bytes_needed = ntohs(real_len);
                            ret = recv(i,(void*)PrenotaId,bytes_needed,0);
                            PrenotaId[bytes_needed]='\0';
                            numComanda++;
                            sprintf(idComanda,"%s%d","com",numComanda);
                            c.stato=0; //comanda appena giunta
                            
                            strncpy(c.idPrenotazione,PrenotaId,20);
                            c.idPrenotazione[8]='\0';
                            fp = fopen("Comande.txt","a");

                            char * t = strtok(comanda," ");
                            t = strtok(NULL," ");
                            while(t != NULL)
                            {
                            strcpy(parziale,t);
                            sprintf(comandeFile,"%s %s %s %d\n",idComanda,c.idPrenotazione,parziale,c.stato);
                            fprintf(fp,comandeFile);
                            t = strtok(NULL," ");
                            }
                            fclose(fp);
                        }
                        if(strcmp(bufferCommand,"conto")==0)
                        {
                            int conteggio=0;
                            int Nportate=0;
                            int parziale=0;
                            char line[100];
                            char code[20];
                            char comanda[BUFFER_SIZE];
                            char BufferTotComande[BUFFER_SIZE];
                            char appoggio[BUFFER_SIZE];

                            fp = fopen("Comande.txt","r");

                            while (fgets(line, sizeof(line), fp)) { 
                                sscanf(line,"%s %s %s",bufferCommand,code,comanda);
                                if(strcmp(code,c.idPrenotazione)==0)
                                {
                                    sprintf(appoggio,"\n%s",comanda);
                                    char *tok = strtok(comanda,"-");
                                    tok[2]='\0';
                                    int ii;
                                    for(ii = 0;ii<MENU_SIZE; ii++)
                                    {
                                            //printf("cod:%c%c, tok:%s.\n",m[ii].codice[0],m[ii].codice[1],tok);
                                        if(m[ii].codice[0]==tok[0] && m[ii].codice[1]==tok[1] /*strcmp(m[ii].codice,tok)==0 */)
                                        {
                                            //printf("costoIF: %d\n",m[ii].costo);                                            
                                            parziale =  m[ii].costo;
                                        }
                                    }
                                    //printf("codice: %s,costo: %d\n",tok,parziale);
                                    tok = strtok(NULL,"-");
                                    //printf("quantita:%s\n",tok);
                                    Nportate= *(tok) -48;
                                    conteggio +=  parziale*Nportate;
                                    strcat(BufferTotComande,appoggio);
                                    //printf("conteggio: %d\n",conteggio);
                                }       
                            }
                            //mando le comande
                            strcpy(buffer,BufferTotComande);
                            len = strlen(buffer) +1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint32_t),0);
                            ret =send(i,(void*)buffer,len,0);
                            //printf("conto da mandare:%s\n",buffer);
                            fclose(fp);

                            //mando il conto e DEVO aggiornare lo stato
                            send(i, &conteggio, sizeof(int), 0);
                        }
                    }
                    if(type=='K')
                    {
                        //ricezione token
                        ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(i,(void*)buffer,bytes_needed,0);
                        sscanf(buffer,"%s",bufferCommand);
                        bufferCommand[bytes_needed-1]='\0';

                        if(strcmp(bufferCommand,"take")==0)
                        {
                            char ComBuffer[BUFFER_SIZE];
                            //char appoggio[BUFFER_SIZE];
                            FILE *fp1;

                            fp = fopen("Comande.txt","r+");
                            if (fp == NULL) {
                                printf("Impossibile aprire il file.");
                                return 1;
                            }
                            fp1 = fopen("Comande2.txt","a");
                            if (fp1 == NULL) {
                                printf("Impossibile aprire il file.");
                                return 1;
                            }
                            char line[100];
                            char comanda[100];
                            char Prenotazione[100];
                            char comandaContr[100];
                            char appoggio[BUFFER_SIZE];
                            char ordine[100];
                            int stato=0;
                            int b = 0; //prende la prima comanda non in preparazione
                            while (fgets(line, sizeof(line), fp)) { 
                                    sscanf(line,"%s %s %s %d",comanda,Prenotazione,ordine,&stato);
                                    //printf("line:%s %s %s %d\n",comanda,Prenotazione,ordine,stato);
                                    if(stato==0 && b == 0) //salvo nome comanda utile
                                    {
                                        b = 1;
                                        /*mando prima il codice comanda*/
                                        strcpy(comandaContr,comanda);
                                        len = strlen(comandaContr) + 1;
                                        real_len=htons(len);
                                        ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                                        ret=send(i,(void*)comandaContr,len,0);
                                        printf("codice comanda:%s\n",comandaContr);
                                    }
                                    if(strcmp(comanda,comandaContr)==0){
                                        stato = 1;
                                        sprintf(appoggio,"%s\n",ordine);
                                        strcat(ComBuffer,appoggio);
                                    }
                                    fprintf(fp1, "%s %s %s %d\n", comanda,Prenotazione,ordine,stato);
                                    stato=0;
                            }
                            /*poi mando gli ordini della comanda al kd*/
                            len = strlen(ComBuffer) + 1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret=send(i,(void*)ComBuffer,len,0);

                            printf("comanda mandata: \n%s\n",ComBuffer);

                            fclose(fp);
                            fclose(fp1);
                            remove("Comande.txt");
                            rename("Comande2.txt","Comande.txt");
                        }else if(strcmp(bufferCommand,"show")==0)
                        {
                            char bufferShow[BUFFER_SIZE];

                            char line[100];
                            char comanda[100];
                            char Prenotazione[100];
                            char comandaContr[100];
                            char appoggio[BUFFER_SIZE];
                            char ComBuffer[BUFFER_SIZE];
                            char ordine[100];
                            int stato=0;
                            int b = 0;
                            fp = fopen("Comande.txt","r");

                            while (fgets(line, sizeof(line), fp)) { 
                                    sscanf(line,"%s %s %s %d",comanda,Prenotazione,ordine,&stato);
                                    if(stato==1 && b == 0)
                                    {
                                        /*mando il codice comanda*/
                                        strcpy(comandaContr,comanda);
                                        len = strlen(comandaContr) + 1;
                                        real_len=htons(len);
                                        ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                                        ret=send(i,(void*)comandaContr,len,0);
                                        printf("codice comanda:%s\n",comandaContr);

                                    }stato=0;b=1;
                                if(strcmp(comandaContr,comanda)==0)
                                {
                                    sprintf(appoggio,"%s\n",ordine);
                                    strcat(ComBuffer,appoggio);
                                }
                            }
                                /*mando tutta la comanda*/
                                len = strlen(ComBuffer) + 1;
                                real_len=htons(len);
                                ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                                ret=send(i,(void*)ComBuffer,len,0);

                                printf("comanda mandata: \n%s\n",ComBuffer);

                        }else if(strcmp(bufferCommand,"ready")==0)
                        {

                        }
                    }
                }
            }
        }
    }
}
    

