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

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct queue_ {
	int elementiAttuali;
	int elementiMax;
	int primo;
	int ultimo;
	int* array;
} queue_t;

queue_t* creaCoda(int elementiMax);

int enqueue(queue_t* coda, int elemento);

int dequeue (queue_t* coda);

void eliminaCoda(queue_t* coda);

#endif /* QUEUE_H_ */
