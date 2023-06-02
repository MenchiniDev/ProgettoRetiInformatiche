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


#define MENU_SIZE 15
#define COMANDA_SIZE 4
#define TAV_SIZE 20

#define MAX_DEVICE 10

#define MAX_DISP 5

/*
    struttura che si inizializza tramite il file filePrenotazioni,
    contenente tutti i giorni di una settimana, gli orari e i tavoli ed i
    relativi status per ogni orario
*/
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

/*strutture usate come appoggio nella copia e manipolazione dei dati*/
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

struct comanda
{
    char idPrenotazione[20];
    char pietanza[200];
    int quantita;
    int stato; // 0->appena arrivata, 1->in gestione 2->in servizio
};
/*
    queste strutture dati identificano i dspositivi, distinguendo tra i tavoli 
    ed i dispositivi generici
*/
typedef struct
{
    int sd;
    char tipo;
}disp;

typedef struct
{
    int sd;
    char tipo[3];
    int stato;
}dispTav;

/*
    questa funzione carica il socket id dentro il file connessioni, 
    con cui identifichiamo quale socket controlla quale tipo di dispositivo    
*/
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
/*
    funzione similare alla precedente;
    differisce soltanto per il tipo di dispositivo, che sono solo tavoli
*/
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
    }
    fclose(fptr);
}

/*
    se le funzioni precedenti scrivevano dentro i file contententi le connessioni attive
    queste vi leggono e restituiscono gli identificatori (se esistono)
*/
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

        //printf("\nil socket è %d, il new %d\n", socket, newsocket);
      if(socket==newsocket){ //se quello che prende dal file è uguale a quello NUOVO
        *newtype=type;       // Aggiorna type
        controllo=1;         // metti controllo a 1
      }
      
    }
    return controllo;       //controllo a 0 se non lo trova
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

        //printf("\nil socket è %d, il new %d\n", socket, newsocket);
      if(socket==newsocket){ //se quello che prende dal file è uguale a quello NUOVO
        *newtype=type;       // Aggiorna type
        controllo=1;         // mette controllo a 1
      }
      
    }
    return controllo;       //se non lo trovi metti controllo a 0
    fclose(fptr);
}
/*
    funzione che dato in ingresso un idPrenotazione (conoscendo il formato con cui sono creati)
    modifica *table con il tavolo relativo, controllo è solo una variabile di controllo 
*/
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

/*
    funzione che chiude tutte le connessioni nei file interessati
*/
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

int main(int argc, char* argv[])
{

    int ret, new_sd, len,bytes_needed;
    int nDisp=0;
    int nDisp2=0;
    int listener;
    int prenIndex;
    struct sockaddr_in cl_addr;
    int i;
    int check =0;
    int check_2=0;
    char buffer[BUFFER_SIZE],bufferCommand[BUFFER_SIZE];
    uint16_t port;
    //if(argc != 2)
        port = 4242;
    //else
    //    port = (uint16_t)strtol(argv[1],NULL,10);
    /*controllo immissione porta nulla*/

    char type;
    int type_2;
    FILE *fp,*fptr;
    uint16_t real_len;
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

    struct prenotazione p;
    struct settimana sett[WEEK_SIZE];
    disp d[MAX_DEVICE];
    dispTav d_2[MAX_DEVICE];
    fd_set master;
    fd_set read_fds;
    int fdmax;

    int b = 1;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    /*inizializzo il menu*/
    fp = fopen("Menu2.txt","r");
    if(fp == NULL)
    {
    printf("errore nell'apertura del file\n");
        exit(1);
    }
    char line[200];
    int ii=0;
    char* token = NULL;

    while (fgets(line, sizeof(line), fp)) { 
        memset(buffer,0,sizeof(buffer));
        token = strtok(line, "-");
        strncpy(m[ii].codice,token,2);
        m[ii].codice[2]='\0';
        token = strtok(NULL, "-"); 
        strcpy(m[ii].pietanza,token);
        token = strtok(NULL, "-"); 
        m[ii].costo = atoi(token) /*- 48*/;
        ii++;
    }/*inizializzazione del menu completata*/
    /*inizializzo la struttura dati con il file di riferimento*/
    if ((fptr = fopen("filePrenotazioni.txt","r")) == NULL){
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
    fclose(fptr);
    /*popolazione della comanda e delle prenotazioni avvenute*/


    listener = socket(AF_INET,SOCK_STREAM,0);
    memset(&cl_addr,0,sizeof(cl_addr));
    cl_addr.sin_family=AF_INET;
    cl_addr.sin_port=htons(port); 
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
    fdmax = listener;


    while(1){
        read_fds = master;
        ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
        if(ret<0){
            perror("ERRORE SELECT");
            exit(1);
        }
        /*modo per operare: ogni volta che arriva un device questi mi manda il suo tipo,
        io lo salvo nel file delle cconnessioni relative e gestisco le possibili richieste con
        una serie di if e strcmp() */
        for(i = 0; i <= fdmax ; i++)
        {
            if(FD_ISSET(i,&read_fds))
            {                   
                if(i==0)
                {
                    /*
                    prendo i valori da terminale per le stampe del server
                    se vengono inseriti anche i caratteri a p s eccetera questi saranno salvati
                    */
                    char input_string[BUFFER_SIZE];
                    char command[6];
                    char plus[20];
                    strcpy(plus,"vuoto");   
                    strcpy(command,"vuoto");                
                    fgets(input_string, 100, stdin);

                    char *token;

                    token = strtok(input_string, " ");
                    strcpy(command,token);
                    token = strtok(NULL, " ");
                    if(token!=NULL){
                        strcpy(plus,token);
                    }
                    plus[strlen(plus)-1]='\0';
                    command[strlen(command)]='\0';


                    if(strcmp(command,"stat\n")==0 || strcmp(command,"stat\0")==0 || strcmp(command,"stat")==0)
                    {   
                        FILE *fp4,*fp5;
                        if(strcmp(plus,"a")==0)
                        {
                            printf("comande in attesa\n");
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
                            if((stato==0 && strcmp(com,com2)!=0) && b==0)
                            {
                                b=1;
                                char tav[10];
                                memset(tav,0,sizeof(tav));
                                getTable(idpren,tav);
                                printf("%s %s\n",com,tav);    
                            }
                            strcpy(com2,com);
                            if(stato==0)
                                printf("%s\n",pieta);
                            
                        }
                            
                        }else if(strcmp(plus,"p")==0)
                        {
                            printf("comande in preparazione\n");
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
                            if((stato==1 && strcmp(com,com2)!=0) && b==0)
                            {
                                b=1;
                                char tav[10];
                                getTable(idpren,tav);
                                printf("%s %s\n",com,tav);    
                            }
                            strcpy(com2,com);
                            if(stato==1)
                                printf("%s\n",pieta);
                        }
                        }else if(strcmp(plus,"s")==0)
                        {
                            printf("comande in servizio\n");
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
                            if((stato==2 && strcmp(com,com2)!=0) && b==0)
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

                        //caso del table generico: tutte le sue comande in corso

                        fp5 = fopen("PrenotazioniCodici.txt","r");
                        char line[100];
                        char id[10];
                        char tav[10];
                        if(strcmp(plus,"")==0 || strcmp(plus,"vuot")==0)
                        {
                            while(fgets(line,sizeof(line),fp5))
                            {
                            sscanf(line,"%s %s",id,tav);
                                /*apro il file Comande.txt e stampo ogni comanda col suo stato*/
                                fp4 = fopen("Comande.txt","r");
                                char line1[100];
                                char com[10];
                                char comAp[10];
                                char idpren[10];
                                char idprenAp[10];
                                char portata[10];
                                char portAp[10];
                                int stato;
                                int sockAp;
                                while(fgets(line1,sizeof(line1),fp4))
                                {
                                    sscanf(line1,"%s %s %s %d %d",comAp,idprenAp,portAp,&stato,&sockAp);
                                    char *tok = strtok(line1," ");
                                    if(strcmp(tok,com)!=0 && strcmp(idprenAp,id)==0){
                                        strcpy(com,tok);
                                        printf("%s",com);
                                        if(stato==0)   
                                            printf(" <in attesa>\n");
                                        if(stato==1)
                                            printf(" <in preparazione>\n");
                                        if(stato==2)
                                            printf(" <in servizio>\n");
                                    }
                                    if(strcmp(id,idprenAp)==0)
                                    {
                                    strcpy(com,tok);
                                    tok = strtok(NULL," ");
                                    strcpy(idpren,tok);
                                    tok = strtok(NULL," ");
                                    strcpy(portata,tok);
                                    tok = strtok(NULL," ");
                                    stato=*(tok) - 48;
                                    tok = strtok(NULL," ");

                                    printf("%s\n",portata);
                                    b=0;
                                    }
                                }
                            }
                        }
                        FILE *fp51 = fopen("PrenotazioniCodici.txt","r");
                        FILE *fp41 = fopen("Comande.txt","r");

                        /*caso in cui si passi il tavolo*/
                        while(fgets(line,sizeof(line),fp51))
                        {
                            sscanf(line,"%s %s",id,tav);
                            if(strcmp(tav,plus)==0 )
                            {/*apro il file Comande.txt e stampo ogni comanda col suo stato*/
                                char line1[100];
                                char com[10];
                                char comAp[10];
                                char idpren[10];
                                char idprenAp[10];
                                char portata[10];
                                char portAp[10];
                                int stato;
                                int sockAp;
                                while(fgets(line1,sizeof(line1),fp41))
                                {
                                    sscanf(line1,"%s %s %s %d %d",comAp,idprenAp,portAp,&stato,&sockAp);
                                    char *tok = strtok(line1," ");
                                    if(strcmp(tok,com)!=0 && strcmp(idprenAp,id)==0){
                                        strcpy(com,tok);
                                        printf("%s",com);
                                        if(stato==0)   
                                            printf(" <in attesa>\n");
                                        if(stato==1)
                                            printf(" <in preparazione>\n");
                                        if(stato==2)
                                            printf(" <in servizio>\n");
                                    }
                                    if(strcmp(idprenAp,id)==0){
                                    strcpy(com,tok);
                                    tok = strtok(NULL," ");
                                    strcpy(idpren,tok);
                                    tok = strtok(NULL," ");
                                    strcpy(portata,tok);
                                    tok = strtok(NULL," ");
                                    stato=*(tok) - 48;
                                    tok = strtok(NULL," ");

                                    printf("%s\n",portata);
                                    b=0;
                                    }
                                }
                            }
                        }
                    }else if(strcmp(command,"stop")==0 || strcmp(command,"stop\n")==0)
                    {
                        printf("spengo tutto\n");
                        FILE *fp7;
                        fp7 = fopen("dispConnessi.txt","r");
                        char line[100];
                        char bufStop[2];
                        strcpy(bufStop,"ce");
                        int sockid;
                        char tipo;
                        while(fgets(line,sizeof(line),fp7))
                        {
                            sscanf(line,"%d %c",&sockid,&tipo);
                            printf("chiudo il socket:%d\n",sockid);
                            int s_i=0;
                            for(s_i=0;s_i<nDisp;s_i++)
                            {
                                len = strlen(bufStop) + 1;
                                real_len=htons(len);
                                ret=send(d[s_i].sd,(void*)&real_len,sizeof(uint16_t),0);
                                ret=send(d[s_i].sd,(void*)bufStop,len,0);
                            }
                            close(sockid);
                        }fclose(fp7);
                        exit(1);
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
                    //BLOCCANTE
                    new_sd = accept(listener, (struct sockaddr *) &cl_addr, &len_p);
                    char buftype[2];
                    ret = recv(new_sd, (void*)buftype, 2, 0);
                    sscanf(buftype, "%c", &type);
                    if(type == 'T'){ 
                        
                        ret = recv(new_sd, (void*)&type_2, sizeof(uint16_t), 0);
                        check_2 = leggi_tipo_2(new_sd, &type_2, nDisp2);
                        if(check_2==0){//se non c'è nella lista lo carico su file
                            
                            d_2[nDisp2].sd=new_sd;
                            d_2[nDisp2].tipo[0]=type_2;
                            d_2[nDisp2].stato=0; //non ha ancora autenticato il codicePrenotazione
                            nDisp2++;
                            caricaTav(d_2,nDisp2);
                        }
                    }
                    check = leggi_tipo(new_sd, &type, nDisp);
                    printf("client:%d\n",new_sd);
                    if(check==0){//se non c'è nella lista lo carico su file
                        
                        d[nDisp].sd=new_sd;
                        d[nDisp].tipo=type;
                        nDisp++;
                        caricaDisp(d,nDisp);
                    }
                    FD_SET(new_sd, &master);
                    if(new_sd > fdmax){fdmax = new_sd;}
                }else
                {
                    check = leggi_tipo(i, &type, nDisp);
                    //PRIMA RICEZIONE C
                    if(type=='C')
                    {

                    //SECONDA RICEZIONE: TOKEN
                    ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                    bytes_needed = ntohs(real_len);
                    ret = recv(i,(void*)bufferCommand,bytes_needed,0);
                    if(ret == 0){
                        FILE *fp;
                        fp = fopen("dispConnessiTMP.txt","w");
                        printf("CHIUSURA rilevata client! %d\n",i);
                        fflush(stdout);
                        /*rimuovo il socket id dal dispConnessi*/
                        int index=0,indexArray = 0;
                        char bufferFile[BUFFER_SIZE];
                        disp dTMP[nDisp-1]; 
                        while(index<=nDisp)
                        {
                            if(d[index].sd != i){
                            memset(bufferFile, 0, sizeof(bufferFile));
                            dTMP[indexArray].sd = d[index].sd;
                            dTMP[indexArray].tipo = d[index].tipo;
                            if((d[index].tipo=='T' || d[index].tipo=='K' || d[index].tipo=='C') && d[index].sd!=0 ){
                            sprintf(bufferFile,"%d %c\n",d[index].sd,d[index].tipo);
                            fprintf(fp,bufferFile);
                            }
                            indexArray++;
                            }
                            index++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a;
                        for(a=0;a<nDisp;a++)
                        {
                            if((d[a].tipo=='T' || d[a].tipo=='K' || d[a].tipo=='C') && dTMP[a].sd!=0 ){
                            d[a].tipo = dTMP[a].tipo;
                            d[a].sd = dTMP[a].sd;
                            }
                        }
                        nDisp=a;
                        close(i);
                        fclose(fp);
                        FD_CLR(i, &master);
                        remove("dispConnessi.txt");
                        rename("dispConnessiTMP.txt","dispConnessi.txt");
                        break;
                    }      
                    if(strcmp("esc",bufferCommand)==0)
                    {
                        printf("arrivederci client!:%d\n",i);
                    }
                    
                    //TERZA RICEZIONE: DATI
                    ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                    bytes_needed = ntohs(real_len);
                    ret = recv(i,(void*)buffer,bytes_needed,0);

                    //FIND
                    if(strcmp("find",bufferCommand)==0)
                    {
                        char nome[20],dataP[20],nometav[6],sala[6],vicinanza[6];
                        int orario;
                        int b = 0;
                        char prenotabili[1024]; //buffer di concatenazione
                        sscanf(buffer, "%s %s %d %s %d" ,bufferCommand, p.cognome, &p.nPersone, p.data, &p.orario);


                        strcpy(Data,p.data); //Data è un array di appoggio per fare il confronto dopo
                        char* token = strtok(p.data, "-");
                        convStringa=atoi(token); //converte stringa intero
                        while (token != NULL) {
                        strcpy(stringa[b],token);
                        token = strtok(NULL, "-");
                        b++;
                        }
                        fp = fopen("Prenotazioni.txt", "r");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }/*ricavo i dati dal file prenotazione per escludere eventuali tavoli*/
                        char line[200];
                        //int indice;
                        while (fgets(line, sizeof(line), fp)) {
                            token = strtok(line, " "); 
                            int a=0;
                            while (token != NULL) {
                            if(a==0)
                                strcpy(nome,token);
                            if(a==1)
                                strcpy(dataP,token);
                            if(a==2)
                                orario=atoi(token);
                            //if(a==3)
                                //indice=atoi(token);
                            if(a==4)
                                strcpy(nometav,token);
                            if(a==5)
                                strcpy(sala,token);
                            if(a==6)
                                strcpy(vicinanza,token);
                            token = strtok(NULL, " "); 
                            a++;
                            }
                        if(strcmp(dataP,Data)==0)
                        {
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
                                fp = fopen("filePrenotazioni.txt", "r");

                        if (fp == NULL) {
                        printf("Errore nell'apertura del file");
                        exit(1);
                        }
                        int ii;int j;int k; int g;int count;
                        for(ii = 0; ii<WEEK_SIZE;ii++){
                        count = 0;             
                        if(convStringa%7 == sett[ii].numeroGiorno){
                            memset(buffer, 0, sizeof(buffer));
                            memset(prenotabili,0,sizeof(prenotabili));
                        for(j = 0; j<ROOM_SIZE; j++){
                            for(k = 0; k<TABLE_SIZE; k++){
                                if(sett[ii].sala[j].tav[k].nPosti >= p.nPersone){
                                    for(g=0; g<HOUR_SIZE;g++){
                                        if(sett[ii].sala[j].tav[k].status[g].orario == p.orario){
                                            if(sett[ii].sala[j].tav[k].status[g].stato == 0){
                                                
                                                count++;
                                                sprintf(buffer,"%d) %s %s %s\n",count, sett[ii].sala[j].tav[k].nome_tavolo, sett[ii].sala[j].nome_sala, sett[ii].sala[j].tav[k].vicinanza);
                                                strcat(prenotabili,buffer);
                                                strcpy(tabInviati[count].nome_sala,sett[ii].sala[j].nome_sala);
                                                strcpy(tabInviati[count].nome_tavolo,sett[ii].sala[j].tav[k].nome_tavolo);
                                                strcpy(tabInviati[count].vicinanza,sett[ii].sala[j].tav[k].vicinanza);
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
                    printf("invio dati!\n");
                    len = strlen(prenotabili) +1;
                    real_len=htons(len);
                    ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                    ret=send(i,(void*)prenotabili,len,0);
                    //BOOK  
                    }else if(strcmp("book",bufferCommand)==0)
                    {
                        FILE *ffp;
                        char bufferFile[BUFFER_SIZE];
                        char BufferOrarioPren[BUFFER_SIZE];
                        char NameBuffer[BUFFER_SIZE];
                        sscanf(buffer, "%s %d" ,bufferCommand, &prenIndex);

                        
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
                                if(strcmp(nTav,tabInviati[prenIndex].nome_tavolo)==0)
                                {
                                b = 1;
                                sprintf(BufferOrarioPren,"%s","NO");
                                }
                        }
                        fclose(ffp);
                        
                        /*ricevo il nome della prenotazione su cui fare la book*/
                        memset(NameBuffer,0,sizeof(NameBuffer));
                        ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(i,(void*)NameBuffer,bytes_needed,0);

                        if(b==0){
                        sprintf(bufferFile,"\n%s %s %d %d %s %s %s",NameBuffer,Data,p.orario,p.nPersone,tabInviati[prenIndex].nome_tavolo, tabInviati[prenIndex].nome_sala,tabInviati[prenIndex].vicinanza);
                        fprintf(fp,bufferFile);
                        sprintf(BufferOrarioPren,"%s%d%s",NameBuffer,p.nPersone,tabInviati[prenIndex].nome_tavolo);
                        printf("prenotazione effettuata! Invio codice in corso\n");
                        }
                        fclose(fp);


                        fp = fopen("PrenotazioniCodici.txt", "a");
                        if (fp == NULL) {
                        printf("Errore nell'apertura del file.\n");
                        exit(1);
                        }
                        //BufferOrarioPren contiene il codice prenotazione
                    
                        len = strlen(BufferOrarioPren)+1;
                        real_len = htons(len);
                        ret = send(i, (void*) &real_len, sizeof(uint16_t),0);
                        ret = send(i, (void*) BufferOrarioPren, len, 0);

                        if(b==0){
                            fprintf(fp,"%s %s\n",BufferOrarioPren,tabInviati[prenIndex].nome_tavolo);
                            fclose(fp);
                        }
                        }
                    }
                    if(type=='T')
                    {

                        /*
                          l'intero serve per evitare di mandare sempre il
                          codice di prenotazione
                        */
                        int ii;
                        for(ii = 0; ii < nDisp2; ii++)
                        {
                            if(d_2[ii].sd==i && d_2[ii].stato==0)
                            {
                                    //ricezione codice prenotazione, a seguito controllo correttezza
                                    char prenoCode[20];
                                    FILE *fp1;
                                    int aut = 0;
                                    while(aut == 0){
                                    ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                                    bytes_needed = ntohs(real_len);
                                    ret = recv(i,(void*)prenoCode,bytes_needed,0);
                                    prenoCode[bytes_needed-2]='\0';

                                    if(ret == 0){
                                    FILE *fp;
                        fp = fopen("tavConnessiTMP.txt","w");
                        printf("CHIUSURA rilevata tav! %d\n",i);
                        fflush(stdout);
                        /*rimuovo il socket id dal tavConnessi.txt*/
                        int index=0,indexArray = 0;
                        char bufferFile[BUFFER_SIZE];
                        dispTav dTMPtav[nDisp2-1]; 
                        while(index<=nDisp2)
                        {
                            if(d[index].sd != i){
                            memset(bufferFile, 0, sizeof(bufferFile));
                            dTMPtav[indexArray].sd = d_2[index].sd;
                            strcpy(dTMPtav[indexArray].tipo,d_2[index].tipo);
                            sprintf(bufferFile,"%d %c\n",d[index].sd,d[index].tipo);
                            fprintf(fp,bufferFile);
                            indexArray++;
                            }
                            index++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a;
                        for(a=0;a<nDisp2;a++)
                        {
                            strcpy(d_2[a].tipo,dTMPtav[a].tipo);
                            d_2[a].sd = dTMPtav[a].sd;
                        }
                        nDisp2--;
                        close(i);
                        fclose(fp);
                        FD_CLR(i, &master);
                        remove("tavConnessi.txt");
                        rename("tavConnessiTMP.txt","tavConnessi.txt");
                        
                        /*faccio lo stesso sui disp*/
                        
                        FILE *fp1;
                        fp1 = fopen("dispConnessiTMP.txt","w");
                        /*rimuovo il socket id dal dispConnessi*/
                        int index0=0,indexArray0 = 0;
                        char bufferFile0[BUFFER_SIZE];
                        disp dTMP[nDisp-1]; 
                        while(index0<=nDisp)
                        {
                            if(d[index0].sd != i){
                            memset(bufferFile0, 0, sizeof(bufferFile0));
                            dTMP[indexArray0].sd = d[index0].sd;
                            dTMP[indexArray0].tipo = d[index0].tipo;
                            if((d[index0].tipo=='T' || d[index0].tipo=='K' || d[index0].tipo=='C') && d[index0].sd!=0 ){
                            sprintf(bufferFile0,"%d %c\n",d[index0].sd,d[index0].tipo);
                            fprintf(fp,bufferFile0);
                            }
                            indexArray0++;
                            }
                            index0++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a0;
                        for(a0=0;a0<nDisp;a0++)
                        {
                            if((d[a0].tipo=='T' || d[a0].tipo=='K' || d[a0].tipo=='C') && dTMP[a0].sd!=0 ){
                            d[a0].tipo = dTMP[a0].tipo;
                            d[a0].sd = dTMP[a0].sd;
                            }
                        }
                        nDisp=a0;
                        fclose(fp1);
                        remove("dispConnessi.txt");
                        rename("dispConnessiTMP.txt","dispConnessi.txt");
                        break;
                    }
                                    printf("codice giunto:%s.\n",prenoCode);

                                    fp1 = fopen("PrenotazioniCodici.txt","r");
                                    if(fp1==NULL)
                                    {
                                        printf("errore nell'apertura del file\n");
                                        exit(1);
                                    }
                                    char linea[20];
                                    char ok = 'n';
                                     /*se non trovo la prenotzione, ok rimane a n e non si valida sul client*/
                                     /*andrà quindi reimmesso un nuovo codice prenotazione*/
                                    char code[8];
                                    char nTav[3];
                                    b=0;
                                    while (fgets(linea, sizeof(linea), fp1))
                                    {   
                                        sscanf(linea,"%s %s",code,nTav);
                                        code[strlen(code)]='\0';

                                        if(strcmp(code,prenoCode)==0){
                                            b = 1;
                                            ok = 's'; /*prenotazione trovata, salvo il valore di ritorno per il client*/
                                            aut = 1;
                                            strcpy(c.idPrenotazione,prenoCode);
                                            /*mi salvo il numero del tavolo relativo alla prenotazione e lo scrivo in tavConnessi*/
                                            int jj;
                                            for(jj=0; jj<nDisp2;jj++)
                                            {
                                                if(d_2[jj].sd==i){
                                                strcpy(d_2[jj].tipo,nTav);
                                                }
                                            }
                                            caricaTav(d_2,nDisp2);
                                            d_2[ii].stato=1; //registrazione fatta
                                            fclose(fp1);
                                            break;
                                        }
                                    }
                        
                                    /*invio la risposta*/
                                    send(i, &ok, sizeof(char), 0);
                                    if(b==0)
                                    {
                                        printf("prenotazione non trovata!\n");
                                    }
                                }
                                b=1;
                            }
                            else
                            {
                                if(d_2[ii].stato==1 && d_2[ii].sd==i){
                                ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                                bytes_needed = ntohs(real_len);
                                ret = recv(i,(void*)buffer,bytes_needed,0);

                                if(ret == 0){
                                printf("CHIUSURA table device rilevata!\n");

                                // rimuovo il descrittore newfd da quelli da monitorare
                            int index=0,indexArray = 0;
                        char bufferFile[BUFFER_SIZE];
                        disp dTMP[nDisp-1]; 
                        while(index<=nDisp)
                        {
                            if(d[index].sd != i){
                            memset(bufferFile, 0, sizeof(bufferFile));
                            dTMP[indexArray].sd = d[index].sd;
                            dTMP[indexArray].tipo = d[index].tipo;
                            sprintf(bufferFile,"%d %c\n",d[index].sd,d[index].tipo);
                            fprintf(fp,bufferFile);
                            indexArray++;
                            }
                            index++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a;
                        for(a=0;a<nDisp;a++)
                        {
                            d[a].tipo = dTMP[a].tipo;
                            d[a].sd = dTMP[a].sd;
                        }
                        nDisp--;
                                FD_CLR(i, &master);
                                remove("dispConnessi.txt");
                                rename("dispConnessiTMP.txt","dispConnessi.txt");

                        /*faccio lo stesso sui disp*/

                        int index0=0,indexArray0 = 0;
                        char bufferFile0[BUFFER_SIZE];
                        disp dTMP0[nDisp-1]; 
                        FILE *fp1 = fopen("dispConnessiTMP.txt","w");
                        while(index0<=nDisp)
                        {
                            if(d[index0].sd != i){
                            memset(bufferFile, 0, sizeof(bufferFile));
                            dTMP0[indexArray0].sd = d[index0].sd;
                            dTMP0[indexArray0].tipo = d[index0].tipo;
                            sprintf(bufferFile,"%d %c\n",d[index0].sd,d[index0].tipo);
                            fprintf(fp1,bufferFile0);
                            indexArray0++;
                            }
                            index0++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a0;
                        for(a0=0;a0<nDisp;a0++)
                        {
                            if((d[a].tipo=='T' || d[a].tipo=='K' || d[a].tipo=='C') && dTMP[a].sd!=0 ){
                            d[a0].tipo = dTMP0[a0].tipo;
                            d[a0].sd = dTMP0[a0].sd;
                            }
                        }
                        nDisp--;
                                FD_CLR(i, &master);
                                remove("dispConnessi.txt");
                                rename("dispConnessiTMP.txt","dispConnessi.txt");
                                fclose(fp1);
                                fflush(stdout);
                                close(i);
                                break;
                                }
                                bufferCommand[bytes_needed-1]='\0';
                                sscanf(buffer,"%s",bufferCommand);
                                }
                            }
                        }
                        
                        
                        if(ret == 0){
    
                        FILE *fp;
                        fp = fopen("tavConnessiTMP.txt","w");
                        printf("CHIUSURA rilevata tavolo! %d\n",i);

                        /*rimuovo il socket id dal tavConnessi.txt*/
                        int index=0,indexArray = 0;
                        char bufferFile[BUFFER_SIZE];
                        dispTav dTMPtav[nDisp2-1]; 
                        while(index<=nDisp2)
                        {
                            if(d[index].sd != i){
                            memset(bufferFile, 0, sizeof(bufferFile));
                            dTMPtav[indexArray].sd = d_2[index].sd;
                            strcpy(dTMPtav[indexArray].tipo,d_2[index].tipo);
                            if((d[index].tipo=='T' || d[index].tipo=='K' || d[index].tipo=='C') && d[index].sd!=0 ){
                            sprintf(bufferFile,"%d %c\n",d[index].sd,d[index].tipo);
                            fprintf(fp,bufferFile);
                            }
                            indexArray++;
                            }
                            index++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a;
                        for(a=0;a<nDisp2;a++)
                        {
                            strcpy(d_2[a].tipo,dTMPtav[a].tipo);
                            d_2[a].sd = dTMPtav[a].sd;
                        }
                        nDisp2--;
                        close(i);
                        fclose(fp);
                        remove("tavConnessi.txt");
                        rename("tavConnessiTMP.txt","tavConnessi.txt");
                                fflush(stdout);                        
                                FD_CLR(i, &master);
                        break;
                    }
                    if(ret < 0 )
                    {
                        printf("ERRORE ret<0\n");
                        fflush(stdout);
                        close(i);
                        FD_CLR(i, &master);
                        break;
                    }

                        if(strcmp(bufferCommand,"menu")==0){
                            FILE *fp;
                            fp = fopen("Menu2.txt","r");
                            if(fp == NULL)
                            {
                                printf("errore nell'apertura del file\n");
                                exit(1);
                            }
                            char bufConcatenazione[BUFFER_SIZE];
                            memset(bufConcatenazione,0,sizeof(bufConcatenazione));
                            char line[200];

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
                                        
                                        strcat(bufConcatenazione,buffer); 
                                        ii++;
                            }
                            len = strlen(bufConcatenazione)+1;
                            real_len = htons(len);
                            ret = send(i, (void*) &real_len, sizeof(uint16_t),0);
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
                            /*mando segnale del tavolo che ha inviato la comanda a tutti i kd*/
                            int sd_i;
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
                            char prenoCode[20];
                            /*innanzitutto verichiamo se il tavolo in questione può richiedere il conto
                            ossia se tutte le sue portate sono contrassegnate come in servizio*/
                            ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                            bytes_needed = ntohs(real_len);
                            ret = recv(i,(void*)prenoCode,bytes_needed,0);
                            prenoCode[strlen(prenoCode)-1]='\0';

                            int conteggio=0;
                            int Nportate=0;
                            int parziale=0;
                            int FallisciConto=0;
                            char line[100];
                            char code[20];
                            char comanda[BUFFER_SIZE];
                            char BufferTotComande[BUFFER_SIZE];
                            memset(BufferTotComande,0,sizeof(BufferTotComande));
                            char appoggio[BUFFER_SIZE];
                            int stato=0;
                            fp = fopen("Comande.txt","r");
                            while (fgets(line, sizeof(line), fp)) { 
                                sscanf(line,"%s %s %s %d",bufferCommand,code,comanda,&stato);
                                if(stato!=2)
                                {
                                    FallisciConto=1; //ci sono portate per questa prenotazione non servite: il conto fallisce
                                    break;
                                }
                                if(strcmp(code,prenoCode)==0)
                                {
                                    sprintf(appoggio,"\n%s",comanda);
                                    char *tok = strtok(comanda,"-");
                                    tok[2]='\0';
                                    int ii;
                                    for(ii = 0;ii<MENU_SIZE; ii++)
                                    {
                                        if(m[ii].codice[0]==tok[0] && m[ii].codice[1]==tok[1])
                                        {
                                            parziale =  m[ii].costo; /*prenod i parziali di costo*/
                                        }
                                    }
                                    tok = strtok(NULL,"-");
                                    Nportate= *(tok) -48;
                                    conteggio +=  parziale*Nportate;
                                    strcat(BufferTotComande,appoggio);
                                }       
                            }
                            if(FallisciConto==1)
                            {
                                strcpy(BufferTotComande,"no\0");
                            }
                            //mando le comande
                            strcpy(buffer,BufferTotComande);
                            len = strlen(buffer) +1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret =send(i,(void*)buffer,len,0);
                            fclose(fp);

                            //mando il conto
                            send(i, &conteggio, sizeof(int), 0);
                        }
                    }
                    if(type=='K')
                    {
                        //ricezione token
                        ret = recv(i,(void*)&real_len,sizeof(uint16_t),0);
                        bytes_needed = ntohs(real_len);
                        ret = recv(i,(void*)buffer,bytes_needed,0);

                        if(ret == 0){
                        printf("CHIUSURA kitchen rilevata!\n");
                        FILE *fp;
                        fp = fopen("dispConnessiTMP.txt","w");
                        printf("CHIUSURA client! %d\n",i);
                        /*rimuovo il socket id dal dispConnessi*/
                        int index=0,indexArray = 0;
                        char bufferFile[BUFFER_SIZE];
                        disp dTMP[nDisp-1]; 
                        while(index<=nDisp)
                        {
                            if(d[index].sd != i){
                            memset(bufferFile, 0, sizeof(bufferFile));
                            dTMP[indexArray].sd = d[index].sd;
                            dTMP[indexArray].tipo = d[index].tipo;
                            if((d[index].tipo=='T' || d[index].tipo=='K' || d[index].tipo=='C') && d[index].sd!=0 ){
                            sprintf(bufferFile,"%d %c\n",d[index].sd,d[index].tipo);
                            fprintf(fp,bufferFile);
                            }
                            indexArray++;
                            }
                            index++;
                        }/*rimosso dal file, faccio lo stesso dalla struct*/
                        int a;
                        for(a=0;a<nDisp;a++)
                        {
                            if((d[a].tipo=='T' || d[a].tipo=='K' || d[a].tipo=='C') && dTMP[a].sd!=0 ){
                            d[a].tipo = dTMP[a].tipo;
                            d[a].sd = dTMP[a].sd;
                            }
                        }
                        nDisp=a;
                        close(i);
                        fclose(fp);
                        FD_CLR(i, &master);
                        fflush(stdout);
                        remove("dispConnessi.txt");
                        rename("dispConnessiTMP.txt","dispConnessi.txt");
                        break;
                        
                        
                        }
                        if(ret < 0){
                        perror("Errore in fase di ricezione: \n");
                        close(i);
                        // rimuovo il descrittore newfd da quelli da monitorare
                        FD_CLR(i, &master);
                        }
                        sscanf(buffer,"%s",bufferCommand);
                        bufferCommand[bytes_needed-1]='\0';
                        if(strcmp(bufferCommand,"take")==0)
                        {
                            char ComBuffer[BUFFER_SIZE];
                            memset(ComBuffer,0,sizeof(ComBuffer));
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
                            int NoTake=0;
                            char line1[100];
                            char idPren[20];
                            char tav[20];
                            int sock = 0;
                            while (fgets(line, sizeof(line), fp)) { /*Comande.txt*/
                                    sscanf(line,"%s %s %s %d %d",comanda,Prenotazione,ordine,&stato,&sock);
                                    if(stato==0 && b == 0) //salvo nome comanda utile
                                    {
                                        NoTake=1;
                                        b = 1;
                                        /*ricavo il tavolo prenotato con Prenotazione*/
                                        while(fgets(line1,sizeof(line1),fp2))
                                        {
                                            sscanf(line1,"%s %s",idPren,tav);
                                            if(strcmp(idPren,Prenotazione)==0)
                                            {
                                                break;
                                            }
                                        }/*mando il codice comanda*/
                                        strcpy(comandaContr,comanda);
                                        
                                        /*dico al tavolo designato che la sua comanda è in preparazione*/

                                        int sdt_i;
                                        char notifyBuffer[BUFFER_SIZE];
                                        for(sdt_i=0;sdt_i<nDisp2;sdt_i++)
                                        {   
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
                                    }
                                    if(strcmp(comanda,comandaContr)==0 && stato==0){
                                        stato = 1;
                                        sprintf(appoggio,"%s\n",ordine);
                                        strcat(ComBuffer,appoggio);
                                        fprintf(fp1, "%s %s %s %d %d\n", comanda,Prenotazione,ordine,stato,i);

                                    }//la i infondo indica il socket che ha preso le comande
                                    else{
                                    fprintf(fp1, "%s %s %s %d %d\n", comanda,Prenotazione,ordine,stato,sock);
                                    }
                                    stato=0;
                                    
                            }
                            if(NoTake==0)
                            {
                                strcpy(comandaContr,"no");
                            }
                            len = strlen(comandaContr) + 1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret=send(i,(void*)comandaContr,len,0);

                            if(NoTake==1){
                            /*mando id del tavolo*/
                            len = strlen(tav) + 1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret=send(i,(void*)tav,len,0);
                            strcpy(comanda,comandaContr);
                            /*poi mando gli ordini della comanda al kd*/
                            len = strlen(ComBuffer) + 1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret=send(i,(void*)ComBuffer,len,0);
                            printf("comanda mandata: \n%s\n",ComBuffer);
                            }

                            /*mi appoggio ad un file temporaneo per modificare lo stato delle comande interessate*/
                            fclose(fp);
                            fclose(fp1);
                            fclose(fp2);
                            remove("Comande.txt");
                            rename("Comande2.txt","Comande.txt");

                        }else if(strcmp(bufferCommand,"show")==0)
                        {
                            //char bufferShow[BUFFER_SIZE];
                            FILE *fp,*fp1;
                            char line[100],line1[100];
                            char comanda[100];
                            char Prenotazione[100];
                            char comandaContr[100];
                            char appoggio[BUFFER_SIZE],appoggio2[BUFFER_SIZE];
                            char ComBuffer[BUFFER_SIZE];
                            memset(ComBuffer,0,sizeof(ComBuffer));
                            char ordine[100];
                            int stato=0;
                            //int b = 0;
                            int currentSd=0;
                            int Prendi[100];int prendiIndex=0;
                            fp = fopen("Comande.txt","r");
                            fp1 = fopen("Comande.txt","r");
                            while (fgets(line, sizeof(line), fp)) { 
                                sscanf(line,"%s %s %s %d %d",comanda,Prenotazione,ordine,&stato,&currentSd);
                                if(stato==1 && strcmp(comanda,comandaContr)!=0)
                                {
                                    if(currentSd==i){
                                    char table[10];
                                    strcpy(comandaContr,comanda);
                                    getTable(Prenotazione,table);
                                    sprintf(appoggio2,"%s %s\n",comanda,table);  
                                    strcat(ComBuffer,appoggio2);
                                    } 
                                }stato=0;b=1;
                                /*if(currentSd!=i)
                                    break;*/
                                while (fgets(line1, sizeof(line1), fp1)) { 
                                     prendiIndex++;
                                    sscanf(line1,"%s %s %s %d %d",comanda,Prenotazione,ordine,&stato,&currentSd);
                                    if(strcmp(comandaContr,comanda)==0 && currentSd==i && Prendi[prendiIndex]!=1)
                                    {
                                        Prendi[prendiIndex]=1;
                                        sprintf(appoggio,"%s\n",ordine);
                                        strcat(ComBuffer,appoggio);
                                    }
                                }prendiIndex=0;
                                rewind(fp1);
                            }
                                len = strlen(ComBuffer) + 1;
                                real_len=htons(len);
                                ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                                ret=send(i,(void*)ComBuffer,len,0);

                                printf("comanda mandata: \n%s\n",ComBuffer);

                            fclose(fp1);
                            fclose(fp);

                        }else if(strcmp(bufferCommand,"ready")==0)
                        { //mette a 2 lo stato della comanda designata e notifico
                            char idComanda[10];
                            char nomeTav[10];
                            FILE *fp1;
                            buffer[bytes_needed-1]='\0';

                            char *t = strtok(buffer," ");
                            t = strtok(NULL,"-");
                            strcpy(idComanda,t);
                            printf("%s\n",idComanda);

                            t = strtok(NULL," ");
                            strcpy(nomeTav,t);
                            nomeTav[strlen(nomeTav)-1]='\0';
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
                            char bufferReady[100];
                            int SettOK=0;
                            while(fgets(line,sizeof(line),fp1))
                            {
                                sscanf(line,"%s %s %s %d %d",comanda,idPren,piet,&stato,&sock);
                                if(strcmp(comanda,idComanda)==0)
                                {
                                    if(stato!=1)
                                    {
                                        sprintf(bufferReady,"%s\n","la comanda non è in preparazione\n");
                                        printf("no!\n");
                                        SettOK=1;
                                        fprintf(fp,"%s %s %s %d %d\n",comanda,idPren,piet,stato,sock);
                                    }else
                                        fprintf(fp,"%s %s %s %d %d\n",comanda,idPren,piet,2,sock);
                                }else
                                    fprintf(fp,"%s %s %s %d %d\n",comanda,idPren,piet,stato,sock);
                                
                            } /*prendo il numero del socket che aveva la comanda definita e dirgli che è in servizio*/
                            /*ricavo il tavolo prenotato con Prenotazione*/
                            if(SettOK != 1){
                            int sdt_i;
                            char notifyBuffer[BUFFER_SIZE];
                            for(sdt_i=0;sdt_i<nDisp2;sdt_i++)
                            {   
                                if(strcmp(d_2[sdt_i].tipo,nomeTav)==0) /*tavolo trovato, mando la notifica al socket*/
                                {
                                    printf("mando la notifica al tavolo del socket:%d\n",d_2[sdt_i].sd);
                                    sprintf(notifyBuffer,"la comanda %s è in servizio\n",idComanda);
                                    len = strlen(notifyBuffer) + 1;
                                    real_len=htons(len);
                                    ret=send(d_2[sdt_i].sd,(void*)&real_len,sizeof(uint16_t),0);
                                    ret=send(d_2[sdt_i].sd,(void*)notifyBuffer,len,0);
                                }
                            }
                            /*come sopra, mi appoggio ad un file temporaneo per modificare lo stato delle comande interessate*/
                            strcpy(bufferReady,"COMANDA IN SERVIZIO\n");
                            }
                            len = strlen(bufferReady) + 1;
                            real_len=htons(len);
                            ret=send(i,(void*)&real_len,sizeof(uint16_t),0);
                            ret=send(i,(void*)bufferReady,len,0);
                            
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
    

