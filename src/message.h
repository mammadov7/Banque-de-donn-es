#ifndef MSGFORMAT
#define MSGFORMAT

#include <stdio.h>
#define INIT 1 //initialisation
#define ECRIRE 2 //requête de client:ecrire
#define LIRE 3 //requête de client:lire
#define FIN 4 // pour finir la communication
#define CONFAIL 5 // Erreur de connexion
#define CONEST 6 // connexion réussie
#define SUCCESS 7 //réussie
#define SUPR 8 // requête de client: suppression 
#define NOPERMISSION 9 // pas de Droits
#define SIZE 512 // taille de tampon
#define ERREUR -1 
#define GETDATA 10 // Pour serveur donnees, il attend des données
#define SENDDATA 11 // Pour serveur donnees, il peut envoyer les données

struct message{
    int type;
    int nbDonnees;
    char donnees[SIZE];
};


#endif