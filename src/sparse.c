/*
file sparse.c
autore Alessandro Antonelli - Università di Pisa - matricola 507264
Si dichiara che il contenuto di questo file è in ogni sua parte
opera originale dell'autore.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sparse.h"

/*
Questo tipo viene utilizzato dalla funzione ausiliare find_trio (vedi sotto)
per poter restituire una tripla di puntatori <prec, cercato, succ>
*/
typedef struct s_result
{
	/*
	L'elemento precedente a quello cercato nella lista relativa a quella riga
	(ossia il primo numero non nullo alla sua sinistra). Se è NULL significa
	che non ci sono elementi prima di quello cercato (quello cercato è - o sarebbe,
	nel caso in cui abbia attualmente valore nullo - il primo della lista)
	*/
	elem_t* prec;

	/*
	L'elemento cercato. Se è NULL significa che l'elemento non è presente nella
	lista (ossia a quell'indice della matrice corrisponde uno zero)
	*/
	elem_t* cercato;

	/*
	L'elemento successivo a quello cercato nella lista relativa a quella riga
	(ossia il primo numero non nullo alla sua destra). Se è NULL significa
	che non ci sono elementi dopo quello cercato (quello cercato è - o sarebbe,
	nel caso in cui abbia attualmente valore nullo - l'ultimo della lista)
	*/
	elem_t* succ;

} search_result;

	/*	MACRO PER IL CONTROLLO DEI VALORI DI RITORNO

	**************** Check (condizione, msg) **********************************
Se la condizione è vera, stampa msg sullo standard error (solo se la macro
DEBUG è definita), esegue l'eventuale codice di cleanup memorizzato nella macro
CLEANUP_CODE (definita sotto, vedi commento relativo) ed esce dalla funzione
restituendo un valore indicante errore, memorizzato nella macro ERR_RETVALUE
(quest'ultima è definita all'inizio del corpo di ciascuna funzione e al termine
del corpo la definizione viene cancellata).
	*/
			#ifdef DEBUG //Caso in cui effettuo stampa di debug

#define Check(condizione, msg)					\
	if(condizione)						\
	{							\
		fprintf(stderr, "Errore (file %s, linea %d): "#msg"\n", __FILE__, __LINE__);	\
		CLEANUP_CODE					\
		return ERR_RETVALUE;				\
	}

			#else //Caso in cui niente stampa di debug

#define Check(condizione, msg)					\
	if(condizione)						\
	{							\
		CLEANUP_CODE					\
		return ERR_RETVALUE;				\
	}
			#endif

	/************* CheckErrno(condizione, codiceErr, msg) ***********************
Come sopra, ma prima di uscire dalla funzione setta errno a codiceErr.
	*/
			#ifdef DEBUG //Caso in cui effettuo stampa di debug

#define CheckErrno(condizione, codiceErr, msg)				\
	if(condizione)							\
	{								\
		fprintf(stderr, "Errore (file %s, linea %d): "#msg"\n", __FILE__, __LINE__);	\
		CLEANUP_CODE						\
		errno = codiceErr;					\
		return ERR_RETVALUE;					\
	}

			#else //Caso in cui niente stampa di debug

#define CheckErrno(condizione, codiceErr, msg)				\
	if(condizione)							\
	{								\
		CLEANUP_CODE						\
		errno = codiceErr;					\
		return ERR_RETVALUE;					\
	}

			#endif
		/*
La macro CLEANUP_CODE viene usata dalle Check e CheckErrno (vedi sopra). Contiene
il codice di cleanup da eseguire in caso di errore prima di ritornare al chiamante.
Qui viene definita con il valore "predefinito" per non eseguire nulla; quando un'area
di codice necessita di codice di cleanup prima di uscire nel caso di eventuali errori,
bisogna delimitare la porzione di codice in questo modo:
	#undef CLEANUP_CODE
	#define CLEANUP_CODE foo(x,y);
[Codice che necessita l'esecuzione di foo(x,y); prima di tornare al chiamante]
	#undef CLEANUP_CODE
	#define CLEANUP_CODE {}
		*/
#define CLEANUP_CODE {}


smatrix_t * new_smat ( unsigned n, unsigned m )
{
	#define ERR_RETVALUE NULL

	//Restituisco NULL (errore) se n o m sono <= 0
	if(n <= 0 || m <= 0) return NULL;

	//Alloco matrice
	smatrix_t * matrice = (smatrix_t *) malloc(sizeof(smatrix_t));
	Check(matrice == NULL, "impossibile allocare la matrice");

	//Alloco array delle righe
	elem_t ** array = (elem_t **) malloc(n * sizeof(elem_t*));
	Check(array == NULL, "impossibile allocare l'array delle righe");

	//Inizializzo righe come vuote
	int i;
	for(i=0; i<n; i++) array[i] = NULL;

	//Setto campi e restituisco la matrice
	(*matrice).nrow = n;
	(*matrice).ncol = m;
	(*matrice).mat = array;

	return matrice;
	#undef ERR_RETVALUE
}

bool_t is_equal_smat ( smatrix_t * a , smatrix_t * b )
{
	//Considero uguali due puntatori NULL,
	//considero diversi un puntatore NULL ed uno non NULL
	if(a == NULL && b == NULL) return TRUE;
	if(a == NULL || b == NULL) return FALSE;
	
	//Caso in cui entrambi i puntatori diversi da NULL
	if(a->nrow != b -> nrow || a->ncol != b -> ncol) return FALSE;

	int righe = a->nrow;
	int i;
	for (i=0; i<righe; i++)
	{
		//riga vuota in entrambe
		if(a->mat[i] == NULL && b->mat[i] == NULL) continue;
		
		//riga vuota in una sola delle due
		if(a->mat[i] == NULL || b->mat[i] == NULL) return FALSE;

		//riga non vuota in entrambe
		elem_t * currA, * currB;
		currA = a->mat[i];
		currB = b->mat[i];
		bool_t fine = FALSE;
		while (fine == FALSE)
		{
			//elemento che differisce per valore o num. colonna
			if(currA->col != currB->col || currA->val != currB->val) return FALSE;

			//fine della riga raggiunta in entrambe
			if(currA->next == NULL && currB->next == NULL) fine=TRUE;
			else
				//fine della riga raggiunta solo in una
				if(currA->next == NULL || currB->next == NULL) return FALSE;
				else
				{
					//fine della riga non raggiunta in entrambe
					currA = currA->next;
					currB = currB->next;
				}
		}
	}

	return TRUE;
}

/*
		find_trio
	Funzione ausiliare che, data la matrice m e la coppia <i,j> restituisce
	una tripla (tipo search_result, vedi commenti sopra) che contiene
	- un puntatore cercato all'elemento <i,j>;
	- un puntatore prec all'elemento che lo precede nella lista (primo numero
	  non nullo alla sinistra della casella <i,j>);
	- un puntatore succ a quello che lo segue nella lista (primo numero
	  non nullo alla destra della casella <i,j>).

	Ciascuno dei tre puntatori può essere NULL ad indicare le seguenti condizioni:
	- cercato è NULL nel caso in cui l'el. <i,j> non è presente (è zero);
	- prec è NULL nel caso in cui l'el. cercato è (o sarebbe, nel caso in cui abbia
	  attualmente valore nullo) il primo della lista;
	- succ è NULL nel caso in cui l'el. cercato è (o sarebbe, nel caso in cui abbia
	  attualmente valore nullo) l'ultimo della lista.
	Se tutti e tre sono NULL significa che la riga i è tutta zero.

	In caso di errore restituisce NULL al posto della tripla.
	La free() della tripla allocata dalla funzione spetta al chiamante.
*/
static search_result * find_trio(smatrix_t * m, unsigned i, unsigned j)
{
	#define ERR_RETVALUE NULL
	Check(m == NULL, "l'argomento è NULL");
	Check(m->nrow <= i || m->ncol <= j, "riga e/o colonna da trovare superano la dimensione della matrice");

	search_result * trio = (search_result *) malloc (sizeof(search_result));
	Check(trio==NULL, "allocazione memoria per risultato ricerca non riuscita");
	trio->prec = NULL;
	trio->cercato = NULL;
	trio->succ = NULL;

	//Mi restringo al caso in cui la riga non è tutta zero (in quel caso
	//va bene restituire la tripla <NULL, NULL, NULL> già assegnata)
	if(m->mat[i] != NULL)
	{
		elem_t * prev = NULL;
		elem_t * curr = m->mat[i];
	
		while(curr != NULL)
		{
			if(curr->col == j)
			{
				//Lo abbiamo trovato
				trio->cercato = curr;
				trio->succ = curr->next;
				break;
			}
			if(curr->col > j)
			{
				//Lo abbiamo superato: l'el. cercato non esiste (è zero)
				trio->cercato = NULL;
				trio->succ = curr;
				break;
			}
			prev = curr;
			curr = curr->next;
		}
		trio->prec = prev;
	}

	return trio;
	#undef ERR_RETVALUE
}

int put_elem ( smatrix_t * m , unsigned i, unsigned j, double d )
{
	#define ERR_RETVALUE -1
	Check(m == NULL, "l'argomento è NULL");
	Check(m->nrow <= i || m->ncol <= j, "riga e/o colonna superano la dimensione della matrice");

	//Cerco l'item in questione (lui) e salvo il prec e il succ
	search_result * risultato = find_trio(m, i, j);
	Check(risultato==NULL, "fallimento della funzione find_trio");
	elem_t * prec = risultato->prec;
	elem_t * lui = risultato->cercato;
	elem_t * succ = risultato->succ;
	free(risultato);

	//Fa le modifiche necessarie
	if(d == 0)
	{
		if(lui == NULL) return 0; //L'elemento deve essere messo a 0 ma è già a 0

		//L'elemento esiste (diverso da zero) e va rimosso (azzerato)
		free(lui);
		if(prec == NULL) m->mat[i] = succ;
		else
			prec->next = succ;
	}
	else
	{
		//Modifica senza azzeramento
		if(lui != NULL) lui->val = d;
		else
		{
			elem_t * nuovo = (elem_t *) malloc(sizeof(elem_t));
			Check(nuovo==NULL, "allocazione memoria per il nuovo elemento non riuscita");
			nuovo->col = j;
			nuovo->val = d;
			nuovo->next = succ;
			if(prec != NULL) prec->next = nuovo;
			else m->mat[i] = nuovo;
		}
	}

	return 0;
	#undef ERR_RETVALUE
}

int get_elem ( smatrix_t * m , unsigned i, unsigned j, double* pd )
{
	#define ERR_RETVALUE -1
	Check(m == NULL, "l'argomento è NULL");
	Check(m->nrow <= i || m->ncol <= j, "riga e/o colonna da leggere superano la dimensione della matrice");

	search_result * risultato = find_trio(m, i, j);
	Check(risultato==NULL, "fallimento della funzione find_trio");

	if(risultato->cercato == NULL) *pd = 0;
	else *pd = risultato->cercato->val;

	free(risultato);
	return 0;
	#undef ERR_RETVALUE
}

//Funzione ausiliare che libera ricorsivamente la memoria occcupata da una lista
static void FreeRigaRicorsiva(elem_t * elemento)
{
	if(elemento != NULL)
	{
		FreeRigaRicorsiva(elemento->next);
		free(elemento);
	}
}

void free_smat (smatrix_t ** pm)
{
	if(pm == NULL) return;
	if((*pm) == NULL) return;

	if((*pm)->mat != NULL)
	{
		int i;
		for(i=0; i < (*pm)->nrow; i++) //dealloco gli elementi di ogni riga
			FreeRigaRicorsiva((*pm)->mat[i]);

		free((*pm)->mat); //dealloco l'array delle righe
	}

	free((*pm)); //dealloco la matrice
	*pm = NULL;
}

smatrix_t* sum_smat (smatrix_t* a, smatrix_t* b)
{
	#define ERR_RETVALUE NULL
	Check(a == NULL || b == NULL, "l'argomento è NULL");
	Check(a->nrow != b->nrow || a->ncol != b->ncol, "le matrici hanno dimensioni diverse");

	smatrix_t* c = new_smat(a->nrow, a->ncol);
	Check(c==NULL, "fallimento della funzione new_smat");
	#undef CLEANUP_CODE
	#define CLEANUP_CODE free_smat(&c);

	int i, j;
	for(i=0; i < a->nrow; i++)
	{
		//Se una stessa riga è nulla in sia A che in B, sarà nulla anche
		//nella matrice somma
		if(a->mat[i] == NULL && b->mat[i] == NULL) continue;

		for(j=0; j < a->ncol; j++)
		{
			double numA, numB, sum;
			Check(get_elem(a, i, j, &numA) < 0, "fallimento della funzione get_elem");
			Check(get_elem(b, i, j, &numB) < 0, "fallimento della funzione get_elem");

			sum = (numA + numB);
			if(sum != 0) Check(put_elem(c, i, j, sum) < 0, "fallimento della funzione put_elem"); 
		}
	}

	return c;
	#undef CLEANUP_CODE
	#define CLEANUP_CODE {}
	#undef ERR_RETVALUE
}


smatrix_t* prod_smat (smatrix_t* a, smatrix_t* b)
{
	#define ERR_RETVALUE NULL
	Check(a == NULL || b == NULL, "l'argomento è NULL");
	Check(a->ncol != b->nrow, "le matrici hanno dimensioni incompatibili");

	//A = m x n	B = n x k	C = m x k
	int m, n, k;
	m = a->nrow;
	n = a->ncol;
	k = b->ncol;
	smatrix_t* c = new_smat(m, k);
	Check(c==NULL, "fallimento della funzione new_smat");
	#undef CLEANUP_CODE
	#define CLEANUP_CODE free_smat(&c);

	int i, j;
	for(i=0; i < m; i++)
	{
		/*
		Piccola ottimizzazione:
		Se la riga i di A è tutta nulla, la stessa riga sarà sicuramente
		nulla anche nella matrice prodotto (infatti ogni elemento <i,j> di quella
		riga si calcola come (riga i di A)*(colonna j di B), ma riga i di A
		è il vettore nullo quindi lo scalare risultante è 0)
		*/
		if(a->mat[i] == NULL) continue;

		for(j=0; j < k; j++)
		{
			/*
			Fissato un elemento <i,j> della matrice risultato, calcolo il suo
			valore come prodotto scalare di (riga i di A)*(colonna j di B),
			ossia come somma (per x che va da 0 a n-1) dei prodotti
			A<i,x> * B<x,j> (memorizzati nelle variabili fattoreA e fattoreB)
			*/			
			double valore = 0;
			int x;
			for(x=0; x < n; x++)
			{
				double fattoreA = 0;
				double fattoreB = 0;

				Check(get_elem(a, i, x, &fattoreA) < 0, "fallimento della funzione get_elem");

			/* Se il primo fattore è venuto zero non è necessario cercare il
 				   secondo (il prodotto viene zero in qualsiasi caso) */
				if(fattoreA != 0) Check(get_elem(b, x,j, &fattoreB) < 0, "fallimento della funzione get_elem");

				valore = valore + (fattoreA * fattoreB);
			}

			if(valore != 0) Check(put_elem(c, i, j, valore) < 0, "fallimento della funzione put_elem"); 
		}
	}
	return c;
	#undef CLEANUP_CODE
	#define CLEANUP_CODE {}
	#undef ERR_RETVALUE
}

smatrix_t* transp_smat (smatrix_t* a)
{
	#define ERR_RETVALUE NULL
	Check(a == NULL, "l'argomento è NULL");

	//A = m x n	C = n x m
	int m, n;
	m = a->nrow;
	n = a->ncol;

	smatrix_t* c = new_smat(n, m);
	Check(c==NULL, "fallimento della funzione new_smat");
	#undef CLEANUP_CODE
	#define CLEANUP_CODE free_smat(&c);

	int i, j;
	for(i=0; i<m; i++)
	{
		for(j=0; j<n; j++)
		{
			// Leggo  <i,j> da A e se non è nullo lo inserisco in C come <j,i>
			double valore = 0;
			Check(get_elem(a, i, j, &valore) < 0, "fallimento della funzione get_elem");
			if(valore != 0) Check(put_elem(c, j, i, valore) < 0, "fallimento della funzione put_elem");
		}
	}
	return c;
	#undef CLEANUP_CODE
	#define CLEANUP_CODE {}
	#undef ERR_RETVALUE
}

	/*
		load_smat
In caso di errore restiruisce NULL e setta errno:
- EBADF		il descrittore è NULL o con flag di errore/fine file attivo
- EINVAL	il file di input ha formato non conforme alla specifica o indica
		una matrice in cui una delle due dimensioni è <= 0
- EIO		errori nell'allocazione o nel riempimento della matrice

Per armonia con l'apertura, il compito di chiudere il file viene lasciato al chiamante
(anche in caso di errore!)
	*/
smatrix_t* load_smat (FILE* fd)
{
	#define ERR_RETVALUE NULL
	//Errori relativi al parametro
	CheckErrno(fd == NULL, EBADF, "il descrittore del file da leggere è NULL");
	CheckErrno(ferror(fd) != 0, EBADF, "il descrittore del file segnala errore");
	CheckErrno(feof(fd) != 0, EBADF, "il descrittore del file segnala End Of File");

	//Leggo numero di righe e di colonne
	int row, col;
	CheckErrno(fscanf(fd, "%d", &row) != 1, EINVAL, "lettura n.righe non riuscita");
	CheckErrno(fscanf(fd, "%d", &col) != 1, EINVAL, "lettura n.colonne non riuscita");
	CheckErrno(row <= 0 || col <= 0, EINVAL, "impossibile caricare matrici con meno di una riga/colonna");

	//Alloco la matrice vuota
	smatrix_t* m = new_smat(row, col);
	CheckErrno(m==NULL, EIO, "fallimento della funzione new_smat");
	#undef CLEANUP_CODE
	#define CLEANUP_CODE free_smat(&m);

	while(1)
	{
		//Finché non raggiungo la fine del file leggo la tripla <i, j, val>
		//e inserisco l'elemento nella matrice
		int retValue;
		unsigned i, j;
		double valore;

		retValue = fscanf(fd, "%u %u %lf", &i, &j, &valore);

		//Se sono alla fine del file esco dal while
		if(retValue == EOF) break;

		//Se non tutte le tre variabili <i,j,valore> sono state lette: errore
		CheckErrno(retValue != 3, EINVAL, "lettura tripla <riga,colonna,valore> non riuscita");

		if(valore != 0) CheckErrno(put_elem(m, i, j, valore) != 0, EIO, "fallimento della funzione put_elem");
	}

	return m;
	#undef CLEANUP_CODE
	#define CLEANUP_CODE {}
	#undef ERR_RETVALUE
}

int save_smat (FILE* fd, smatrix_t* mat)
{
	#define ERR_RETVALUE -1

	//Errori relativi ai parametri
	CheckErrno(fd == NULL, EBADF, "il descrittore del file da scrivere è NULL");
	CheckErrno(ferror(fd) != 0, EBADF, "il descrittore del file segnala errore");
	CheckErrno(mat == NULL, EINVAL, "il puntatore alla matrice da salvare è NULL");

	//Scrivo numero di righe e di colonne
	int m = mat->nrow;
	int n = mat->ncol;
	CheckErrno(fprintf(fd, "%d\n", m) < 0, EIO, "scrittura n. righe non riuscita");
	CheckErrno(fprintf(fd, "%d\n", n) < 0, EIO, "scrittura n. colonne non riuscita");

	int i;
	for(i=0; i<m; i++)
	{
		if(mat->mat[i] == NULL) continue;
		elem_t* elem = mat->mat[i];
		while(elem != NULL)
		{
			CheckErrno(fprintf(fd, "%d %d %lf\n", i, elem->col, elem->val) < 0, EIO, "scrittura tripla <riga,colonna,valore> non riuscita");
			elem = elem->next;
		}
	}

	return 0;
	#undef ERR_RETVALUE
}

	/*		loadbin_smat
In caso di errore restiruisce NULL e setta errno:
- EBADF		il descrittore è NULL o con flag di errore/fine file attivo
- EINVAL	il file di input ha formato non conforme alla specifica o indica
		una matrice in cui una delle due dimensioni è <= 0
- EIO		errori nell'allocazione o nel riempimento della matrice



Le matrici vengono lette/scritte in binario con il seguente formato:

sizeof(int) byte con l'intero che indica quante righe ha la matrice
sizeof(int) byte con l'intero che indica quante colonne ha la matrice
---
sizeof(int) byte con 1 int che indica la riga del 1° elem. non nullo della matrice
sizeof(int) byte con 1 int che indica la colonna del 1° elem. non nullo della matrice
sizeof(double) byte con 1 double che contiene il valore del 1° elem.non nullo della matrice
---
      "          "            "        la riga del 2° elem.non nullo della matrice
      "          "            "        la colonna del 2° elem.non nullo della matrice
      "          "            "        il valore del 2° elem.non nullo della matrice

... e così via fino alla fine del file.

Attenzione: sono memorizzati solo gli elementi con valore non nullo (quelli che non
vengono letti dal file binario devono essere supposti nulli).
	*/
smatrix_t* loadbin_smat (FILE* fd)
{
	#define ERR_RETVALUE NULL

	//Errori relativi al parametro
	CheckErrno(fd == NULL, EBADF, "il descrittore del file binario da leggere è NULL");
	CheckErrno(ferror(fd) != 0, EBADF, "il descrittore del file binario da leggere segnala errore");
	
	//Leggo numero di righe e di colonne
	int row, col;
	CheckErrno(fread(&row, sizeof(int), 1, fd) != 1, EINVAL, "lettura del numero di righe non riuscita");
	CheckErrno(fread(&col, sizeof(int), 1, fd) != 1, EINVAL, "lettura del numero di colonne non riuscita");
	CheckErrno(row <= 0 || col <= 0, EINVAL, "impossibile caricare matrici con meno di una riga/colonna");

	//Alloco la matrice vuota
	smatrix_t* m = new_smat(row, col);
	CheckErrno(m==NULL, EIO, "fallimento della funzione new_smat");
	#undef CLEANUP_CODE
	#define CLEANUP_CODE free_smat(&m);

	while(1)
	{
		//Finché non raggiungo la fine del file leggo la tripla <i, j, val>
		//e inserisco l'elemento nella matrice
		int retValue;
		unsigned i, j;
		double valore;

		//Leggo i
		retValue = fread(&i, sizeof(int), 1, fd);
		CheckErrno(retValue != 1 && ferror(fd), EIO, "lettura tripla <riga,colonna,valore> non riuscita");
		//Se con l'ultima tripla letta si era raggiunto l'end of file:
		//tutto normale, esco dal ciclo
		if(retValue != 1 && feof(fd)) break;

		//Leggo j
		retValue = fread(&j, sizeof(int), 1, fd);
		CheckErrno(retValue != 1 && ferror(fd), EIO, "lettura tripla <riga,colonna,valore> non riuscita");
		CheckErrno(retValue != 1 && feof(fd), EINVAL, "file binario malformato: si interrompe prima di indicare numero di colonna e valore dell'elemento");

		//Leggo valore
		retValue = fread(&valore, sizeof(double), 1, fd);
		CheckErrno(retValue != 1 && ferror(fd), EIO, "lettura tripla <riga,colonna,valore> non riuscita");
		CheckErrno(retValue != 1 && feof(fd), EINVAL, "file binario malformato: si interrompe prima di indicare il valore dell'elemento");

		//Inserisco l'elemento
		if(valore != 0) CheckErrno(put_elem(m, i, j, valore) != 0, EIO, "fallimento della funzione put_elem");
	}

	return m;
	#undef CLEANUP_CODE
	#define CLEANUP_CODE {}
	#undef ERR_RETVALUE
}

int savebin_smat (FILE* fd, smatrix_t* mat)
{
	#define ERR_RETVALUE -1

	//Errori relativi ai parametri
	CheckErrno(mat == NULL, EINVAL, "il puntatore alla matrice da salvare è NULL");
	CheckErrno(fd == NULL, EBADF,"il descrittore del file binario da scrivere è NULL");
	CheckErrno(ferror(fd) != 0, EBADF, "il descrittore del file binario da scrivere segnala errore");

	//Scrivo numero di righe e di colonne
	int row = mat->nrow;
	int col = mat->ncol;
	CheckErrno(row <= 0 || col <= 0, EINVAL, "impossibile salvare matrici con meno di una riga/colonna");
	CheckErrno(fwrite(&row, sizeof(int), 1, fd) != 1, EINVAL, "scrittura del numero di righe non riuscita");
	CheckErrno(fwrite(&col, sizeof(int), 1, fd) != 1, EINVAL, "scrittura del numero di colonne non riuscita");

	int i;
	for(i=0; i<row; i++)
	{
		if(mat->mat[i] == NULL) continue;
		elem_t* elem = mat->mat[i];
		while(elem != NULL)
		{
			//Scrivo numero riga
			CheckErrno(fwrite(&i, sizeof(int), 1, fd) != 1, EIO, "scrittura tripla <riga,colonna,valore> non riuscita");

			//Scrivo numero colonna
			CheckErrno(fwrite(&(elem->col), sizeof(int), 1, fd) != 1, EIO, "scrittura tripla <riga,colonna,valore> non riuscita");

			//Scrivo valore elemento
			CheckErrno(fwrite(&(elem->val), sizeof(double), 1, fd) != 1, EIO, "scrittura tripla <riga,colonna,valore> non riuscita");

			elem = elem->next;
		}
	}

	return 0;
	#undef ERR_RETVALUE
}
