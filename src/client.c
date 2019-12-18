/*
 * membox Progetto del corso di LSO 2016 
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Pelagatti, Torquati
 * 
 */
/**
 * @file client.c
 * @brief Semplice client di test. 
 *
 *        Esempio di utilizzo:
 *        - progname -l socket_path -c 100:0:8192
 *           Invia una richiesta di PUT (0) per un oggetto di chiave 100 con size 
 *           8192 bytes.
 *
 *        -  progname  -l socket_path -c 100:2:0 -c 100:3:0
 *           Invia una richiesta di GET (2) per un oggetto di chiave 100 seguita da
 *           una richiesta di REMOVE (3) dello stesso oggetto. La seconda richiesta
 *           viene eseguita solo se la prima richiesta e' andata a buon fine.
 *
 *        -  progname -s 500 -l socket_path -c 100:0:8192 -c 100:2:0
 *           Invia una richiesta di PUT (0) per un oggetto di chiave 100 con size 
 *           8192 bytes, se la richiesta va a buon fine invia una seconda 
 *           richiesta per la chiave 100 di tipo GET (2). 
 *           Tra le due richieste trascorrono circa 500 millisecondi (opzione -s 500)
 *
 */
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <signal.h>

#include <connections.h>
#include <ops.h>


typedef struct {
    membox_key_t key;
    op_t op;
    long size;
} operation_t;

static void use(const char * filename) {
    fprintf(stderr, 
	    "use:\n"
	    " %s -l unix_socket_path -c keyid:op:size [-c keyid:op:size] [-s milliseconds] -h\n"
	    "   op: %d PUT, %d UPDATE, %d GET, %d REMOVE, %d LOCK, %d UNLOCK, %d LOCK_OBJ_OP, %d UNLOCK_OBJ_OP, %d DUMP_OP\n"
	    "   il campo size e' significativo solo per l'operazione PUT ed UPDATE, altrimenti viene ignorato\n"
	    "   milliseconds sono i millisecondi da attendere nell'invio di due operazioni consecutive\n\n",
	    filename, PUT_OP, UPDATE_OP, GET_OP, REMOVE_OP, LOCK_OP, UNLOCK_OP, LOCK_OBJ_OP, UNLOCK_OBJ_OP, DUMP_OP);
}


static void init_data(membox_key_t *key, char *data, unsigned int len) {
    long *p = (long*)data;
    for(int i=0;i<(len/sizeof(unsigned long));++i) setKey((membox_key_t*)&p[i], key);
    int r = len % sizeof(unsigned long);
    for(int i=1;i<=r;++i) data[len-i] = 0x01;
}

static int check_data(membox_key_t *key, char *data, unsigned int len) {
    long *p = (long*)data;
    for(int i=0;i<(len/sizeof(unsigned long));++i)
	if (p[i] != *key) {
	    return -1;
	}
    int r = len % sizeof(unsigned long);
    for(int i=1;i<=r;++i) 
	if (data[len-i] != 0x01) return -1;
    return 0;
}

static int execute_op(int connfd, operation_t *o) {
    membox_key_t key = o->key;
    op_t op = o->op;
    long size = o->size;

    if (op == PUT_OP || op == UPDATE_OP) {
	if (size == 0) {
	    fprintf(stderr, "per le operazioni di put/update la size deve essere > 0\n");
	    return -1;
	}
    } else size = 0;
    message_t msg;
    setData(&msg, NULL, 0);
    setHeader(&msg, op, &key);
    if (op == PUT_OP || op == UPDATE_OP) {
	char *buf = malloc(size);
	if (buf == NULL) {
	    perror("malloc");
	    return -1;
	}
	init_data(&key, buf, size);
	setData(&msg, buf, size);
    } 
    if (sendRequest(connfd, &msg) == -1 && (errno != EPIPE)) {
	perror("request");
	return -1;
    }
    if (msg.data.buf) { free(msg.data.buf); msg.data.buf = NULL; } //Modifica effettuata dal candidato (Alessandro Antonelli, mat. 507264): il puntatore va impostato a NULL, altrimenti la memoria liberata viene acceduta alla riga 115!
    if (readHeader(connfd, &msg.hdr) == -1) {
	perror("reply header");
	return -1;
    }
    switch(msg.hdr.op) {
    case OP_OK: break;
    case OP_FAIL: {
	if (msg.data.buf) 
	    fprintf(stderr, "Operation failed: %s\n", msg.data.buf);
	else
	    fprintf(stderr, "Operation failed\n");
	return -OP_FAIL;
    };
    case OP_PUT_ALREADY: {
	fprintf(stderr, "PUT: l'oggetto con chiave %ld e' gia' in mbox\n", key);
	return -OP_PUT_ALREADY;
    };
    case OP_PUT_TOOMANY: {
	fprintf(stderr, "PUT: troppi oggetti nel repository\n");
	return -OP_PUT_TOOMANY;
    };
    case OP_PUT_SIZE: {
	fprintf(stderr, "PUT: l'oggetto con chiave %ld e' troppo grande\n", key);
	return -OP_PUT_SIZE;
    };
    case OP_PUT_REPOSIZE: {
	fprintf(stderr, "PUT: il repository ha raggiunto la size massima\n");
	return -OP_PUT_REPOSIZE;
    };
    case OP_GET_NONE: {
	fprintf(stderr, "GET: l'oggetto con chiave %ld non e' presente in mbox\n", key);
	return -OP_GET_NONE;
    };
    case OP_REMOVE_NONE: {
	fprintf(stderr, "REMOVE: l'oggetto con chiave %ld non è presente in mbox\n", key);
	return -OP_REMOVE_NONE;
    };
    case OP_UPDATE_SIZE: {
	fprintf(stderr, "UPDATE: l'oggetto con chiave %ld ha una size non valida\n", key);
	return -OP_UPDATE_SIZE;
    };
    case OP_UPDATE_NONE: {
	fprintf(stderr, "UPDATE: l'oggetto con chiave %ld non e' presente in mbox\n", key);
	return -OP_UPDATE_NONE;
    };
    case OP_LOCKED: {
	fprintf(stderr, "Operazione non permessa, sistema in stato di lock\n");
	return -OP_LOCKED;
    };
    case OP_LOCK_NONE: {
	fprintf(stderr, "UNLOCK: repository non in stato di lock\n");
	return -OP_LOCK_NONE;
    };
    case OP_LOCK_OBJ_NONE: {
	fprintf(stderr, "LOCK/UNLOCK OBJ: l'oggetto con chiave %ld non e' presente in mbox\n", key);
	return -OP_LOCK_OBJ_NONE;
    };
    case OP_LOCK_OBJ_DLK: {
	fprintf(stderr, "LOCK OBJ: l'oggetto con chiave %ld e' gia' in stato di lock (DEADLOCK)\n", key);
	return -OP_LOCK_OBJ_DLK;
    };
    case OP_NO_DUMP: {
	fprintf(stderr, "DUMP: l'operazione di dump non puo' essere eseguita\n");
	return -OP_NO_DUMP;
    };
    default: {
	fprintf(stderr, "Invalid reply op\n");
	return -1;
    }
    }
    if (op == GET_OP) {
	if (readData(connfd, &msg.data) == -1) {
	    perror("reply data");
	    return -1; 
	}	
	if (check_data(&key, msg.data.buf, msg.data.len) != 0) {
	    fprintf(stderr, "dati ricevuti non corretti!\n");
	    return -1;
	}
    }    
    return 0;
}

int main(int argc, char *argv[]) {
    const char optstring[] = "l:c:s:h";
    int optc;
    char *spath = NULL;
    operation_t *ops = NULL;
    long msleep=0;

    if (argc <= 4) {
	use(argv[0]);
	return -1;
    }

    ops = (operation_t*)malloc(sizeof(operation_t)*(argc-4));
    if (!ops) {
	perror("malloc");
	return -1;
    }
    int k=0;
    // parse command line options
    while ((optc = getopt(argc, argv,optstring)) != -1) {
 	switch (optc) {
        case 'l':   spath=optarg;
	    break;
        case 'c': {
	    char *arg = strdup(optarg);
	    char *p;
	    p = strchr(arg, ':');
	    if (!p) {
		use(argv[0]);
		free(ops);
		return -1;
	    }
	    *p++ = '\0';
	    ops[k].key = strtol(arg,NULL,10);
	    arg = p;
	    p = strchr(arg, ':');
	    if (!p) {
		use(argv[0]);
		free(ops);
		return -1;
	    }
	    *p++ = '\0';
	    ops[k].op = strtol(arg,NULL,10);
	    arg = p;
	    ops[k].size = strtol(arg,NULL,10);

	    if (ops[k].op >END_OP) { 
		fprintf(stderr, "Operazione non valida!\n");
		use(argv[0]);
		free(ops);
		return -1;
	    }
	    ++k;
	} break;
	case 's': {
	    msleep= strtol(optarg,NULL,10);
	} break;
        case 'h':
	default: {
	    use(argv[0]);
	    free(ops);
	    return -1;
	}
	}
    }
    // verify options
    if (spath==NULL) {
	use(argv[0]);
	free(ops);
	return -1;
    }
    int connfd;
    // faccio 10 tentativi aspettando 1 secondo
    if ((connfd=openConnection(spath, 10, 1))<0) {
	fprintf(stderr, "error trying to connect...\n");
	free(ops);
	return -1;
    }

    struct sigaction s;
    memset(&s,0,sizeof(s));    
    s.sa_handler=SIG_IGN;
    // ignoro SIGPIPE per evitare di essere terminato da una scrittura su un socket chiuso
    if ( (sigaction(SIGPIPE,&s,NULL) ) == -1 ) {   
	perror("sigaction");
	free(ops);
	return -1;
    } 
    struct timespec req = { msleep/1000, (msleep%1000)*1000000L };  
  
    int r=0;
    for(int i=0;i<k;++i) {
	r = execute_op(connfd, &ops[i]);
	if (r == 0)  printf("Successo!\n");
	else break;  // non appena una operazione fallisce esco

	// tra una operazione e l'altra devo aspettare msleep millisecondi
	if (msleep>0) nanosleep(&req, (struct timespec *)NULL);
    }
    close(connfd);
    free(ops);
    return r;
}

