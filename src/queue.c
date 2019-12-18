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

#include <stdio.h>
#include <stdlib.h>
#include "tools.h"
#include "queue.h"

/*
	creaCoda
Crea una coda vuota capace di ospitare al massimo elementiMax elementi contemporaneamente in coda.
Restituisce un puntatore alla coda creata, oppure NULL in caso di errore.
*/
queue_t* creaCoda(int elementiMax)
{
	CheckNULL(elementiMax <= 0, FALSE, "impossibile creare coda con 0 posti o meno");
	queue_t* coda = malloc(sizeof(queue_t));
	CheckNULL(coda == NULL, FALSE, "malloc della coda");

	coda->array = malloc(elementiMax * sizeof(int));
	CheckNULL(coda->array == NULL, FALSE, "malloc dell'array della coda");

	coda->elementiAttuali = 0;
	coda->elementiMax = elementiMax;
	coda->primo = 0;
	coda->ultimo = 0;

	return coda;
}

/*
	enqueue
Inserisce elemento in fondo alla coda.
Restituisce 0 in caso di successo, -1 in caso di fallimento
*/
int enqueue(queue_t* coda, int elemento)
{
	CheckMenoUno(coda == NULL, FALSE, "enqueue in una coda non creata");
	CheckMenoUno(coda->elementiAttuali >= coda->elementiMax, FALSE, "coda piena, impossibile inserire un ulteriore elemento");
	coda->array[coda->ultimo] = elemento;
	coda->elementiAttuali++;
	coda->ultimo = (coda->ultimo + 1) % coda->elementiMax;

	return 0;
}

/*
	dequeue
In caso di successo, restituisce l'elemento in testa alla coda
In caso di fallimento, restituisce -1
*/
int dequeue (queue_t* coda)
{
	CheckMenoUno(coda == NULL, FALSE, "dequeue in una coda non creata");
	CheckMenoUno(coda->elementiAttuali <= 0, FALSE, "coda vuota, impossibile estrarre un elemento");
	int elemento;
	elemento = coda->array[coda->primo];
	coda->elementiAttuali--;
	coda->primo = (coda->primo + 1) % coda->elementiMax;

	return elemento;
}

void eliminaCoda(queue_t* coda)
{
	free(coda->array);
	free(coda);
	return;
}
