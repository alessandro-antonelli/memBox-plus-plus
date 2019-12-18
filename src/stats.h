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

#if !defined(MEMBOX_STATS_)
#define MEMBOX_STATS_

#include <stdio.h>
#include <time.h>
#include "tools.h"
#include <pthread.h>
#include <stdlib.h>

/**
 *  @struct statistics
 *  @brief Struttura dati contenente le informazioni da monitorare.
 */
struct statistics {
    unsigned long nput;                         // n. di put
    unsigned long nput_failed;                  // n. di put fallite 
    unsigned long nupdate;                      // n. di update
    unsigned long nupdate_failed;               // n. di update fallite
    unsigned long nget;                         // n. di get
    unsigned long nget_failed;                  // n. di get fallite
    unsigned long nremove;                      // n. di remove
    unsigned long nremove_failed;               // n. di remove fallite
    unsigned long nlock;                        // n. di lock
    unsigned long nlock_failed;                 // n. di lock fallite
    unsigned long nobjlock;                     // n. di lock sugli oggetti
    unsigned long nobjlock_failed;              // n. di lock sugli oggetti falliete
    unsigned long ndump;                        // n. di dump
    unsigned long ndump_failed;                 // n. di dump fallite
    unsigned long concurrent_connections;       // n. di connessioni concorrenti
    unsigned long current_size;                 // size attuale del repository (bytes)
    unsigned long max_size;                     // massima size raggiunta dal repository
    unsigned long current_objects;              // numero corrente di oggetti
    unsigned long max_objects;                  // massimo n. di oggetti raggiunto
};

/**
 * @function printStats
 * @brief Stampa le statistiche nel file passato come argomento
 *
 * @param fout descrittore del file aperto in append.
 *
 * @return 0 in caso di successo, -1 in caso di fallimento 
 */
static inline int printStats(FILE *fout) {
    extern struct statistics stats;
    extern pthread_mutex_t statsMutex;

    MyLock(statsMutex);

    if (fprintf(fout, "%ld - %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",
		(unsigned long)time(NULL),
		stats.nput, stats.nput_failed,
		stats.nupdate, stats.nupdate_failed,
		stats.nget, stats.nget_failed,
		stats.nremove, stats.nremove_failed,
		stats.nlock, stats.nlock_failed,
		stats.nobjlock, stats.nobjlock_failed,
		stats.ndump, stats.ndump_failed,
		stats.concurrent_connections,
		stats.current_size, stats.max_size,
		stats.current_objects, stats.max_objects) < 0) { MyUnlock(statsMutex); return -1; }

    MyUnlock(statsMutex);
    fflush(fout);
    return 0;
}

#endif /* MEMBOX_STATS_ */
