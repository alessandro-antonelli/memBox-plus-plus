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
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <assert.h>
#include <ops.h>

/**
 * @file  message.h
 * @brief Contiene il formato del messaggio
 */


typedef unsigned long membox_key_t;

/**
 *  @struct header
 *  @brief header del messaggio 
 *
 *  @var op tipo di operazione
 *  @var key chiave dell'oggetto su cui si vuole fare l'operazione
 */
typedef struct {
    op_t           op;   
    membox_key_t   key;  
} message_hdr_t;

/**
 *  @struct data
 *  @brief body del messaggio 
 *
 *  @var len lunghezza del buffer dati
 *  @var buf buffer dati
 */
typedef struct {
    unsigned int   len;  
    char          *buf;
} message_data_t;

/**
 *  @struct messaggio
 *  @brief tipo del messaggio 
 *
 *  @var hdr header
 *  @var data dati
 */
typedef struct {
    message_hdr_t  hdr;
    message_data_t data;
} message_t;

/* ------ funzioni di utilità ------- */

/**
 * @function setKey
 * @brief copia la chiave da in in out
 */
static inline void setKey(membox_key_t *key_out, membox_key_t *key_in) {
    assert(key_out); 
    assert(key_in);
    *key_out = *key_in;   
}
/**
 * @function setheader
 * @brief scrive l'header del messaggio
 *
 * @param msg puntatore al messaggio
 * @param op tipo di operazione da eseguire
 * @param key chiave dell'oggetto
 */
static inline void setHeader(message_t *msg, op_t op, membox_key_t *key) {
#if defined(MAKE_VALGRIND_HAPPY)
    ((long*)&(msg->hdr))[0] = 0;
    ((long*)&(msg->hdr))[1] = 0;
#endif
    msg->hdr.op  = op;
    setKey(&msg->hdr.key, key);
}
/**
 * @function setData
 * @brief scrive la parte dati del messaggio
 *
 * @param msg puntatore al messaggio
 * @param buf puntatore al buffer 
 * @param len lunghezza del buffer
 */
static inline void setData(message_t *msg, const char *buf, unsigned int len) {
    msg->data.len = len;
    msg->data.buf = (char *)buf;
}


#endif /* MESSAGE_H_ */
