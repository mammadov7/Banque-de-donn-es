#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "traverserFichier.h"
#include "check.h"
#include "message.h"
#include "fichierLectureEcriture.h"

#define PORT 7000
#define IP "127.00.00.02"

int main(int argc, char const *argv[]){
  int accessDonnesSoc;
  struct message m1;
  int tailleDeDonnees = SIZE , utilise = 0;
  char *donnees = (char *)calloc( SIZE, sizeof(char ));
  char filename[30]; sprintf(filename, "fichierDonnees_%d", getpid());
  int fd = open( filename,O_CREAT | O_RDWR | O_APPEND, 0666 );
//Creation de Socket
  CHECK((accessDonnesSoc=socket(PF_INET, SOCK_DGRAM, 0))!=-1);
//Creation l'adress de Socket 
  struct sockaddr_in addrDonnes;
  addrDonnes.sin_family = AF_INET;
  addrDonnes.sin_port = htons (PORT);
  addrDonnes.sin_addr.s_addr = inet_addr (IP);

  //CHECK(bind(accessDonnesSoc, (struct sockaddr *) &addrDonnes, sizeof(addrDonnes))==0);
  socklen_t addrlen=sizeof(struct sockaddr_in);

  struct sockaddr_in addrServAccess;
  socklen_t addrlenServAccess=sizeof(struct sockaddr_in);
  
  //Envoi a Serveur d'Access: Bonjour, je suis serveur de age
  strcpy(m1.donnees,argv[1]);

 
  CHECK(sendto(accessDonnesSoc,&m1,sizeof(struct message), 0,(struct sockaddr*)  &addrDonnes, addrlen)!=0);
  memset(m1.donnees,'\0',SIZE);
  
  while(1){
    CHECK((recvfrom(accessDonnesSoc, &m1, sizeof(struct message), 0,(struct sockaddr*) &addrServAccess, &addrlenServAccess )) != 0);
    if(m1.type == SUCCESS){
      printf("SUCCESS\n");
      break;
    }
    else if( m1.type == GETDATA ){
      int rd;
      printf("En attente des donnees:\n");
      do{
        memset(m1.donnees,'\0',SIZE);
        CHECK((recvfrom(accessDonnesSoc, &m1, sizeof(struct message), 0,(struct sockaddr*) &addrServAccess, &addrlenServAccess )) != -1);
        rd = strlen( m1.donnees );
        strcpy( donnees + utilise, m1.donnees );
        utilise += rd;
        if( utilise == tailleDeDonnees ){
          char *tmpptr;
          tailleDeDonnees += SIZE; 
          tmpptr = realloc(donnees,tailleDeDonnees); // faire croître les donnes
          if(tmpptr) // verifier si il n'y a pas de probleme
            donnees = tmpptr;
          else 
            perror("Realloc erreur");
        }//fin if
      } while ( rd !=0 );
      printf("Recu: %s\n", donnees);
      write(fd,donnees,utilise);
      printf("SUCCESS\n");
      break;
    }
    // obtention des raquetes
  }
  struct sockaddr_in addressR;
  socklen_t addressRlen=sizeof(struct sockaddr_in);
  while(1){
    CHECK((recvfrom(accessDonnesSoc, &m1, sizeof(struct message), 0,(struct sockaddr*) &addressR, &addressRlen )) != 0);
    if( m1.type == SENDDATA ){ // Envoi de donnes
      printf("Requet a envoyer des donnees sur %d\n", ntohs(addressR.sin_port));
      int rd;
      memset(m1.donnees,'\0',SIZE);
      fd = open( filename,O_RDWR, 0666 );
      // lseek(fd,0,0);
      do{ // jusqu'a il n'y en reste rien
        rd = read(fd,m1.donnees,SIZE);
        CHECK( sendto(accessDonnesSoc,&m1 ,sizeof(struct message), 0, (struct sockaddr*)&addrDonnes, addrlen) !=0 );
        memset(m1.donnees,'\0',SIZE); //faire vider le tampon
      } while( rd > 0);
      printf("Serveur a envoye toutes les donnees!\n");
    }else if(m1.type==ECRIRE){
      char copy[256];strcpy(copy, m1.donnees);
      char *user=strtok(copy, ":");
      char *data=strtok(NULL, ":");
      printf("Requet a ecrire %s:%s\n", user, data);
      writeOrReplaceLine(filename, user, data);
      struct message response;
      response.type=SUCCESS;
      CHECK( sendto(accessDonnesSoc, &response, sizeof(struct message), 0, (struct sockaddr*)&addressR, addressRlen)!=0);
      printf("ECRIRE a ete effectue!\n");
    }else if(m1.type==SUPR){
      printf("Requet a supprimer ligne de %s\n", m1.donnees);
      deleteLine(filename, m1.donnees);
      struct message response;
      response.type=SUCCESS;
      CHECK( sendto(accessDonnesSoc, &response, sizeof(struct message), 0, (struct sockaddr*)&addressR, addressRlen)!=0);
      printf("SUPPRIMER a ete effectue!\n");
    }else if( m1.type == LIRE ){
      if( lire(filename) == -1 ){
        perror("ERREUR de raquete LIRE");
        exit(-1);
      }
      int temp_fd = open("new",O_RDWR, 0666);
      printf("Requete de lire de Champ: %s bien recu\n",argv[1]);
      int rd;
      struct message response;
      response.type=SUCCESS;
      do{ // jusqu'a il n'y en reste rien         
        memset(response.donnees,'\0',SIZE); //faire vider le tampon
        rd = read(temp_fd,response.donnees,SIZE-1);
        response.donnees[strlen(response.donnees)]='\n';
        CHECK( sendto(accessDonnesSoc,&response ,sizeof(struct message), 0, (struct sockaddr*)&addressR, addressRlen) !=0 );
      } while( rd !=0 );
      close(temp_fd);
      remove("new"); //removing temporary file which was created for sending data to client without client ID
      printf("LIRE a ete effectue!\n");
    }

  }
  close(fd);
  remove(filename);
  free( donnees );
  return 0;
}
