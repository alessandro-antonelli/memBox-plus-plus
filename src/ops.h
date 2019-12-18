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

#if !defined(OPS_H_)
#define OPS_H_

/**
 * @file  ops.h
 * @brief Contiene i codici delle operazioni di richiesta e risposta
 */


typedef enum {

    /* ----------------------------------- */
    /*      operazioni da implementare     */
    /* ------------------------------------*/
    PUT_OP          = 0,   /// inserimento di un oggetto nel repository
    UPDATE_OP       = 1,   /// aggiornamento di un oggetto gia' presente
    GET_OP          = 2,   /// recupero di un oggetto dal repository
    REMOVE_OP       = 3,   /// eliminazione di un oggetto dal repository
    LOCK_OP         = 4,   /// acquisizione di una lock su tutto il repository
    UNLOCK_OP       = 5,   /// rilascio della lock su tutto il repository
    LOCK_OBJ_OP     = 6,   /// acquisizione di una lock su un oggetto
    UNLOCK_OBJ_OP   = 7,   /// rilascio della lock su un oggetto
    DUMP_OP         = 8,   /// richiesta di dump

    /* ----------------------------------- */
    /* ----------------------------------- */

    END_OP          = 9,   // numero di operazioni definite    

    /* ----------------------------------- */
    /*    messaggi di ritorno dal server   */
    /* ------------------------------------*/

    OP_OK           = 11,  // operazione eseguita con successo    
    OP_FAIL         = 12,  // generico messaggio di fallimento
    OP_PUT_ALREADY  = 13,  // put di un oggetto gia' presente nel repository 
    OP_PUT_TOOMANY  = 14,  // raggiunto il massimo numero di oggetti
    OP_PUT_SIZE     = 15,  // put di un oggetto troppo grande
    OP_PUT_REPOSIZE = 16,  // superata la massima size del repository
    OP_GET_NONE     = 17,  // get di un oggetto non presente
    OP_REMOVE_NONE  = 18,  // rimozione di un oggetto non presente
    OP_UPDATE_SIZE  = 19,  // size dell'oggetto da aggiornare non corrispondente
    OP_UPDATE_NONE  = 20,  // update di un oggetto non presente
    OP_LOCKED       = 21,  // operazione non permessa perche' il sistema e' in stato di lock
    OP_LOCK_NONE    = 22,  // richiesta di unlock ma il sistema non e' in stato di lock
    OP_LOCK_OBJ_NONE= 23,  // richiesta di lock su un oggetto ma l'oggetto non e' presente
    OP_LOCK_OBJ_DLK = 24,  // richiesta di lock su un oggetto gia' in stato di lock dallo stesso client
    OP_NO_DUMP      = 25,  // richiesta un'operazione di dump che non puo' essere eseguita

    /* 
     * aggiungere qui altri messaggi di ritorno che possono servire 
     */

    OP_UNKNOWN      = 26   // la richiesta conteneva un'operazione sconosciuta

} op_t;
    

#endif /* OPS_H_ */
