=========================================================================  
ISTRUZIONI e SPECIFICHE per il progetto B (membox++) a.a.  2015/16
========================================================================  
Il progetto prevede lo sviluppo di un server concorrente che implementa 
un repository di oggetti. Il nome dato al server e' membox.
  
------------------------------------------------  
Estrarre il materiale dal KIT  
------------------------------------------------  
Creare una directory temporanea, copiare kit_f2b.tar    
nella directory e spostarsi nella directory appena creata. Es.  
  
$$ mkdir Progetto_MemBox  
$$ mv kit_f2b.tar Progetto_MemBox  
$$ cd Progetto_MemBox  
  
S-tarare i file del kit con  
  
$$ tar xvf kit_f2b.tar  
  
questo comando crea nella directory corrente una directory "membox" con i seguenti file  
  
$$ ls membox 

client.c	       : client fornito dai docenti che deve essere utilizzato
		         per effettuare i tests. Per far funzionare il client 
			 e' necessario implementare alcuni metodi 
			 (es. openConnection, sendRequest, readHeader, ...).
			 (NON MODIFICARE IL CODICE DEL CLIENT)

test_hash.c            : file fornito dai docenti che mostra come utilizzare 
		         la tabella hash icl_hash implementata nei file 
			 icl_hash.[ch]

connections.h
message.h
ops.h
stats.h                : 
		         files di riferimento forniti dai docenti, questi file 
			 possono/devono essere modificati dagli studenti
 

icl_hash.[ch]          : implemantazione di una tabella hash
		         (NON MODIFICARE)

membox.c	       : file di riferimento contenente il main del server membox. 
		         Tale file va modificato dallo studente come meglio ritiene.

testput.sh
testlock.sh
testsize.sh
teststats.sh
testleaks.sh
teststress.sh
testdump.sh
testlockobj.sh
		       : script di test
		         (NON MODIFICARE)
Makefile  
		       : file per il make. Il Makefile va modificato solo nelle
		         parti indicate.
  
DATI/*		       : file di dati necessari per i tests. Da non modificare
		         se non per le opzioni UnixPath e StatFileName (vedere anche
			 i commenti nel Makefile).
                         (NON MODIFICARE)  
  
README.txt	       : questo file  
README.doxygen	       : informazioni sul dformato doxygen dei commenti 
                         alle funzioni (FACOLTATIVO)
  
gruppo-check.pl	       : uno script Perl che controlla il formato del file  
		         gruppo.txt prima di effettuare la consegna  
		         (NON MODIFICARE)  
  
gruppo.txt	       : un file di esempio di specifica del gruppo  
		         (massimo 2 studenti per gruppo)  
			 (deve essere aggiornato con i dati di chi consegna,  
			 secondo il formato esemplificato)  
  
------------------  
Come procedere :  
-----------------  
  
1) leggere attentamente le specifiche e capire il funzionamento il codice di test  
   fornito dai docenti. In particolare leggere attentamente il codice del client
   per capire cosa e' necessario implementare.   
  
2) implementare le funzioni richieste ed effettuare testing  
   preliminare utilizzando programmi sviluppati allo scopo  
  
3) testare il software con i test forniti dai docenti.   
  
       bash:~$ make test1  
       bash:~$ make test2  
       bash:~$ make test3  
       bash:~$ make test4  
       ...
  
4) preparare la documentazione: ovvero commentare adeguatamente il/i file che  
   contengono la soluzione  ed inserire un' intestazione contenente il nome  
   dello sviluppatore ed una dichiarazione di originalita'   
  
   /** \file pippo.c  
       \author Giuseppe Garibaldi 456789 & Nino Bixio 123456  
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
     originale degli autori.  */  
  
  
5) aggiornare il file "gruppo.txt" con nomi e dati  
  
6) inserire nel Makefile il nome dei file da consegnare 
   (variabile FILE_DA_CONSEGNARE) ed il nome del tarball da produrre
   (variabile TARNAME)
  
8) consegnare il file ESCLUSIVAMENTE eseguendo  
  
      bash:~$ make consegna  
  
   e seguendo le istruzioni. 
   Il target consegna crea un file tar che deve essere inviato  
   alla lista lso.di@listgateway.unipi.it (NON E' NECESSARIO 
   ISCRIVERSI ALLA LISTA) con oggetto:
  
   "lso16: Progetto B - corso A"  
   oppure con oggetto:
   "lso16: Progetto B - corso B"  
  
   Tutte le consegne verranno confermate con un messaggio entro 2/3  
   giorni all'indirizzo da cui e' stata effettuata la consegna. In  
   caso questo non si verifichi contattare il docente.  
     
---------------------------------------  
 NOTE IMPORTANTI: LEGGERE ATTENTAMENTE  
---------------------------------------  
  
1) gli eleborati contenenti tar non creati con "make consegna" o
   comunque seguendo le istruzioni riportate sopra non verranno
   accettati (gli studenti sono invitati a controllare che il tar
   contenga tutti i file richiesti sopra con il
   comando " tar tvf nomefiletar " prima di inviare il file)
  
2) tutti gli elaborati verranno confrontati fra di loro con tool automatici  
   per stabilire eventuali situazioni di PLAGIO. Se gli elaborato sono   
   ragionevolmente simili verranno scartati.  
  
3) Chi in sede di orale risulta palesemente non essere l'autore del software  
   consegnato dovra' ricominicare da zero con un altro progetto
  
4) Tutti i comportamenti scorretti ai punti 2 e 3 verranno segnalati  
   ufficialmente al presidente del corso di laurea  
  
  
