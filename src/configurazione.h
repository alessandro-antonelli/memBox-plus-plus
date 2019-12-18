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

#ifndef CONFIGURAZIONE_H_
#define CONFIGURAZIONE_H_

#define MAXLINELEN 1001
//numero massimo di caratteri di cui si assume sia composta ciascuna riga del file di configurazione

typedef struct config_ {
	char* UnixPath;		// path utilizzato per la creazione del socket AF_UNIX
	int MaxConnections;	// numero massimo di connessioni pendenti
	int ThreadsInPool;	// numero di thread nel pool
	int StorageSize;	// dimensione dello storage in numero di oggetti (0 nessun limite)
	int StorageByteSize;	// dimensione dello storage in numero di bytes (0 nessun limite)
	int MaxObjSize;		// dimensione massima di un oggetto nello storage (0 nessun limite)
	char* StatFileName;	// file nel quale verranno scritte le statistiche
	char* DumpFile;		// file nel qual verrà effettuato il dump del repository
} config_t;

int LeggiConfigurazione(char* path, config_t* config);

#endif /* CONFIGURAZIONE_H_ */
