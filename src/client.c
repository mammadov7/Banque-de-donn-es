#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "message.h"
#include "check.h"

#define SERVPORT 6000
#define IP "127.00.00.01"
#define BUFFSIZE 256


//identificateur de type de message: INIT | SUPRIMER | LIRE | ECRIRE | ERREUR | FIN
struct message *gererMsg( char *tampon  ){
  struct message *msg;    
  msg = (struct message *)malloc( sizeof(struct message) );
  if( !memcmp(tampon,"FIN",3) ){ 
    msg->type = FIN;
    strcpy(msg->donnees, tampon);
    msg->nbDonnees=strlen(tampon);
  }
  else if( !memcmp(tampon,"LIRE",4) ){
    msg->type = LIRE;
    //tampon = "LIRE utilisateur:motDePasse" ==> tampon + 5 = "utilisateur:motDePasse"
    strcpy(msg->donnees, tampon+5); 
    msg->nbDonnees=strlen(tampon+5);
  }
  else if( !memcmp(tampon,"SUPPRIMER",8) ){
    msg->type = SUPR; 
    strcpy(msg->donnees, tampon+9);
    msg->nbDonnees=strlen(tampon+9);
  }
  else if( !memcmp(tampon,"ECRIRE",6) ){
    msg->type = ECRIRE;
    strcpy(msg->donnees, tampon+7);
    msg->nbDonnees=strlen(tampon+7);
  }
  else if( !memcmp(tampon,"INIT",4) ){ 
    msg->type = INIT;
    strcpy(msg->donnees, tampon+5);
    msg->nbDonnees=strlen(tampon+5);
  }
  else{
    return NULL;
  }
  return msg;
}


int main(int argc, char const *argv[]){

  if(argc!=2){
    printf("Arguments:  %s portNumper\n", argv[0]);
    exit(0);
  }
  short clientPort=atoi(argv[1]);
  int clientSocket;
  char buff[BUFFSIZE];
  //SETUP SOCKET
  if ((clientSocket=socket(AF_INET, SOCK_DGRAM,0)) == 0){ 
    perror("Socket Échoué!!!"); 
    exit(EXIT_FAILURE); 
  }
  struct sockaddr_in clientAddr;
  clientAddr.sin_family=AF_INET;
  clientAddr.sin_addr.s_addr=INADDR_ANY;
  clientAddr.sin_port=htons(clientPort);
  socklen_t clientAddrlen=sizeof(struct sockaddr_in);
  CHECK(bind(clientSocket,(struct sockaddr*) &clientAddr, clientAddrlen)!=-1);

  struct sockaddr_in serverAccessAddr;
  struct sockaddr_in clConnAddr;
  clConnAddr.sin_family = AF_INET; 
  clConnAddr.sin_port = htons(SERVPORT);
  clConnAddr.sin_addr.s_addr = inet_addr(IP);
  memset(clConnAddr.sin_zero, '\0', sizeof(clConnAddr.sin_zero));
  socklen_t clAddrlen=sizeof(struct sockaddr_in);
  serverAccessAddr=clConnAddr;
  
  struct sockaddr_in servSock_addr;
  socklen_t servAddrlen;

  //COMMENCER LA COMMUNICATION
  do{
    memset(buff, '\0', BUFFSIZE);
    struct message *msg;
    printf("Votre requete: ");
    fgets(buff,SIZE,stdin); //on utilise fgets au lieu de scanf, parce que scanf ne lit pas apres espace
    buff[ strlen( buff ) - 1 ]='\0'; // fgets returner un string qui finis '\n' et on change ca avec '\0'
    msg = gererMsg(buff); if(msg==NULL) continue;

    //ENVOI
    if ((sendto(clientSocket ,msg ,sizeof(struct message), 0, (struct sockaddr*)&clConnAddr, clAddrlen)) == -1){ 
      perror("sendto failed"); 
      exit(EXIT_FAILURE); 
    }else {
      printf("A envoye %s\n", buff);
    }
    if(msg->type==FIN){
      printf("Connexion ferme!\n");
      clConnAddr=serverAccessAddr;
      continue;
    }
    //RÉCEPTION  
    struct message response;
    if ((recvfrom(clientSocket ,&response ,sizeof(struct message), 0, (struct sockaddr*)&clConnAddr, &clAddrlen)) == -1){ 
      perror("sendto failed"); 
      close(clientSocket);
      exit(EXIT_FAILURE); 
    }else{
        switch (response.type)
        {
        case CONFAIL:
          printf("Connection failed\n");
          clConnAddr=serverAccessAddr;
        break;
        case LIRE:
          do{
            memset(response.donnees,'\0',SIZE);
            CHECK(recvfrom(clientSocket ,&response ,sizeof(struct message), 0, (struct sockaddr*)&clConnAddr, &clAddrlen) != -1 );
            printf( "%s",response.donnees); 
          }while( response.type != FIN );
        break;
        case CONEST:
          printf("Port de connexion %d\n", atoi(response.donnees));
          printf("Connexion etablis\n");
          //extraire le numéro de port et définir la nouvelle connexion
          clConnAddr.sin_port=htons(atoi(response.donnees));
        break;
        case SUCCESS:
          if(msg->type==ECRIRE && strlen( msg->donnees ) == 9 ){
            printf("Echec d'ecriture de certaines champs : %s\nEssayez renvoyer le requet\n", response.donnees);  
          }
          else {
            printf("Ça marche\n");
          }

        break;
        case NOPERMISSION:
            printf("Client ne possede pas des permissions pour certains champs!\n");
        break;

        case ERREUR:
          printf("Echec d'ecriture de certaines champs : %s\nEssayez renvoyer le requet\n", response.donnees);
          break;
        default:
          break;
        }

    } 
  }while( memcmp(buff,"bye",4) );
  close(clientSocket);
  return 0;
}
