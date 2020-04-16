#include "traverserFichier.h"


char *readFile(const char *fileName){
    int readCurrent=0;
    FILE *fp;CHECK((fp=fopen(fileName, "r"))!=NULL);
    char *buffer;CHECK((buffer=calloc(READFILE+1, sizeof(char)))!=NULL);
    fseek(fp, lastReadBytes, SEEK_SET);
    readCurrent=fread(buffer, sizeof(char), READFILE, fp);
    if(readCurrent==0){
        fclose(fp);
        lastReadBytes=0;
        return NULL;
    }
    else if(readCurrent<READFILE){
        int gap=strrchr(buffer, '\n')-buffer;
        lastReadBytes+=(gap>0?gap:readCurrent);
    }else lastReadBytes+=READFILE;
    CHECK(fclose(fp)==0);
    return buffer;
}


bool authentifierUtilisateur(char *nom, char *motdepasse, int *id, char *fichier){
    char *text;
    while((text=readFile(fichier))!=NULL){
        char searchToken[256];sprintf(searchToken, "%s:%s:", nom, motdepasse);
        char *p;
        if((p=strstr(text, searchToken))!=NULL){
            p+=strlen(searchToken);
            char *uid=strtok(p, " ");
            if(uid!=NULL) *id=atoi(uid);
            else return false;
            lastReadBytes=0;
            return true;
        }
    }
    lastReadBytes=0;
    return false;
}


int calculerNbChamps(char *text){
    char * cp1=malloc(strlen(text)+1);
    strcpy(cp1, text);
    //printf("%s\n\n", cp1);
    int nb=0;
    cp1=strtok(cp1, "\n");
    char *x=strtok(cp1, ":");
    while(x!=NULL){
        nb++;
        x=strtok(NULL, ":");
    }
    return nb;

}

char **trouverChamps(char *nom, char *motdepasse, int uid,  char *fichier, int *nbCh){
    char *text;
    while((text=readFile(fichier))!=NULL){
        char searchToken[256];sprintf(searchToken, "%s:%s:%d ", nom, motdepasse, uid);
        char *champs;
        if((champs=strstr(text, searchToken))!=NULL){
            champs+=strlen(searchToken);
            int nbChamps=calculerNbChamps(champs);
            *nbCh=nbChamps;
            char **list=calloc(nbChamps, sizeof(char*));
            
            char * cp1=malloc(strlen(champs)+1);
            strcpy(cp1, champs);
            //printf("%s\n\n", cp1);
            int nb=0;
            cp1=strtok(cp1, "\n");
            char *x=strtok(cp1, ":");
            while(x!=NULL){
                list[nb]=malloc(strlen(x)+1);
                strcpy(list[nb++], x);
                x=strtok(NULL, ":");
            }      
            lastReadBytes=0;
            return list;
        }else{
            printf("trouverChamps error\n");
        }
    }
    lastReadBytes=0;
    *nbCh=0;
    return NULL;
}
