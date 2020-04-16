#ifndef TRFICH
#define  TRFICH

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "check.h"

#define READFILE 1024

static int lastReadBytes=0;
char *readFile(const char *fileName);

/*
une fonction pour verifier que l'utilisateur avec des credintiels existe
*/
bool authentifierUtilisateur(char *nom, char *motdepasse, int *uid, char *fichier);


/*
une fonction pour calculer nombre de champs de l'utilisateur
*/
int calculerNbChamps(char *text);

/*Fonction à retourner une liste des champs auxquelles l'utilisateur a l'accès, leur nombre est écrit dans variable nbCh*/
char **trouverChamps(char *nom, char *motdepasse,int uid, char *fichier, int *nbCh);

#endif