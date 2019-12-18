# memBox++
Un'applicazione client-server che realizza un repository di oggetti costituiti da sequenze continue di bytes, gestendo la concorrenza tra più client attivi contemporaneamente.

Supporta [nove tipi di operazioni](src/ops.h) sugli oggetti, il salvataggio/caricamento del repository in un file di dump e la generazione di statistiche sull'attività del server (con inoltre uno [script bash](src/memboxstat.sh) per la loro visualizzazione).

Per dettagli sui requisiti richiesti, vedere [__Struttura complessivia del progetto__](Struttura%20complessivia%20del%20progetto.pdf) e [__Istruzioni e specifiche__](Istruzioni%20e%20specifiche.txt); per dettagli sulle scelte implementative vedere la [__Relazione__](Relazione/Relazione.pdf).

Progetto finale del Laboratorio di Programmazione Sistema dell'A.A. 2015/16, tenuto dal professor Massimo Torquati (facente parte dell'esame di Sistemi operativi e laboratorio, codice 277AA), nel corso di laurea triennale in Informatica dell'Università di Pisa.

Homepage del corso: http://didawiki.cli.di.unipi.it/doku.php/informatica/sol/laboratorio16
