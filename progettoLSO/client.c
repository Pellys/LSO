/* client.c */

//librerie
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
//colori
#define CYANO   "\x1B[1;36m"
#define YELLOW  "\x1B[1;33m"
#define RED     "\x1b[1;31m"
#define GREEN   "\x1b[1;32m"
#define RESET   "\x1b[0m"
#define BLU  	"\x1B[1;34m"
#define tempoTotale 120

//funzioni
void stampa(char *messaggio);
void menu();
void pulisciBuffer();
void errore(char *messaggio);
void stampaMappa(char A[][20],int,int,int,char *);
void gioco(int sock);
void sendSocket(int conn,char *mess,int nbytes);
void sighandler(int segnale_ricevuto);
void sighandler_1(int segnale_ricevuto);
void sighandler_2(int segnale_ricevuto);
void sighandler_3(int segnale_ricevuto);
void sighandler_4(int segnale_ricevuto);
void attivaVittoria();
void attivaSconfitta();
void pausaGioco();
int giocato=0;
int inGioco=0;

//variabili globali
int flagConnessione=0;
int flagPosizione1=0;
int flagPosizione2=0;

//inizio Main
int main(int argc, char const *argv[]){
      
    int bytes_recieved, uscita=0,sock;
    char Login[1024], risposta;
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
        errore(RED"Error : Man = ./client ip(xxx.xxx.xxx) port(5XXX)\n"RESET);
    }
    
    // nella variabile host
    host = gethostbyname(argv[1]);

    if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
        errore(RED"ERROR : Socket\n"RESET);
    }

       	server_addr.sin_family = AF_INET;     
       	server_addr.sin_port = htons(atoi(argv[2]));  //SALVA NUMERO DI PORTA
       	server_addr.sin_addr = *((struct in_addr *)host->h_addr); //INDIRIZZO
        bzero(&(server_addr.sin_zero),8);
    
    //errore di connessione
    if(connect(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
        errore(RED"ERROR : Connect\n"RESET);
    }
    
    flagConnessione=sock; //Sock = dati di connessione (ip,porta ecc) salvati in variabile
    system("clear");

    //inizio ciclo do-while
    //all'inizio uscita è == 0
    // viene settata a 1 solo se premo 'c' quindi LOGOUT
    
    do{
        
        if(giocato==1){
            sleep(1);
            
            sprintf(Login,"b%s:%s\n",Nickname,Password);
            if(write(sock,Login,strlen(Login))<0){
                errore("Errore WRITE\n");
            }
        		
            if(read(sock,&risposta,1)<0){
                errore("Error Read\n");
            }
            inGioco=1;
            gioco(sock);
        }
        else{

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
        		errore("errore write\n");
        	}
            
//creazione account
                  
        	if(scelta=='a'){
        		pulisciBuffer();
        		stampa(BLU"Inserire Nickname e Password\n"RESET);
        		stampa("Nickname : ");
                
              //salva il numero di byte del nickname nella variabile bytesnickname
        		if((bytesnickname=read(1,Nickname,256))<0){
        			errore("Errore write\n");
				}
                
        		Nickname[bytesnickname-1]='\0';
        		stampa("\nPassword : ");
                
                //salva numero di byte della password nella variabile bytepassword
        		if((bytepassword=read(1,Password,256))<0){
        			errore("Errore write\n");
        		}
        		Password[bytepassword-1]='\0';
      			
                // salva nick e password nelle rispettive variabili
        		sprintf(Login,"a%s:%s\n",Nickname,Password);

                //errore
        		if(write(sock,Login,strlen(Login))<0){
        			errore("Errore WRITE\n");
        		}
        		
                //legge da sock (pipe fra server e client)
        		if(read(sock,&risposta,1)<0){
        			errore("Error Read\n");
        		}
                
                //in base alla risposta del server
        		switch(risposta){
        			
                    case 'y':
                        stampa(GREEN"Registrazione avvenuta con Successo, verrai indirizzato alla schermata iniziale\n"RESET);pausaGioco(); system("clear");
                    break;
        			
                    case 'n':
                        stampa(RED"Il Nickname è gia stato usato, verrai indirizzato alla schermata iniziale\n"RESET); pausaGioco(); system("clear");
                    break;
        			
                    default :
                        errore("Impossibile comunicare con il server\n");
                    break;
        		}
        	} // fine scelta 'a'
                  
            
            //scelta b = login
        	else if(scelta=='b'){
        		pulisciBuffer();

        		stampa(BLU"Inserire Nickname e Password Per Il Login\n"RESET);
        		stampa("Nickname : ");

        		if((bytesnickname=read(1,Nickname,256))<0){
        			errore("Errore write\n");
				}
        		Nickname[bytesnickname-1]='\0';

        		stampa("\nPassword : ");
        		if((bytepassword=read(1,Password,256))<0){
        			errore("Errore write\n");
        		}
        		Password[bytepassword-1]='\0';
      
        		sprintf(Login,"b%s:%s\n",Nickname,Password);
                
        		if(write(sock,Login,strlen(Login))<0){
        			errore("Errore WRITE\n");
        		}
                
        		if(read(sock,&risposta,1)<0){
        			errore("Error Read\n");
        		}
        		printf("Ho ricevuto la risposta dal server\n");

                
                //switch in base alla risposta del server
        		switch(risposta){
        			case 'y':
                        system("clear"); stampa(GREEN"Adesso puoi iniziare a giocare!\n"RESET);
        				inGioco=1;
        				gioco(sock); //PARTE THE GAMEEEE
                    break;

        			case 'n':
                        stampa(RED"Login fallito utente non registrato\n"RESET); pausaGioco(); system("clear");
                    break;

        			case 'g':
                        stampa(RED"Login fallito utente già Online\n"RESET); pausaGioco(); system("clear");
                    break;

        			default :
                        stampa("Impossibile comunicare con il server\n");
                    break;
                }
        	} //fine sceta 'b'
            
            
             // sceta c = logout
        	else if(scelta=='c'){
        		stampa(YELLOW"Chiusura Del Client In Corso\n"RESET);
        		sleep(1);

        		if(write(sock,"c",1)<0){
        			errore("errore write\n");
        		}
        		uscita=1;
        	} //fine scelta c
            
            //qualsiasi altro caso
        	else{
        		system("clear");
        		pulisciBuffer();
        		stampa(RED"Error input riprova\n"RESET);		
        	}

        } // fine else
    }while(uscita == 0); // fine ciclo
    
    
close(sock);
pulisciBuffer();
return 0;
} // fine main

//___________________________________________________________________________________

// funzione frontend menu
void menu(){
	system("clear");
	stampa(GREEN);
    stampa("  PROGETTO ESAME 'Lab Sistemi Operativi \n  Docente : Alberto Finzi  \nANNO 2018/2019\n"RESET);
	stampa(BLU"\nAlberto Panzera N86002772\nChiara Pellecchia N86002277\n\n"RESET);
	stampa("\n\n");
    stampa("______________________\n");
    stampa("    MENU DI GIOCO     \n");
    stampa("-----------------------\n\n");
	stampa(" a) Crea un Account  \n");
	stampa(" b) Login            \n");
	stampa(" c) Logout           \n");
	stampa("-----------------------\n");
}

//___________________________________________________________________________________

void stampa(char *messaggio){
	int l=strlen(messaggio);
	if(write(0,messaggio,l)<0){
		errore("Error write\n");
	}
}

//___________________________________________________________________________________

void pulisciBuffer(){
	char ch;
    while((ch=getchar())!='\n');
}

//___________________________________________________________________________________

void errore(char *messaggio){
	perror(messaggio);
	exit(1);
}

//___________________________________________________________________________________
//funzione gioco (funzione principale)

void gioco(int sock){
	int i=0, j=0;
	int k=0, vittoria=0;
	char miaPosizione[3], NuovaPosizione[3], y=0;
	char mappa[20][20], lista[5000], prova;
	char messaggio[256];
	int id_partita_corrente;
	char online[5000];
	int tempo=0;		
	char tempo_loc[256];

	//leggere qui l'id della partita dal server
	if(read(sock,&id_partita_corrente,sizeof(int))<0){
		errore("errore read\n");
	}
	//fine lettura primo id
	if(read(sock,miaPosizione,3)<0){
		errore("errore read\n");
	}
    
    //Inizializzo mappa con celle vuote
	for(i=0;i<20;i++){
		for(j=0; j<20; j++){
			mappa[i][j]='-';
		}
	}

	i=miaPosizione[0];
	j=miaPosizione[1];
	mappa[i][j]=miaPosizione[2];


	system("clear");
	stampaMappa(mappa,i,j,id_partita_corrente,NULL);
	do{
		signal(SIGINT,sighandler);
		signal(SIGQUIT,sighandler);
		signal(SIGHUP,sighandler_3);
		signal(SIGSTOP,sighandler_3);
		signal(SIGTERM,sighandler_3);
		signal(SIGTSTP,sighandler_3);
		signal(SIGPIPE,sighandler_4);
		signal(SIGABRT,sighandler_3);
		flagPosizione1=i;
		flagPosizione2=j;
        
		if(read(1,&NuovaPosizione[0],1)<=0){
			errore("errore read\n");
		}
		pulisciBuffer();
		int z=0;
		for(z=0; z<256; z++){
			messaggio[z]='\0';
		}
        
        //azioni del gioco in corso
		switch(NuovaPosizione[0]){

                
                // Sposto su
            case 'w':
				NuovaPosizione[1]=i;
				NuovaPosizione[2]=j;
				sendSocket(sock,NuovaPosizione,3);
				//write id del client verso il server
				if(write(sock,&id_partita_corrente,sizeof(int))<0){
					errore("Errore write");
				}
                //legge la risposta del server
				if(read(sock,miaPosizione,3)<0){
					errore(RED"Error Read\n"RESET);
					inGioco=0; giocato=0;
				}
                
				if(miaPosizione[0]=='f') { //Tempo scaduto o vince un altro
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					inGioco=0;
					giocato=1;
				}

				
                //bomba trovata (perdo)
				if(miaPosizione[2]=='#'){
					attivaSconfitta();
					sleep(1);
					inGioco=0;
					vittoria=0;
					giocato=1;
				}
                
				else{
					mappa[i][j]=' ';
					i=miaPosizione[0]; j=miaPosizione[1];
					mappa[i][j]='O';

                    for(k=0;k<5000;k++){
        			lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        errore("errore read\n");
                    }

        		stampaMappa(mappa,i,j,id_partita_corrente,lista);
				
                }
                break;

                
                // Sposto giu
			case 's':
                
                NuovaPosizione[1]=i;
                NuovaPosizione[2]=j;
				sendSocket(sock,NuovaPosizione,3);

				//invio dell id al server
                if(write(sock,&id_partita_corrente,sizeof(int))<0){
					errore("Errore write");
				}
                
				if(read(sock,miaPosizione,3)<0){
					errore(RED"Error Read\n"RESET);
					inGioco=0; giocato=0;
				}
				if(miaPosizione[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					inGioco=0;
					giocato=1;
				}

                if(miaPosizione[2]=='#'){ //bomba trovata (perdo)
                    attivaSconfitta();
                    sleep(1);
                    inGioco=0;
                    vittoria=0;
		    giocato=1;
                }

                else{
                    mappa[i][j]=' ';
                    i=miaPosizione[0]; j=miaPosizione[1];
                    mappa[i][j]='O';
                    
                    for(k=0;k<5000;k++){
                        lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        errore("errore read\n");
                    }

                    stampaMappa(mappa,i,j,id_partita_corrente,lista);
                    }
			break;

                
                // Sposto sinistra
			case 'a': 

				NuovaPosizione[1]=i;
				NuovaPosizione[2]=j;
				sendSocket(sock,NuovaPosizione,3);

				//invio dell id al server
				if(write(sock,&id_partita_corrente,sizeof(int))<0){
					errore("Errore write");
				}
				if(read(sock,miaPosizione,3)<0){
					errore(RED"Error Read\n"RESET);
					inGioco=0; giocato=0;
				}
				if(miaPosizione[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					inGioco=0;
					giocato=1;
				}
                if(miaPosizione[2]=='#'){ //bomba trovata (perdo)
					attivaSconfitta();
					sleep(1);
					inGioco=0;
					vittoria=0;
					giocato=1;
                }

                else{
                    mappa[i][j]=' ';
                    i=miaPosizione[0]; j=miaPosizione[1];
                    mappa[i][j]='O';
                    
                    for(k=0;k<5000;k++){
                        lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        errore("errore read\n");
                    }
				
                    stampaMappa(mappa,i,j,id_partita_corrente,lista);
		}
			break;

                
                // Sposto destra
			case 'd':
				NuovaPosizione[1]=i;
				NuovaPosizione[2]=j;
				sendSocket(sock,NuovaPosizione,3);
                
				//invio dell id al server
				if(write(sock,&id_partita_corrente,sizeof(int))<0){
					errore("Errore write");
				}
				if(read(sock,miaPosizione,3)<0){
					errore(RED"Error Read\n"RESET);
					inGioco=0; giocato=0;
				}
                
				if(miaPosizione[0]=='v'){
					attivaVittoria();
					sleep(1);
					inGioco=0;
					giocato=1;
				}
				else if(miaPosizione[0]=='f') {
					system("clear");
					stampa(RED"PARTITA TERMINATA"RESET);
					inGioco=0;
					giocato=1;

				}
                else if(miaPosizione[2]=='#'){ //bomba trovata (perdo)
					attivaSconfitta();
					sleep(1);
					inGioco=0;
					giocato=1;
                }

                else{
                    mappa[i][j]=' ';
                    i=miaPosizione[0]; j=miaPosizione[1];
                    mappa[i][j]='O';
                    for(k=0;k<5000;k++){
        			lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        errore("errore read\n");
                    }
					
        		stampaMappa(mappa,i,j,id_partita_corrente,lista);
			}
			break;

                // eseguo logout
		case 'c':
               		stampa(YELLOW"Stai per uscire dal gioco e tornare al menu iniziale\n"RESET);
			NuovaPosizione[1]=i;
			NuovaPosizione[2]=j;
			sendSocket(sock,NuovaPosizione,3);			    	
			sleep(1);
		        inGioco=0;
		       	giocato=0;

		break;

		
		case 'i':
				
			
			for(k=0;k<5000;k++){
			online[k]='\0';
			}
			sendSocket(sock,"i",1);
			if(read(sock,online,5000)<0){
				errore("errore read\n");
			}
			stampa(online);
			pausaGioco();

			for(k=0;k<5000;k++){
        			lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        errore("errore read\n");
                    }
					
        		stampaMappa(mappa,i,j,id_partita_corrente,lista);
			
				
	
		break;


                // richiedo info tempo
        	case 't':
                
        		sendSocket(sock,"t",1);
        		if(read(sock,&tempo,sizeof(int))<0){
        			errore("errore read\n");
        		}
        		int tempoRimanente=tempoTotale-tempo;
        		sprintf(tempo_loc,GREEN"Tempo :%d secondi"RESET RED"\n\nMancano ancora :%d secondi\n"RESET,tempo,tempoRimanente);
        		stampa(tempo_loc);
        		pausaGioco();

			for(k=0;k<5000;k++){
        			lista[k]='\0';
                    }
                    
                    sendSocket(sock,"k",1);
                    if(read(sock,lista,5000)<0){
                        errore("errore read\n");
                    }
					
        		stampaMappa(mappa,i,j,id_partita_corrente,lista);
			
        	break;	

			default :
                stampa(RED"Inserimento Non Valido\n"RESET);
                pulisciBuffer();
            break;
		
        } // fine switch case
        
	}while(inGioco); // fine ciclo

	sleep(1);
	pausaGioco();
} // fine funzione gioco (funzione principale)



//__________________________________________
//Funzione che stampa la mappa con nemici aggiornata

void stampaMappa(char mappa[][20],int pos1, int pos2,int id_partita_corrente,char *otherplayers){

	int i=0, j=0;
	int l1=0,l2=0,i1=0,j1=0;
	char messaggio[256];
	char flag[256];

		//pausaGioco();
	for(i=0;i<20;i++){
		for(j=0; j<20; j++){
			if(i==pos1 && j==pos2);
			else
			mappa[i][j]='-';
		}
	}

    //analisi stringa nemici
    int in;
    int temp=0;
    int nem[40];
    int indNem=0;
    
    if(otherplayers != NULL){

        //scorro la stringa
        for(in=0 ; in<strlen(otherplayers); in++){
            
            //se e un numero
            if(otherplayers[in]!='?' && otherplayers[in]!='&' && otherplayers[in]!='\0' ){
                temp = 0; //svuoto temp
                temp = (int)otherplayers[in]-'0'; // lo salvo

                // solo se dopo c'è un altro numero faccio questo
                if(otherplayers[in+1]!='?' && otherplayers[in+1]!='&' && otherplayers[in+1]!='\0'){
                    temp = 0; //svuoto temp
                    temp = (int)otherplayers[in+1]-'0'; // lo salvo
                    temp = temp+10; // lo incremento di 10
                    in++; //in questo caso avanzo di uno (me ne fotto del primo)
                }
                
                //a questo punto in temp ho il numero giuso (1 cifra o 2 cifre)
                //dopo [in] c'è sicuro un carattere
                
                if(otherplayers[in+1]=='?' || otherplayers[in+1]=='&' || otherplayers[in+1]=='\0'){
                    nem[indNem]=temp;
                    indNem++;
                }
            }
        } //fine for
        
        
        int indNem2; //indice array nemico (per scorrerer)
        int mappaX;
        int mappaY;
        
        //a questo punto ho un array di interi dove ogni 2 caselle sono [x][y][x][y]...
        
        for (indNem2=0;indNem2<indNem;indNem2++){
            mappaX= nem[indNem2];
            indNem2++;
            mappaY= nem[indNem2];
            //segno il nemico in posizione giusta
            mappa[mappaX][mappaY] = 'O';
        }
    } //fine if !=NULL
    //fine posizionamento nemici
	
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
				stampa(BLU"@"RESET);
			}
			else if(mappa[i][j]=='O'  && (i!=pos1) && (j!=pos2)){
				stampa("@");
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
			write(0,"|\tw) Sopra\n",sizeof("|\tw) Sopra\n"));
		}else if(i==14){
			write(0,"|\ta) Sinistra\n",sizeof("|\ta) Sinistra\n"));
		}else if(i==15){
			write(0,"|\ts) Giu'\n",sizeof("|\ts) Giu'\n"));
		}else if(i==16){
			write(0,"|\td) Destra\n",sizeof("|\tw) Destra\n"));
		}else if(i==17){
			write(0,"|\ti) My Info\n",sizeof("|\ti) My Info\n"));
		}else if(i==18){
			write(0,"|\tt) Tempo\n",sizeof("|\tt) Tempo\n"));
		}else if(i==19){
			write(0,"|\tc) Logout\n",sizeof("|\tc) Logout\n"));
		}else{
			write(0,"|\n",sizeof("|\n"));
		}
	}
	write(0,"\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n",sizeof("\t    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n"));
	sprintf(messaggio,YELLOW"Ti trovi in Posizione [%d][%d] (Stai partecipando alla partita ID: [%d])\n"RESET,pos1,pos2,id_partita_corrente);
	stampa(messaggio);
} //fine funzione StampaMappa


//__________________________________________
void sendSocket(int conn,char *mess,int nbytes){
	if(write(conn,mess,nbytes)<0){
		errore("Errore write\n");
	}
}

//__________________________________________
void sighandler(int segnale_ricevuto)	/* gestione del segnale */ {
	char flag[3];
	flag[0]='c';
	flag[1]=flagPosizione1;
	flag[2]=flagPosizione2;
	stampa(RED"\nUscita Forzata Catturata\n"RESET);
	write(flagConnessione,flag,sizeof(flag));
	giocato=0;
	inGioco=0;
	sleep(1);
	write(flagConnessione,"l",sizeof(char));
	exit(0);
}

//__________________________________________
void sighandler_1(int segnale_ricevuto){
	stampa(RED"\nUscita Forzata Catturata\n"RESET);
	write(flagConnessione,"l",1);
	giocato=0;
	inGioco=0;
	sleep(1);
	exit(0);
}

//__________________________________________
void sighandler_2(int segnale_ricevuto){
	stampa(RED"\nUscita Forzata Catturata\n"RESET);
	write(flagConnessione,"l",1);
	giocato=0;
	inGioco=0;
	sleep(1);
	exit(0);
}

//__________________________________________
void sighandler_3(int segnale_ricevuto){
	stampa(RED"\nUscita Forzata Catturata\n"RESET);
	char flag[3];
	flag[0]='c';
	flag[1]=flagPosizione1;
	flag[2]=flagPosizione2;
	write(flagConnessione,flag,sizeof(flag));
	giocato=0;
	inGioco=0;
	sleep(1);
	write(flagConnessione,"l",sizeof(char));
	sleep(1);
	exit(0);
}

//__________________________________________
void sighandler_4(int segnale_ricevuto){
	stampa(RED"Il server è andato OFFLINE\n"RESET);
	close(flagConnessione);
	exit(0);
}

//__________________________________________
void attivaVittoria(){
	int i=0;
	system("clear");
	signal(SIGINT,sighandler);
	signal(SIGQUIT,sighandler);
	signal(SIGHUP,sighandler_3);
	signal(SIGSTOP,sighandler_3);
	signal(SIGTERM,sighandler_3);
	signal(SIGPIPE,sighandler_4);
	signal(SIGTSTP,sighandler_3);
	stampa(GREEN"________________________\n");
	stampa("*      !!HAI VINTO!!     \n");
	stampa("________________________\n"RESET);
	sleep(1);
	system("clear");
	sleep(1);
}
//__________________________________________

void attivaSconfitta(){
	int i=0;
	system("clear");
	signal(SIGINT,sighandler);
	signal(SIGQUIT,sighandler);
	signal(SIGHUP,sighandler_3);
	signal(SIGSTOP,sighandler_3);
	signal(SIGTERM,sighandler_3);
	signal(SIGPIPE,sighandler_4);
	signal(SIGTSTP,sighandler_3);

    stampa(RED"________________________\n");
    stampa("*   !!BOMBA SEI MORTO!!     \n");
    stampa("________________________\n"RESET);
	sleep(1);
	system("clear");
	sleep(1);
}   

//__________________________________________
void pausaGioco(){
	stampa("\nPremere Invio Per Continuare...\n");
	pulisciBuffer();
}

//_______________________________________________
//FINE PROGRAMMA CLIENT
//_______________________________________________

