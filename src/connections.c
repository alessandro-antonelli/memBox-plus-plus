/*
 * membox++ Progetto del corso di LSO 2016 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Pelagatti, Torquati
 *
 *
 * Studente: Alessandro Antonelli
 * Matricola 507264
 *
 * Si dichiara che il programma è, in ogni sua parte,
 * opera originale dell'autore.
 * -- Alessandro Antonelli
 * 
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "connections.h"
#include "tools.h"

int openConnection(char* path, unsigned int ntimes, unsigned int secs) //il client apre una connessione verso il server
{
	if(ntimes > MAX_RETRIES) ntimes = MAX_RETRIES;
	if(secs > MAX_SLEEPING) secs = MAX_SLEEPING;

	//creo file socket
	int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	CheckMenoUno(sfd == -1, TRUE, "creazione socket per connessione al server");

	struct sockaddr_un indirizzo;
	indirizzo.sun_family = AF_UNIX;
	strncpy(indirizzo.sun_path, path, strlen(path) + sizeof(char)); // +1 char per il terminatore \0

	while(ntimes > 0)
	{
		StampaTest("[client] Tentativo di connessione...");
		errno = 0;
		int esito = connect(sfd, (struct sockaddr*) &indirizzo, sizeof(indirizzo));
		if(esito == -1)
		{
			//Connessione fallita
			#ifdef DEBUG
			perror("[client] Tentativo di connessione fallito");
			#endif
			ntimes--;
			if(ntimes > 0) sleep(secs);
		}
		else
		{
			//Connessione riuscita
			StampaTest("[client] Tentativo di connessione riuscito");
			return sfd;
		}
	}
	//tutti i tentativi falliti
	return -1;
}

/*	sendRequest
Utilizzata sia dal client che dal server.
Invia il messaggio tramite varie write: 1) write in un colpo solo dell'header 2) se il campo dati è vuoto fine, altrimenti 3) write della lunghezza, unsigned int 4) write del buffer

la memoria allocata per il messaggio che viene passato come parametro, e per ciascuno dei suoi campi (hdr, data), deve essere allocata e anche liberata dal chiamante. NON viene liberata dalla funzione sendRequest.
*/
int sendRequest(long fd, message_t *msg)
{
	CheckMenoUno(msg == NULL, FALSE, "impossibile inviare un messaggio == NULL");
	int retval;

	//Scrivo header
	retval = write(fd, &(msg->hdr), sizeof(message_hdr_t));
	CheckMenoUno(retval != sizeof(message_hdr_t), retval==-1, "write header");

	if(msg->data.buf == NULL || msg->data.len == 0) return 0; //niente corpo

	//Scrivo lunghezza file
	retval = write(fd, &(msg->data.len), sizeof(unsigned int));
	CheckMenoUno(retval != sizeof(unsigned int), retval==-1, "write lunghezza");

	//Scrivo file
	int byteRimanenti = msg->data.len;
	char* writePointer = msg->data.buf;
	while(byteRimanenti > 0)
	{
		retval = write(fd, writePointer, byteRimanenti);
		CheckMenoUno(retval == -1, TRUE, "write buffer");

		byteRimanenti = byteRimanenti - retval;
		writePointer = writePointer + retval;
		assert(writePointer <= msg->data.buf + msg->data.len);
	}

	return 0;
}

/*	readHeader
Utilizzata sia dal client che dal server.
Legge l'header con una sola read, e lo salva nella struttura passata al secondo parametro (che deve puntare ad un header allocato dal chiamante).
*/
int readHeader(long fd, message_hdr_t *hdr)
{
	CheckMenoUno(hdr == NULL, FALSE, "impossibile memorizzare l'header letto su un buffer == NULL");
	int retval = read(fd, hdr, sizeof(message_hdr_t));
	if(retval == 0) return 1; //nessun byte letto: connessione chiusa dall'altro partecipante
	CheckMenoUno(retval != sizeof(message_hdr_t), TRUE, "read dell'header");
	return 0;
}

/*	readData
Utilizzata sia dal client che dal server.
Legge i dati con due read: 1) read della lunghezza, unsigned int 2) read del buffer. Lo salva nella struttura passata al secondo parametro (che deve puntare ad un campo dati allocato dal chiamante). La stringa / buffer dati (data->buf) viene allocata qui e la deallocazione spetta al chiamante, come anche la deallocazione dell'intero campo dati (2° parametro).
*/
int readData(long fd, message_data_t *data)
{
	CheckMenoUno(data == NULL, FALSE, "impossibile memorizzare i dati letti su un buffer == NULL");
	int retval;

	//Leggo lunghezza file
	unsigned int len;
	retval = read(fd, &len, sizeof(unsigned int));
	CheckMenoUno(retval != sizeof(unsigned int), TRUE, "read della lunghezza in byte del corpo del messaggio");
	data->len = len;

	//Leggo file
	data->buf = malloc(len);
	CheckMenoUno(data->buf == NULL, FALSE, "malloc buffer per la lettura del corpo del messaggio");

	int byteRimanenti = len;
	char* writePointer = data->buf;
	while(byteRimanenti > 0)
	{
		retval = read(fd, writePointer, byteRimanenti);
		CheckMenoUno(retval == -1, TRUE, "read del corpo del messaggio");

		byteRimanenti = byteRimanenti - retval;
		writePointer = writePointer + retval;
		assert(writePointer <= data->buf + len);
	}

	return 0;
}
