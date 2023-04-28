#include <arpa/inet.h> /*server.c 28/04/2023*/
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

#define MAX_DISP 5


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

typedef struct
{
    int sd;
    char tipo;
}disp;

typedef struct
{
    int sd;
    char tipo[3];
}dispTav;


void caricaDisp(disp*d, int nDisp){
    FILE*fptr;
    fptr=fopen("dispConnessi.txt","w");
    if(fptr == NULL)
    {
      printf("Error!");   
      exit(1);             
    }
    int i_f;
    for(i_f=0; i_f<nDisp; i_f++){
        fprintf(fptr,"%d %c \n", d[i_f].sd, d[i_f].tipo);
    }
    fclose(fptr);
}

void caricaTav(dispTav*d2, int nDisp){
    FILE*fptr;
    fptr=fopen("tavConnessi.txt","w");
    if(fptr == NULL)
    {
      printf("Error!");   
      exit(1);             
    }
    int i_f;
    for(i_f=0; i_f<nDisp; i_f++){
        fprintf(fptr,"%d %s \n", d2[i_f].sd, d2[i_f].tipo);
        //printf("TAVOLI:%d %s \n", d2[i_f].sd, d2[i_f].tipo);
    }
    fclose(fptr);
}

int  leggi_tipo(int newsocket, char* newtype, int nDisp){
    FILE* fp;
    int socket=0;
    char type;
    int controllo=0;
    int i;
    if((fp = fopen("dispConnessi.txt","r")) == NULL){
       printf("Error! opening file\n");
       exit(1);
    }
    for( i=0; i<nDisp; i++){
      fscanf(fp,"%d %c",&socket,&type );

        printf("\nil socket è %d, il new %d\n", socket, newsocket);
      if(socket==newsocket){ //se quello che prende dal file è uguale a quello NUOVO
        *newtype=type;       // Aggiorna type
        controllo=1;         // metti controllo a 1
      }
      
    }
    return controllo;       //se non lo trovi metti controllo a 0
    fclose(fp);
}
int leggi_tipo_2(int newsocket, int* newtype, int nDisp){
    FILE* fptr;
    int socket=0;
    int type;
    int controllo=0;
    int i;
    if((fptr = fopen("tavConnessi.txt","r")) == NULL){
       printf("Error! opening file\n");
       // Program exits if the file pointer returns NULL.
       exit(1);
    }
    for( i=0; i<nDisp; i++){
      fscanf(fptr,"%d %d",&socket,&type );

        printf("\nil socket è %d, il new %d\n", socket, newsocket);
      if(socket==newsocket){ //se quello che prende dal file è uguale a quello NUOVO
        *newtype=type;       // Aggiorna type
        controllo=1;         // metti controllo a 1
      }
      
    }
    return controllo;       //se non lo trovi metti controllo a 0
    fclose(fptr);
}

int getTable(char *idpren,char *table)
{
    int controllo=0;
    char id[10];
    FILE *fp;
    fp = fopen("PrenotazioniCodici.txt","r");
    char line[100];
    while(fgets(line,sizeof(line),fp))
    {
        sscanf(line,"%s %s",id,table);
        if(strcmp(id,idpren)==0)
        {
            fclose(fp);
            return controllo;
        }
    }controllo=1;
    fclose(fp);
    return controllo;
}


void chiudi_tutto(){
    FILE *fp6;
    
    fp6 = fopen("dispConnessi.txt", "r");
    if (fp6 == NULL) {
        printf("Errore nell'apertura dei file\n");
        
    }char str[100];
    int pp=0;
    while (fgets(str, 100, fp6) != NULL) { 
        fscanf(fp6,"%d", &pp);
        printf("\nil socket:%d",pp);
        fscanf(fp6,"%*c");
       
        close(pp);
    }

    fclose(fp6);
}

int main()
{
    int ret, sd, new_sd, len,bytes_needed,cl;
    int nDisp=0;
    int nDisp2=0;
    int listener;
    int prenIndex;
    struct sockaddr_in cl_addr, cl2_addr;
    int i;
    int check =0;
    int check_2=0;
    char buffer[1024],bufferCommand[1024],buftype[2];
    char type;
    int type_2;
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
    disp d[MAX_DEVICE];
    dispTav d_2[MAX_DEVICE];
    int nn;
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
       printf("Error! opening file\n");
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
                    char input_string[BUFFER_SIZE];
                    char command[5];
                    char plus[20];
                    fgets(input_string, 100, stdin);
                    sscanf(input_string,"%s %s",command,plus);
                    //printf("command:%s\n",command);
                    if(strcmp(command,"stat")==0)
                    {   
                        FILE *fp4,*fp5;
                        if(strcmp(plus,"a")==0)
                        {
                            /*comande in attesa*/
                        fp4 = fopen("Comande.txt","r");
                        char line[100];
                        char com[10];
                        char com2[10];
                        char idpren[10];
                        char pieta[10];
                        int stato;
                        int sock;
                        int b = 0;
                        while(fgets(line,sizeof(line),fp4))
                        {
                            sscanf(line,"%s %s %s %d %d",com,idpren,pieta,&stato,&sock);
                            //printf("here\n");
                            if(stato==0 && strcmp(com,com2)!=0 || b==0)
                            {
                                b=1;
                                char tav[10];
                                getTable(idpren,tav);
                                printf("%s %s\n",com,tav);    
                            }
                            //printf("LINE:%s %s %s %d %d\n",com,idpren,pieta,stato,sock);
                            strcpy(com2,com);
                            if(stato==0)
                                printf("%s\n",pieta);
                            
                        }
                            
                        }else if(strcmp(plus,"p")==0)
                        {
                            /*comande in preparazione*/
                        fp4 = fopen("Comande.txt","r");
                        char line[100];
                        char com[10];
                        char com2[10];
                        char idpren[10];
                        char pieta[10];
                        int stato;
                        int sock;
                        int b = 0;
                        while(fgets(line,sizeof(line),fp4))
                        {
                            sscanf(line,"%s %s %s %d %d",com,idpren,pieta,&stato,&sock);
                            //printf("here\n");
                            if(stato==1 && strcmp(com,com2)!=0 || b==0)
                            {
                                b=1;
                                char tav[10];
                                getTable(idpren,tav);
                                printf("%s %s\n",com,tav);    
                            }
                            //printf("LINE:%s %s %s %d %d\n",com,idpren,pieta,stato,sock);
                            strcpy(com2,com);
                            if(stato==1)
                                printf("%s\n",pieta);
                        }
                        }else if(strcmp(plus,"s")==0)
                        {
                        fp4 = fopen("Comande.txt","r");
                        char line[100];
                        char com[10];
                        char com2[10];
                        char idpren[10];
                        char pieta[10];
                        int stato;
                        int sock;
                        int b = 0;
                        while(fgets(line,sizeof(line),fp4))
                        {
                            sscanf(line,"%s %s %s %d %d",com,idpren,pieta,&stato,&sock);
                            if(stato==2 && strcmp(com,com2)!=0 || b==0)
                            {
                                b=1;
                                char tav[10];
                                getTable(idpren,tav);
                                printf("%s %s\n",com,tav);    
                            }
                            strcpy(com2,com);
                            if(stato==2)
                                printf("%s\n",pieta);
                        }
                        }

                        //caso del table: tutte le sue comande in corso

                        fp5 = fopen("PrenotazioniCodici.txt","r");
                        char line[100];
                        char id[10];
                        char tav[10];
                        while(fgets(line,sizeof(line),fp5))
                        {
                            sscanf(line,"%s %s",id,tav);
                            if(strcmp(tav,plus)==0)
                            {/*apro il file Comande.txt e stampo ogni comanda col suo stato*/
                                fp4 = fopen("Comande.txt","r");
                                char line1[100];
                                char com[10];
                                char comAp[10];
                                char idpren[10];
                                char idprenAp[10];
                                char portata[10];
                                char portAp[10];
                                int stato;
                                int statoApp;
                                int sock;
                                int sockAp;
                                int b = 0,a=1; //necessaria per la stampa
                                while(fgets(line1,sizeof(line1),fp4))
                                {
                                    sscanf(line1,"%s %s %s %d %d",comAp,idprenAp,portAp,&stato,&sockAp);
                                    char *tok = strtok(line1," ");
                                    if(strcmp(tok,com)!=0){
                                        b = 1;
                                        strcpy(com,tok);
                                        printf("%s",com);
                                        if(stato==0)   
                                            printf(" <in attesa>\n");
                                        if(stato==1)
                                            printf(" <in preparazione>\n");
                                        if(stato==2)
                                            printf(" <in servizio>\n");
                                    }//a = 0;
                                    strcpy(com,tok);
                                    tok = strtok(NULL," ");
                                    strcpy(idpren,tok);
                                    tok = strtok(NULL," ");
                                    strcpy(portata,tok);
                                    tok = strtok(NULL," ");
                                    stato=*(tok) - 48;
                                    tok = strtok(NULL," ");
                                    sock=*(tok) - 48;

                                    //printf("%s %s %s %d %d\n",com,idpren,portata,stato,sock);
                                    printf("%s\n",portata);
                                    b=0;
                                }
                            }
                        }
                    fclose(fp4);
                    fclose(fp5);
                    }else if(strcmp(command,"stop")==0)
                    {
                        printf("spengo tutto\n");
                        FILE *fp7;
                        fp7 = fopen("dispConnessi.txt","r+");
                        char line[100];
                        int sockid;
                        char tipo;
                        while(fgets(line,sizeof(line),fp7))
                        {
                            sscanf(line,"%d %c",&sockid,&tipo);
                            printf("chiudo il socket:%d\n",sockid);
                            close(sockid);
                        }fclose(fp7);
                    }

                }
                if(i==listener)
                {
                    printf("Nuovo cliente rilevato!\n");
                    fflush(stdout);

                    // Dimensione dell'indirizzo del client
                    len = sizeof(cl_addr);
                    socklen_t len_p = sizeof(struct sockaddr);
                    // Accetto nuove connessioni 
                    //*** ATTENZIONE: BLOCCANTE!!! ***
                    new_sd = accept(listener, (struct sockaddr *) &cl_addr, &len_p);
                    printf("il new_sd è:%d\n", new_sd);
                    char buftype[2];
                    ret = recv(new_sd, (void*)buftype, 2, 0);
                    sscanf(buftype, "%c", &type);

                    printf("il tipo è %c\n", type);
                    if(type == 'T'){ 
                        
                        ret = recv(new_sd, (void*)&type_2, sizeof(uint16_t), 0);
                        printf("il tipo_2 è %d\n", type_2);
                        check_2 = leggi_tipo_2(new_sd, &type_2, nDisp2);
                        printf("sono il check_2:%d\n",check_2);
                        if(check_2==0){//se non c'è nella lista lo carico su file
                            
                            d_2[nDisp2].sd=new_sd;
                            d_2[nDisp2].tipo[0]=type_2;
                            nDisp2++;
                            caricaTav(d_2,nDisp2);
                            printf("\nl'ho caricato nel file\n");
                        }
                    }
                    check = leggi_tipo(new_sd, &type, nDisp);
                    printf("sono il check:%d\n",check);
                    if(check==0){//se non c'è nella lista lo carico su file
                        
                        d[nDisp].sd=new_sd;
                        d[nDisp].tipo=type;
                        nDisp++;
                        caricaDisp(d,nDisp);
                        printf("\nl'ho caricato nel file\n");
                    }
                    FD_SET(new_sd, &master);
                    if(new_sd > fdmax){fdmax = new_sd;}
                    printf("\nsto aspettando qui DUE\n");
                }else
                {
                    printf("\nci son cascato di nuovo %d, %c, %d",i, type,nDisp);
                    check = leggi_tipo(i, &type, nDisp);
                    printf("\nsono il check due %d\n", check);
                    printf("\ndopo ci son cascato %c\n", type);
                    //PRIMA RICEZIONE C
                    if(type=='C')
                    {

                    printf("sta parlando il client:%d\n",i);

                    //SECONDA RICEZIONE: TOKEN
                    ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                    bytes_needed = ntohs(real_len);
                    ret = recv(i,(void*)bufferCommand,bytes_needed,0);
                    printf("bufcom:%s\n",bufferCommand);
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
                    ret = recv(i,(void*)&real_len,sizeof(uint32_t),0);
                    bytes_needed = ntohs(real_len);

                    ret = recv(i,(void*)buffer,bytes_needed,0);

                    printf("dati ricevuti:%s",buffer);

                    //FIND
                    if(strcmp("find",bufferCommand)==0)
                    {
                        char nome[20],dataP[20],nometav[6],sala[6],vicinanza[6];
                        int orario,indice;
                        char PrenEffettuate[30];
                        int b = 0;
                        char prenotabili[1024]; //buffer di concatenazione
                        sscanf(buffer, "%s %s %d %s %d" ,bufferCommand, p.cognome, &p.nPersone, p.data, &p.orario);


                        strcpy(Data,p.data); //Data è un array di appoggio per fare il confronto dopo
                        char* token = strtok(p.data, "-");
                        convStringa=atoi(token); //converte stringa intero
                        while (token != NULL) {
                        strcpy(stringa[b],token);
                        token = strtok(NULL, "-");
                        //printf("stringa: %s\n",stringa[i]);
                        b++;
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
                            int a=0;
                            while (token != NULL) {
                            //printf("%s\n", token);  // stampa il singolo parametro
                            if(a==0)
                                strcpy(nome,token);
                            if(a==1)
                                strcpy(dataP,token);
                            if(a==2)
                                orario=atoi(token);
                            if(a==3)
                                indice=atoi(token);
                            if(a==4)
                                strcpy(nometav,token);
                            if(a==5)
                                strcpy(sala,token);
                            if(a==6)
                                strcpy(vicinanza,token);
                            token = strtok(NULL, " "); 
                            a++;
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
                    printf("invio dati!");
                    len = strlen(prenotabili) +1;
                    //printf("lunghezza buffer: %d\n",len); 
                    real_len=htons(len);
                    ret=send(i,(void*)&real_len,sizeof(uint32_t),0);
                    ret=send(i,(void*)prenotabili,len,0);
                    //BOOK  
                    }else if(strcmp("book",bufferCommand)==0)
                    {
                        FILE *ffp;
                        char PrenotazioneCode[1024];
                        char bufferFile[1024];
                        char BufferOrarioPren[1024];
                        sscanf(buffer, "%s %d" ,bufferCommand, &prenIndex);
                        //printf("ossia il tavolo: %d %s %s %s\n", prenIndex,tabInviati[prenIndex].nome_tavolo, tabInviati[prenIndex].nome_sala,tabInviati[prenIndex].vicinanza);
                        
                        
                        fp = fopen("Prenotazioni.txt", "a");
                        ffp = fopen("PrenotazioniCodici.txt","r");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }/*controllo che il tavolo desiderato non sia gia stato preso parallelamente*/
                        char line[100];
                        char nTav[10];
                        char idPren[10];
                        int b=0;
                        while(fgets(line,sizeof(line),ffp))
                        {
                            sscanf(line,"%s %s",idPren,nTav);
                            printf("nomeTav:%s tavolo:%s data: %s ora:%d\n",nTav,tabInviati[prenIndex].nome_tavolo,p.data,p.orario);
                            //if(){ /*data e ora corrispondono DA FARE FORSE*/
                                if(strcmp(nTav,tabInviati[prenIndex].nome_tavolo)==0)
                                {
                                b = 1;
                                //printf("la prenotazione per questo tavolo è gia stata trovata!\n");
                                sprintf(BufferOrarioPren,"%s","NO");
                                }
                            //}
                        }
                        fclose(ffp);
                        if(b==0){
                        sprintf(bufferFile,"\n%s %s %d %d %s %s %s",p.cognome,Data,p.orario,p.nPersone,tabInviati[prenIndex].nome_tavolo, tabInviati[prenIndex].nome_sala,tabInviati[prenIndex].vicinanza);
                        fprintf(fp,bufferFile);
                        sprintf(BufferOrarioPren,"%s%d%s",p.cognome,p.nPersone,tabInviati[prenIndex].nome_tavolo);
                        printf("prenotazione effettuata! Invio codice in corso\n");
                        }
                        fclose(fp);


                        fp = fopen("PrenotazioniCodici.txt", "a");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }
                        //BufferOrarioPren contiene il codice prenotazione
                       
                        //fprintf(fp,"%s %s",PrenotazioneCode,tabInviati[prenIndex].nome_tavolo);

                        len = strlen(BufferOrarioPren)+1;
                        real_len = htons(len);
                        ret = send(i, (void*) &real_len, sizeof(uint32_t),0);
                        ret = send(i, (void*) BufferOrarioPren, len, 0);

                        //printf("mando il codice: %s\n",BufferOrarioPren);
                        if(b==0){
                            fprintf(fp,"%s %s\n",BufferOrarioPren,tabInviati[prenIndex].nome_tavolo);
                            fclose(fp);
                        }
                        }
                    }else if(strcmp("esc",bufferCommand)==0)
                    {
                        printf("chiusura client:%d\n",i);
                    }
                    if(type=='T')
                    {
                        //int check = leggi_tipo(new_sd, &type, numdevice);

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
                        char nTav[3];
                        while (fgets(linea, sizeof(linea), fp1))
                        {   
                            sscanf(linea,"%s %s",code,nTav);
                            code[strlen(code)]='\0';
                            //printf("confronto con:%s.\n",code);

                            if(strcmp(code,prenoCode)==0){
                                b = 1;
                                ok = 's';
                                aut = 1;
                                strcpy(c.idPrenotazione,prenoCode);
                                //c.idPrenotazione[8]='\0';
                                printf("c.idpren|:%s\n",c.idPrenotazione);
                                /*mi salvo il numero del tavolo relativo alla prenotazione e lo scrivo in tavConnessi*/
                                int jj;
                                for(jj=0; jj<nDisp2;jj++)
                                {
                                    if(d_2[jj].sd==i){
                                        strcpy(d_2[jj].tipo,nTav);
                                        printf("nTav!:%s\n",nTav);
                                    }
                                }
                                caricaTav(d_2,nDisp2);
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
                        b=1;
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
                        }//COMANDA
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
                            PrenotaId[bytes_needed-2]='\0';
                            printf("prenotaid: %s,\n",PrenotaId);
                            numComanda++;
                            sprintf(idComanda,"%s%d","com",numComanda);
                            c.stato=0; //comanda appena giunta
                            fp = fopen("Comande.txt","a");

                            char * t = strtok(comanda," ");
                            t = strtok(NULL," ");
                            while(t != NULL)
                            {
                            strcpy(parziale,t);
                            sprintf(comandeFile,"%s %s %s %d %d\n",idComanda,PrenotaId,parziale,c.stato,0);
                            fprintf(fp,comandeFile);
                            t = strtok(NULL," ");
                            }
                            /*chiudo handshake*/
                            char bufret[3];
                            strcpy(bufret,"OK\0");
                            len = strlen(bufret) +1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret =send(i,(void*)bufret,len,0);
                            fclose(fp);
                            /*mando segnale del tavolo che ha inviato la comanda a tutti i kd?*/
                            int sd_i;
                            char sglBuffer[BUFFER_SIZE];
                            for(sd_i=0;sd_i<nDisp;sd_i++)
                            { 
                                if(d[sd_i].tipo=='K')
                                {
                                    printf("mando il segnale al socket:%d\n",d[sd_i].sd);
                                    sprintf(bufferCommand,"comanda da %s in attesa",PrenotaId);
                                    len = strlen(bufferCommand) +1;
                                    real_len=htons(len);
                                    ret=send(d[sd_i].sd,(void*)&real_len,sizeof(uint16_t),0);
                                    ret=send(d[sd_i].sd,(void*)bufferCommand,len,0);
                                }
                            }
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
                                printf("CODE:%s c.idPren:%s\n",code,c.idPrenotazione);
                                if(strcmp(code,c.idPrenotazione)==0)
                                {
                                    printf("siamo dentro\n");
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
                            FILE *fp1,*fp2;

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
                            fp2 = fopen("PrenotazioniCodici.txt","r");
                            if (fp2 == NULL) {
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
                            
                            char line1[100];
                            char idPren[20];
                            char tav[20];
                            
                            while (fgets(line, sizeof(line), fp)) { /*Comande.txt*/
                                    sscanf(line,"%s %s %s %d",comanda,Prenotazione,ordine,&stato);
                                    if(stato==0 && b == 0) //salvo nome comanda utile
                                    {
                                        printf("prenotazione:%s.\n",Prenotazione);
                                        b = 1;
                                        /*ricavo il tavolo prenotato con Prenotazione*/
                                        while(fgets(line1,sizeof(line1),fp2))
                                        {
                                            sscanf(line1,"%s %s",idPren,tav);
                                            printf("idpren:%s.\n",idPren);
                                            if(strcmp(idPren,Prenotazione)==0)
                                            {
                                                printf("il tav è : %s\n",tav);
                                                break;
                                            }
                                        }/*mando il codice comanda*/
                                        strcpy(comandaContr,comanda);
                                        len = strlen(comandaContr) + 1;
                                        real_len=htons(len);
                                        ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                                        ret=send(i,(void*)comandaContr,len,0);
                                        printf("codice comanda:%s\n",comandaContr);
                                        /*mando id del tavolo*/
                                        len = strlen(tav) + 1;
                                        real_len=htons(len);
                                        ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                                        ret=send(i,(void*)tav,len,0);
                                    }
                                    if(strcmp(comanda,comandaContr)==0){
                                        stato = 1;
                                        sprintf(appoggio,"%s\n",ordine);
                                        strcat(ComBuffer,appoggio);
                                        fprintf(fp1, "%s %s %s %d %d\n", comanda,Prenotazione,ordine,stato,i);

                                    }//la i infondo indica il socket che ha preso le comande
                                    else{
                                    fprintf(fp1, "%s %s %s %d %d\n", comanda,Prenotazione,ordine,stato,0);
                                    }
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
                            fclose(fp2);
                            remove("Comande.txt");
                            rename("Comande2.txt","Comande.txt");

                            /*invio al tavolo designato che la sua comanda è in preparazione*/
                            int sdt_i;
                            char notifyBuffer[BUFFER_SIZE];
                            for(sdt_i=0;sdt_i<nDisp2;sdt_i++)
                            {   
                                printf("tipo e tav %s %s\n",d_2[sdt_i].tipo,tav);
                                if(strcmp(d_2[sdt_i].tipo,tav)==0) /*tavolo trovato, mando la notifica al socket*/
                                {
                                    printf("mando la notifica al tavolo del socket:%d\n",d_2[sdt_i].sd);
                                    sprintf(notifyBuffer,"la comanda %s è in preparazione\n",comanda);
                                    len = strlen(notifyBuffer) + 1;
                                    real_len=htons(len);
                                    ret=send(d_2[sdt_i].sd,(void*)&real_len,sizeof(uint16_t),0);
                                    ret=send(d_2[sdt_i].sd,(void*)notifyBuffer,len,0);
                                }
                            }

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
                            int currentSd=0;
                            fp = fopen("Comande.txt","r");

                            while (fgets(line, sizeof(line), fp)) { 
                                    sscanf(line,"%s %s %s %d %d",comanda,Prenotazione,ordine,&stato,&currentSd);
                                    if(stato==1 && b == 0 && currentSd==i)
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
                        { //mette a 2 lo stato della comanda designata

                            char idComanda[10];
                            char nomeTav[10];
                            FILE *fp1;
                            ret = recv(sd,(void*)&real_len,sizeof(uint16_t),0);
                            bytes_needed = ntohs(real_len);
                            ret = recv(sd,(void*)buffer,bytes_needed,0);
                            buffer[bytes_needed-1]='\0';
                            printf("%s\n",buffer);

                            char *t = strtok(buffer," ");
                            t = strtok(NULL,"-");
                            strcpy(idComanda,t);
                            printf("%s\n",idComanda);

                            t = strtok(NULL," ");
                            strcpy(nomeTav,t);
                            printf("%s\n",nomeTav);
                            fp = fopen("Comande2.txt","w");
                            fp1 = fopen("Comande.txt","r");
                            if(fp == NULL || fp1 == NULL)
                            {
                                printf("errore mell'apertura\n");
                                exit(1);
                            }
                            char line[100];
                            char comanda[100];
                            char idPren[20];
                            char piet[20];
                            int stato;
                            int sock=0;
                           
                            while(fgets(line,sizeof(line),fp1))
                            {
                                sscanf(line,"%s %s %s %d %d",comanda,idPren,piet,&stato,&sock);
                                printf("%s %s %s %d %d\n",comanda,idPren,piet,stato,sock);
                                if(strcmp(comanda,idComanda)==0)
                                {
                                    fprintf(fp,"%s %s %s %d %d\n",comanda,idPren,piet,2,sock);
                                }else
                                    fprintf(fp,"%s %s %s %d %d\n",comanda,idPren,piet,stato,sock);
                                
                            } /*dovrei prendere il numero del socket che aveva la comanda definita e dirgli che è in servizio*/
                            fclose(fp);
                            fclose(fp1);
                            remove("Comande.txt");
                            rename("Comande2.txt","Comande.txt");
                        }
                    }
                }
            }
        }
    }
}
    

