#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

#include <stdbool.h>
#include "check.h"
#include "traverserFichier.h"
#include "message.h"

#define PORT_ACCESS_CLIENT 6000
#define PORT_ACCESS_DONNEES 7000
#define IP_ACCESS_CLIENT "127.00.00.01"
#define IP_ACCESS_DONNEES "127.00.00.02"

#define CLIENTINFO "fichierUtilisateur.txt"
#define MAXCONNECTIONS 10

struct clientConnection{
    char *login;
    int id;//en fait c'est le numero de port de l'utilisateur
    struct sockaddr_in adr;
    int nbCh;
    char **champs;
};

struct serveurDeDonnees{
    struct sockaddr_in addr; // adress de node 
    int type; // GETDATA - attend des donnees || SETDATA - peut envoyer
    char champ[128]; // age||nom||taille...
};

struct connectiondata{
    struct clientConnection client;
    int sockIP;
    short sockPort;
};

struct clientConnection connectedClients[MAXCONNECTIONS];
struct serveurDeDonnees SDs[MAXCONNECTIONS];
int nbConnections=0;
int nbSDs=0;
short portCounter;
short portDonnesCounter;

pthread_mutex_t mutexCompteur;
pthread_mutex_t mutexPortDonnes;
pthread_mutex_t mutexClientConnections;
pthread_mutex_t mutexSD;

int ajouterNouveauSD( struct sockaddr_in addr, char *champ,int type){
    if( nbSDs < MAXCONNECTIONS ){
        pthread_mutex_lock(&mutexSD);
        SDs[nbSDs].addr = addr;
        strcpy(SDs[nbSDs].champ,champ);
        SDs[nbSDs].type = type;
        nbSDs++;
        pthread_mutex_unlock(&mutexSD);
        return 1;
    }
    else
        return -1;
}

int checkSDs(char *champ){
    for (int i = 0; i < nbSDs; i++) {
        if( strcmp(champ,SDs[i].champ) == 0 )
            return GETDATA;
    }
    return SENDDATA;
}

int champ2SD(char *champ){
  for (int i = 0; i < nbSDs; i++) {
    if( strcmp(champ,SDs[i].champ) == 0 && SDs[i].type == SENDDATA )
      return i;
  }
  return -1;
}

void suprSD(int i){
  memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
  SDs[i].addr=SDs[nbSDs-1].addr;
  SDs[i].type=SDs[nbSDs-1].type;
  strcpy(SDs[i].champ, SDs[nbSDs-1].champ);
  memset(&SDs[nbSDs-1], sizeof(struct serveurDeDonnees), 0);
  nbSDs--;
}

void printCurrentClients();
int ajouterNouveauClient(char *login, int id,struct sockaddr_in adr, char **champs, int nbCh);

int ajouterNouveauClient(char *login, int id, struct sockaddr_in adr,char **champs, int nbCh){
    pthread_mutex_lock(&mutexClientConnections);
    struct clientConnection *nouvCl=malloc(sizeof(struct clientConnection));
    nouvCl->login=login;
    nouvCl->id=id;
    nouvCl->adr=adr;
    nouvCl->champs=(champs!=NULL?champs:NULL);
    nouvCl->nbCh=(champs!=NULL?nbCh:0);
    if(nbConnections>=MAXCONNECTIONS)   return -1;
    connectedClients[nbConnections++]=*nouvCl;
    pthread_mutex_unlock(&mutexClientConnections);
    return 0;
}

void printCurrentClients(){
    pthread_mutex_lock(&mutexClientConnections);
    printf("\nClients en connexion sur serveur d'access (%d):\n", nbConnections);
    for(int i=0;i<nbConnections;i++){
        printf("client: %s %d\n\t", connectedClients[i].login, connectedClients[i].id);
        for(int j=0;j<connectedClients[i].nbCh;j++){
            printf("%s, ", connectedClients[i].champs[j]);
        }
        printf("\n\n");
    }
    pthread_mutex_unlock(&mutexClientConnections);
}

char **droitAuxChamps(int port, char *champs, int *nbChamps ){ //pour trouver les droits de Client aux champs 
    pthread_mutex_lock(&mutexClientConnections);
    int i, j = 0;
    for(i=0;i<nbConnections;i++)  
        if( connectedClients[i].id == port ) // on cherche le client qui a envoyé le ràquete
            break;
    char **droits = calloc(connectedClients[i].nbCh, sizeof(char*));
    for(int k = 0; k < connectedClients[i].nbCh; k++){
        if( strstr(champs, connectedClients[i].champs[k])!=NULL ){
            droits[ j ]=malloc(strlen( connectedClients[i].champs[k] )+1);
            strcpy(droits[ j++ ], connectedClients[i].champs[k] );            
        }
    }

    *nbChamps = j;
    pthread_mutex_unlock(&mutexClientConnections);
    return droits;
}

//-------------------------------------CLIENT CONNECTION THREAD------------------------------------------

void *connexionClient(void *arg){
    struct connectiondata client=*((struct connectiondata*)arg);

//Creation de Socket pour ServeurDonnees
    pthread_mutex_lock(&mutexPortDonnes);
    short mySDPort=portDonnesCounter++;
    pthread_mutex_unlock(&mutexPortDonnes);
    int socAccessDonnes;
    CHECK((socAccessDonnes=socket(PF_INET, SOCK_DGRAM, 0))!=-1);
    struct sockaddr_in addrAccessDonnes;
    addrAccessDonnes.sin_family = AF_INET;
    addrAccessDonnes.sin_port = htons (mySDPort);
    addrAccessDonnes.sin_addr.s_addr = inet_addr (IP_ACCESS_DONNEES);
    socklen_t addrAccessDonneslen=sizeof(struct sockaddr_in);
    CHECK(bind(socAccessDonnes, (struct sockaddr *) &addrAccessDonnes, addrAccessDonneslen)==0);

//Creation de socket pour communication avec client
    int connSocket;
    CHECK((connSocket=socket(PF_INET, SOCK_DGRAM, 0))!=-1);
    struct sockaddr_in addrSocket;
    addrSocket.sin_family = AF_INET;
    pthread_mutex_lock(&mutexCompteur);
    addrSocket.sin_port = htons(client.sockPort);
    pthread_mutex_unlock(&mutexCompteur);
    addrSocket.sin_addr.s_addr = inet_addr (IP_ACCESS_CLIENT);
    socklen_t sockaddrlen=sizeof(struct sockaddr_in); 
    CHECK( bind(connSocket, (struct sockaddr *) &addrSocket, sockaddrlen)==0);
    
    struct sockaddr_in addrClient;
    socklen_t clsockaddrlen=sizeof(struct sockaddr_in);
    
    fd_set listener;
    while(1){
        printf("Connexion client en ecoute:\n");
        FD_ZERO(&listener);
        FD_SET(connSocket, &listener);
        FD_SET(socAccessDonnes, &listener);
        select(connSocket+1, &listener, NULL, NULL, NULL);
        if(FD_ISSET(connSocket, &listener)){
            struct message request;
            CHECK( recvfrom(connSocket, &request, sizeof(struct message), 0, (struct sockaddr*)&addrClient, &clsockaddrlen) != -1);
        if(request.type==LIRE){
          struct message response;
          int nbDroits; //avant la demande, nous vérifions si le client a access aux champs
        
          //VÉRIFIER LES DROITS
          char *saveptr;
          char *champ=strtok_r(request.donnees, ":", &saveptr);
          if( champ == NULL ){
            response.type = ERREUR;
            memset(response.donnees,'\0',SIZE);
            strcpy(response.donnees,"LIRE nomDeChamp");
            printf("Nom de champ n'est pas vraie\n");
            CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );              
            continue;
          }
          char **champs;
          champs = droitAuxChamps(client.client.id,champ,&nbDroits);
          if(nbDroits!=0){
            pthread_mutex_lock(&mutexSD);
            //envoyer une demande au serveur des donnees
            printf("Access verified!\n");
            response.type = LIRE;
            CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );              
            for( int i = 0; i < nbDroits; i++ ){
              int temp_sd = champ2SD(champs[i]);
              if( temp_sd == -1 ){
                printf("Il n'existe pas de serveur de cette champ\n");
                break;
              }
              //envoi de raquete LIRE a Serveur de Donnes 
              printf("Attente des données du serveur %d\n", ntohs(SDs[temp_sd].addr.sin_port) );
              printf("Envoie des données vers Client %d\n", ntohs(addrClient.sin_port) );
              response.type = LIRE;
              CHECK(sendto(socAccessDonnes, &response, sizeof(struct message), 0,(struct sockaddr*) &(SDs[temp_sd].addr), addrAccessDonneslen ) !=0);
              int rd;
              do{
                memset(response.donnees,'\0',SIZE);
                
                CHECK(recvfrom(socAccessDonnes, &response, sizeof(struct message), 0,(struct sockaddr*) NULL, NULL) != -1 );
                rd = strlen( response.donnees );
                CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );  
              }while( rd != 1  );
            }
            memset(response.donnees,'\0',SIZE);
            response.type = FIN;
            CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );          
            pthread_mutex_unlock(&mutexSD);
          }else{
            //envoyer un message d'erreur au client
            printf("Sans droits qux certqins champs!\n");
            response.type=NOPERMISSION;
            CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );   
          }    
        }else if(request.type==ECRIRE){//requet pour ecrire ou modifier plusieurs champs
          printf("\tServeur a recu une requet: ECRIRE \n");
          struct message response;
          sprintf(response.donnees, "Failed: ");
          //TODO
          char *saveptr;
          char copy[strlen(request.donnees)];strcpy(copy, request.donnees);
          char *champ;char *data;
          champ=strtok_r(copy, ":", &saveptr);
          data=strtok_r(NULL, " ", &saveptr);
          /*
              Traiter chaque requet de champ:data
          */
          while(champ!=NULL && data!=NULL){
              int sentSuccessfully=0;
              //find all servers for the champ
              pthread_mutex_lock(&mutexSD);
              int nbServeurs=0, nbSuccess=0;
              /*
                  Trouver tous serveurs responsables pour la champ, et les envoyer des requets
              */
              for(int i=0;i<nbSDs;i++){
                if(strcmp(SDs[i].champ, champ)==0){
                  nbServeurs++;
                  //envoyer une raqute au serveur de données
                  struct message ecrireMsg;
                  struct message ecrireResponse;
                  ecrireMsg.type=ECRIRE;
                  sprintf(ecrireMsg.donnees, "%d:%s", client.client.id, data);
                  printf("\tEnvoyer message: \"%s\" a serveur sur port %d\n", ecrireMsg.donnees, ntohs(SDs[i].addr.sin_port));
                  CHECK( sendto(socAccessDonnes, &ecrireMsg, sizeof(struct message), 0,(struct sockaddr*)  &SDs[i].addr, addrAccessDonneslen )!=0 );
                  //obtenir une réponse du serveur de données
                  fd_set fset;
                  struct timeval tv;tv.tv_sec=2;
                  FD_ZERO(&fset);
                  FD_SET(socAccessDonnes, &fset);
                  if(select(socAccessDonnes+1, &fset, NULL, NULL,&tv) && recvfrom(socAccessDonnes, &ecrireResponse, sizeof(struct message), 0,(struct sockaddr*) NULL, NULL)!=-1 ){
                    if(ecrireResponse.type==ERREUR){
                      /* Au cas d'echec supprimer le serveur, car il n'est plus disponible */
                      printf("\tServeur n'a pas reussi a modifier le champ!\n");
                      //serveur en panne
                      //il faut le supprimer de la liste
                      if(i==nbSDs-1){
                        memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                        nbSDs--;
                      }else{
                        //swap
                        memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                        SDs[i].addr=SDs[nbSDs-1].addr;
                        SDs[i].type=SDs[nbSDs-1].type;
                        strcpy(SDs[i].champ, SDs[nbSDs-1].champ);
                        memset(&SDs[nbSDs-1], sizeof(struct serveurDeDonnees), 0);
                        nbSDs--;
                        i--;
                      }
                      //TODO
                    }else if(ecrireResponse.type==SUCCESS){
                        printf("\tChamps %s ont ete modifies !\n", champ);
                        sentSuccessfully+=1;
                    }
                  }else{
                    /*
                        Au cas d'echec supprimer le serveur, car il n'est plus disponible
                    */
                    printf("\tServeur en panne!\n\tAucune reponse pendant 2 secondes!\n");
                    //serveur en panne
                    //il faut le supprimer de la liste                             
                    if(i==nbSDs-1){
                        memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                        nbSDs--;
                    }else{
                        //swap
                        memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                        SDs[i].addr=SDs[nbSDs-1].addr;
                        SDs[i].type=SDs[nbSDs-1].type;
                        strcpy(SDs[i].champ, SDs[nbSDs-1].champ);
                        memset(&SDs[nbSDs-1], sizeof(struct serveurDeDonnees), 0);
                        nbSDs--;
                        i--;
                    }
                  }
                }
              } //fin for
                  pthread_mutex_unlock(&mutexSD);
                  if(sentSuccessfully==0){//aucun serveur responsable pour ce champ n'a pas reussi a ecrire dans ce champ
                      sprintf(response.donnees, "%s ", champ);//ajouter le champ echoue dans la liste a renvoyer
                  }
                  champ=strtok_r(NULL, ":", &saveptr);
                  data=strtok_r(NULL, " ", &saveptr);
              }
              printf("\tComplet!\nEnvoi de message de reussite\n");
              response.type=SUCCESS;
                  //renvoyer la réponse au client
              CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );
          }else if(request.type==SUPR){
                printf("\tServeur a recu une requet: SUPPRIMER \n");
                struct message response;
                sprintf(response.donnees, "Failed: ");
                //TODO
                int removedSuccessfully=0;
                //find all servers for the champ
                pthread_mutex_lock(&mutexSD);
                int nbServeurs=0, nbSuccess=0;
                /*
                    Trouver tous serveurs , et les envoyer des requets
                */
                for(int i=0;i<nbSDs;i++){
                    nbServeurs++;
                //envoyer une demande au serveur de données
                    struct message supprMsg;
                    struct message supprResponse;
                    supprMsg.type=SUPR;
                    sprintf(supprMsg.donnees, "%d", client.client.id);

                    printf("\tEnvoyer message: \"%s\" a serveur sur port %d\n", supprMsg.donnees, ntohs(SDs[i].addr.sin_port));
                    CHECK( sendto(socAccessDonnes, &supprMsg, sizeof(struct message), 0,(struct sockaddr*)  &SDs[i].addr, addrAccessDonneslen )!=0 );
                    //obtenir une réponse du serveur de données
                    fd_set fset;
                    struct timeval tv;tv.tv_sec=5;
                    FD_ZERO(&fset);
                    FD_SET(socAccessDonnes, &fset);
                    if(select(socAccessDonnes+1, &fset, NULL, NULL,&tv) && recvfrom(socAccessDonnes, &supprResponse, sizeof(struct message), 0,(struct sockaddr*) NULL, NULL)!=-1 ){
                        if(supprResponse.type==ERREUR){
                            /*
                                Au cas d'echec supprimer le serveur, car il n'est plus disponible
                            */
                            printf("\tServeur n'a pas reussi a modifier le champ!\n");
                            //serveur en panne
                            //il faut le supprimer de la liste
                            if(i==nbSDs-1){
                                memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                                nbSDs--;
                            }else{
                            //swap
                                memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                                SDs[i].addr=SDs[nbSDs-1].addr;
                                SDs[i].type=SDs[nbSDs-1].type;
                                strcpy(SDs[i].champ, SDs[nbSDs-1].champ);
                                memset(&SDs[nbSDs-1], sizeof(struct serveurDeDonnees), 0);
                                nbSDs--;
                                i--;
                            }
                            //TODO
                        }else if(supprResponse.type==SUCCESS){
                            printf("\tChamps %s ont ete supprimes de serveur sur port %d!\n", SDs[i].champ, SDs[i].addr.sin_port);
                            removedSuccessfully+=1;
                        }
                    }else{
                        /*
                            Au cas d'echec supprimer le serveur, car il n'est plus disponible
                        */
                        printf("\tServeur en panne!\nAucun response pendant 2 secondes!\n");
                        //serveur en panne
                        //il faut le supprimer de la liste
                        if(i==nbSDs-1){
                            memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                            nbSDs--;
                        }else{
                        //swap
                            memset(&SDs[i], sizeof(struct serveurDeDonnees), 0);
                            SDs[i].addr=SDs[nbSDs-1].addr;
                            SDs[i].type=SDs[nbSDs-1].type;
                            strcpy(SDs[i].champ, SDs[nbSDs-1].champ);
                            memset(&SDs[nbSDs-1], sizeof(struct serveurDeDonnees), 0);
                            nbSDs--;
                            i--;
                        }
                    }
                }
                pthread_mutex_unlock(&mutexSD);
                if(removedSuccessfully==0){//aucun serveur responsable pour ce champ n'a pas reussi a ecrire dans ce champ
                    sprintf(response.donnees, "Echec des serveurs!\n ");//ajouter le champ echoue dans la liste a renvoyer
                }

                printf("\tComplet!\n\tEnvoi de message de reussite\n");
                response.type=SUCCESS;
                    //return response to client
                CHECK( sendto(connSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, clsockaddrlen )!=-1 );

            }
            else if(request.type==FIN){
                printf("A bientot!\n");
            
                close(connSocket);
                //remove client from connected clients
                pthread_mutex_lock(&mutexClientConnections);
                for( int i=0;i<nbConnections;i++){
                    if(connectedClients[i].id=client.client.id){
                        if(i==nbConnections-1){
                            memset(&connectedClients[i], sizeof(struct clientConnection), 1);
                            nbConnections--;
                        }else{
                            //swap
                            memset(&connectedClients[i], sizeof(struct clientConnection), 1);
                            memset(&connectedClients[nbConnections-1], sizeof(struct clientConnection), 1);
                            connectedClients[i]=connectedClients[nbConnections-1];
                            nbConnections--;
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&mutexClientConnections);
                return NULL;
            }
        }
    }
    return NULL;
}

//----------------------------------------------MAIN-----------------------------------------------------
int main(int argc, char const *argv[]){

    portCounter=PORT_ACCESS_CLIENT+1;
    portDonnesCounter=PORT_ACCESS_DONNEES+1;
    pthread_t clientConnections[MAXCONNECTIONS];
    pthread_mutex_init(&mutexCompteur, NULL);
    pthread_mutex_init(&mutexPortDonnes, NULL);
    pthread_mutex_init(&mutexClientConnections, NULL);
    pthread_mutex_init(&mutexSD, NULL);

    int tcounter=0;

//Creation de Socket entre Serveurs de Donnees et Server d'Access
    int socAccessDonnes;
    CHECK((socAccessDonnes=socket(PF_INET, SOCK_DGRAM, 0))!=-1);

//Creation de Socket adress
    struct sockaddr_in addrAccessDonnes;
    addrAccessDonnes.sin_family = AF_INET;
    addrAccessDonnes.sin_port = htons (PORT_ACCESS_DONNEES);
    addrAccessDonnes.sin_addr.s_addr = inet_addr (IP_ACCESS_DONNEES);
    struct sockaddr_in addrSD; //Adress de Serveur de Donnees
    socklen_t addrlenSD=sizeof(struct sockaddr_in);    

//Bind   
    CHECK(bind(socAccessDonnes, (struct sockaddr *) &addrAccessDonnes, sizeof(addrAccessDonnes))==0);

//Creation de Socket entre Clients et Server d'Access
    int servClientSocket;
    CHECK((servClientSocket=socket(PF_INET, SOCK_DGRAM, 0))!=-1);
    int optval = 1;
    setsockopt(servClientSocket, SOL_SOCKET, SO_REUSEADDR, 
         (const void *)&optval , sizeof(int));

//Creation de Socket adress
    struct sockaddr_in addrConnectionClient;
    addrConnectionClient.sin_family = AF_INET;
    addrConnectionClient.sin_port = htons (PORT_ACCESS_CLIENT);
    addrConnectionClient.sin_addr.s_addr = inet_addr (IP_ACCESS_CLIENT);

//Bind   
    CHECK(bind(servClientSocket, (struct sockaddr *) &addrConnectionClient, sizeof(addrConnectionClient))==0);
    socklen_t addrlen=sizeof(struct sockaddr_in);

    struct sockaddr_in addrClient;
    socklen_t addrlenCl=sizeof(struct sockaddr_in);
    fd_set listener;
    while(1){
        printf("En ecoute:\n");
        FD_ZERO(&listener);
        FD_SET(servClientSocket, &listener);
        FD_SET(socAccessDonnes, &listener);
        select(servClientSocket+1, &listener, NULL, NULL, NULL);
        if(FD_ISSET(servClientSocket, &listener)){
            struct message msgRq;
            CHECK(recvfrom(servClientSocket, &msgRq, sizeof(struct message), 0,(struct sockaddr*) &addrClient, &addrlenCl )!=0);
            char *motDePasse;
            char *utilisateur;
            if(msgRq.type==INIT){
                printf("Serveur a recu un requet de connexion\n");
                char tmp_req[256];
                strcpy(tmp_req,msgRq.donnees); // tmp = " utilisateur:motDePasse "
                utilisateur = strtok(tmp_req,":"); // strtok() = utilisateur
                motDePasse= strtok(NULL,":"); //prochaine est motDePasse

                printf("Client a se connecter:%s, %s\n",utilisateur, motDePasse);
                struct message response;
                int uid;
                if(authentifierUtilisateur(utilisateur, motDePasse, &uid, "fichierUtilisateur.txt")==true){
                    printf("\tClient est authorise!\n");
                    //trouver tous les champs pour utilisateur
                    int nbCh=0;
                    char **list=trouverChamps(utilisateur, motDePasse,uid, "fichierUtilisateur.txt", &nbCh);
                    //add client to connectedclients list
                    if(ajouterNouveauClient(utilisateur, uid,addrClient ,list, nbCh)==0){
                        response.type=CONEST;
                        sprintf(response.donnees, "%d", portCounter);
                        //start  a new connection thread
                        struct connectiondata data;
                        data.client=connectedClients[nbConnections-1];
                        data.sockIP=inet_addr(IP_ACCESS_CLIENT);
                        data.sockPort=portCounter;
                        pthread_create( &clientConnections[tcounter], NULL, connexionClient, &data );
                        tcounter++;
                        portCounter++;
                    }
                    else
                        response.type=CONFAIL;    
                }else{
                    response.type=CONFAIL;
                    printf("\tClient n'a pas ete trouve!\n");
                }
                // printCurrentClients();
                //return response
                CHECK(sendto(servClientSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, addrlenCl  )>0);
            }else{
                struct message response;
                response.type=ERREUR;
                sprintf(response.donnees,"Serveur n'accepte que des requetes de connexion\n");
                CHECK(sendto(servClientSocket, &response, sizeof(struct message), 0,(struct sockaddr*)  &addrClient, addrlenCl  )>0);
            }
        }else if(FD_ISSET(socAccessDonnes, &listener)){//En cas de serveur de données
            struct message msgSD;
            CHECK(recvfrom(socAccessDonnes, &msgSD, sizeof(struct message), 0,(struct sockaddr*) &addrSD, &addrlenSD )!=0);
            int sdType = checkSDs(msgSD.donnees); // vérifie s'il existe dans le tableau un nœud ayant le même champ
            ajouterNouveauSD(addrSD,msgSD.donnees,sdType);
            if(  sdType == SENDDATA  ){ //pouvez recevoir les requête et envoyer les données à un autre noeud
                printf("Nouveau serveur de donnees connecté\n");
                memset(msgSD.donnees,'\0',SIZE);
                msgSD.type=SUCCESS;
                CHECK(sendto(socAccessDonnes, &msgSD, sizeof(struct message), 0,(struct sockaddr*) &addrSD, addrlenSD ) >0);
            }
            else if( sdType == GETDATA){ // doit obtenir les données d'un autre nœud
                char curChamp[128]; strcpy(curChamp,msgSD.donnees);
                int sender,receiver = nbSDs -1;
                for (int i = 0; i < nbSDs; i++){ //trouve le serveur avec le même champ que celui qui peut envoyer des données
                    if( strcmp(SDs[i].champ,curChamp) == 0 &&  SDs[i].type == SENDDATA )
                        sender = i;
                } 
                printf("En train de connection de nouveau serveur\n");
                memset(msgSD.donnees,'\0',SIZE);
                msgSD.type=SENDDATA; // le noeud qui peut envoyer les données 
                CHECK(sendto(socAccessDonnes, &msgSD, sizeof(struct message), 0,(struct sockaddr*) &(SDs[sender].addr), addrlenSD ) >0);
                msgSD.type=GETDATA; // celui qui doit obtenir
                CHECK(sendto(socAccessDonnes, &msgSD, sizeof(struct message), 0,(struct sockaddr*) &(SDs[receiver].addr), addrlenSD ) >0);

                int rd;
                
                do{ // transition de Donnes entre eux
                    memset(msgSD.donnees,'\0',SIZE);
                    printf("\tEttente de donnees de serveur sur port %d\n", ntohs(SDs[sender].addr.sin_port) );
                    CHECK(recvfrom(socAccessDonnes, &msgSD, sizeof(struct message), 0,(struct sockaddr*) &(SDs[sender].addr), &addrlenSD) != 0);
                    rd = strlen( msgSD.donnees );
                    msgSD.type=GETDATA;
                    printf("\tEnvoie de donnees ver serveur sur port %d\n", ntohs(SDs[receiver].addr.sin_port) );
                    CHECK(sendto(socAccessDonnes, &msgSD, sizeof(struct message), 0,(struct sockaddr*) &(SDs[receiver].addr), addrlenSD ) !=0);
                }while( rd !=0 );
                SDs[receiver].type=SENDDATA;            
                printf("\tNouveau serveur connecté\n");
            }            
        }
    } // fin de while(1)


    return 0;
}
