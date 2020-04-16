all: 
	cc client.c -o client
	cc serveurAccess.c traverserFichier.c -o serv -pthread
	cc serveurDonnees.c fichierLectureEcriture.c -o sd

serv:
	cc serveurAccess.c traverserFichier.c -o serv -pthread

client:
	cc client.c -o client

sd: 
	cc serveurDonnees.c fichierLectureEcriture.c -o sd

free: 
	rm -rf fichierDonnees_*

clean:free	
	rm sd 2>/dev/null
	rm serv 2>/dev/null
	rm client 2>/dev/null
	

	