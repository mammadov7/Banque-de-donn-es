#include "fichierLectureEcriture.h"



int findToken(char *filename, char *word){
   char line[1024] ;
   int lineNb=0;
   FILE* fp = fopen(filename, "r") ;
   while (fgets(line , sizeof(line) , fp )!= NULL)
   {
      lineNb++;
      if (strstr(line , word )!= NULL)
      {
          fclose(fp);
         return lineNb;
      }
   }
   fclose(fp);
   return -1;
}

int writeOrReplaceLine(char *filename, char *user, char *data){
    int lineNb=findToken(filename, user);
    char buffer[BUFFER_SIZE];
    char newline[BUFFER_SIZE];
    sprintf(newline, "%s:%s\n", user, data);
    int count=0;   

    if(lineNb==-1){//aucun ligne trouvé, insertion d'une nouvelle ligne au but de fichier

         FILE *fPtr  = fopen(filename, "a");
         fputs(newline, fPtr);
         fclose(fPtr);
    }else{
        FILE *fPtr  = fopen(filename, "r");
        FILE *fTemp = fopen("replace.tmp", "w"); 

        /* fopen() renvoie NULL si il échoe à ouvrir le fichier */
        if (fPtr == NULL || fTemp == NULL)
        {
            /* Echoue lors d'ouverture de fichier */
            printf("\nEchoue d'ouverture de fichier.\n");
            printf("Verifiex l'existence de fichier et votre droits.\n");
            exit(EXIT_SUCCESS);
        }


        /*
        * Lire une ligne de source et l'ecrire dans le fichier destinateur
        * en remplacant celle d'avant.
        */


        while ((fgets(buffer, BUFFER_SIZE, fPtr)) != NULL)
        {
            count++;

            /* Si c'est la ligne à remplacer */
            if (count == lineNb)
                fputs(newline, fTemp);
            else
                fputs(buffer, fTemp);
        }


        /* Fermeture de tous les fichiers */
        fclose(fPtr);
        fclose(fTemp);


        /* Supprimer des fichiers */
        remove(filename);

        /* Renommer le fichier temporel */
        rename("replace.tmp", filename);
    }
    return 0;
}

int deleteLine(char *filename, char *userid){
    int lineNb=findToken(filename, userid);
    char buffer[BUFFER_SIZE];
    int count=0;   

    if(lineNb==-1){//Aucune ligne trouvé

         return -1;//USER pas trouvé
    }else{
        FILE *fPtr  = fopen(filename, "r");
        FILE *fTemp = fopen("replace.tmp", "w"); 

        /* fopen() renvoie NULL si il échoe à ouvrir le fichier  */
        if (fPtr == NULL || fTemp == NULL)
        {
            /* Echec d'ouverture */
            printf("\nUnable to open file.\n");
            printf("Please check whether file exists and you have read/write privilege.\n");
            exit(EXIT_SUCCESS);
        }
        /*
        * Lire une ligne de fichier et le placer dans fichier destinateur
        */


        while ((fgets(buffer, BUFFER_SIZE, fPtr)) != NULL)
        {
            count++;

            /* Si c'est la ligne à remplacer */
            if (count != lineNb)
                fputs(buffer, fTemp);
        }


        /*Fermeture de tous les fichiers */
        fclose(fPtr);
        fclose(fTemp);


        /* Supprimer le fichier originel */
        remove(filename);

        /* Renommer le fichier temporel */
        rename("replace.tmp", filename);
    }
    return 0;

}

int lire(char *filename){ 
    FILE *fp;
    char str[BUFFER_SIZE];
    char *tmp;
    int fd = open("new",O_CREAT|O_RDWR|O_APPEND, 0666);
    fp = fopen(filename, "r");
    if (fp == NULL){
        printf("Echec d'ouverture de fichier %s",filename);
        return -1;
    }
    while (fgets(str, BUFFER_SIZE, fp) != NULL){
        strtok(str,":");
        tmp=strtok(NULL,":");
        write(fd,tmp,strlen(tmp)); 
    }
    fclose(fp);
    close(fd);
    return 1;
}