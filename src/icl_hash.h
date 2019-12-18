/**
 * @file
 *
 * Header file for icl_hash routines.
 *
 */
/* $Id$ */
/* $UTK_Copyright: $ */

#ifndef icl_hash_h
#define icl_hash_h

#include <stdio.h>
#include "queue.h"
#include <pthread.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

// Struttura che rappresenta il file del repository. Aggiunta effettuata dal candidato (Alessandro Antonelli, mat. 507264)
typedef struct _hashTableEntry
{
	char* data;
	unsigned int len;

	//Campi necessari per la gestione delle lock sugli oggetti (operazione LOCK_OBJ)

	int detentoreLock;	/* Indica se l'oggetto è in stato di lock ed eventualmente chi la detiene.
				   Un valore positivo indica che l'oggetto E' in stato di lock;
					in questo caso il valore indica il socket file descriptor del client che attualmente detiene la lock.
				   -1  significa che l'oggetto NON è in stato di lock.
				   -2  è un valore speciale, indica che l'oggetto è un fase di eliminazione. Un oggetto con detentoreLock == -2 deve essere trattato
					a tutti gli effetti come un oggetto INESISTENTE. Non esiste agli occhi del client e non è conteggiato nelle statistiche.
					Questo valore è necessario per gestire il caso particolare di una remove effettuata su un oggetto con altri client già in attesa di
					acquisire la lock. In tale caso il thread che effettua la remove imposta il valore a -2 e risveglia i thread in attesa della lock;
					questi ultimi, leggendolo, si tolgono dalla lista di attesa e si occupano di rimuovere definitivamente l'oggetto.
				*/

	queue_t* codaRichiedentiLock;	/* Coda dei client che sono in attesa di acquisire la lock sull'oggetto (ma ancora non la detengono). I valori sono i socket file descriptor.
						
					   Alla creazione dell'oggetto è NULL, e continua ad essere NULL quando un client acquisisce la lock;
					   la coda viene creata solo quando un client richiede la lock ma già la detiene qualcun altro. */

	pthread_cond_t* rilascioLockObj; /* Variabile di condizione usata per segnalare un rilascio della lock ai client in attesa di acquisirla (che sono in codaRichiedentiLock).
						---
					    Alla creazione dell'oggetto è NULL, e continua ad essere NULL quando un client acquisisce la lock;
					    la variabile viene creata solo quando un client richiede la lock ma già la detiene qualcun altro.
						---
					    Quando un client rilascia la lock, deve fare una broadcast su questa variabile; poi sarà compito dei client che vengono svegliati
					    controllare se è arrivato il loro turno o se sono semplicemente avanzati di una posizione nella coda.	*/
} hashTableEntry;

typedef struct icl_entry_s {
    void* key;
    void *data;
    struct icl_entry_s* next;
} icl_entry_t;

typedef struct icl_hash_s {
    int nbuckets;
    int nentries;
    icl_entry_t **buckets;
    unsigned int (*hash_function)(void*);
    int (*hash_key_compare)(void*, void*);
} icl_hash_t;

icl_hash_t *
icl_hash_create( int nbuckets, unsigned int (*hash_function)(void*), int (*hash_key_compare)(void*, void*) );

void
* icl_hash_find(icl_hash_t *, void* );

icl_entry_t
* icl_hash_insert(icl_hash_t *, void*, void *),
    * icl_hash_update_insert(icl_hash_t *, void*, void *, void **);

int
icl_hash_destroy(icl_hash_t *, void (*)(void*), void (*)(void*)),
    icl_hash_dump(int, icl_hash_t *);
/*
La signature della icl_hash_dump è stata modificata dal candidato (Alessandro Antonelli, mat. 507264) cambiando il tipo del primo parametro da FILE* a int, per rendere la funzione conforme a quanto richiesto nelle specifiche di membox++ (uso delle syscall write/read in luogo delle chiamate di libreria standard fprintf/fscanf)
*/

int icl_hash_delete( icl_hash_t *ht, void* key, void (*free_key)(void*), void (*free_data)(void*) );


#define icl_hash_foreach(ht, tmpint, tmpent, kp, dp)    \
    for (tmpint=0;tmpint<ht->nbuckets; tmpint++)        \
        for (tmpent=ht->buckets[tmpint];                                \
             tmpent!=NULL&&((kp=tmpent->key)!=NULL)&&((dp=tmpent->data)!=NULL); \
             tmpent=tmpent->next)


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* icl_hash_h */
