/* server.c */

//librerie
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h> 
#include <pthread.h>
#include <time.h>
#include <signal.h>

//colori
#define CYANO   "\x1B[1;36m"
#define YELLOW  "\x1B[1;33m"
#define RED     "\x1b[1;31m"
#define GREEN   "\x1b[1;32m"
#define RESET   "\x1b[0m"
#define BLU  	"\x1B[1;34m"

//dichiarazione del tempo massimo iniziale
#define TEMPO 120

//dichiarazioni delle strutture dati utilizzate
struct utente{
	char nickname[512];
	pid_t pid;
	int x;
	int y;
	struct utente *next;
};

//puntatori al primo elemento delle strutture dati
struct utente *Ut=NULL;

//funzioni Thread
void *Menu(void *ptr);
void *GeneraMapp(void *ptr);

//funzioni generiche int(booleane)
int controlloLogin(char *,int);
int controlloNome(char *,int);
int trovaOnline(char * ,struct utente *);

//funzioni generiche void
void errore(char *);
void stampaLog(int ,char *);
void DataAttuale();
void prendiNome(char *flag,char *);
void sendSocket(int ,char *,int );
void gioco(int conn,char *,struct utente*);
void stampaMappa(int fd,char mappa[][20]);
void handlerpipe(int x);
void handlerCtrlC(int x);

//funzioni per la lista di utenti connessi
struct utente *EliminaScollegato(struct utente *,char *);
void stampaUtentiLoggati(struct utente *);
void stringaUtentiLoggati(struct utente *,char*);
void deallocaUtente(struct utente *);
void aggiungiUtente(struct utente **top,char *,pid_t);
void aggiungiCoordinate(struct utente *top,char *Nick,int i,int j);


/* Dichiarazioni Globali */
pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;

char mappa[20][20];
int fdUtente;
int fdLog;
int utentiConnessi=0;
int isMapp=0;
int tempoGioco=0;
int utentiInGioco=0;
int idPartita=0;
int flagVittoria=0;


//inizio main
int main(int argc, char const *argv[]){
        
    int sock;            //socket
	int connected;		//risultato accept
	int bytes_recieved=0;
	char riceviDati[1024];
    struct sockaddr_in server_addr,client_addr;
    int sin_size,i=0,j=0;

   	 //apre file (database per login)
    fdUtente=open("utenti.txt", O_RDWR | O_CREAT |O_APPEND ,S_IRWXU);
    fdLog=open("log.txt",O_WRONLY | O_CREAT | O_APPEND,S_IRWXU);
    
    pthread_t tred;
    pthread_t tredServer;
    char data[256];

    signal(SIGINT,handlerCtrlC);
    signal(SIGQUIT,SIG_IGN);
    signal(SIGHUP,SIG_IGN);
    signal(SIGSTOP,SIG_IGN);
    signal(SIGTERM,SIG_IGN);
    signal(SIGABRT,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGPIPE,handlerpipe);
    
    //erori open file degli utenti e log
    if(fdUtente<0 || fdLog<0){
        errore("Errore Permessi FILE\n");
    }

    // ERRORE IN CASO DI MANCATO INPUT ALL'AVVIO
    if(argc!=2){
        errore("Errore Inserire La Porta : Man D'Uso  ./server 5xxx\n");
    }
    
    
    //CREO SOCKET
    sock = socket(AF_INET, SOCK_STREAM, 0);
    //VEDO SE ESISTE
    if (sock == -1) {
        errore("Socket\n");
    }
    
    server_addr.sin_family = AF_INET;         	//host+porta (formato)
    server_addr.sin_port = htons(atoi(argv[1]));    //porta
    server_addr.sin_addr.s_addr = INADDR_ANY; 	//indirizzo

	//bind assegna indirizzo di socket locale alla remota
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        errore("Unable to bind\n");
    }

	//grandezza indirizzo socket
    sin_size = sizeof(struct sockaddr_in);

	//listen (socket , lunghezza coda di connessioni) 
    if (listen(sock, 5) == -1) {
        errore("Listen\n");
    }
    
    DataAttuale(data);
    
    //salva informazioni nel log con info+data
    bytes_recieved=sprintf(riceviDati,"The server is up on %s port Date: %s\n",argv[1],data);
    stampaLog(fdLog,riceviDati);
		
    signal(SIGPIPE,handlerpipe);

    while(1){

        if((connected=accept(sock, (struct sockaddr *)&client_addr,&sin_size))==-1){
            errore("Connessione refused\n");
        }
        utentiConnessi++;
        
        if((pthread_create(&tred,NULL,Menu,(void*)&connected)) < 0){
            errore("pthread_create() failed\n");
        }
        
        pthread_detach(tred);
        sprintf(riceviDati,"Nuova Connessione Da : (%s,%d)\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
        stampaLog(fdLog,riceviDati);
        }       

    close(fdUtente);
    close(sock);
    close(fdLog);
    deallocaUtente(Ut);
    return 0;
} 
// fine main


//______________________________________________________________
//funzione principale

void *Menu(void *ptr){
    
	char Addnickname[512],messaggio[512],scelta[1024],data[256];
	int  flag=1;
	int *connect=(int*)ptr;
	int conn=*connect;
	int bNick=0, bPassw=0;
	int fdUtente_loc;
	struct utente *tmp=NULL;
	int fdLogLoc, GiaOnline=0;
	int i=0;
	pthread_t tredServer;

	signal(SIGPIPE,handlerpipe);
	pthread_mutex_lock(&mymutex);//blocca il mutex
	fdUtente_loc=fdUtente;
	fdLogLoc=fdLog;
	pthread_mutex_unlock(&mymutex);//sblocca il mutex

	signal(SIGINT,handlerCtrlC);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGSTOP,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGABRT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGPIPE,handlerpipe);
	
    do{
		for(i=0; i<1024; i++){
			scelta[i]='\0';
		}
		if(bNick=read(conn,scelta,1024)<0){
			errore("Errore read\n");
		}
		
        if(scelta[0]=='a'){
			
			scelta[bNick]='\0';
			if(controlloNome(scelta+1,fdUtente_loc)){
				stampaLog(fdUtente_loc,scelta+1);
				DataAttuale(data);
				sprintf(messaggio,"Nuovo utente registrato con %s in data : %s\n",scelta+1,data);
				stampaLog(fdLogLoc,messaggio);
				sendSocket(conn,"y",1);
			}else{
				DataAttuale(data);
				sprintf(messaggio,"Impossibile registrare l'utente con %s in data : %s\n",scelta+1,data);
				stampaLog(fdLogLoc,messaggio);
				sendSocket(conn,"n",1);
			}
		}else if(scelta[0]=='b'){
		
			scelta[bNick]='\0';
			prendiNome(Addnickname,scelta+1);
			
			pthread_mutex_lock(&mymutex);//blocca il mutex
			GiaOnline=trovaOnline(Addnickname,Ut);
			struct utente **tmp=&Ut;
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
		
			if(GiaOnline==1){
				DataAttuale(data);
				sprintf(messaggio,"Impossibile effettuare il login l'utente è gia loggato con %s in data : %s\n",scelta+1,data);
				stampaLog(fdLogLoc,messaggio);
				sendSocket(conn,"g",1);
				GiaOnline=0;
			}
			else{
				DataAttuale(data);
				sprintf(messaggio,"L'utente %s si e' connesso in data : %s\n",Addnickname,data);
				stampaLog(fdLogLoc,messaggio);
				if(!controlloLogin(scelta+1,fdUtente_loc)){
					sendSocket(conn,"y",1);
					pthread_mutex_lock(&mymutex);//blocca il mutex
					aggiungiUtente(tmp,Addnickname,pthread_self());
					pthread_mutex_unlock(&mymutex);//sblocca il mutex
					if(isMapp==0){
        				if(pthread_create(&tredServer,NULL,GeneraMapp,NULL) < 0){
        					errore("Errore Thread\n");
        				}
        			}
					gioco(conn,Addnickname,Ut);
				}else{
					sendSocket(conn,"n",1);
				}
			}
            
		}else if(scelta[0]=='c'){
			int fileLog_loc2=fdLog;
			DataAttuale(data);
			pthread_mutex_lock(&mymutex);//blocca il mutex
			char messaggio[256];
			sprintf(messaggio,"Utente non loggato disconnesso in Data %s\n",data);
			stampaLog(fileLog_loc2,messaggio);
			utentiConnessi--;
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
			flag=0;
		}
        
        else if(scelta[0]=='l'){
			pthread_mutex_lock(&mymutex);//blocca il mutex
			int fileLog_loc2=fdLog;
			DataAttuale(data);
			char messaggio[256];
			sprintf(messaggio,"Utente non loggato disconnesso in maniera anomala in Data %s\n",data);
			stampaLog(fileLog_loc2,messaggio);
			utentiConnessi--;
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
			flag=0;
			
		}
        
        else{
			pthread_mutex_lock(&mymutex);//blocca il mutex
			int fileLog_loc3=fdLog;
			DataAttuale(data);
			sprintf(messaggio,"Errore di comunicazione con client in Data %s\n",data);
			stampaLog(fileLog_loc3,messaggio);
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
			flag=0;
		}
	}while(flag==1);

pthread_exit(NULL);
}

//______________________________________________________________

void sendSocket(int conn,char *mess,int nbytes)
{
	if(write(conn,mess,nbytes)<0){
		errore("Errore write\n");
	}
}

//______________________________________________________________

void errore(char *messaggio){
	perror(messaggio);
	pthread_mutex_lock(&mymutex);//blocco il mutex in scrittura del file
	int fdLogLoc=fdLog;
	pthread_mutex_unlock(&mymutex);//sblocco il mutex in scrittura del file

	stampaLog(fdLogLoc,messaggio);
	exit(1);
}



//_______________________________________________________________
//scrive nel log
void stampaLog(int fd,char *messaggio)
{	
	if(write(fd,messaggio,strlen(messaggio))<0){
		errore("Permessi Write\n");
	}
}

//_________________________________________________________________

void DataAttuale(char *dataT)
{
	int gm,m,a,gs,h,min,s;
 	time_t data;
 	struct tm * leggibile = NULL;
	time (&data);

 	leggibile = localtime (&data);
 	gm=leggibile->tm_mday;
 	m=leggibile->tm_mon +1;
 	a=leggibile->tm_year+1900;
 	gs=leggibile->tm_wday+1; // 1 = Domenica - 7 = Sabato
 	h=leggibile->tm_hour;
 	min=leggibile->tm_min;
 	s=leggibile->tm_sec;
 	sprintf(dataT,"%d/%d/%d Ora :(%d:%d:%d)",gm,m,a,h,min,s);
}

//_________________________________________________________________

int controlloNome(char *Nick,int fd)
{
	char flag[1024], file[1024], nome[1024];
	int i=0,nbytes=0;
	off_t currpos;

	currpos = lseek(fd, 0, SEEK_SET);
	prendiNome(flag,Nick);
	while(read(fd, &file[i], 1) == 1){
		if(file[i]=='\n'||file[i]==0x0){
			file[i]='\0';
			prendiNome(nome,file);
			if(strcmp(flag,nome)==0){
				return 0;
			}
		i=0;
		continue;
		}
	i++;
	}	
return 1;
}

//_____________________________________________________________________
//scorro lista nickname

void prendiNome(char *flag,char *Nick)
{
	int i=0;
	
	while(Nick[i]!=':'){
		flag[i]=Nick[i];
		i++;
	}
	flag[i]='\0';
}

//_______________________________________________________________________

int controlloLogin(char *Nick, int fd)
{
	char flag[1024], file[1024], nome[1024];
	int i=0,nbytes=0;
	off_t currpos;

	currpos = lseek(fd, 0, SEEK_SET);
	while(read(fd, &file[i], 1) == 1){
		if(file[i]=='\n'||file[i]==0x0){
			file[i]='\n';
			file[i+1]='\0';
			if(strcmp(file,Nick)==0){
				return 0;
			}
		i=0;
		continue;
		}
	i++;
	}	
return 1;
}

//_______________________________________________________________________
//funzione gioco

void gioco(int conn, char *Nick, struct utente* Ut){
	
	//cordinate personaggio (array)
	char coordinateClient[3];

	//NUOVA POSIZIONE
	char nuova_posizione[3];
	srand(time(NULL));

	//coordinate x e y
	int i=0, j=0;
	int flag=1, q=0,k=0, posizionato=0, Ric_messaggio=0, idControllo=0;
	char messaggio[1000];

	//lista di nome coordinate e indizi trovate (info utente)
	char lista[5000], data[256];
	char stringaloggati[5000];

	// se = 1 c'è un vincitore 
	int vittoria=0;
	int terminata=0;

	pthread_t tredServer;
    
	int idPartitaClient;
	pthread_mutex_lock(&mymutex);//blocco il mutex
	utentiInGioco++;
	pthread_mutex_unlock(&mymutex);//sblocco il mutex

	//inizializza a null nuova posizione (attende movimento)
	nuova_posizione[0]='\0';
	nuova_posizione[1]='\0';
	nuova_posizione[2]='\0';

//posizionami
    do{
		i=rand()%9;  j=0;
        
	    if(mappa[i][j]==' '){
	    	pthread_mutex_lock(&mymutex);//blocco il mutex
	    	mappa[i][j]='O';
	    	pthread_mutex_unlock(&mymutex);//blocco il mutex
	    	posizionato=1;
	    }
	}while(posizionato==0); //FINCHE NON SONO POSIZIONATO

	//DETERMINA INFO PERSONAGGIO
	coordinateClient[0]=i;
	coordinateClient[1]=j;
	coordinateClient[2]='O';
	
	//inviare qui l'id partita al client (INVIO N1)
	if(write(conn,&idPartita,sizeof(int))<0){
		errore("Errore write\n");
	}
	//fine invio id partita
	//(INVIO N2)
sendSocket(conn,coordinateClient,3);
    
	signal(SIGINT,handlerCtrlC);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGSTOP,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGABRT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGPIPE,handlerpipe);


//inizio ciclo
/* utilizzo la variabile flag per far continuare il ciclo
flag inizalmente = 1
ciclo termina quando flag = 0 */

	char segnale;
	do{
		pthread_mutex_lock(&mymutex);//blocco il mutex
		// salva info utente in UT(struct utente) al personaggio (nickname) con coordinate x e y
		aggiungiCoordinate(Ut,Nick,coordinateClient[0],coordinateClient[1]);
		pthread_mutex_unlock(&mymutex);//sblocco il mutex

		nuova_posizione[0]=0;

		//legge da client cambia_posizione 
		if(read(conn,nuova_posizione,3)<0){
			flag=0;
		}
		else if(tempoGioco==TEMPO){       
			sendSocket(conn,"f",1);
			flag=0;
		}
		//se non ci sono errori attivo switch per leggere movimento
		else{

		switch(nuova_posizione[0]){


                //sopra
			case 'w':

			//CONTROLLOCONNESSIONE
			if(read(conn,&idPartitaClient,sizeof(int))<0){
				errore("Errore read");
			}
			// Controllo collegamento ID
			else if(idPartita!=idPartitaClient){
				sendSocket(conn,"f",1);
				idControllo=1;
				flag=0;
			}
			// se collegamento è ok via!
				else{
					//gestione margine superiore
					if((nuova_posizione[1]-1)<0){
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
					}
					else{
						//se al movimento incontra bomba (resta fermo e invia # al client)
						if(mappa[nuova_posizione[1]-1][nuova_posizione[2]]=='#'){
							
							coordinateClient[0]=nuova_posizione[1];
							coordinateClient[1]=nuova_posizione[2];
							coordinateClient[2]='#';
							terminata = 1;
						}
						//se invece incontra un altro utente resta fermo e invia O
						else if(mappa[nuova_posizione[1]-1][nuova_posizione[2]]=='O'){
							coordinateClient[0]=nuova_posizione[1];
							coordinateClient[1]=nuova_posizione[2];
							coordinateClient[2]='O';
						}
						else{
							pthread_mutex_lock(&mymutex);//blocco il mutex
							if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
								mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
							}
							coordinateClient[0]=nuova_posizione[1]-1;
							coordinateClient[1]=nuova_posizione[2];
							coordinateClient[2]=' ';
							mappa[nuova_posizione[1]-1][nuova_posizione[2]]='O';
							pthread_mutex_unlock(&mymutex);//sblocco il mutex
						}
					}
				}

			

			if(terminata==1){
				pthread_mutex_lock(&mymutex);//blocco il mutex
				deallocaUtente(Ut);
				Ut=NULL;
				flag = 0;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}

			//se non ho vinto , e il tempo è in corso
			
				if(idControllo==0){
					sendSocket(conn,coordinateClient,3);
				}	
			
			break;

                //giu
            case 's':

				if(read(conn,&idPartitaClient,sizeof(int))<0){
					errore("Errore read");
				}
				if(idPartita!=idPartitaClient){
					sendSocket(conn,"f",1);
					idControllo=1;
					flag=0;
				}
				else{
				if((nuova_posizione[1]+1)>19){
					coordinateClient[0]=nuova_posizione[1];
					coordinateClient[1]=nuova_posizione[2];
				}
				else{
					if(mappa[nuova_posizione[1]+1][nuova_posizione[2]]=='#'){
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]='#';
						terminata = 1;
					}
					else if(mappa[nuova_posizione[1]+1][nuova_posizione[2]]=='O'){
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]='O';
					}
					else{
						pthread_mutex_lock(&mymutex);//blocco il mutex
						if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
							mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
						}
						coordinateClient[0]=nuova_posizione[1]+1;
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]=' ';
						mappa[nuova_posizione[1]+1][nuova_posizione[2]]='O';
						pthread_mutex_unlock(&mymutex);//sblocco il mutex
					}
				}
			}
			

			if(terminata==1){

				pthread_mutex_lock(&mymutex);//blocco il mutex
				deallocaUtente(Ut);
				Ut=NULL;
				flag = 0;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}
			//se non ho vinto , e il tempo è in corso
			
				if(idControllo==0){
					sendSocket(conn,coordinateClient,3);
				}	
			
			break;


                //sinistra
			case 'a':
				if(read(conn,&idPartitaClient,sizeof(int))<0){
					errore("Errore read");
				}
				if(idPartita!=idPartitaClient){
					sendSocket(conn,"f",1);
					idControllo=1;
					flag=0;
				}
				else{
				if((nuova_posizione[2]-1)<0){
					coordinateClient[0]=nuova_posizione[1];
					coordinateClient[1]=nuova_posizione[2];
				}
				else{
					if(mappa[nuova_posizione[1]][nuova_posizione[2]-1]=='#'){
						
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]='#';
						terminata = 1;
					}
					else if(mappa[nuova_posizione[1]][nuova_posizione[2]-1]=='O'){
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]='O';
					}
					else{
						pthread_mutex_lock(&mymutex);//blocco il mutex
						if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
							mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
						}
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2]-1;
						coordinateClient[2]=' ';
						mappa[nuova_posizione[1]][nuova_posizione[2]-1]='O';
						pthread_mutex_unlock(&mymutex);//sblocco il mutex
					}	
				}
			}

			

			if(terminata==1){

				pthread_mutex_lock(&mymutex);//blocco il mutex
				deallocaUtente(Ut);
				Ut=NULL;
				flag = 0;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}
			//se non ho vinto , e il tempo è in corso
			
				if(idControllo==0){
					sendSocket(conn,coordinateClient,3);
				}	
			
			break;

                //destra
			case 'd':
				if(read(conn,&idPartitaClient,sizeof(int))<0){
					errore("Errore read");
				}
				if(idPartita!=idPartitaClient){
					sendSocket(conn,"f",1);
					idControllo=1; //Non è possibile giocare
					flag=0;
				}
				else{
				if((nuova_posizione[2]+1)>19){
					coordinateClient[0]=nuova_posizione[1];
					coordinateClient[1]=nuova_posizione[2];
				}
				if(nuova_posizione[2]==19){
						vittoria=1;
				}

				else{
					if(mappa[nuova_posizione[1]][nuova_posizione[2]+1]=='#'){
						
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]='#';
						terminata = 1;
					}
					else if(mappa[nuova_posizione[1]][nuova_posizione[2]+1]=='O'){
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2];
						coordinateClient[2]='O';
					}
					else{
						pthread_mutex_lock(&mymutex);//blocco il mutex
						if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
							mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
						}
						coordinateClient[0]=nuova_posizione[1];
						coordinateClient[1]=nuova_posizione[2]+1;
						coordinateClient[2]=' ';
						mappa[nuova_posizione[1]][nuova_posizione[2]+1]='O';
						pthread_mutex_unlock(&mymutex);//sblocco il mutex
					}
				}
			}
			//caso in cui ho vinto
			if(vittoria==1){
				sendSocket(conn,"v",1);
				pthread_mutex_lock(&mymutex);//blocco il mutex
				deallocaUtente(Ut);
				Ut=NULL;
				flag=0; //termina partita x tutti
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}

			if(terminata==1){

				pthread_mutex_lock(&mymutex);//blocco il mutex
				deallocaUtente(Ut);
				Ut=NULL;
				flag = 0;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
				
			}

		
				if(idControllo==0){ //Basta che un client entri nella partita per giocare, non deve necessariamente aspettare gli altri
					sendSocket(conn,coordinateClient,3);
					}
				
			break;

                //logout
			case 'c':
				DataAttuale(data);
				pthread_mutex_lock(&mymutex);//blocca il mutex
				int fileLog_loc=fdLog;
				if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
					mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
				}
				Ut=EliminaScollegato(Ut,Nick);
				char messaggio[256];
				sprintf(messaggio,"Utente %s Disconnesso in Data %s\n",Nick,data);
				stampaLog(fileLog_loc,messaggio);
				terminata = 1; //per generare una nuova mappa				
				flag=0;
				pthread_mutex_unlock(&mymutex);//sblocca il mutex
				
			break;


			case 'i':

				pthread_mutex_lock(&mymutex);//blocca il  mutex
				struct utente* tmp2 = Ut;
				pthread_mutex_unlock(&mymutex);//sblocca il mutex
				for(k=0;k<5000;k++){
				stringaloggati[k]='\0';
				}
				strcat(stringaloggati,GREEN"Utenti online:  "RESET);
				while(tmp2!=NULL){
					char coordinateLoc[50];
					sprintf(coordinateLoc,"[%d][%d]",tmp2->x,tmp2->y);
					strcat(stringaloggati,GREEN"\nUtente  "RESET);
					strcat(stringaloggati,tmp2->nickname);
					strcat(stringaloggati,GREEN" in posizione  "RESET);
					strcat(stringaloggati,coordinateLoc);
					strcat(stringaloggati,"\n");
					tmp2=tmp2->next;
				}
				sendSocket(conn,stringaloggati,strlen(stringaloggati));
				
		
			break;

                //info tempo
			case 't':
				pthread_mutex_lock(&mymutex);//blocca il  mutex
				int tempo_loc=tempoGioco;
				pthread_mutex_unlock(&mymutex);//sblocca il mutex
				if(write(conn,&tempo_loc,sizeof(int))<0){
					errore("Errore write\n");
				}
			break;

                //segnale k per visualizzare utenti
			case 'k':
				pthread_mutex_lock(&mymutex);//blocca il  mutex
				struct utente *tmp=Ut;
				pthread_mutex_unlock(&mymutex);//sblocca il mutex
				char lista2[5000];
					for(k=0;k<5000;k++){
        				lista2[k]='\0';
        			}
					while(tmp!=NULL){
						char coordinateLoc[50];
						sprintf(coordinateLoc,"%d?%d",tmp->x,tmp->y);
						strcat(lista2,coordinateLoc);
						strcat(lista2,"&");
						tmp=tmp->next;
					}
					lista2[strlen(lista2)-1]='\0';
				sendSocket(conn,lista2,strlen(lista2));
			break;

                //caso default
			default :
				flag=0;
			break;
		}
	  }
	}while(flag==1); 
//fine ciclo
    
    
	pthread_mutex_lock(&mymutex);
	if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
		mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
	}
	utentiInGioco--;
	pthread_mutex_unlock(&mymutex);
	Ut=EliminaScollegato(Ut,Nick);
	if(vittoria==1 || terminata == 1){
		if(pthread_create(&tredServer,NULL,GeneraMapp,NULL) < 0){
        	errore("Errore Thread\n");
        }
	}
}

// fine funzione gioco

//___________________________________________________________________________________

void stampaMappa(int fd,char mappa[][20]){
	int i=0, j=0,z=0;
	char messaggio[20]="Debug\0";
	write(fd,"------------l--------\n",22);
	for(i=0; i<20; i++){
		write(fd,"|",1);
		for(j=0; j<20; j++){
			write(fd,&mappa[i][j],1);
		}
		write(fd,"|\n",2);
	}
	write(fd,"---------------------\n",22);
}


//___________________________________________________________________________________
void *GeneraMapp(void *ptr){
	int i=0, j=0, k=0;
	int nBombe=8;  //N BOMBE
	int fdLogLoc;
	int i0,j0,i1,j1,i2,j2,i3,j3;
	int posizionato=0;
	int cordinata1=0,cordinata2=0;
	char data[50], newmappa[50];
	srand(time(NULL));
	time_t start=0,end=0;
	char messId[50];

	pthread_mutex_lock(&mymutex);
	idPartita++;//aumenta id partita
	sprintf(messId,"Inizia la partita numero ( %d )\n",idPartita);

	flagVittoria=0;
	fdLogLoc=fdLog;
	pthread_mutex_unlock(&mymutex);

	//inizializzazione della matrice
	for(i=0; i<20; i++){
		for(j=0; j<20; j++){ 
			mappa[i][j]=' ';
		}
	}
	//posizonamento delle BOMBE 
    for(i=0; i<nBombe; i++){
		cordinata1=1+rand()%18;
		cordinata2=1+rand()%18;
		if(mappa[cordinata1][cordinata2]==' '){
			mappa[cordinata1][cordinata2]='#';
		}
	}
	isMapp=1;
	sprintf(newmappa,"Nuova mappa generata in data %s\n",data);
	stampaLog(fdLogLoc,newmappa);
	stampaMappa(fdLogLoc,mappa);
	DataAttuale(data);
	tempoGioco=0;
	do{
		sleep(1);
		tempoGioco++;
	}while(tempoGioco<TEMPO && flagVittoria==0);
	isMapp=0;
pthread_exit(NULL);
}

//___________________________________________________________________________________

void aggiungiUtente(struct utente **top,char *Nick,  pid_t pidl ){

		struct utente *tmp=(struct utente *)calloc(1,sizeof(struct utente)); // creo temp
		strcpy(tmp->nickname,Nick);
		tmp->pid=pidl; 
		tmp->next=NULL;
		
		if(*top == NULL){
			*top = tmp;
		}
		else{
			struct utente *coda;
			coda=*top;
			while(coda->next!=NULL){
				coda=coda->next;
			}
			coda->next=tmp;
		}
}

//___________________________________________________________________________________

struct utente *EliminaScollegato(struct utente *top,char *Nick){
	if(top!=NULL){
		if(strcmp(top->nickname,Nick)==0){
			struct utente *tmp=NULL;
			tmp=top;
			top=top->next;
			free(tmp);
		}else{
			top->next=EliminaScollegato(top->next,Nick);
		}
	}
return top;
}

//___________________________________________________________________________________

int trovaOnline(char *nickname ,struct utente *top){
	int flag=0;
	if(top!=NULL){
		if(strcmp(top->nickname,nickname)==0){
			flag=1;
		}else{
			flag=trovaOnline(nickname,top->next);
		}
	}
return flag;
}


//___________________________________________________________________________________

void stampaUtentiLoggati(struct utente *top){
	if(top!=NULL){
		sprintf("%s in posizione [%d][%d]",top->nickname,top->x,top->y);
		printf("\n");
		stampaUtentiLoggati(top->next);
	}else{
		printf("\nFine Lista\n");
	}
}

//___________________________________________________________________________________

void stringaUtentiLoggati(struct utente *top,char* stringaloggati){
	if(top!=NULL){
		sprintf(stringaloggati,"%d%d",top->x,top->y);
		printf("\n");
		stringaUtentiLoggati(top->next,stringaloggati);
	}
}

//___________________________________________________________________________________

void deallocaUtente(struct utente *top){
	if(top!=NULL){
		deallocaUtente(top->next);
		free(top);
	}
}

//___________________________________________________________________________________

void aggiungiCoordinate(struct utente *top,char *Nick,int i,int j){
	if(top!=NULL){
		if(strcmp(top->nickname,Nick)==0){
			top->x=i;
			top->y=j;
		}else{
			aggiungiCoordinate(top->next,Nick,i,j);
		}
	}
}

//___________________________________________________________________________________

struct utente *getNode(pid_t pid,struct utente *top){
	struct utente *flag=0;
	if(top!=NULL){
		if(top->pid==pid){
			flag=top;
		}else{
			flag=getNode(pid,top->next);
		}
	}
return flag;
}

//___________________________________________________________________________________

void handlerpipe(int x){
	void *status;
	char buffer[256];
	struct utente *utente_local=NULL;
	time_t ora;
	pthread_mutex_lock(&mymutex);//blocco il mutex
	utente_local=getNode(pthread_self(),Ut);

	if(mappa[utente_local->x][utente_local->y]=='O'){
		mappa[utente_local->x][utente_local->y]=' ';
	}

	pthread_mutex_unlock(&mymutex);
	if(utente_local){
		ora=time(NULL);
		sprintf(buffer,"\t%s\tchiusura anomala\t%s\n",utente_local->nickname,asctime(localtime(&ora)));
		pthread_mutex_lock(&mymutex);//sblocco il mutex
		Ut=EliminaScollegato(Ut,utente_local->nickname);
		pthread_mutex_unlock(&mymutex);
	}
pthread_exit(status);
}

//_________________________________________________________________________________________

void handlerCtrlC(int x){

	void *status;
	pthread_mutex_lock(&mymutex);
	struct utente *local=Ut;
	pthread_mutex_unlock(&mymutex);

	while(local!=NULL){
		pthread_exit(&local->pid);
		local=local->next;
	}
pthread_exit(status);
}

//___________________________________________________________________________________

//FINE SERVER









