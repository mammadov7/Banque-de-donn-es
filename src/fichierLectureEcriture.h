#ifndef FLE
#define FLE
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1000

/*
une fonction pour trouver l'occurence d'un mot dans ligne
renvoyer -1 si la ligne n'est pas trouvé
*/
int findToken(char *filename, char *word);

/*
une fonction pour inserer une nouvelle ligne dans le fichier
    filename - nom de fichier qui continent toutes les données
    user - unique user identifier
    data - données de client
*/
int writeOrReplaceLine(char *filename, char *user, char *data);

/*
une fonction pour supprimer une ligne de client avec uid=userid
*/
int deleteLine(char *filename, char *userid);

int lire(char *filename);


#endif