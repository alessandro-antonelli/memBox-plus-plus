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

#ifndef TOOLS_H_
#define TOOLS_H_

#include <stdio.h>
#include <errno.h>
#include <time.h>

#define FALSE 0
#define TRUE 1

	/*				MACRO PER IL CONTROLLO DEI VALORI DI RITORNO

**************** CheckExit (condizione, ErrnoSignificativo?, msg) **********************************
Se la condizione è vera stampa un messaggio di errore ed esce dal programma.
Il messaggio di errore contiene il testo msg, e anche Errno se ErrnoSignificativo? è impostato a 1.
	*/
#define CheckExit(condizione, stampareEno, msg)			\
	if(condizione)						\
	{							\
		if(stampareEno)					\
		{						\
			perror("Errore: "#msg"");		\
			fprintf(stderr, "(file %s, linea %d)\n", __FILE__, __LINE__);			\
		}						\
		else						\
			fprintf(stderr, "Errore (file %s, linea %d): "#msg"\n", __FILE__, __LINE__);	\
		fflush(stderr);					\
		exit(EXIT_FAILURE);				\
	}

/**************** CheckMenoUno (condizione, ErrnoSignificativo?, msg) *********************************
Se la condizione è vera ritorna -1.
Se la macro DEBUG è definita, viene stampato il messaggio di errore msg, e se ErrnoSignificativo? è impostato a 1
viene stampato anche il valore di Errno.
	*/
			#ifdef DEBUG //Caso in cui effettuo stampa di debug

#define CheckMenoUno(condizione, stampareEno, msg)		\
	if(condizione)						\
	{							\
		if(stampareEno)					\
		{						\
			perror("Errore: "#msg"");		\
			fprintf(stderr, "(file %s, linea %d)\n", __FILE__, __LINE__);			\
		}						\
		else						\
			fprintf(stderr, "Errore (file %s, linea %d): "#msg"\n", __FILE__, __LINE__);	\
		return -1;					\
	}

			#else //Caso in cui niente stampa di debug

#define CheckMenoUno(condizione, stampareEno, msg)		\
	if(condizione)						\
	{							\
		return -1;					\
	}
			#endif

/**************** CheckNULL (condizione, ErrnoSignificativo?, msg) *********************************
Se la condizione è vera ritorna NULL.
Se la macro DEBUG è definita, viene stampato il messaggio di errore msg, e se ErrnoSignificativo? è impostato a 1
viene stampato anche il valore di Errno.
	*/
			#ifdef DEBUG //Caso in cui effettuo stampa di debug

#define CheckNULL(condizione, stampareEno, msg)		\
	if(condizione)						\
	{							\
		if(stampareEno)					\
		{						\
			perror("Errore: "#msg"");		\
			fprintf(stderr, "(file %s, linea %d)\n", __FILE__, __LINE__);			\
		}						\
		else						\
			fprintf(stderr, "Errore (file %s, linea %d): "#msg"\n", __FILE__, __LINE__);	\
		return NULL;					\
	}

			#else //Caso in cui niente stampa di debug

#define CheckNULL(condizione, stampareEno, msg)		\
	if(condizione)						\
	{							\
		return NULL;					\
	}
			#endif


/**************** MyLock (semaforo) *********************************
Macro per alleggerire la sintassi delle lock necessarie alla mutua esclusione.
Il parametro deve essere un semaforo mutex (tipo pthread_mutex_t). La macro proverà ad eseguire la lock su tale semaforo;
se l'operazione non riesce, stampa un messaggio di errore con file, riga e nome del semaforo ed esce dal programma.
	*/
#define MyLock(semaforo)				\
	if(pthread_mutex_lock(&semaforo) != 0)		\
	{						\
		fprintf(stderr, "Errore (file %s, linea %d): lock del semaforo #semaforo non riuscita\n", __FILE__, __LINE__);		\
		exit(EXIT_FAILURE);			\
	}

/**************** MyUnlock (semaforo) *********************************
Macro per alleggerire la sintassi delle unlock necessarie alla mutua esclusione.
Il parametro deve essere un semaforo mutex (tipo pthread_mutex_t). La macro proverà ad eseguire la unlock su tale semaforo;
se l'operazione non riesce, stampa un messaggio di errore con file, riga e nome del semaforo ed esce dal programma.
	*/
#define MyUnlock(semaforo)				\
	if(pthread_mutex_unlock(&semaforo) != 0)	\
	{						\
		fprintf(stderr, "Errore (file %s, linea %d): unlock del semaforo #semaforo non riuscita\n", __FILE__, __LINE__);		\
		exit(EXIT_FAILURE);			\
	}

/******************* LockHash (key) *********************************
Macro che consente di effettuare l'operazione di lock sul mutex corrispondente all'oggetto del repository con chiave indicata.
Dopo averla chiamata, garantisce la mutua esclusione sull'oggetto con tale chiave.
Da usare solo in membox.c (fa affidamento sul fatto che il puntatore alla tabella hash si chiami hashTable e che nMutex contenga il numero dei mutex)
	*/
#define LockHash(key)		\
{				\
	 MyLock(hashMutex[((* hashTable->hash_function)(&key) % hashTable->nbuckets) % nMutex]);	\
}

/******************* UnlockHash (key) *********************************
Da usare solo in membox.c (fa affidamento sul fatto che il puntatore alla tabella hash si chiami hashTable e che nMutex contenga il numero dei mutex)
	*/
#define UnlockHash(key)		\
{				\
	 MyUnlock(hashMutex[((* hashTable->hash_function)(&key) % hashTable->nbuckets) % nMutex]);	\
}

/**************** StampaTest (testo) *********************************
Funzione per effettuare una stampa di test su stderr. Effettuata solo se la macro STAMPETEST è definita.
	*/
		#ifdef STAMPETEST
#define StampaTest(testo)							\
{										\
	fprintf(stderr, "[%0d sec] %s : %d | %s\n", time(NULL) % 60, __FILE__, __LINE__, testo);	\
	fflush(stderr);								\
}
		#else
#define StampaTest(testo)	{ }
		#endif


#endif /* TOOLS_H_ */
