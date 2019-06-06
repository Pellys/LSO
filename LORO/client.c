/* tcpclient.c */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#define CYANO   "\x1B[1;36m"
#define YELLOW  "\x1B[1;33m"
#define RED     "\x1b[1;31m"
#define GREEN   "\x1b[1;32m"
#define RESET   "\x1b[0m"
#define BLU      "\x1B[1;34m"
#define tempoTot 120

void stampa(char *messaggio);
void menu();
void cleanBuffer();
void error(char *messaggio);
void stampaMappa(char A[][20],int,int,int);
void gioco(int sock);
void send_socket(int conn,char *mess,int nbytes);
void sighandler(int segnale_ricevuto);
void sighandler_1(int segnale_ricevuto);
void sighandler_2(int segnale_ricevuto);
void sighandler_3(int segnale_ricevuto);
void sighandler_4(int segnale_ricevuto);
void stampaVittoria();
void stampaSconfitta();
void pauseT();

int giocato=0;
int ingame=0;
int connessione_flag=0;
int flag_posizione1=0;
int flag_posizione2=0;

int main(int argc, char const *argv[])
{
    
    int bytes_recieved, uscita=0,sock;
    char Login[1024],recv_data[1024], risposta;
    struct hostent *host;
    struct sockaddr_in server_addr;
    int i=0, j=0, bytesnickname=0, bytepassword=0, inserito=0;
    char scelta;
    char Nickname[256], Password[256], Log[512];
    
    if(argc!=3){
        error(RED"Error : Man = ./client ip(xxx.xxx.xxx) port(5XXX)\n"RESET);
    }
    
    host = gethostbyname(argv[1]);
    
    if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
        error(RED"ERROR : Socket\n"RESET);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8);
    
    if(connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        error(RED"ERROR : Connect\n"RESET);
    }
    connessione_flag=sock;
    system("clear");
    
    do{
        if(giocato==1){
            sleep(1);
            sprintf(Login,"b%s:%s\n",Nickname,Password);
            if(write(sock,Login,strlen(Login))<0){
                error("Errore WRITE\n");
            }
            
            if(read(sock,&risposta,1)<0){
                error("Error Read\n");
            }
            ingame=1;
            gioco(sock);
        }else{
            
            scelta=0;
            menu();
            
            signal(SIGINT,sighandler_1);
            signal(SIGQUIT,sighandler_1);
            signal(SIGHUP,sighandler_2);
            signal(SIGSTOP,sighandler_2);
            signal(SIGTERM,sighandler_2);
            signal(SIGABRT,sighandler_2);
            signal(SIGTSTP,sighandler_2);
            signal(SIGPIPE,sighandler_4);
            
            if(read(1,&scelta,1)<0){
                error("error write\n");
            }
            if(scelta=='a'){
                cleanBuffer();
                
                stampa(BLU"Inserire Nickname e Password\n"RESET);
                stampa("Nickname : ");
                
                if((bytesnickname=read(1,Nickname,256))<0){
                    error("Errore write\n");
                }
                Nickname[bytesnickname-1]='\0';
                
                stampa("\nPassword : ");
                if((bytepassword=read(1,Password,256))<0){
                    error("Errore write\n");
                }
                Password[bytepassword-1]='\0';
                
                sprintf(Login,"a%s:%s\n",Nickname,Password);
                
                if(write(sock,Login,strlen(Login))<0){
                    error("Errore WRITE\n");
                }
                
                if(read(sock,&risposta,1)<0){
                    error("Error Read\n");
                }
                switch(risposta){
                    case 'y': stampa(GREEN"Registrazione avvenuta con Successo, verrai indirizzato alla schermata iniziale\n"RESET);pauseT(); system("clear");break;
                    case 'n': stampa(RED"Il Nickname è gia stato usato, verrai indirizzato alla schermata iniziale\n"RESET); pauseT(); system("clear"); break;
                    default : error("Impossibile comunicare con il server\n"); break;
                }
                
            }
            else if(scelta=='b'){
                cleanBuffer();
                
                stampa(BLU"Inserire Nickname e Password Per Il Login\n"RESET);
                stampa("Nickname : ");
                
                if((bytesnickname=read(1,Nickname,256))<0){
                    error("Errore write\n");
                }
                Nickname[bytesnickname-1]='\0';
                
                stampa("\nPassword : ");
                if((bytepassword=read(1,Password,256))<0){
                    error("Errore write\n");
                }
                Password[bytepassword-1]='\0';
                
                sprintf(Login,"b%s:%s\n",Nickname,Password);
                if(write(sock,Login,strlen(Login))<0){
                    error("Errore WRITE\n");
                }
                
                if(read(sock,&risposta,1)<0){
                    error("Error Read\n");
                }
                printf("Ho ricevuto la risposta dal server\n");
                
                switch(risposta){
                    case 'y':system("clear"); stampa(GREEN"Adesso puoi iniziare a giocare!\n"RESET);
                        ingame=1;
                        gioco(sock);
                        break;
                        
                    case 'n': stampa(RED"Login fallito utente non registrato\n"RESET); pauseT(); system("clear");
                        break;
                        
                    case 'g': stampa(RED"Login fallito utente già Online\n"RESET); pauseT(); system("clear");
                        break;
                        
                    default : stampa("Impossibile comunicare con il server\n");
                        break;
                }
            }
            else if(scelta=='c'){
                stampa(YELLOW"Chiusura Del Client In Corso\n"RESET);
                sleep(1);
                
                if(write(sock,"c",1)<0){//comunica al server che è stata inserita una c
                    error("error write\n");
                }
                uscita=1;
            }
            else{
                system("clear");
                cleanBuffer();
                stampa(RED"Error input riprova\n"RESET);
            }
        }
    }while(!uscita);
    
    
    close(sock);
    cleanBuffer();
    return 0;
}

void menu()
{
    system("clear");
    stampa(GREEN);
    char mess[40];
    char mess1[40];
    char mess2[40];
    char mess3[40];
    char mess4[40];
    char simb=92;
    
    sprintf(mess,"| |     / ____|/ __ %c\n",simb);
    sprintf(mess1,"| |    | (___ | |  | |\n",simb);
    sprintf(mess2,"| |     %c___ %c| |  | |\n",simb,simb);
    sprintf(mess3,"| |____ ____) | |__| |\n");
    sprintf(mess4,"|______|_____/ %c____/",simb);
    
    stampa(" _       _____  ____\n");
    stampa(mess);
    stampa(mess1);
    stampa(mess2);
    stampa(mess3);
    stampa(mess4);
    stampa("  PROGETTO ANNO 2016/2017\n"RESET);
    stampa(BLU"\nNICOLA LECCISI N86001759\nERNESTO STARITA N86001627\n\n"RESET);
    stampa("************************\n");
    stampa("*   A) New Account     *\n");
    stampa("*   B) Login           *\n");
    stampa("*   C) Exit            *\n");
    stampa("************************\n");
    
    
    
}

void stampa(char *messaggio)
{
    int l=strlen(messaggio);
    if(write(0,messaggio,l)<0){
        error("Error write\n");
    }
}
void cleanBuffer()
{
    char ch;
    while((ch=getchar())!='\n');
}
void error(char *messaggio)
{
    perror(messaggio);
    exit(1);
}
void gioco(int sock)
{
    int i=0, j=0;
    int k=0, vittoria=0;
    char coordinate[3], cambia_posizione[3], y=0;
    char mappa[20][20], lista[5000], prova;
    char messaggio[256];
    int id_corrente, id_server;
    
    
    for(i=0;i<20;i++){
        for(j=0; j<20; j++){
            mappa[i][j]='-';
        }
    }
    
    //leggere qui il primo id
    if(read(sock,&id_corrente,sizeof(int))<0){
        error("errore read\n");
    }
    //fine lettura primo id
    
    if(read(sock,coordinate,3)<0){
        error("errore read\n");
    }
    
    i=coordinate[0];
    j=coordinate[1];
    mappa[i][j]=coordinate[2];
    
    do{
        system("clear");
        stampaMappa(mappa,i,j,id_corrente);
        
        signal(SIGINT,sighandler);
        signal(SIGQUIT,sighandler);
        signal(SIGHUP,sighandler_3);
        signal(SIGSTOP,sighandler_3);
        signal(SIGTERM,sighandler_3);
        signal(SIGTSTP,sighandler_3);
        signal(SIGPIPE,sighandler_4);
        signal(SIGABRT,sighandler_3);
        
        flag_posizione1=i;
        flag_posizione2=j;
        
        if(read(1,&cambia_posizione[0],1)<=0){
            error("error read\n");
        }
        cleanBuffer();
        int z=0;
        for(z=0; z<256; z++){
            messaggio[z]='\0';
        }
        
        switch(cambia_posizione[0]){
                
                
            case 'w': cambia_posizione[1]=i;cambia_posizione[2]=j;
                send_socket(sock,cambia_posizione,3);
                
                //write id del client verso il server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,coordinate,3)<0){//legge la risposta del server
                    error(RED"Error Read\n"RESET);
                    ingame=0; giocato=0;
                }
                if(coordinate[0]=='f'){
                    system("clear");
                    ingame=0;
                    giocato=1;
                }
                else{
                    if(coordinate[2]=='#'){
                        stampaSconfitta();
                        sleep(1);
                        ingame=0;
                    }
                    else if(coordinate[2]=='O'){
                        i=coordinate[0]; j=coordinate[1];
                    }
                    
                    else{
                         mappa[i][j]='-';
                        i=coordinate[0]; j=coordinate[1];
                        mappa[i][j]='O';
                        
                    }
                }
                break;
                
            case 's': cambia_posizione[1]=i;cambia_posizione[2]=j;
                send_socket(sock,cambia_posizione,3);
                
                //invio dell id al server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,coordinate,3)<0){
                    error(RED"Error Read\n"RESET);
                    ingame=0; giocato=0;
                }
                
                if(coordinate[0]=='f'){
                    system("clear");
                    ingame=0;
                    giocato=1;
                }
                else{
                    if(coordinate[2]=='#'){
                        stampaSconfitta();
                        sleep(1);
                        ingame=0;
                    }
                    else if(coordinate[2]=='O'){
                        i=coordinate[0]; j=coordinate[1];
                    }
                    
                    else{
                         mappa[i][j]='-';
                        i=coordinate[0]; j=coordinate[1];
                        mappa[i][j]='O';
                    }
                }
                break;
                
            case 'a': cambia_posizione[1]=i;cambia_posizione[2]=j;
                send_socket(sock,cambia_posizione,3);
                
                //invio dell id al server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,coordinate,3)<0){
                    error(RED"Error Read\n"RESET);
                    ingame=0; giocato=0;
                }
                
                if(coordinate[0]=='f'){
                    system("clear");
                    ingame=0;
                    giocato=1;
                }
                else{
                    if(coordinate[2]=='#'){
                        stampaSconfitta();
                        sleep(1);
                        ingame=0;
                    }
                    else if(coordinate[2]=='O'){
                        i=coordinate[0]; j=coordinate[1];
                    }
                    
                    else{
                         mappa[i][j]='-';
                        i=coordinate[0]; j=coordinate[1];
                        mappa[i][j]='O';
                    }
                }
                break;
                
            case 'd': cambia_posizione[1]=i;cambia_posizione[2]=j;
                send_socket(sock,cambia_posizione,3);
                
                //invio dell id al server
                if(write(sock,&id_corrente,sizeof(int))<0){
                    error("Errore write");
                }
                
                if(read(sock,coordinate,3)<0){
                    error(RED"Error Read\n"RESET);
                    ingame=0; giocato=0;
                }
                
                if(coordinate[0]=='v'){
                    stampaVittoria();
                    sleep(1);
                    ingame=0;
                    vittoria=1;
                    giocato=1;
                }else if(coordinate[0]=='f'){
                    system("clear");
                    ingame=0;
                    giocato=1;
                }
                else{
                    if(coordinate[2]=='#'){
                        stampaSconfitta();
                        sleep(1);
                        ingame=0;
                    }
                    else if(coordinate[2]=='O'){
                        i=coordinate[0]; j=coordinate[1];
                    }
                    
                    else{
                         mappa[i][j]='-';
                        i=coordinate[0]; j=coordinate[1];
                        mappa[i][j]='O';
                    }
                }
                break;
                
            case 'c': stampa(YELLOW"Stai per uscire dal gioco e tornare al menu iniziale\n"RESET);
                cambia_posizione[1]=i;cambia_posizione[2]=j;
                send_socket(sock,cambia_posizione,3);
                sleep(1); ingame=0; giocato=0;
                break;
                
            case 'i': system("clear");
                for(k=0;k<5000;k++){
                    lista[k]='\0';
                }
                send_socket(sock,"i",1);
                if(read(sock,lista,5000)<0){
                    error("error read\n");
                }
                stampa(lista);
                pauseT();
                break;
                
            case 't': system("clear");
                int tempo=0;
                char tempo_loc[256];
                send_socket(sock,"t",1);
                if(read(sock,&tempo,sizeof(int))<0){
                    error("error read\n");
                }
                int tempoRimanente=tempoTot-tempo;
                sprintf(tempo_loc,GREEN"Tempo Trascorso :%d(sec)"RESET RED"\n\nTempo Rimanente :%d(sec)\n"RESET,tempo,tempoRimanente);
                stampa(tempo_loc);
                pauseT();
                break;
                
            default :stampa(RED"Inserimento Non Valido\n"RESET); cleanBuffer(); break;
        }
    }while(ingame);
    
    sleep(1);
    pauseT();
}
void stampaMappa(char mappa[][20],int pos1, int pos2,int id_corrente)
{
    int i=0, j=0;
    char messaggio[256];
    char flag[256];
    
    write(0,"\n",sizeof("\n"));
    write(0,"\t     0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 \n",sizeof("\t     0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 \n"));
    write(0,"\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n",sizeof("\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"));
    for(i=0; i<20; i++){
        if(i<10){
            sprintf(flag,"\t %d)| ",i);
        }else{
            sprintf(flag,"\t%d)| ",i);
        }
        stampa(flag);
        for(j=0; j<20; j++){
            if(mappa[i][j]=='O'){
                stampa(BLU"0"RESET);
            }else if(mappa[i][j]=='#'){
                stampa(RED"#"RESET);
            }else{
                write(0,&mappa[i][j],1);
            }
            if(j==19){
                write(0," ",sizeof(" "));
            }else{
                write(0,"  ",sizeof("  "));
            }
        }
        if(i==13){
            write(0,"|\tw) Move Up\n",sizeof("|\tw) Move Up\n"));
        }else if(i==14){
            write(0,"|\ta) Move Left\n",sizeof("|\ta) Move Left\n"));
        }else if(i==15){
            write(0,"|\ts) Move Down\n",sizeof("|\ts) Move Down\n"));
        }else if(i==16){
            write(0,"|\td) Move Right\n",sizeof("|\tw) Move Right\n"));
        }else if(i==17){
            write(0,"|\ti) Users Info\n",sizeof("|\ti) Users Info\n"));
        }else if(i==18){
            write(0,"|\tt) Time\n",sizeof("|\tt) Time\n"));
        }else if(i==19){
            write(0,"|\tc) Exit\n",sizeof("|\tc) Exit\n"));
        }else{
            write(0,"|\n",sizeof("|\n"));
        }
    }
    write(0,"\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n",sizeof("\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"));
    sprintf(messaggio,YELLOW"Ti trovi in Posizione [%d][%d] (Stai partecipando alla partita ID: [%d])\n"RESET,pos1,pos2,id_corrente);
    stampa(messaggio);
}
void send_socket(int conn,char *mess,int nbytes)
{
    if(write(conn,mess,nbytes)<0){
        error("Errore write\n");
    }
}
void sighandler(int segnale_ricevuto)    /* gestione del segnale */
{
    char flag[3];
    flag[0]='c';
    flag[1]=flag_posizione1;
    flag[2]=flag_posizione2;
    
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    
    write(connessione_flag,flag,sizeof(flag));
    giocato=0;
    ingame=0;
    sleep(1);
    write(connessione_flag,"l",sizeof(char));
    exit(0);
}

void sighandler_1(int segnale_ricevuto)
{
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    write(connessione_flag,"l",1);
    giocato=0;
    ingame=0;
    sleep(1);
    exit(0);
}

void sighandler_2(int segnale_ricevuto)
{
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    write(connessione_flag,"l",1);
    giocato=0;
    ingame=0;
    sleep(1);
    exit(0);
    
}

void sighandler_3(int segnale_ricevuto)
{
    stampa(RED"\nUscita Forzata Catturata\n"RESET);
    char flag[3];
    flag[0]='c';
    flag[1]=flag_posizione1;
    flag[2]=flag_posizione2;
    
    write(connessione_flag,flag,sizeof(flag));
    giocato=0;
    ingame=0;
    sleep(1);
    write(connessione_flag,"l",sizeof(char));
    sleep(1);
    exit(0);
}

void sighandler_4(int segnale_ricevuto){
    stampa(RED"Il server è andato OFFLINE\n"RESET);
    close(connessione_flag);
    exit(0);
}

void stampaVittoria(){
    int i=0;
    system("clear");
    while(i<2){
        signal(SIGINT,sighandler);
        signal(SIGQUIT,sighandler);
        signal(SIGHUP,sighandler_3);
        signal(SIGSTOP,sighandler_3);
        signal(SIGTERM,sighandler_3);
        signal(SIGPIPE,sighandler_4);
        signal(SIGTSTP,sighandler_3);
        
        stampa(GREEN"*****************************\n");
        stampa("*                           *\n");
        stampa("* Complimenti Hai Vinto !!! *\n");
        stampa("*                           *\n");
        stampa("*****************************\n"RESET);
        sleep(1);
        system("clear");
        sleep(1);
        i++;
    }
}



void stampaSconfitta(){
    int i=0;
    system("clear");
    while(i<2){
        signal(SIGINT,sighandler);
        signal(SIGQUIT,sighandler);
        signal(SIGHUP,sighandler_3);
        signal(SIGSTOP,sighandler_3);
        signal(SIGTERM,sighandler_3);
        signal(SIGPIPE,sighandler_4);
        signal(SIGTSTP,sighandler_3);
        
        stampa(GREEN"*****************************\n");
        stampa("*                           *\n");
        stampa("* bjhenfgysefzkuyfgnekruxgukguk *\n");
        stampa("*                           *\n");
        stampa("*****************************\n"RESET);
        sleep(1);
        system("clear");
        sleep(1);
        i++;
    }
}






void pauseT(){
    stampa("\nPremere Invio Per Continuare...\n");
    cleanBuffer();
}
