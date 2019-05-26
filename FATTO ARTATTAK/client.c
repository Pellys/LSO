

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
#define BLU  	"\x1B[1;34m"
#define tempoTot 120

void stampa(char *messaggio);
void menu();
void cleanBuffer();
void error(char *messaggio);
void stampaMappa(char A[][20],int,int,int,char *);
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
void mandaSegnale(); //manda segnale k per avere le coordinate
int giocato=0;
int ingame=0;


int connessione_flag=0;
int flag_posizione1=0;
int flag_posizione2=0;


char stringa[50];






//___________________________________________________MAIN______________________________________________________________-



int main(int argc, char const *argv[])
{
      
        int bytes_recieved, uscita=0,sock;
    
    
    
        char Login[1024],recv_data[1024], risposta;
        struct hostent *host;
        struct sockaddr_in server_addr;  
        int i=0, j=0, bytesnickname=0, bytepassword=0, inserito=0;
    
    //carattere per selezionare un opzione del menu
        char scelta;
    
    
    //dati dell'utente
        char Nickname[256], Password[256], Log[512];

    
    //da errore se il programma in input non riceve 3 parametri
    // 0. NOME DEL PROGRAMMA
    // 1. indirizzo ip
    // 2. numero di porta
        if(argc!=3){
        	error(RED"Error : Man = ./client ip(xxx.xxx.xxx) port(5XXX)\n"RESET);
        }
    
    
    
    // nella variabile host
        host = gethostbyname(argv[1]);

    
    
        if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
        	error(RED"ERROR : Socket\n"RESET);
        }

       	server_addr.sin_family = AF_INET;     
       	server_addr.sin_port = htons(atoi(argv[2]));  //SALVA NUMERO DI PORTA
       	server_addr.sin_addr = *((struct in_addr *)host->h_addr); //INDIRIZZO
        bzero(&(server_addr.sin_zero),8); 

    
    //ERRORE DI CONNESSIONE
        if(connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        	error(RED"ERROR : Connect\n"RESET);
        }
    
    
    
        connessione_flag=sock; //Sock = dati di connessione (ip,porta ecc) salvati in variabile
    
    
        system("clear");

    
    
    
    
    
    //_____________________________________START DO WHILE_____________________________________//

    //all'inizio uscita è == 0
    // viene settata a 1 solo se premo 'c' quindi LOGOUT
    
    
    
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

                  
       //BACKGROUND DEL MENU
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
		
                  //errore input
        	if(read(1,&scelta,1)<0){
        		error("error write\n");
        	}
                  
                  
//______A_______creazione account_____________
                  
        	if(scelta=='a'){
        		cleanBuffer();

        		stampa(BLU"Inserire Nickname e Password\n"RESET);
        		stampa("Nickname : ");

              //salva il numero di byte del nickname nella variabile bytesnickname
        		if((bytesnickname=read(1,Nickname,256))<0){
        			error("Errore write\n");
				}
        		Nickname[bytesnickname-1]='\0';

        		stampa("\nPassword : ");
                
                //salva numero di byte della password nella variabile bytepassword
        		if((bytepassword=read(1,Password,256))<0){
        			error("Errore write\n");
        		}
        		Password[bytepassword-1]='\0';
      			
                // salva nick e password nelle rispettive variabili
        		sprintf(Login,"a%s:%s\n",Nickname,Password);

                //errore
        		if(write(sock,Login,strlen(Login))<0){
        			error("Errore WRITE\n");
        		}
        		
                //legge da sock (pipe fra server e client)
        		if(read(sock,&risposta,1)<0){
        			error("Error Read\n");
        		}
                //in base alla risposta del server
        		switch(risposta){
        			case 'y': stampa(GREEN"Registrazione avvenuta con Successo, verrai indirizzato alla schermata iniziale\n"RESET);pauseT(); system("clear");break;
        			case 'n': stampa(RED"Il Nickname è gia stato usato, verrai indirizzato alla schermata iniziale\n"RESET); pauseT(); system("clear"); break;
        			default : error("Impossibile comunicare con il server\n"); break; 
        		}
        		
        	}
                  
                  
                  
//______B_______Login_____________
                  
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
        				gioco(sock); //PARTE THE GAMEEEE
        				break;

        			case 'n': stampa(RED"Login fallito utente non registrato\n"RESET); pauseT(); system("clear");
        				break;

        			case 'g': stampa(RED"Login fallito utente già Online\n"RESET); pauseT(); system("clear");
        				break;

        			default : stampa("Impossibile comunicare con il server\n");
        				break; 
        			}
        	}
                  
                   // LOGOUT
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
    }while(uscita == 0);
    
    
    
    
    
    //_____________________________________END DO WHILE_____________________________________//
   
    
    

close(sock);
cleanBuffer();
return 0;
}

//_____________________________________________________FINE MAIN____________________________________________________






//FUNZIONE CHE MOSTRA IL FRONTEND DEL MENU____________________________________
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
	stampa("  PROGETTO ANNO 2018/2019\n"RESET);
	stampa(BLU"\nAlberto Panzera N86002772\nChiara Pellecchia N86002277\n\n"RESET);
	stampa("************************\n");
	stampa("*   A) Crea un Account     *\n");
	stampa("*   B) Login           *\n");
	stampa("*   C) Logout            *\n");
	stampa("************************\n");

}

//___________________________________________________________________________________




void stampa(char *messaggio)
{
	int l=strlen(messaggio);
	if(write(0,messaggio,l)<0){
		error("Error write\n");
	}
}


//___________________________________________________________________________________



void cleanBuffer()
{
	char ch;
    while((ch=getchar())!='\n');
}

//___________________________________________________________________________________


void error(char *messaggio)
{
	perror(messaggio);
	exit(1);
}















//______________________________________________________________________________________________________________________________________________________
//                                 T H E   G A M E
//______________________________________________________________________________________________________________________________________________________


void gioco(int sock)
{	
	int i=0, j=0;
	int k=0, vittoria=0;
	char coordinate[3], cambia_posizione[3], y=0;
	char mappa[20][20], lista[5000], prova;
	char messaggio[256];
	int id_corrente, id_server;
	char avversari[300];

	//leggere qui l'id della partita dal server
	if(read(sock,&id_corrente,sizeof(int))<0){
		error("errore read\n");
	}
	//fine lettura primo id

	if(read(sock,coordinate,3)<0){
		error("errore read\n");
	}
	
    //Inizializzo mappa con celle vuote

	for(i=0;i<20;i++){
		for(j=0; j<20; j++){
			mappa[i][j]='-';
		}
	}

	i=coordinate[0];
	j=coordinate[1];
    
	mappa[i][j]=coordinate[2];

	//
	// stampaMappa con nuovo thread 
	//
	
	system("clear");
	stampaMappa(mappa,i,j,id_corrente,NULL);

	do{	
		
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
        
        //________________________azione del movimento_____________________________________
        
		switch(cambia_posizione[0]){

			case 'w': 

				cambia_posizione[1]=i;
				cambia_posizione[2]=j; 
				send_socket(sock,cambia_posizione,3);

				//write id del client verso il server
				if(write(sock,&id_corrente,sizeof(int))<0){
					error("Errore write");
				}

				if(read(sock,coordinate,3)<0){//legge la risposta del server
					error(RED"Error Read\n"RESET);
					ingame=0; giocato=0;
				}
				if(coordinate[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					ingame=0;
					giocato=1;

				}
				if(coordinate[2]=='#'){ //bomba trovata (perdo)
					stampaSconfitta();
					sleep(1);
					ingame=0;
					vittoria=0;
					giocato=1;
				}

				else if(coordinate[2]=='O'){
					i=coordinate[0]; j=coordinate[1];
						mappa[i-1][j]='u';
				}

				else{
					mappa[i][j]=' ';
					i=coordinate[0]; j=coordinate[1];
					mappa[i][j]='O';


					//OOOOOOOOOOOOOOOOOOOOOOOOOOOO
			for(k=0;k<5000;k++){
        			lista[k]='\0';
        		}
        		send_socket(sock,"k",1);
        		if(read(sock,lista,5000)<0){
        			error("error read\n");
        		}



				//trasforma lista in coordinate
				char A[5000], inseriti=0, giot=0;

				for(giot=0; giot<strlen(lista); giot++){
					
					if(lista[giot]!='?' && lista[giot]!='&'){
						A[inseriti]=lista[giot];
						inseriti++;
					}
				}
				A[inseriti]=-1;

        		stampaMappa(mappa,i,j,id_corrente,A);
				


			}
			//OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO	

		

			break;

                
//Sposto giu___________
                
                
			case 's':
                
                cambia_posizione[1]=i;
                cambia_posizione[2]=j;
                
				send_socket(sock,cambia_posizione,3);

				//invio dell id al server
				
                		if(write(sock,&id_corrente,sizeof(int))<0){
					error("Errore write");
				}

				if(read(sock,coordinate,3)<0){
					error(RED"Error Read\n"RESET);
					ingame=0; giocato=0;
				}
				if(coordinate[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					ingame=0;
					giocato=1;

				}


					if(coordinate[2]=='#'){ //bomba trovata (perdo)
					stampaSconfitta();
					sleep(1);
					ingame=0;
					vittoria=0;
					giocato=1;
					}


					else if(coordinate[2]=='O'){
						i=coordinate[0]; j=coordinate[1];
						mappa[i+1][j]='u';
					}

					else{
						mappa[i][j]=' ';
						i=coordinate[0]; j=coordinate[1];
						mappa[i][j]='O';
			for(k=0;k<5000;k++){
        			lista[k]='\0';
        		}
        		send_socket(sock,"k",1);
        		if(read(sock,lista,5000)<0){
        			error("error read\n");
        		}



				//trasforma lista in coordinate
				char A[5000], inseriti=0, giot=0;

				for(giot=0; giot<strlen(lista); giot++){
					
					if(lista[giot]!='?' && lista[giot]!='&'){
						A[inseriti]=lista[giot];
						inseriti++;
					}
				}
				A[inseriti]=-1;

        		stampaMappa(mappa,i,j,id_corrente,A);
		}


			break;

                
//Sposto sinistra___________

			case 'a': 

				cambia_posizione[1]=i;
				cambia_posizione[2]=j; 
				send_socket(sock,cambia_posizione,3);

				//invio dell id al server
				if(write(sock,&id_corrente,sizeof(int))<0){
					error("Errore write");
				}

				if(read(sock,coordinate,3)<0){
					error(RED"Error Read\n"RESET);
					ingame=0; giocato=0;
				}
				if(coordinate[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					ingame=0;
					giocato=1;

				}

					if(coordinate[2]=='#'){ //bomba trovata (perdo)
					stampaSconfitta();
					sleep(1);
					ingame=0;
					vittoria=0;
					giocato=1;
					}


					else if(coordinate[2]=='O'){
						i=coordinate[0]; j=coordinate[1];
						mappa[i][j-1]='u';
					}

					else{
						mappa[i][j]=' ';
						i=coordinate[0]; j=coordinate[1];
						mappa[i][j]='O';

for(k=0;k<5000;k++){
        			lista[k]='\0';
        		}
        		send_socket(sock,"k",1);
        		if(read(sock,lista,5000)<0){
        			error("error read\n");
        		}



				//trasforma lista in coordinate
				char A[5000], inseriti=0, giot=0;

				for(giot=0; giot<strlen(lista); giot++){
					
					if(lista[giot]!='?' && lista[giot]!='&'){
						A[inseriti]=lista[giot];
						inseriti++;
					}
				}
				A[inseriti]=-1;

        		stampaMappa(mappa,i,j,id_corrente,A);
					}


			break;

                
//Sposto destra___________

			case 'd': 

				cambia_posizione[1]=i;
				cambia_posizione[2]=j; 
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
				}

				else if(coordinate[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					ingame=0;
					giocato=1;

				}

					else if(coordinate[2]=='#'){ //bomba trovata (perdo)
					stampaSconfitta();
					sleep(1);
					ingame=0;
					vittoria=0;
					giocato=1;
					}


					else if(coordinate[2]=='O'){
						i=coordinate[0]; j=coordinate[1];
						mappa[i][j+1]='u';
					}

					else{
						mappa[i][j]=' ';
						i=coordinate[0]; j=coordinate[1];
						mappa[i][j]='O';

for(k=0;k<5000;k++){
        			lista[k]='\0';
        		}
        		send_socket(sock,"k",1);
        		if(read(sock,lista,5000)<0){
        			error("error read\n");
        		}



				//trasforma lista in coordinate
				char A[5000], inseriti=0, giot=0;

				for(giot=0; giot<strlen(lista); giot++){
					
					if(lista[giot]!='?' && lista[giot]!='&'){
						A[inseriti]=lista[giot];
						inseriti++;
					}
				}
				A[inseriti]=-1;

        		stampaMappa(mappa,i,j,id_corrente,A);
					}




			break;

                
                
//ESEGUO LOGOUT (MAGGJ RUTT O CAZZ)___________

			case 'c': stampa(YELLOW"Stai per uscire dal gioco e tornare al menu iniziale\n"RESET);
				cambia_posizione[1]=i;cambia_posizione[2]=j;
				send_socket(sock,cambia_posizione,3);
			    sleep(1); ingame=0; giocato=0;
		    break;


                
//RICHIEDO INFO TEMPO _________________________

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



void stampaMappa(char mappa[][20],int pos1, int pos2,int id_corrente,char *otherplayers)
{
	int i=0, j=0;
	char messaggio[256];
	char flag[256];

		//pauseT();
	for(i=0;i<20;i++){
		for(j=0; j<20; j++){
			if(i==pos1 && j==pos2);
			else
			mappa[i][j]='-';
		}
	}

	
	if(otherplayers!=NULL){
		int otherplayersIndex = 0;
		for(otherplayersIndex=0; otherplayersIndex<(strlen(otherplayers)); otherplayersIndex+=2){
			i=(int)otherplayers[otherplayersIndex]-'0';
			j=(int)otherplayers[otherplayersIndex+1]-'0';
			mappa[i][j]='O';
			//system("clear");
			printf("%d %d ",i,j);
		}

		
	}
	

	

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
	
			if(mappa[i][j]=='O'  && (i==pos1) && (j==pos2)){
				stampa(BLU"0"RESET);
			}
			else if(mappa[i][j]=='O'  && (i!=pos1) && (j!=pos2)){
				stampa("O");
			}
			else if(mappa[i][j]=='#'){
				stampa(RED"#"RESET);
			}
			else{
				write(0,&mappa[i][j],1);
			}
			if(j==19){
				write(0," ",sizeof(" "));
			}else{
				write(0,"  ",sizeof("  "));
			}
		}
		if(i==13){
			write(0,"|\tw) Sposta Su\n",sizeof("|\tw) Sposta Su\n"));
		}else if(i==14){
			write(0,"|\ta) Sposta Sinistra\n",sizeof("|\ta) Sposta Sinistra\n"));
		}else if(i==15){
			write(0,"|\ts) Sposta Giu'\n",sizeof("|\ts) Sposta Giu'\n"));
		}else if(i==16){
			write(0,"|\td) Move Destra\n",sizeof("|\tw) Move Destra\n"));
		}else if(i==17){
			write(0,"|\ti) Info Utente\n",sizeof("|\ti) Info Utente\n"));
		}else if(i==18){
			write(0,"|\tt) Tempo\n",sizeof("|\tt) Tempo\n"));
		}else if(i==19){
			write(0,"|\tc) Esegui Logout\n",sizeof("|\tc) Esegui Logout\n"));
		}else{
			write(0,"|\n",sizeof("|\n"));
		}
	}
	write(0,"\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n",sizeof("\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"));
	sprintf(messaggio,YELLOW"Ti trovi in Posizione [%d][%d] (Stai partecipando alla partita ID: [%d])\n"RESET,pos1,pos2,id_corrente);
	stampa(messaggio);



}






// ___________________________________________ E N D __ G A M E __________________________________________ //






void send_socket(int conn,char *mess,int nbytes)
{
	if(write(conn,mess,nbytes)<0){
		error("Errore write\n");
	}
}
void sighandler(int segnale_ricevuto)	/* gestione del segnale */
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
	stampa("* ALLAHU AKBAR BOOOOM hai perso !!! *\n");
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


void mandaSegnale(int conn){

	send_socket(conn,"k",1); //ogni volta che stampa la mappa manda 
		//il segnale per le coordinate

}
