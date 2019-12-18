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
/**
 * @file membox.c
 * @brief File principale del server membox
 */

/* --------------------------------------------------------------------------------------------------- */
/*                                            PREPROCESSORE                                            */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

/* inserire gli altri include che servono */

#include "stats.h"
#include "message.h"
#include "connections.h"
#include "tools.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "icl_hash.h"
#include "queue.h"
#include "configurazione.h"
#include <errno.h>
#include <signal.h>
#include <math.h>

#define DEFBUCKETS 1024
// Numero di default dei buckets che compongono la tabella hash (utilizzato quando la configurazione non specifica quale sia il numero massimo di elementi).

#define MAXBUCKETS 3172
// Numero massimo dei buckets che compongono la tabella hash (utilizzato quando la configurazione specifica un numero max troppo alto - maggiore di 15000).

#define DEFMUTEX 40
// Numero di default dei mutex in cui mappare i bucket della tabella hash

#define MAXMUTEX 125
// Numero massimo dei mutex in cui mappare i bucket della tabella hash

#define MAXLOCKEDOBJ 1000
// Numero massimo degli oggetti di cui un client può detenere la lock contemporaneamente

/*                                            PREPROCESSORE                                            */
/* --------------------------------------------------------------------------------------------------- */
/*                                          VARIABILI GLOBALI                                          */

struct statistics stats = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
pthread_mutex_t statsMutex = PTHREAD_MUTEX_INITIALIZER;
// struttura che memorizza le statistiche del server, struct statistics è definita in stats.h 

config_t config = {"", -1, -1, -1, -1, -1, "", ""};
// struttura che memorizza la configurazione con cui è avviato il server, definita in configurazione.h
// NB: non c'è un mutex associato a questa variabile globale perché è acceduto in scrittura una volta sola, cioè allo startup del server quando c'è un unico thread in esecuzione, e dopo la creazione dei thread viene acceduto solo in lettura e mai in scrittura, per cui non è necessaria la mutua esclusione.

icl_hash_t *hashTable = NULL;
// tabella hash dove saranno effettivamente memorizzati i file
static pthread_mutex_t* hashMutex = NULL;
// array dei mutex che gestiscono la mutua esclusione tra i bucket della hash table
int nMutex = 0;
// cardinalità di hashMutex (numero dei mutex in cui vengono mappati i bucket)

queue_t *coda = NULL;
pthread_mutex_t codaMutex = PTHREAD_MUTEX_INITIALIZER;
// coda dove saranno inserite le connessioni accettate prima di essere prese in carico da un thread del pool

pthread_cond_t arrivatoClient = PTHREAD_COND_INITIALIZER;
// variabile di condizione che segnala l'arrivo di un nuovo elemento nella coda

pthread_mutex_t TermMutex = PTHREAD_MUTEX_INITIALIZER;
//unico mutex a garanzia della mutua esclusione di TermDolce e TermImmediata (acquisita la lock si può accedere ad entrambe le var. globali)

int TermDolce = FALSE;
// se true, indica che si è ricevuto un segnale SIGUSR2 e pertanto bisogna smettere di accettare nuove connessioni
// Può essere impostata a TRUE solo dal signal handler; viene letta dai thread del pool e dal thread che accetta le connessioni

int TermImmediata = FALSE;
// se true, indica che si è ricevuto un segnale SIGINT, SIGTERM o SIGQUIT e pertanto bisogna chiudere tutte le connessioni aperte
// Può essere impostata a TRUE solo dal signal handler; viene letta dai thread del pool e dal thread che accetta le connessioni

int lockRepository = -1;
pthread_mutex_t lockRepositoryMutex = PTHREAD_MUTEX_INITIALIZER;
// variabile utilizzata per tenere conto dell'acquisizione della lock sull'intero repository (operazioni LOCK_OP e UNLOCK_OP).
// La variabile vale -1 quando il repository NON è in stato di lock. Quando il valore è diverso da -1 è in stato di lock,
// e il valore della variabile è il file descriptor del socket relativo alla connessione che attualmente detiene la lock.

/*                                          VARIABILI GLOBALI                                          */
/* --------------------------------------------------------------------------------------------------- */
/*                                               FUNZIONI                                              */

//Funzione per liberare una chiave della tabella hash
void LiberaChiave (void* chiave)
{
	if(chiave != NULL)
		free((membox_key_t*) chiave);
	return;
}

//Funzione per liberare una entry della tabella hash
void LiberaEntry (void* _entry)
{
	hashTableEntry* entry = (hashTableEntry*) _entry;

	if(entry != NULL)
	{
		if(entry->data != NULL)
			free(entry->data);
		if(entry->codaRichiedentiLock != NULL)
			eliminaCoda(entry->codaRichiedentiLock);
		if(entry->rilascioLockObj != NULL)
			free(entry->rilascioLockObj);
		free(entry);
	}
	return;
}

static inline unsigned int fnv_hash_function( void *key, int len ) {
    unsigned char *p = (unsigned char*)key;
    unsigned int h = 2166136261u;
    int i;
    for ( i = 0; i < len; i++ )
        h = ( h * 16777619 ) ^ p[i];
    return h;
}

static inline unsigned int ulong_hash_function( void *key )
{
    int len = sizeof(unsigned long);
    unsigned int hashval = fnv_hash_function( key, len );
    return hashval;
}

static inline int ulong_key_compare( void *key1, void *key2  )
{
    return ( *(unsigned long*)key1 == *(unsigned long*)key2 );
}

void* attendiSegnali(void* statFile)
{
	int segnaleArrivato;
	sigset_t set;
	CheckExit(sigemptyset(&set) == -1, FALSE, "settaggio a 0 di tutti i segnali dell'insieme");
	CheckExit(sigaddset(&set, SIGINT) == -1, FALSE, "settaggio a 1 del segnale SIGINT");
	CheckExit(sigaddset(&set, SIGTERM) == -1, FALSE, "settaggio a 1 del segnale SIGTERM");
	CheckExit(sigaddset(&set, SIGQUIT) == -1, FALSE, "settaggio a 1 del segnale SIGQUIT");
	CheckExit(sigaddset(&set, SIGUSR1) == -1, FALSE, "settaggio a 1 del segnale SIGUSR1");
	CheckExit(sigaddset(&set, SIGUSR2) == -1, FALSE, "settaggio a 1 del segnale SIGUSR2");

	while(1)
	{
		StampaTest("[thread gestione segnali] mi sospendo");
		//Mi sospendo in attesa di uno dei segnali
		CheckExit(sigwait(&set, &segnaleArrivato) != 0, FALSE, "sospesione in attesa di uno dei cinque segnali (term immediata / dolce, stampa statistiche)");
		StampaTest("[thread gestione segnali] mi risveglio");

		//Riconosco segnale

		if(segnaleArrivato == SIGUSR1)
		{
			StampaTest("[thread gestione segnali] arrivato segnale stampa statistiche");
			//stampa statistiche
			if(strcmp(config.StatFileName, "") != 0)
				CheckNULL(printStats((FILE*) statFile) != 0, FALSE, "errore nella stampa delle statistiche");
			continue;
		}

		if(segnaleArrivato == SIGINT || segnaleArrivato == SIGTERM || segnaleArrivato == SIGQUIT)
		{
			StampaTest("[thread gestione segnali] arrivato segnale terminazione immediata");
			//terminazione immediata
			MyLock(TermMutex);
			TermImmediata = 1;
			MyUnlock(TermMutex);
				//risveglio tutti i thread del pool in attesa di connessioni da servire
			CheckNULL(pthread_cond_broadcast(&arrivatoClient) != 0, FALSE, "signal arrivatoClient");
			break;
		}

		if(segnaleArrivato == SIGUSR2)
		{
			StampaTest("[thread gestione segnali] arrivato segnale terminazione dolce");
			//terminazione dolce
			MyLock(TermMutex);
			TermDolce = 1;
			MyUnlock(TermMutex);
				//risveglio tutti i thread del pool in attesa di connessioni da servire
			CheckNULL(pthread_cond_broadcast(&arrivatoClient) != 0, FALSE, "signal arrivatoClient");
			break;
		}
	}

	return (void*) NULL;
}

/*
	AccettaConnessioni
E' la funzione eseguita dal thread che si occupa di accettare le connessioni in arrivo dai client. Accettata una connessione, agisce da produttore,
inserendo il file descriptor della nuova connessione nella coda e svegliando i thread del pool che erano in attesa (consumatori).

Nel caso in cui la coda sia piena (si è superato MaxConnection) la connessione appena accettata viene chiusa (subito dopo
avergli inviato una risposta con codice OP_FAIL) e la procedura torna ad attendere la successiva richiesta di connessione.
*/
void* AccettaConnessioni(void* param)
{
	int sfd = *((int*) param);
	while(1)
	{
		//esco dal while (cioè termino l'esecuzione del thread) nel caso sia arrivata una richiesta di terminazione dolce o immediata
		MyLock(TermMutex);
		if(TermDolce || TermImmediata)
		{
			MyUnlock(TermMutex);
			break;
		}
		else
		{
			MyUnlock(TermMutex);
		}

		StampaTest("[thread accetta connessioni] sto per fare accept");
		//accetto una nuova connessione
		int sockConn = accept(sfd, NULL, NULL);
		CheckExit(sockConn == -1, TRUE, "accept di una nuova connessione");
		StampaTest("[thread accetta connessioni] Connessione accettata");

		MyLock(statsMutex);
		if(stats.concurrent_connections == config.MaxConnections) //raggiunte le MaxConnection
		{
			MyUnlock(statsMutex);
			StampaTest("[thread accetta connessioni] Raggiunto il numero massimo di connessioni: connect rifiutata");

			//rispondo con messaggio di fallimento
			message_t* risposta = malloc(sizeof(message_t));
			CheckExit(risposta == NULL, FALSE, "malloc risposta OP_FAIL");

			membox_key_t* chiave = malloc(sizeof(membox_key_t));
			CheckExit(chiave == NULL, FALSE, "malloc chiave -1");

			*chiave = -1;
			setHeader(risposta, OP_FAIL, chiave);
			free(chiave);

			risposta->data.len = 0;
			risposta->data.buf = NULL;

			CheckExit(sendRequest((long) sockConn, risposta) != 0, FALSE, "invio risposta OP_FAIL");

			//chiudo la connessione e ricomincio il ciclo con l'accept di una nuova connessione
			CheckExit(close(sockConn) == -1, TRUE, "chiusura connessione che aveva provocato sforamento MaxConnection");
			free(risposta);
			continue;
		}

		//numero di connessioni non superato: metto il client in coda
		stats.concurrent_connections++;
		MyUnlock(statsMutex);

		MyLock(codaMutex);
		assert(coda->elementiAttuali < coda->elementiMax);

		CheckExit(enqueue(coda, sockConn) == -1, FALSE, "enqueue");
		MyUnlock(codaMutex);
		StampaTest("[thread accetta connessioni] Connessione messa in coda");

		//sveglio un thread del pool
		CheckExit(pthread_cond_signal(&arrivatoClient) != 0, FALSE, "signal arrivatoClient");
	}
	return (void*) NULL;
}

int SalvaDump(); //definita più in basso

/*
	DoUnlock(elemento)
Contiene il codice che esegue effettivamente la unlock dell'oggetto del repository passato per argomento, occupandosi di toglierla all'attuale detentore e di assegnarla al primo client tra quelli in coda (se ce ne sono).
Viene chiamata da EseguiRichiesta (durante l'esecuzione di una UNLOCK_OBJ_OP) e da ServiClient (alla disconnessione del client, nel caso avesse lasciato degli oggetti in stato di lock).

Deve essere invocata solo dopo aver acquisito la mutua esclusione sulla tabella hash, cioè dopo aver invocato LockHash(key).

Restituisce 0 in caso di successo, -1 in caso di errore.
	*/
static int DoUnlock(hashTableEntry* elemento)
{
	CheckMenoUno(elemento == NULL, FALSE, "puntatore NULL all'elemento da sbloccare");
	CheckMenoUno(elemento->detentoreLock == -1, FALSE, "sblocco di oggetto non in stato di lock");

	if(elemento->codaRichiedentiLock == NULL)
	{
		elemento->detentoreLock = -1;
		StampaTest("[pool] Lock obj rilasciata - non c'era nessun altro in coda a cui assegnarla");
	}
	else
	{
		//assegno la lock al prossimo e lo risveglio
		elemento->detentoreLock = dequeue(elemento->codaRichiedentiLock);

		CheckMenoUno(elemento->detentoreLock == -1, FALSE, "dequeue del nuovo detentore lock oggetto");
		StampaTest("[pool] Lock obj rilasciata e assegnata al primo in coda");
		if(elemento->codaRichiedentiLock->elementiAttuali == 0)
		{
			eliminaCoda(elemento->codaRichiedentiLock);
			elemento->codaRichiedentiLock = NULL;
			StampaTest("[pool] Quello estratto era l'ultimo client in attesa di lock obj: smantellata coda");
		}
		CheckMenoUno(elemento->rilascioLockObj == NULL, FALSE, "la variabile di condizione rilascio lock oggetto è NULL!");
		CheckMenoUno(pthread_cond_broadcast(elemento->rilascioLockObj) != 0, FALSE, "segnalazione rilascio lock oggetto");
	}
	return 0;
}

/*	EseguiRichiesta
Viene invocata esclusivamente da ServiClient, e pertanto è eseguita dai thread del pool che servono il client.
La funzione interpreta il messaggio ricevuto dal client, esegue l'operazione richiesta e aggiorna le statistiche
Restituisce un messaggio che contiene la risposta da inviare al client con l'esito dell'operazione (la risposta è allocata da questa funzione e deve essere deallocata dal chiamante, dopo averla inviata).
*/
static message_t* EseguiRichiesta(message_hdr_t* headerRichiesta, message_data_t* corpoRichiesta, int sfd)
{
	message_t* reply = (message_t*) malloc(sizeof(message_t));
	CheckExit(reply == NULL, FALSE, "malloc messaggio di risposta al client");

	setHeader(reply, OP_UNKNOWN, &(headerRichiesta->key));
	reply->data.len = 0;
	reply->data.buf = NULL;


	switch (headerRichiesta->op)
	{
		case PUT_OP : // inserimento di un oggetto nel repository
		{
			StampaTest("[pool] Ricevuta PUT");
			CheckExit(corpoRichiesta == NULL || corpoRichiesta->buf == NULL || corpoRichiesta->len <= 0, FALSE, "operazione di PUT senza il contenuto da inserire");

			//Controllo se la PUT è autorizzata
			MyLock(lockRepositoryMutex);
			if(lockRepository != -1 && lockRepository != sfd) reply->hdr.op = OP_LOCKED; //repository in stato di lock
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op == OP_UNKNOWN)
			{
				if(config.MaxObjSize > 0 && corpoRichiesta->len > config.MaxObjSize) reply->hdr.op = OP_PUT_SIZE; //put di un oggetto troppo grande
				else
				{
					MyLock(statsMutex);
					if(config.StorageSize > 0 && stats.current_objects >= config.StorageSize)
						reply->hdr.op = OP_PUT_TOOMANY; //raggiunto il massimo numero di oggetti
					if(config.StorageByteSize > 0 && stats.current_size >= config.StorageByteSize)
						reply->hdr.op = OP_PUT_REPOSIZE; // superata la massima size del repository
					MyUnlock(statsMutex);
				}
			}
			
			hashTableEntry* new = NULL;
			if(reply->hdr.op == OP_UNKNOWN)
			{
				while(1) //Controllo se la chiave è già presente
				{
					LockHash(headerRichiesta->key);
					new = icl_hash_find(hashTable, (void*) &(headerRichiesta->key));
					if(new == NULL) break; //non presente, OK

					if(new->detentoreLock == -2)
					{
						//Chiave già presente ma in eliminazione: attendo finché non è eliminata davvero
						UnlockHash(headerRichiesta->key);
						sleep(1);
					}
					else
					{
						reply->hdr.op = OP_PUT_ALREADY; //Chiave già presente
						break;
					}
				}

				//Se autorizzata: eseguo la PUT
				if(reply->hdr.op == OP_UNKNOWN)
				{
					membox_key_t* chiave = malloc(sizeof(membox_key_t));
					CheckExit(chiave == NULL, FALSE, "malloc chiave durante PUT");
					*chiave = headerRichiesta->key;

					new = malloc(sizeof(hashTableEntry));
					CheckExit(new == NULL, FALSE, "malloc nuova entry della tabella hash durante PUT");

					new->len = corpoRichiesta->len;
					new->data = corpoRichiesta->buf;
					new->detentoreLock = -1;
					new->codaRichiedentiLock = NULL;
					new->rilascioLockObj = NULL;


					if(icl_hash_insert(hashTable, (void*) chiave, (void*) new) != NULL)
						reply->hdr.op = OP_OK; //Successo
					else
					{
						reply->hdr.op = OP_FAIL; //Fallimento
						free(chiave);
						free(new);
					}
				}
				UnlockHash(headerRichiesta->key);
			}
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.nput++;
			if(reply->hdr.op != OP_OK) stats.nput_failed++;
			else
			{
				stats.current_size += new->len;
				if(stats.max_size < stats.current_size) stats.max_size = stats.current_size;
				stats.current_objects++;
				if(stats.max_objects < stats.current_objects) stats.max_objects = stats.current_objects;
			}
			MyUnlock(statsMutex);

			break;
		}

		case UPDATE_OP : // aggiornamento di un oggetto gia' presente
		{
			StampaTest("[pool] Ricevuta UPDATE");
			CheckExit(corpoRichiesta == NULL || corpoRichiesta->buf == NULL || corpoRichiesta->len <= 0, FALSE, "operazione di PUT senza il contenuto da inserire");

			//Controllo se la UPDATE è autorizzata
			MyLock(lockRepositoryMutex);
			if(lockRepository != -1 && lockRepository != sfd) reply->hdr.op = OP_LOCKED; //repository in stato di lock
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op == OP_UNKNOWN)
				if(config.MaxObjSize > 0 && corpoRichiesta->len > config.MaxObjSize) reply->hdr.op = OP_PUT_SIZE; //put di un oggetto troppo grande
			
			if(reply->hdr.op == OP_UNKNOWN)
			{
				LockHash(headerRichiesta->key);
				hashTableEntry* entry = (hashTableEntry*) icl_hash_find(hashTable, (void*) &(headerRichiesta->key));
				if(entry == NULL)
					reply->hdr.op = OP_UPDATE_NONE; //update di un oggetto non presente
				else
				{
					if(entry->len != corpoRichiesta->len) reply->hdr.op = OP_UPDATE_SIZE; //size dell'oggetto da aggiornare non corrispondente
					if(entry->detentoreLock == -2) reply->hdr.op = OP_UPDATE_NONE; //update di un oggetto non presente
					else if(entry->detentoreLock != -1 && entry->detentoreLock != sfd) reply->hdr.op = OP_LOCKED; //oggetto in stato di lock
				}

				//Se autorizzata: eseguo UPDATE
				if(reply->hdr.op == OP_UNKNOWN)
				{
					hashTableEntry* oldData = NULL;

					membox_key_t* chiave = malloc(sizeof(membox_key_t));
					CheckExit(chiave == NULL, FALSE, "malloc chiave durante UPDATE");
					*chiave = headerRichiesta->key;

					hashTableEntry* new = malloc(sizeof(hashTableEntry));
					CheckExit(new == NULL, FALSE, "malloc nuova entry della tabella hash durante UPDATE");

					new->len = corpoRichiesta->len;
					new->data = corpoRichiesta->buf;
					new->detentoreLock = entry->detentoreLock;
					new->codaRichiedentiLock = entry->codaRichiedentiLock;
					new->rilascioLockObj = entry->rilascioLockObj;

					if(icl_hash_update_insert(hashTable, (void*) chiave, (void*) new, (void**) &oldData) != NULL)
					{
						reply->hdr.op = OP_OK;
						/* Chiamo LiberaEntry e, prima di farlo, svuoto i puntatori alle strutture dati riutilizzate
						   nella nuova entry, per evitare che vengano liberate corrompendo i puntatori della nuova entry */
						oldData->codaRichiedentiLock = NULL;
						oldData->rilascioLockObj = NULL;
						LiberaEntry((void*) oldData);
					}
					else
					{
						reply->hdr.op = OP_FAIL;
						free(chiave);
						free(new);
					}
				}
				UnlockHash(headerRichiesta->key);
			}
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.nupdate++;
			if(reply->hdr.op != OP_OK) stats.nupdate_failed++;
			MyUnlock(statsMutex);

			break;
		}
		case GET_OP : // recupero di un oggetto dal repository
		{
			StampaTest("[pool] Ricevuta GET");

			MyLock(lockRepositoryMutex);
			if(lockRepository != -1 && lockRepository != sfd) reply->hdr.op = OP_LOCKED; //repository in stato di lock
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op == OP_UNKNOWN)
			{
				//Eseguo GET controllando eventuale assenza dell'elemento
				LockHash(headerRichiesta->key);
				hashTableEntry* entry = (hashTableEntry*) icl_hash_find(hashTable, (void*) &(headerRichiesta->key));

				if(entry == NULL)
				{
					StampaTest("Get di un oggetto non presente!");
					reply->hdr.op = OP_GET_NONE; //get di un oggetto non presente
				}
				else
				{
					if(entry->detentoreLock == -2) reply->hdr.op = OP_GET_NONE; //get di un oggetto non presente
					else
					{	//OGGETTO PRESENTE
						if(entry->detentoreLock != -1 && entry->detentoreLock != sfd) reply->hdr.op = OP_LOCKED; //oggetto in stato di lock
						else
						{
							StampaTest("GET di un oggetto presente...");
							reply->hdr.op = OP_OK;
							unsigned int len = entry->len;

							reply->data.len = len;
							reply->data.buf = entry->data;
						}
					}
				}
				UnlockHash(headerRichiesta->key);
			}
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.nget++;
			if(reply->hdr.op != OP_OK) stats.nget_failed++;
			MyUnlock(statsMutex);

			break;
		}
		case REMOVE_OP : // eliminazione di un oggetto dal repository
		{
			StampaTest("[pool] Ricevuta REMOVE");
			unsigned int len = 0;
			hashTableEntry* oldItem = NULL;

			/* -------------------------- Controllo se la REMOVE è autorizzata -------------------------- */

			MyLock(lockRepositoryMutex);
			if(lockRepository != -1 && lockRepository != sfd) reply->hdr.op = OP_LOCKED; //repository in stato di lock
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op != OP_LOCKED) LockHash(headerRichiesta->key);

			if(reply->hdr.op == OP_UNKNOWN)
			{
				oldItem = (hashTableEntry*) icl_hash_find(hashTable, (void*) &(headerRichiesta->key));
				if(oldItem == NULL) reply->hdr.op = OP_REMOVE_NONE; //rimozione di un oggetto non presente
			}

			if(reply->hdr.op == OP_UNKNOWN)
				if(oldItem->detentoreLock == -2) reply->hdr.op = OP_REMOVE_NONE; //rimozione di un oggetto non presente

			if(reply->hdr.op == OP_UNKNOWN)
				if(oldItem->detentoreLock != -1 && oldItem->detentoreLock != sfd) reply->hdr.op = OP_LOCKED; //oggetto in stato di lock

			/* -------------------------- Eseguo la REMOVE -------------------------- */
			if(reply->hdr.op == OP_UNKNOWN)
			{
				len = oldItem->len;
				if(oldItem->codaRichiedentiLock != NULL)
				{ //ci sono client in attesa di ottenere lock: lo marco "in eliminazione" poi ci penseranno loro
					oldItem->detentoreLock = -2;
					reply->hdr.op = OP_OK;
					//sveglio tutti i client, che altrimenti attenderebbero all'infinito
					CheckExit(pthread_cond_broadcast(oldItem->rilascioLockObj) != 0, FALSE, "risveglio client in attesa di lock su oggetto cancellato");
				}
				else
				{ //non ci sono client in attesa di ottenere la lock: posso eliminarlo io
					if(icl_hash_delete(hashTable, (void*) &(headerRichiesta->key), LiberaChiave, LiberaEntry) == 0)
						reply->hdr.op = OP_OK;
					else
						reply->hdr.op = OP_FAIL;
				}
			}

			if(reply->hdr.op != OP_LOCKED) UnlockHash(headerRichiesta->key);
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.nremove++;
			if(reply->hdr.op != OP_OK) stats.nremove_failed++;
			else
			{
				stats.current_size -= len;
				stats.current_objects--;
			}
			MyUnlock(statsMutex);

			break;
		}
		case LOCK_OP : // acquisizione di una lock su tutto il repository
		{
			StampaTest("[pool] Ricevuta LOCK");

			MyLock(lockRepositoryMutex);
			if(lockRepository != -1) reply->hdr.op = OP_LOCKED; //sistema già in stato di lock (fatta da questo o altri client)
			else
			{
				//posso fare lock
				lockRepository = sfd;
				reply->hdr.op = OP_OK;
			}
			MyUnlock(lockRepositoryMutex);
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.nlock++;
			if(reply->hdr.op != OP_OK) stats.nlock_failed++;
			MyUnlock(statsMutex);

			break;
		}
		case UNLOCK_OP : // rilascio della lock su tutto il repository
		{
			StampaTest("[pool] Ricevuta UNLOCK");

			MyLock(lockRepositoryMutex);
			if(lockRepository == -1) reply->hdr.op = OP_LOCK_NONE; //non era in stato di lock
			else
			{
				if(lockRepository != sfd) reply->hdr.op = OP_LOCKED; //è in stato di lock, ma fatta da qualcun altro
				else
				{
					//posso fare unlock
					lockRepository = -1;
					reply->hdr.op = OP_OK;
				}
			}
			MyUnlock(lockRepositoryMutex);

			break;
		}
		case LOCK_OBJ_OP : // acquisizione di una lock su un oggetto
		{
			StampaTest("[pool] Ricevuta LOCK OBJ");
			hashTableEntry* elemento = NULL;

			/* -------------------------- Controllo se la LOCK OBJ è autorizzata -------------------------- */

			MyLock(lockRepositoryMutex);
			if(lockRepository != -1 && lockRepository != sfd) reply->hdr.op = OP_LOCKED; //intero repository già in stato di lock
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op != OP_LOCKED) LockHash(headerRichiesta->key);

			if(reply->hdr.op == OP_UNKNOWN)
			{
				elemento = icl_hash_find(hashTable, (void*) &(headerRichiesta->key));
				if(elemento == NULL) reply->hdr.op = OP_LOCK_OBJ_NONE; //chiave non esistente
			}
			
			if(reply->hdr.op == OP_UNKNOWN)
				if(elemento->detentoreLock == -2) reply->hdr.op = OP_LOCK_OBJ_NONE; //chiave non esistente

			if(reply->hdr.op == OP_UNKNOWN)
				if(elemento->detentoreLock == sfd) reply->hdr.op = OP_LOCK_OBJ_DLK; //già locked dallo stesso client

			/* -------------------------- Eseguo la LOCK OBJ -------------------------- */

			if(reply->hdr.op == OP_UNKNOWN)
			{
				if(elemento->detentoreLock == -1) //non era locked
				{ 
					//Assegno la lock dell'oggetto al client
					elemento->detentoreLock = sfd;
					reply->hdr.op = OP_OK;
					StampaTest("[pool] Ho acquisito la LOCK OBJ senza dover passare per la coda!");
				}
				else //già locked da qualcun altro
				{

					//metto il client nella coda dei richiedenti di quell'oggetto
					if(elemento->codaRichiedentiLock == NULL)
					{	elemento->codaRichiedentiLock = creaCoda(config.ThreadsInPool);
						CheckExit(elemento->codaRichiedentiLock == NULL, FALSE, "creazione coda richiedenti LOCK_OBJ");
					}

					CheckExit(enqueue(elemento->codaRichiedentiLock, sfd) == -1, FALSE, "inserimento richiedente LOCK_OBJ in coda");

					if(elemento->rilascioLockObj == NULL)
					{
						elemento->rilascioLockObj = malloc(sizeof(pthread_cond_t));
						CheckExit(elemento->rilascioLockObj == NULL, FALSE, "malloc var. condizione rilascio lock oggetto");
						CheckExit(pthread_cond_init(elemento->rilascioLockObj, NULL) != 0, FALSE, "inizializz. var. condizione rilascio lock oggetto");
					}

					StampaTest("[pool] Sono in coda e mi sospendo in attesa di acquisire la LOCK OBJ!");

					while(1)
					{
						pthread_mutex_t* indMutex = &hashMutex[((* hashTable->hash_function)((void*) &headerRichiesta->key) % hashTable->nbuckets) % nMutex];
						CheckExit(pthread_cond_wait(elemento->rilascioLockObj, indMutex) != 0, FALSE, "attesa rilascio lock oggetto");
						StampaTest("Mi sono risvegliato dall'attesa per la LOCK OBJ ma sono ancora dentro il ciclo");

						/* Aggiorno il puntatore all'entry di cui si richiede la lock: potrebbe non essere più valido
						   perché il detentore potrebbe aver effettuato una UPDATE */
						elemento = icl_hash_find(hashTable, (void*) &(headerRichiesta->key));

						if(elemento->detentoreLock == sfd)
						{	//Mi hanno finalmente dato la lock dell'oggetto!
							reply->hdr.op = OP_OK;
							break;
						}
						if(elemento->detentoreLock == -2)
						{	//Il detentore ha eliminato l'oggetto che attendevo :(
							reply->hdr.op = OP_LOCK_OBJ_NONE;
							break;
						}
					}

					if(reply->hdr.op == OP_LOCK_OBJ_NONE) //esco dal while ma l'oggetto non c'è più
					{
						StampaTest("[pool] Ero in attesa per la LOCK_OBJ ma l'oggetto è stato eliminato!");
						elemento->codaRichiedentiLock->elementiAttuali--;
						if(elemento->codaRichiedentiLock->elementiAttuali == 0)
						{	//Sono l'ultimo di quelli in coda ad accorgermi che hanno cancellato l'oggetto: provvedo a rimuoverlo
							CheckExit(icl_hash_delete(hashTable, (void*) &(headerRichiesta->key), LiberaChiave, LiberaEntry) == -1, \
							FALSE, "eliminazione definitiva oggetto con detentoreLock == -2");
						}
					}
					else //esco dal while ed ho acquisito la lock
					{
						StampaTest("[pool] Ho terminato l'attesa in coda e ho acquisito la LOCK OBJ!");
						if(elemento->codaRichiedentiLock == NULL)
						{
							//Nel caso in cui io ero l'ultimo in coda, libero la variabile di condizione che segnala il rilascio della lock
							free(elemento->rilascioLockObj);
							elemento->rilascioLockObj = NULL;
						}
					}
				}
			}

			if(reply->hdr.op != OP_LOCKED) UnlockHash(headerRichiesta->key);
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.nobjlock++;
			if(reply->hdr.op != OP_OK) stats.nobjlock_failed++;
			MyUnlock(statsMutex);

			break;
		}
		case UNLOCK_OBJ_OP : // rilascio della lock su un oggetto
		{
			StampaTest("[pool] Ricevuta UNLOCK OBJ");
			hashTableEntry* elemento = NULL;

			/* -------------------------- Controllo se la UNLOCK OBJ è autorizzata -------------------------- */

			MyLock(lockRepositoryMutex);
			if(lockRepository != -1 && lockRepository != sfd) reply->hdr.op = OP_LOCKED; //intero repository in stato di lock
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op != OP_LOCKED) LockHash(headerRichiesta->key);

			if(reply->hdr.op == OP_UNKNOWN)
			{
				elemento = icl_hash_find(hashTable, (void*) &(headerRichiesta->key));
				if(elemento == NULL) reply->hdr.op = OP_LOCK_OBJ_NONE; //chiave non esistente
			}

			if(reply->hdr.op == OP_UNKNOWN)
				if(elemento->detentoreLock == -2) reply->hdr.op = OP_LOCK_OBJ_NONE; //chiave non esistente

			if(reply->hdr.op == OP_UNKNOWN)
				if(elemento->detentoreLock == -1) reply->hdr.op = OP_LOCK_NONE; //l'elemento non era in stato di lock

			if(reply->hdr.op == OP_UNKNOWN)
				if(elemento->detentoreLock != sfd) reply->hdr.op = OP_FAIL; //unlock di elemento che era stato messo in lock da un altro client

			/* -------------------------- Eseguo la UNLOCK OBJ -------------------------- */

			if(reply->hdr.op == OP_UNKNOWN)
			{
				StampaTest("[pool] Sto per fare la unlock dietro richiesta esplicita");
				if(DoUnlock(elemento) == 0) reply->hdr.op = OP_OK;
				else reply->hdr.op = OP_FAIL;
			}

			if(reply->hdr.op != OP_LOCKED) UnlockHash(headerRichiesta->key);
			break;
		}
		case DUMP_OP : // richiesta di dump
		{
			StampaTest("[pool] Ricevuta DUMP");

			MyLock(lockRepositoryMutex);
			if(lockRepository == -1) reply->hdr.op = OP_NO_DUMP; //insuccesso: il repository non è in stato di lock
			else if(lockRepository != sfd) reply->hdr.op = OP_LOCKED; //insuccesso: qualcun altro ha fatto la lock sul repository
			MyUnlock(lockRepositoryMutex);

			if(reply->hdr.op == OP_UNKNOWN)
			{
				if(strcmp(config.DumpFile, "") == 0) reply->hdr.op = OP_NO_DUMP; //insuccesso: nella configurazione non è indicato il percorso dove salvare il dump
				else		if(SalvaDump() == 0) reply->hdr.op = OP_OK;
						else reply->hdr.op = OP_FAIL;
			}
			
			//aggiorno statistiche
			MyLock(statsMutex);
			stats.ndump++;
			if(reply->hdr.op != OP_OK) stats.ndump_failed++;
			MyUnlock(statsMutex);

			break;
		}
		default:
		{
			// Caso in cui la richiesta pervenuta ha un tipo di operazione sconosciuto. La riga vuota è stata inserita per evitare il warning.
			// Non c'è nulla da fare perché il messaggio "reply", che viene restituito al chiamante, ha già il campo op settato a OP_UNKNOWN,
			// la parte dati a NULL e la lunghezza dei dati a 0.
		}
	}

	return reply;
}

void* ServiClient(void* param)
{
	int i, numOggettiLocked = 0;
	membox_key_t* oggettiLocked = malloc(MAXLOCKEDOBJ * sizeof(membox_key_t));
	/* Array che contiene le chiavi degli oggetti su cui il client servito ha fatto un'operazione di lock (LOCK_OBJ) ma non ancora una unlock (UNLOCK_OBJ);
	   sono significative solo le prime numOggettiLocked posizioni dell'array.
	   Serve per sbloccare eventuali oggetti rimasti in stato di lock dopo la disconnessione del client, senza dover scandire tutto il repository.  */

	while(1)
	{
	/*	Se è arrivata una richiesta di terminazione immediata termino l'esecuzione del thread.
		Se la coda è vuota mi sospendo in attesa che vi venga inserito almeno un client; ad ogni risveglio
		ripeto anche il controllo della terminazione immediata per maggiore reattività
		Inoltre: esco dal while (cioè termino l'esecuzione del thread) nel caso sia arrivata una richiesta di terminazione dolce
		(sennò si sospende, per assenza di connessioni da prelevare, e quando l'handler del segnale lo risveglia si rimette a dormire)		*/

		MyLock(codaMutex); //lock per controllo emptyness e dequeue
		MyLock(TermMutex);
		while(coda->elementiAttuali == 0 || TermImmediata || TermDolce)
		{
			if(TermImmediata || TermDolce)
			{
				MyUnlock(TermMutex);
				MyUnlock(codaMutex);
				free(oggettiLocked);
				return (void*) NULL;
			}
			else	MyUnlock(TermMutex);

			StampaTest("[pool] mi sospendo in attesa di connessioni");
			//mi metto in attesa di essere svegliato
			CheckExit(pthread_cond_wait(&arrivatoClient, &codaMutex) != 0, FALSE, "wait di almeno un client nella coda");

			MyLock(TermMutex);
		}
		MyUnlock(TermMutex);

		//Estraggo dalla coda il socket file descriptor del client con cui dovrò comunicare
		int sockConn = dequeue(coda);
		MyUnlock(codaMutex); //dopo controllo emptyness e dequeue posso fare unlock
		CheckExit(sockConn == -1, FALSE, "dequeue");
		StampaTest("[pool] Connessione letta dalla coda!");

	/*		Abbiamo un client connesso da servire: si apre il ciclo delle richieste provenienti da questa connessione	*/

		message_hdr_t* header = malloc(sizeof(message_hdr_t));
		CheckExit(header == NULL, FALSE, "malloc header");

		message_data_t* corpo = malloc(sizeof(message_data_t));
		CheckExit(corpo == NULL, FALSE, "malloc corpo");
		corpo->buf = NULL;
		corpo->len = 0;

		int esito = readHeader((long) sockConn, header);

		while(esito == 0)
		{
			StampaTest("[pool] letto header");

			//Leggo l'eventuale corpo del messaggio
			if (header->op == PUT_OP || header->op == UPDATE_OP)
			{
				if(readData((long) sockConn, corpo) != 0) break; //!= 0 significa che il client si è disconnesso prima di comunicare il corpo (che era necessario)
				StampaTest("[pool] letti dati");
			}

			//interpreto msg, eseguo operazione richiesta, aggiorno statistiche
			message_t* risposta = NULL;
			if (header->op == PUT_OP || header->op == UPDATE_OP) risposta = EseguiRichiesta(header, corpo, sockConn);
			else risposta = EseguiRichiesta(header, NULL, sockConn);

			if ((header->op == PUT_OP || header->op == UPDATE_OP) && risposta->hdr.op != OP_OK) if(corpo->buf != NULL) free(corpo->buf);

			//aggiorno elenco oggetti locked dal client
			if(header->op == LOCK_OBJ_OP && risposta->hdr.op == OP_OK)
			{
				StampaTest("[pool] Aggiunto un file all'elenco di quelli da sbloccare alla disconnessione");
				assert(numOggettiLocked < MAXLOCKEDOBJ); //se fallisce, alzare il valore contenuto nella macro MAXLOCKEDOBJ
				oggettiLocked[numOggettiLocked] = header->key;
				numOggettiLocked++;
			}
			if((header->op == UNLOCK_OBJ_OP || header->op == REMOVE_OP) && risposta->hdr.op == OP_OK)
			{
				int posizione = -1;
				for(i=0; i<numOggettiLocked; i++)
					if(oggettiLocked[i] == header->key) { posizione = i; break; }
				if(posizione != -1)
				{
					if((posizione+1) != numOggettiLocked)
					{	//sposto indietro
						for(i=posizione; i<(numOggettiLocked-1); i++)
							oggettiLocked[i] = oggettiLocked[i+1];
					}
					numOggettiLocked--;
					StampaTest("[pool] Rimosso un file dall'elenco di quelli da sbloccare alla disconnessione");
				} else assert(header->op != UNLOCK_OBJ_OP);
			}

			//invio risposta al client
			if(sendRequest((long) sockConn, risposta) != 0) break;

			free(risposta);
			StampaTest("[pool] inviata risposta al client");

			//Tra una richiesta e l'altra, controllo se è arrivato un segnale di terminazione immediata
			MyLock(TermMutex);
			if(TermImmediata)
			{
				MyUnlock(TermMutex);
				CheckExit(close(sockConn) == -1, TRUE, "chiusura connessione in seguito a TermImmediata");
				free(header);
				free(corpo);
				free(oggettiLocked);
				return (void*) NULL;
			}
			MyUnlock(TermMutex);

			//Leggo richiesta successiva
			esito = readHeader((long) sockConn, header);
		}

		/*	si esce dal while quando readHeader restituisce -1 (errore) o 1 (nessun byte letto, cioè connessione chiusa regolarmente dal client)
		oppure quando si è ricevuta una PUT o una UPDATE, e readData restituisce -1
		In ogni caso chiudo il file descriptor della connessione e ricomincio il while(1) servendo un'altra connessione.	*/

		CheckExit(close(sockConn) == -1, TRUE, "chiusura connessione lato server");

		//faccio la unlock del repository nel caso il client se ne fosse scordato
		MyLock(lockRepositoryMutex);
		if(lockRepository == sockConn) lockRepository = -1;
		MyUnlock(lockRepositoryMutex);

		//faccio la unlock degli oggetti nel caso il client se ne fosse scordato
		for(i=0; i<numOggettiLocked; i++)
		{
			StampaTest("[pool] Faccio unlock di un oggetto lasciato dal client in stato di lock");
			//unlock dell'oggetto oggettiLocked[i] con assegnazione della lock al prossimo
			LockHash(oggettiLocked[i]);
			hashTableEntry* elemento = icl_hash_find(hashTable, (void*) &(oggettiLocked[i]));
			CheckExit(elemento == NULL, FALSE, "inconsistenza dell'array oggettiLocked: contiene chiave inesistente");
			CheckExit(elemento->detentoreLock == -1, FALSE, "inconsistenza dell'array oggettiLocked: contiene oggetto non locked");
			CheckExit(elemento->detentoreLock != sockConn, FALSE, "inconsistenza dell'array oggettiLocked: contiene oggetto lockato da altro client");

			StampaTest("[pool] Sto per fare la unlock d'ufficio");
			CheckExit(DoUnlock(elemento) != 0, FALSE, "esecuzione della unlock d'ufficio");
			UnlockHash(oggettiLocked[i]);
		}
		numOggettiLocked = 0;

		free(header);
		free(corpo);

		MyLock(statsMutex);
		stats.concurrent_connections--;
		MyUnlock(statsMutex);
		StampaTest("[pool] Connessione chiusa");
		StampaTest("=======================================");
	}
	free(oggettiLocked);
	return (void*) NULL;
}

/*	RinominaDump
Funzione che controlla se al percorso indicato in config.DumpFile esiste già un file, e in quel caso lo rinomina appendendogli il suffisso ".bak" (un file eventualmente preesistente con quello stesso nome viene sovrascritto). Viene invocata da SalvaDump e da CaricaDump.
 */
static int RinominaDump()
{
	CheckMenoUno(strcmp(config.DumpFile, "") == 0, FALSE, "path del dump file non presente nella configurazione");
	int retVal = 0;

	errno = 0;
	retVal = access(config.DumpFile, F_OK);
	CheckMenoUno(errno != 0 && errno != ENOENT, TRUE, "controllo esistenza file dump");

	if(retVal == 0)
	{
		StampaTest("Esisteva già mbox.dump: lo sposto");

		//dump esiste
		char dumpBakPath[1024];
		strncpy(dumpBakPath, config.DumpFile, 1024);
		strncat(dumpBakPath, ".bak", 1024-strlen(config.DumpFile));

		errno = 0;
		retVal = access(dumpBakPath, F_OK);
		CheckMenoUno(errno != 0 && errno != ENOENT, TRUE, "controllo esistenza file dump.bak");

		if(retVal == 0)
		{
			StampaTest("Esisteva pure mbox.dump.bak: lo elimino");
			//dump.bak esiste: lo elimino
			CheckMenoUno(unlink(dumpBakPath) == -1, TRUE, "eliminazione file dump.bak");
		}
		//rinomino dump in dump.bak
		CheckMenoUno(rename(config.DumpFile, dumpBakPath) == -1, TRUE, "rinomina del file dump in dump.bak");
	}
	return 0;
}

/*
	SalvaDump()
Si occupa di salvare un dump del contenuto del repository nel file il cui path è letto dal file di configurazione e memorizzato in config.DumpFile.
E' invocata quando il client richiede l'operazione DUMP_OP.

E' compito del chiamante, prima di invocare la funzione:
- controllare che il file di configurazione contenesse realmente il path, e cioè che config.DumpFile sia diverso da ""
- controllare che il client che richiede la DUMP abbia in precedenza acquisito la lock del repository

Restituisce 0 in caso di successo, -1 in caso di errore.
	*/
int SalvaDump()
{
	CheckMenoUno(RinominaDump() != 0, FALSE, "errore nella rinominazione del vecchio file di dump");
	StampaTest("Ho spostato il file eventualmente preesistente - ora posso scrivere in mbox.dump");

	//Da qui in poi possiamo supporre che non esista alcun file al percorso config.DumpFile (non c'era, o l'abbiamo spostato a .bak)
	int dump = open(config.DumpFile, O_WRONLY|O_CREAT|O_EXCL, 0666);
	CheckMenoUno(dump == -1, TRUE, "apertura in scrittura file di dump");

	MyLock(statsMutex);

	//Scrittura n. di oggetti
	CheckMenoUno(write(dump, &stats.current_objects, sizeof(unsigned long)) != sizeof(unsigned long), TRUE, "scrittura del dump - n. oggetti");

	//Scrittura size dello storage
	CheckMenoUno(write(dump, &stats.current_size, sizeof(unsigned long)) != sizeof(unsigned long), TRUE, "scrittura del dump - size storage");

	MyUnlock(statsMutex);

	//Scrittura oggetti (NB: non serve fare la LockHash perché il client ha acquisito la lock sul repository intero)
	CheckMenoUno(icl_hash_dump(dump, hashTable) == -1, FALSE, "scrittura del dump - file del repository");

	CheckMenoUno(close(dump) == -1, TRUE, "chiusura file dump");
	return 0;
}

/*
	CaricaDump(path file di dump)
Funzione che si occupa di leggere un file di dump dal percorso passato come argomento, e di inserire nel repository i dati in esso contenuti.
Viene eventualmente eseguita un'unica volta, chiamata dal main, subito dopo lo startup del server (prima dell'avvio dei thread e dell'accettazione di connessioni).

Restituisce 0 in caso di successo, -1 in caso di errore.

Oltre a quelle standard, vengono controllate e segnalate le seguenti condizioni di errore:
- inconsistenza nel dump tra size totale del repository e somma delle size degli oggetti
- presenza nel dump di meno file di quanti dichiarati nella prima riga
- al contrario l'eventuale presenza di più file di quanti dichiarati NON è controllata né segnalata come errore;
  i file eccedenti saranno ignorati senza essere inseriti nel repository
	*/
int CaricaDump(char* fileDump)
{
	int dump = open(fileDump, O_RDONLY);
	CheckMenoUno(dump == -1, TRUE, "apertura in sola lettura file di dump durante il caricamento del contenuto");

	int i;
	unsigned long actualSize = 0;
	unsigned long objects, size;

	//Lettura n. di oggetti
	CheckMenoUno(read(dump, &objects, sizeof(unsigned long)) != sizeof(unsigned long), TRUE, "lettura dal dump - n. oggetti");

	//Lettura size dello storage
	CheckMenoUno(read(dump, &size, sizeof(unsigned long)) != sizeof(unsigned long), TRUE, "lettura dal dump - size storage");

	MyLock(statsMutex);
	stats.current_objects = objects;
	stats.max_objects = objects;
	stats.current_size = size;
	stats.max_size = size;
	MyUnlock(statsMutex);

	//Lettura oggetti
	for(i=0; i<objects; i++)
	{
		//Chiave
		membox_key_t* chiave = malloc(sizeof(membox_key_t));
		CheckMenoUno(chiave == NULL, FALSE, "malloc chiave durante caricamento dump");
		CheckMenoUno(read(dump, chiave, sizeof(membox_key_t)) != sizeof(membox_key_t), TRUE, "lettura dal dump - chiave dell'oggetto");

		//Size
		hashTableEntry* new = malloc(sizeof(hashTableEntry));
		CheckMenoUno(new == NULL, FALSE, "malloc nuova entry della tabella hash durante caricamento dump");
		CheckMenoUno(read(dump, &(new->len), sizeof(unsigned int)) != sizeof(unsigned int), TRUE, "lettura dal dump - size dell'oggetto");
		actualSize = actualSize + new->len;

		//Dati
		char* file = malloc(new->len * sizeof(char));
		CheckMenoUno(file == NULL, FALSE, "malloc contenuto file durante caricamento dump");
		new->data = file;
		CheckMenoUno(read(dump, file, new->len) != new->len, TRUE, "lettura dal dump - dati dell'oggetto");

		new->detentoreLock = -1;
		new->codaRichiedentiLock = NULL;
		new->rilascioLockObj = NULL;
		LockHash(*chiave);
		CheckMenoUno(icl_hash_insert(hashTable, (void*) chiave, (void*) new) == NULL, FALSE, "inserimento nella tabella hash dell'elemento letto dal dump");
		UnlockHash(*chiave);
	}

	CheckMenoUno(close(dump) == -1, TRUE, "chiusura file dump");
	CheckMenoUno(actualSize != size, FALSE, "file di dump incoerente (dimensione del repository)");

	CheckMenoUno(RinominaDump() != 0, FALSE, "errore nella rinominazione del vecchio file di dump"); //dopo averlo caricato, rinomina dump in dump.bak
	return 0;
}

int main(int argc, char *argv[])
{
	int i;

	//Maschero tutti i segnali
	sigset_t insieme;
	CheckExit(sigfillset(&insieme) == -1, FALSE, "settaggio a 1 di tutti i flag dell'insieme");
	CheckExit(pthread_sigmask(SIG_SETMASK, &insieme, NULL) != 0, FALSE, "mascheramento iniziale di tutti i segnali");

	//Gestisco i segnali che voglio gestire in modo personalizzato/ignorare (SIGPIPE, ecc)
	struct sigaction ignorare;
	memset(&ignorare, 0, sizeof(ignorare));
	ignorare.sa_handler = SIG_IGN;
	CheckExit(sigaction(SIGPIPE, &ignorare, NULL) == -1, TRUE, "syscall per ignorare segnale SIGPIPE");
	CheckExit(sigaction(SIGSEGV, &ignorare, NULL) == -1, TRUE, "syscall per ignorare segnale SIGSEGV");

	//tolgo la maschera inserita all'inizio, lasciandola solo per i segnali da gestire con thread
	CheckExit(sigemptyset(&insieme) == -1, FALSE, "settaggio a 0 di tutti i flag dell'insieme");
	CheckExit(sigaddset(&insieme, SIGINT) == -1, FALSE, "settaggio a 1 del flag per SIGINT");
	CheckExit(sigaddset(&insieme, SIGTERM) == -1, FALSE, "settaggio a 1 del flag per SIGTERM");
	CheckExit(sigaddset(&insieme, SIGQUIT) == -1, FALSE, "settaggio a 1 del flag per SIGQUIT");
	CheckExit(sigaddset(&insieme, SIGUSR1) == -1, FALSE, "settaggio a 1 del flag per SIGUSR1");
	CheckExit(sigaddset(&insieme, SIGUSR2) == -1, FALSE, "settaggio a 1 del flag per SIGUSR2");
	CheckExit(pthread_sigmask(SIG_SETMASK, &insieme, NULL) != 0, FALSE, "rimozione mascheramento iniziale dei segnali ignorati / gestiti in modo personalizzato");

	//Leggo parametri passati a membox da linea di comando
	char* fileConf = NULL;
	char* fileDump = NULL;

	for(i=1; (i+1)<argc; i=i+2)
	{
		if(strcmp(argv[i], "-f") == 0) fileConf = argv[i+1];
		if(strcmp(argv[i], "-d") == 0) fileDump = argv[i+1];
	}
	CheckExit(fileConf == NULL, FALSE, "argomento -f (file di configurazione del server) mancante");

	//Leggo file impostazioni
	int retval = LeggiConfigurazione(fileConf, &config);
	CheckExit(retval == 2, FALSE, "file di configurazione malformato (almeno un'impostazione fondamentale mancante)");
	StampaTest("File di configurazione letto correttamente");
	if(retval == 3) StampaTest("(ma impostazioni StatFileName e/o DumpFile mancanti)");

	/******* INIZIALIZZO STRUTTURE DATI *******/

	//Leggo o stimo il numero massimo di oggetti
	int maxOggetti = -1;
	if(config.StorageSize != -1 && config.StorageSize != 0) maxOggetti = config.StorageSize;
	else
	{	//nel caso in cui manchi StorageSize, stimo il numero max di oggetti come StorageByteSize / dimensione media di un oggetto (anch'essa stimata come 0.66 * MaxObjSize)
		if(config.StorageByteSize != -1 && config.StorageByteSize != 0 && config.MaxObjSize != -1 && config.MaxObjSize != 0)
			maxOggetti = config.StorageByteSize / (config.MaxObjSize * 0.66);
	}

	//calcolo numero di buckets
	int buckets = 0;

	if(maxOggetti <= -1) buckets = DEFBUCKETS;
	else
	{
		if(maxOggetti > 0 && maxOggetti <= 2500) buckets = maxOggetti / (0.000368 * maxOggetti + 0.33);
		if(maxOggetti > 2500 && maxOggetti <= 15000) buckets = 0.09376 * maxOggetti + 1766;
		if(maxOggetti > 15000) buckets = MAXBUCKETS;
	}

	//creo tabella hash
	hashTable = icl_hash_create(buckets, ulong_hash_function, ulong_key_compare);
	CheckExit(hashTable == NULL, FALSE, "creazione tabella hash vuota");

	//calcolo numero di mutex
	if(maxOggetti <= -1) nMutex = DEFMUTEX;
	else
	{
		if(maxOggetti > 0 && maxOggetti <= 10) nMutex = maxOggetti; //uno per ciascun potenziale oggetto
		if(maxOggetti > 10 && maxOggetti <= 45000) nMutex = 10 + pow(log(maxOggetti - 9),2);
		if(maxOggetti > 45000) nMutex = MAXMUTEX;
	}

	//inizializzo mutex
	hashMutex = malloc(nMutex * sizeof(pthread_mutex_t));
	CheckExit(hashMutex == NULL, FALSE, "malloc array con i mutex della hashtable");
	for(i=0; i < nMutex; i++) pthread_mutex_init(&hashMutex[i], NULL);

	//coda
	int numeroPosti = config.MaxConnections - config.ThreadsInPool;
	if(numeroPosti <= 0) numeroPosti = 1;
	coda = creaCoda(numeroPosti);
	CheckExit(coda == NULL, FALSE, "creazione coda");

	StampaTest("Coda e tabella hash inizializzate!");

	//carico file dump
	if(fileDump != NULL)
		CheckExit(CaricaDump(fileDump) != 0, FALSE, "file di dump malformato o errore di lettura");

	/******* INIZIALIZZO CONNESSIONE *******/
	//socket
	int sfd = -1;

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	CheckExit(sfd == -1, TRUE, "creazione socket");

	//bind
	struct sockaddr_un indirizzo;
	indirizzo.sun_family = AF_UNIX;
	strncpy(indirizzo.sun_path, config.UnixPath, strlen(config.UnixPath) + sizeof(char)); //+1 char per il terminatore \0

	CheckExit(bind(sfd, (struct sockaddr*) &indirizzo, sizeof(indirizzo)) == -1, TRUE, "bind indirizzo al socket");

	//listen
	CheckExit(listen(sfd, SOMAXCONN) == -1, TRUE, "listen");

	StampaTest("Connessione inizializzata!");

	/******* AVVIO DEI THREAD *******/
	pthread_t idAccettaConnessioni, idSegnali;

	//avvio il thread per accettare le connessioni
	assert(sizeof(int) <= sizeof (void*));
	CheckExit(pthread_create(&idAccettaConnessioni, NULL, AccettaConnessioni, (void*) &sfd) != 0, FALSE, "creazione thread per accettare connessioni");
	StampaTest("Avviato thread che accetta le connessioni");

	//avvio pool di thread per servire i client
	pthread_t* idPool = malloc(config.ThreadsInPool * sizeof(pthread_t));
	CheckExit(idPool == NULL, FALSE, "malloc array id thread del pool");

	for(i=0; i < config.ThreadsInPool; i++)
	{
		CheckExit(pthread_create(&idPool[i], NULL, ServiClient, (void*) NULL) != 0, FALSE, "creazione thread del pool");
		StampaTest("Avviato thread del pool");
	}

	//apro file statistiche
	FILE* statFile = NULL;
	if(strcmp(config.StatFileName, "") != 0)
	{
		statFile = fopen(config.StatFileName, "a");
		CheckExit(statFile == NULL, TRUE, "apertura file per memorizzare le statistiche");
	}

	//avvio i thread per gestire i segnali di terminazione dolce/immediata e stampa statistiche
	assert(sizeof(statFile) <= sizeof(void*));
	CheckExit(pthread_create(&idSegnali, NULL, attendiSegnali, (void*) statFile) != 0, FALSE, "creazione thread per attesa segnali");
	StampaTest("Avviato thread che gestisce i segnali");



	/******* TERMINAZIONE DEL SERVER *******/

	//attendo la terminazione di tutti i thread
	CheckExit(pthread_join(idSegnali, NULL) != 0, FALSE, "join del thread per attesa segnali");
	StampaTest("Thread segnali terminato");

	for(i=0; i < config.ThreadsInPool; i++)
	{
		CheckExit(pthread_join(idPool[i], NULL) != 0, FALSE, "join di un thread del pool");
		StampaTest("Thread del pool terminato");
	}
	free(idPool);

	retval = pthread_cancel(idAccettaConnessioni);
	CheckExit(retval != 0 && retval != 3, FALSE, "cancel (terminazione) del thread che accetta connessioni");
	//NB: non considero errore il codice 3 (No such process); se il thread è terminato spontaneamente, perché non era bloccato nella accept, meglio così

	CheckExit(pthread_join(idAccettaConnessioni, NULL) != 0, FALSE, "join del thread che accetta connessioni");
	StampaTest("Thread accettazione connessioni terminato");

	//stampa statistiche, cancella socket e file temporanei, deallocazioni, chiusura
	if(strcmp(config.StatFileName, "") != 0)
	{
		CheckExit(printStats(statFile) != 0, FALSE, "errore nella stampa delle statistiche");
		CheckExit(fclose(statFile) != 0, TRUE, "chiusura file per memorizzare le statistiche");
		free(config.StatFileName);
		StampaTest("Statistiche stampate");
	}

	CheckExit(unlink(config.UnixPath) == -1, TRUE, "eliminazione file temporaneo socket");
	free(config.UnixPath);

	eliminaCoda(coda);
	CheckExit(icl_hash_destroy(hashTable, LiberaChiave, LiberaEntry) != 0, FALSE, "deallocazione tabella hash");

	StampaTest("Server terminato");
	return 0;
}
