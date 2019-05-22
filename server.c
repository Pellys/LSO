/* tcpserver.c */

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
	char nick[512];
	pid_t pid;
	int x;
	int y;
	struct utente *next;
};



//puntatori al primo elemento delle strutture dati
struct utente *Ut=NULL;


/* Prototipi Funzione */

//funzioni Thread
void *funzione(void *ptr);
void *GeneraMapp(void *ptr);

//funzioni generiche int(booleane)
int checkLogin(char *,int);
int checkName(char *,int);
int TrovaGiaOnline(char * ,struct utente *);
void casta_array(char *, char*);


//funzioni generiche void
void error(char *);
void stampaLog(int ,char *);
void DataNow();
void prendinome(char *flag,char *);
void send_socket(int ,char *,int );
void gioco(int conn,char *,struct utente*);
void stampaMappa(int fd,char mappa[][20]);
void sig_handler(int signo);
void handlerpipe(int x);
void handlerCtrlC(int x);

//funzioni per la lista di utenti connessi

struct utente *EliminaScollegato(struct utente *,char *);
void stampaUtentiLoggati(struct utente *);
void stringaUtentiLoggati(struct utente *,char*);
void dealloca(struct utente *);
void aggiungi_utente(struct utente **top,char *,pid_t);
void AggiungiCoordinate(struct utente *top,char *Nick,int i,int j);

void prova(struct utente*, char* );


/* Dichiarazioni Globali */

pthread_mutex_t mymutex=PTHREAD_MUTEX_INITIALIZER;

char mappa[20][20];
char utentevincente[50];

int fdutente;
int fdlog;
int utenti_connessi=0;
int ismapp=0;
int tempoGioco=0;

int utenti_che_giocano=0;
int id_partita=0;
int flag_vittoria=0;





//_____________________________________________________MAIN____________________________________________________//

int main(int argc, char const *argv[])
{
        
    	int sock;             	//socket
	int connected;		//risultato accept
	int bytes_recieved=0;
	int bytes_iviated=0;  

	char send_data [1024]; 
	char recv_data[1024]; 

        struct sockaddr_in server_addr,client_addr;    
        int sin_size,i=0,j=0;

    
   	 //apre file (database per login)
        fdutente=open("utenti.txt", O_RDWR | O_CREAT |O_APPEND ,S_IRWXU);
        fdlog=open("log.txt",O_WRONLY | O_CREAT | O_APPEND,S_IRWXU);
    
    
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
        if(fdutente<0 || fdlog<0){
        	error("Errore Permessi FILE\n");
        }




    
    // ERRORE IN CASO DI MANCATO INPUT ALL'AVVIO
        if(argc!=2){
        	error("Errore Inserire La Porta : Man D'Uso  ./server 5xxx\n");
        }
    
    
  
  
    //CREO SOCKET
    sock = socket(AF_INET, SOCK_STREAM, 0);
    //VEDO SE ESISTE
        if (sock == -1) {
            error("Socket\n");
        }
    
    
    
    
        server_addr.sin_family = AF_INET;         	//host+porta (formato)
        server_addr.sin_port = htons(atoi(argv[1]));    //porta
        server_addr.sin_addr.s_addr = INADDR_ANY; 	//indirizzo

    
	//bind assegna indirizzo di socket locale alla remota
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
            error("Unable to bind\n");
        }


	//grandezza indirizzo socket
        sin_size = sizeof(struct sockaddr_in);

	
	//listen (socket , lunghezza coda di connessioni) 
        if (listen(sock, 5) == -1) {
            error("Listen\n");
        }


        DataNow(data);

		//salva informazioni nel log con info+data
		bytes_recieved=sprintf(recv_data,"The server is up on %s port Date: %s\n",argv[1],data);
		stampaLog(fdlog,recv_data);
		


		signal(SIGPIPE,handlerpipe);


        while(1){  

            if((connected=accept(sock, (struct sockaddr *)&client_addr,&sin_size))==-1){
            	error("Connessione refused\n");
            }
            utenti_connessi++;
          
            if((pthread_create(&tred,NULL,funzione,(void*)&connected)) < 0){
   			 	error("pthread_create() failed\n");
  			}
  			pthread_detach(tred);
            sprintf(recv_data,"Nuova Connessione Da : (%s,%d)\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
            stampaLog(fdlog,recv_data);
        }       

close(fdutente);
close(sock);
close(fdlog);
dealloca(Ut);
return 0;
} 




//__________________________________________________________________END MAIN___________________________________________________________________________________












//__________________________________________________________________MENU PRINCIPALE___________________________________________________________________________________


void *funzione(void *ptr)
{
	char Addnick[512],messaggio[512],scelta[1024],data[256];
	int  flag=1;
	int *connect=(int*)ptr;
	int conn=*connect;
	int bNick=0, bPassw=0;
	int fdutente_loc;
	struct utente *tmp=NULL;
	int fdlog_loc, GiaOnline=0;
	int i=0;
	pthread_t tredServer;

	signal(SIGPIPE,handlerpipe);
	pthread_mutex_lock(&mymutex);//blocca il mutex
	fdutente_loc=fdutente; 
	fdlog_loc=fdlog;
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
			error("Errore read\n");
		}
		
		
		if(scelta[0]=='a'){
			
			scelta[bNick]='\0';

			if(checkName(scelta+1,fdutente_loc)){
				stampaLog(fdutente_loc,scelta+1);
				DataNow(data);
				sprintf(messaggio,"Nuovo utente registrato con %s in data : %s\n",scelta+1,data);
				stampaLog(fdlog_loc,messaggio);
				send_socket(conn,"y",1);
			}else{
				DataNow(data);
				sprintf(messaggio,"Impossibile registrare l'utente con %s in data : %s\n",scelta+1,data);
				stampaLog(fdlog_loc,messaggio);
				send_socket(conn,"n",1);
			}
		}else if(scelta[0]=='b'){
		
			scelta[bNick]='\0';

			prendinome(Addnick,scelta+1);
			
			pthread_mutex_lock(&mymutex);//blocca il mutex
			GiaOnline=TrovaGiaOnline(Addnick,Ut);
			struct utente **tmp=&Ut;
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
		
			if(GiaOnline==1){
				DataNow(data);
				sprintf(messaggio,"Impossibile effettuare il login l'utente è gia loggato con %s in data : %s\n",scelta+1,data);
				stampaLog(fdlog_loc,messaggio);
				send_socket(conn,"g",1);
				GiaOnline=0;
			}
			else{
				DataNow(data);
				sprintf(messaggio,"L'utente %s si e' connesso in data : %s\n",Addnick,data);
				stampaLog(fdlog_loc,messaggio);
				if(!checkLogin(scelta+1,fdutente_loc)){
					send_socket(conn,"y",1);
					pthread_mutex_lock(&mymutex);//blocca il mutex
					aggiungi_utente(tmp,Addnick,pthread_self());
					pthread_mutex_unlock(&mymutex);//sblocca il mutex
					if(ismapp==0){
        				if(pthread_create(&tredServer,NULL,GeneraMapp,NULL) < 0){
        					error("Errore Thread\n");
        				}
        			}
					gioco(conn,Addnick,Ut);
				}else{
					send_socket(conn,"n",1);
				}

			}
		}else if(scelta[0]=='c'){
			int fileLog_loc2=fdlog;
			DataNow(data);
			pthread_mutex_lock(&mymutex);//blocca il mutex
			char messaggio[256];
			sprintf(messaggio,"Utente non loggato disconnesso in Data %s\n",data);
			stampaLog(fileLog_loc2,messaggio);
			utenti_connessi--;
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
			flag=0;
		}else if(scelta[0]=='l'){
			pthread_mutex_lock(&mymutex);//blocca il mutex
			int fileLog_loc2=fdlog;
			DataNow(data);
			char messaggio[256];
			sprintf(messaggio,"Utente non loggato disconnesso in maniera anomala in Data %s\n",data);
			stampaLog(fileLog_loc2,messaggio);
			utenti_connessi--;
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
			flag=0;
			
		}else{
			pthread_mutex_lock(&mymutex);//blocca il mutex
			int fileLog_loc3=fdlog;
			DataNow(data);
			sprintf(messaggio,"Errore di comunicazione con client in Data %s\n",data);
			stampaLog(fileLog_loc3,messaggio);
			pthread_mutex_unlock(&mymutex);//sblocca il mutex
			flag=0;
		}
	}while(flag==1);

pthread_exit(NULL);
}




//______________________________________________________________________________________________________________________________________________________



void send_socket(int conn,char *mess,int nbytes)
{
	if(write(conn,mess,nbytes)<0){
		error("Errore write\n");
	}
}



//_____________________________________________________________________ERRORE_________________________________________________________________________________



void error(char *messaggio)
{
	perror(messaggio);
	pthread_mutex_lock(&mymutex);//blocco il mutex in scrittura del file
	int fdlog_loc=fdlog;
	pthread_mutex_unlock(&mymutex);//sblocco il mutex in scrittura del file

	stampaLog(fdlog_loc,messaggio);
	exit(1);
}



//__________________________________________________________________SCRIVE NEL LOG______________________________________________________________________________



void stampaLog(int fd,char *messaggio)
{	
	if(write(fd,messaggio,strlen(messaggio))<0){
		error("Permessi Write\n");
	}
}



//____________________________________________________________________FORMATO DATA_____________________________________________________________________________



void DataNow(char *dataT)
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




//_____________________________________________________________CONTROLLO NICKNAME ESISTENTE__________________________________________________________________________



int checkName(char *Nick,int fd)
{
	char flag[1024], file[1024], nome[1024];
	int i=0,nbytes=0;
	off_t currpos;

	currpos = lseek(fd, 0, SEEK_SET);
	prendinome(flag,Nick);
	while(read(fd, &file[i], 1) == 1){
		if(file[i]=='\n'||file[i]==0x0){
			file[i]='\0';
			prendinome(nome,file);
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



//_____________________________________________________________________SCORRO LISTA NICK____________________________________________________________________________



void prendinome(char *flag,char *Nick)
{
	int i=0;
	
	while(Nick[i]!=':'){
		flag[i]=Nick[i];
		i++;
	}
	flag[i]='\0';
}


//________________________________________________________________________CONTROLLO LOGIN_________________________________________________________________________



int checkLogin(char *Nick, int fd)
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






//______________________________________________________________________________________________________________________________________________________
//                                 T H E   G A M E
//______________________________________________________________________________________________________________________________________________________




void gioco(int conn, char *Nick, struct utente* Ut)
{
	
	//cordinate personaggio (array)
	char personaggio[3]; 
	char stringaloggati[300]; //coordinate da mandare al client

	//NUOVA POSIZIONE (client = cambia_posizione) 
	char nuova_posizione[3];
	srand(time(NULL));

	//coordinate x e y
	int i=0, j=0;

	int flag=1, q=0,k=0, posizionato=0, Ric_messaggio=0, giottatadellanno=0;


	char messaggio[1000];

	//lista di nome coordinate e indizi trovate (info utente)
	char lista[5000], data[256];

	// se = 1 c'è un vincitore 
	int vittoria=0;


	pthread_t tredServer;

	//
	int id_partita_client;
	pthread_mutex_lock(&mymutex);//blocco il mutex
	utenti_che_giocano++;
	pthread_mutex_unlock(&mymutex);//sblocco il mutex

	//inizializza a null nuova posizione (attende movimento)
	nuova_posizione[0]='\0';
	nuova_posizione[1]='\0';
	nuova_posizione[2]='\0';




// FAI QUESTO = POSIZIONAMI
	do{
		i=rand()%19;  j=0;

	    if(mappa[i][j]==' '){
	    	pthread_mutex_lock(&mymutex);//blocco il mutex
	    	mappa[i][j]='O';
	    	pthread_mutex_unlock(&mymutex);//blocco il mutex
	    	posizionato=1;
	    }
	}while(posizionato==0); //FINCHE NON SONO POSIZIONATO

	
	

	//DETERMINA INFO PERSONAGGIO
	personaggio[0]=i;
	personaggio[1]=j;
	personaggio[2]='O';
	


	//inviare qui l'id partita al client (INVIO N1)
	if(write(conn,&id_partita,sizeof(int))<0){
		error("Errore write\n");
	}


	//fine invio id partita
	
	
	
	
	//(INVIO N2)

send_socket(conn,personaggio,3);


	
//send_socket(conn,stringa,strlen(stringa));

	signal(SIGINT,handlerCtrlC);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGSTOP,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGABRT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGPIPE,handlerpipe);




//___________________INIZIO DO WHILE ENORME________________________________________

/* utilizzo la variabile flag per far continuare il ciclo
flag inizalmente = 1
ciclo termina quando flag = 0 */


	char segnale;


	do{   
		
		

		
		stringaUtentiLoggati(Ut,stringaloggati);
		
		
		printf("%s",stringaloggati);

		/*if(write(conn,stringaloggati,strlen(stringaloggati))<0){
			error("Errore write\n");	
		}*/


		


		/*____________________________________________*/

		pthread_mutex_lock(&mymutex);//blocco il mutex

		// salva info utente in UT(struct utente) al personaggio (nick) con coordinate x e y
		AggiungiCoordinate(Ut,Nick,personaggio[0],personaggio[1]);
		
		 
		pthread_mutex_unlock(&mymutex);//sblocco il mutex

		nuova_posizione[0]=0;

		//legge da client cambia_posizione 
		if(read(conn,nuova_posizione,3)<0){
			flag=0;
		}

		else if(tempoGioco==TEMPO){       
				send_socket(conn,"f",1);
				flag=0;
				}



		//se non ci sono errori attivo switch per leggere movimento
		else{


			



		switch(nuova_posizione[0]){



//_________CASO SOPRA_____________________________________


			case 'w':


			//CONTROLLOCONNESSIONE

				if(read(conn,&id_partita_client,sizeof(int))<0){ 
					error("Errore read");
				}

			
			// Controllo collegamento ID

				else if(id_partita!=id_partita_client){
					send_socket(conn,"f",1);
					giottatadellanno=1;
					flag=0;
				}

			// se collegamento è ok via!

				else{	

					//gestione margine superiore
					if((nuova_posizione[1]-1)<0){
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
					}

					
					else{

				
						//se al movimento incontra bomba (resta fermo e invia # al client)
						if(mappa[nuova_posizione[1]-1][nuova_posizione[2]]=='#'){
							
							personaggio[0]=nuova_posizione[1];
							personaggio[1]=nuova_posizione[2];
							personaggio[2]='#';
						}

						//se invece incontra un altro utente resta fermo e invia O
						else if(mappa[nuova_posizione[1]-1][nuova_posizione[2]]=='O'){
							personaggio[0]=nuova_posizione[1];
							personaggio[1]=nuova_posizione[2];
							personaggio[2]='O';
						}

						

						else{
							pthread_mutex_lock(&mymutex);//blocco il mutex
							if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
								mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
							}
							personaggio[0]=nuova_posizione[1]-1;
							personaggio[1]=nuova_posizione[2];
							personaggio[2]=' ';
							mappa[nuova_posizione[1]-1][nuova_posizione[2]]='O';
							pthread_mutex_unlock(&mymutex);//sblocco il mutex
						}
					}
				}



			//caso in cui ho vinto
			if(vittoria==1){
				send_socket(conn,"v",1);
				pthread_mutex_lock(&mymutex);//blocco il mutex
				dealloca(Ut);
				Ut=NULL;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}

		
			//se non ho vinto , e il tempo è in corso
			else{
				if(giottatadellanno==0){
					send_socket(conn,personaggio,3);
					
				}	
			}
			break;




//_________CASO GIU_____________________________________
			case 's':

				if(read(conn,&id_partita_client,sizeof(int))<0){ 
					error("Errore read");
				}
				if(id_partita!=id_partita_client){
					send_socket(conn,"f",1);
					giottatadellanno=1;
					flag=0;
				}
				

				else{
				if((nuova_posizione[1]+1)>19){
					personaggio[0]=nuova_posizione[1];
					personaggio[1]=nuova_posizione[2];
				}
				else{
					if(mappa[nuova_posizione[1]+1][nuova_posizione[2]]=='#'){
						
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
						personaggio[2]='#';
					}
					else if(mappa[nuova_posizione[1]+1][nuova_posizione[2]]=='O'){
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
						personaggio[2]='O';
					}

					

					else{
						pthread_mutex_lock(&mymutex);//blocco il mutex
						if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
							mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
						}
						personaggio[0]=nuova_posizione[1]+1;
						personaggio[1]=nuova_posizione[2];
						personaggio[2]=' ';
						mappa[nuova_posizione[1]+1][nuova_posizione[2]]='O';
						pthread_mutex_unlock(&mymutex);//sblocco il mutex
					}
				}
			}


			//caso in cui ho vinto
			if(vittoria==1){
				send_socket(conn,"v",1);
				pthread_mutex_lock(&mymutex);//blocco il mutex
				dealloca(Ut);
				Ut=NULL;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}

		
			//se non ho vinto , e il tempo è in corso
			else{
				if(giottatadellanno==0){
					send_socket(conn,personaggio,3);
					
				}	
			}
			break;






//_________CASO SINISTRA_____________________________________

			case 'a':

				if(read(conn,&id_partita_client,sizeof(int))<0){ 
					error("Errore read");
				}
				if(id_partita!=id_partita_client){
					send_socket(conn,"f",1);
					giottatadellanno=1;
					flag=0;
				}
				

				else{
				if((nuova_posizione[2]-1)<0){
					personaggio[0]=nuova_posizione[1];
					personaggio[1]=nuova_posizione[2];
				}
				else{
					if(mappa[nuova_posizione[1]][nuova_posizione[2]-1]=='#'){
						
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
						personaggio[2]='#';
					}
					else if(mappa[nuova_posizione[1]][nuova_posizione[2]-1]=='O'){
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
						personaggio[2]='O';
					}

					
					else{
						pthread_mutex_lock(&mymutex);//blocco il mutex
						if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
							mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
						}
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2]-1;
						personaggio[2]=' ';
						mappa[nuova_posizione[1]][nuova_posizione[2]-1]='O';
						pthread_mutex_unlock(&mymutex);//sblocco il mutex
					}	
				}
			}


			//caso in cui ho vinto
			if(vittoria==1){
				send_socket(conn,"v",1);
				pthread_mutex_lock(&mymutex);//blocco il mutex
				dealloca(Ut);
				Ut=NULL;
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}

		
			//se non ho vinto , e il tempo è in corso
			else{
				if(giottatadellanno==0){
					send_socket(conn,personaggio,3);
					
				}	
			}
			break;






//_________CASO DESTRA____________________________________

			case 'd':

				if(read(conn,&id_partita_client,sizeof(int))<0){ 
					error("Errore read");
				}
				if(id_partita!=id_partita_client){
					send_socket(conn,"f",1);
					giottatadellanno=1; //Non è possibile giocare
					flag=0;
				}
				

				else{

				if((nuova_posizione[2]+1)>19){
					personaggio[0]=nuova_posizione[1];
					personaggio[1]=nuova_posizione[2];
				}
				if(nuova_posizione[2]==19){
						vittoria=1;
						}

				else{
					
			
					if(mappa[nuova_posizione[1]][nuova_posizione[2]+1]=='#'){
						
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
						personaggio[2]='#';
					}
					else if(mappa[nuova_posizione[1]][nuova_posizione[2]+1]=='O'){
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2];
						personaggio[2]='O';
					}

					
					else{
						pthread_mutex_lock(&mymutex);//blocco il mutex
						if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
							mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
						}
						personaggio[0]=nuova_posizione[1];
						personaggio[1]=nuova_posizione[2]+1;
						personaggio[2]=' ';
						mappa[nuova_posizione[1]][nuova_posizione[2]+1]='O';
						pthread_mutex_unlock(&mymutex);//sblocco il mutex
					}
				}
			}


			//caso in cui ho vinto
			if(vittoria==1){
				send_socket(conn,"v",1);
				pthread_mutex_lock(&mymutex);//blocco il mutex
				dealloca(Ut);
				Ut=NULL;
				flag=0; //termina partita x tutti
				printf("ABBIAMO UN VINCITORE (partita termina qui)");
				pthread_mutex_unlock(&mymutex);//sblocco il mutex
			}

		
			//se non ho vinto , e il tempo è in corso
			else{
				if(giottatadellanno==0){ //Basta che un client entri nella partita per giocare, non deve necessariamente aspettare gli altri
					send_socket(conn,personaggio,3);
					
					}
				}	
			
			break;





//_________CASO LOGOUT _____________________________________

			case 'c':
				
				DataNow(data);
				pthread_mutex_lock(&mymutex);//blocca il mutex
				int fileLog_loc=fdlog;
				if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
					mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
				}
				Ut=EliminaScollegato(Ut,Nick);
				char messaggio[256];
				sprintf(messaggio,"Utente %s Disconnesso in Data %s\n",Nick,data);
				stampaLog(fileLog_loc,messaggio);
				pthread_mutex_unlock(&mymutex);//sblocca il mutex
				flag=0;
			break;

			

			case 't':
				pthread_mutex_lock(&mymutex);//blocca il  mutex
				int tempo_loc=tempoGioco;
				pthread_mutex_unlock(&mymutex);//sblocca il mutex
				if(write(conn,&tempo_loc,sizeof(int))<0){
					error("Errore write\n");	
				}
			break;




				

			



			default :
				flag=0;
			break;
		}


	  }

		
		
	}while(flag==1 ); 


//___________________FINE DO WHILE ENORME______________________________________________________________________



	pthread_mutex_lock(&mymutex);
	if(mappa[nuova_posizione[1]][nuova_posizione[2]]=='O'){
		mappa[nuova_posizione[1]][nuova_posizione[2]]=' ';
	}
	utenti_che_giocano--;

	pthread_mutex_unlock(&mymutex);

	Ut=EliminaScollegato(Ut,Nick);

	if(vittoria==1){
		if(pthread_create(&tredServer,NULL,GeneraMapp,NULL) < 0){
        	error("Errore Thread\n");
        }
	}
}




//______________________________________________________________________________________________________________________________________________________
								//END GAME//
//______________________________________________________________________________________________________________________________________________________











void stampaMappa(int fd,char mappa[][20])
{
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



//______________________________________________________________________________________________________________________________________________________



void *GeneraMapp(void *ptr)
{	


	int i=0, j=0, k=0;
	
	int n_ostacoli=8;  //N BOMBE

	
	int fdlog_loc;


	int i0,j0,i1,j1,i2,j2,i3,j3;

	int posizionato=0;

	int cordinata1=0,cordinata2=0;

	char data[50], newmappa[50];
	srand(time(NULL));
	time_t start=0,end=0;
	char messId[50];


	pthread_mutex_lock(&mymutex);
	id_partita++;//aumenta id partita
	sprintf(messId,"Inizia la partita numero ( %d )\n",id_partita);
	
	
	//nonvittoria=0;
	flag_vittoria=0;
	fdlog_loc=fdlog;
	pthread_mutex_unlock(&mymutex);

	//inizializzazione della matrice
	for(i=0; i<20; i++){
		for(j=0; j<20; j++){ 
			mappa[i][j]=' ';
		}
	}

	//posizonamento delle BOMBE 
	for(i=0; i<8; i++){
		cordinata1=1+rand()%18;
		cordinata2=1+rand()%18;
		if(mappa[cordinata1][cordinata2]==' '){
			mappa[cordinata1][cordinata2]='#';
		}
	}




	ismapp=1;

	sprintf(newmappa,"Nuova mappa generata in data %s\n",data);
	stampaLog(fdlog_loc,newmappa);
	stampaMappa(fdlog_loc,mappa);

	DataNow(data);
	tempoGioco=0;
	do{
		sleep(1);
		tempoGioco++;
	}while(tempoGioco<TEMPO && flag_vittoria==0);

	ismapp=0;

pthread_exit(NULL);
}



//______________________________________________________________________________________________________________________________________________________



void aggiungi_utente(struct utente **top,char *Nick,  pid_t pidl ){
	
		struct utente *tmp=(struct utente *)calloc(1,sizeof(struct utente)); // creo temp
		strcpy(tmp->nick,Nick); 
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





//______________________________________________________________________________________________________________________________________________________



struct utente *EliminaScollegato(struct utente *top,char *Nick){
	if(top!=NULL){
		if(strcmp(top->nick,Nick)==0){
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




//______________________________________________________________________________________________________________________________________________________


int TrovaGiaOnline(char *nick ,struct utente *top){
	int flag=0;
	if(top!=NULL){
		if(strcmp(top->nick,nick)==0){
			flag=1;
		}else{
			flag=TrovaGiaOnline(nick,top->next);
		}
	}
return flag;
}





//______________________________________________________________________________________________________________________________________________________


void stampaUtentiLoggati(struct utente *top){
	if(top!=NULL){
		sprintf("%s in posizione [%d][%d]",top->nick,top->x,top->y);
		printf("\n");
		stampaUtentiLoggati(top->next);
	}else{
		printf("\nFine Lista\n");
	}
}

//______________________________________________________________________________________________________________________________________________________


void stringaUtentiLoggati(struct utente *top,char* stringaloggati){
	if(top!=NULL){
		sprintf(stringaloggati,"%d%d",top->x,top->y);
		printf("\n");
		stringaUtentiLoggati(top->next,stringaloggati);
	}
}


//______________________________________________________________________________________________________________________________________________________



void dealloca(struct utente *top){
	if(top!=NULL){
		dealloca(top->next);
		free(top);
	}
}

//______________________________________________________________________________________________________________________________________________________



void AggiungiCoordinate(struct utente *top,char *Nick,int i,int j){
	if(top!=NULL){
		if(strcmp(top->nick,Nick)==0){
			top->x=i;
			top->y=j;
		}else{
			AggiungiCoordinate(top->next,Nick,i,j);
		}
	}
}


//______________________________________________________________________________________________________________________________________________________



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


//______________________________________________________________________________________________________________________________________________________



void handlerpipe(int x)
{
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
		sprintf(buffer,"\t%s\tchiusura anomala\t%s\n",utente_local->nick,asctime(localtime(&ora)));
		pthread_mutex_lock(&mymutex);//sblocco il mutex
		Ut=EliminaScollegato(Ut,utente_local->nick);
		pthread_mutex_unlock(&mymutex);
	}
pthread_exit(status);
}


//______________________________________________________________________________________________________________________________________________________



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



//______________________________________________________________________________________________________________________________________________________



//____________________________________________________________________________________________________









