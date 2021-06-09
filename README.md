# memBox++
![screenshot](screenshot/statistiche%20ultimo%20timestamp%20aggiornamento%20continuo.gif "Esecuzione di una batteria di operazioni con visualizzazione delle statistiche in tempo reale")

Un'applicazione client-server che realizza un repository di oggetti costituiti da sequenze continue di bytes, gestendo la concorrenza tra più client attivi contemporaneamente.

Supporta nove tipi di operazioni sugli oggetti, il salvataggio/caricamento del repository in/da un file di dump e la generazione di statistiche sull'attività del server (con inoltre uno [script bash](src/memboxstat.sh) per la loro visualizzazione).

Per dettagli sui requisiti richiesti, vedere [__Struttura complessiva del progetto__](Struttura%20complessiva%20del%20progetto.pdf) e [__Istruzioni e specifiche__](Istruzioni%20e%20specifiche.txt); per dettagli sulle scelte implementative vedere la [__Relazione__](Relazione/Relazione.pdf).

Realizzato come progetto finale del Laboratorio di Programmazione di Sistema dell'A.A. 2015/16, tenuto dal professor Massimo Torquati (facente parte dell'esame di Sistemi operativi e laboratorio, codice 277AA), nel corso di laurea triennale in Informatica dell'Università di Pisa. Valutato con il voto di 30/30. Homepage del corso: http://didawiki.cli.di.unipi.it/doku.php/informatica/sol/laboratorio16

## Istruzioni per l'esecuzione
``` Shell Session
#download sorgenti
git clone https://github.com/alessandro-antonelli/memBox-plus-plus
cd memBox-plus-plus/src/
chmod +x testput.sh testdump.sh testlock.sh testsize.sh testleaks.sh teststats.sh memboxstat.sh teststress.sh testlockobj.sh gruppo-check.pl

#compilazione
make all

#avvio server
./membox -f ./DATA/membox.conf1 &
```

Spegnimento server: `killall membox -SIGUSR2`

### Invio di richieste dal client
Per eseguire il client (e richiedere al server l'esecuzione di un'operazione) la sintassi è:

``` Shell Session
./client -l /tmp/mbox_socket -c K:O:B
```

dove __K__ è la chiave dell'oggetto su cui si vuole operare, __O__ è il numero dell'operazione da eseguire sull'oggetto (per i codici vedere [ops.h](src/ops.h)) e __B__ è la quantità di byte relativa all'operazione.

Esempio (richiesta di operazione PUT [op. 0] di un oggetto di 8192 byte con chiave 0):

![screenshot](screenshot/comando%20singolo.png "Esecuzione di un comando singolo")

Esecuzione di una batteria di 9 test prestabiliti: `make -s anytest`

### Visualizzazione delle statistiche
Il programma è corredato di uno script bash che consente di visualizzare le statistiche del server.

Per visualizzare il valore corrente delle statistiche, usare il comando `./memboxstat.sh /tmp/mbox_stats.txt` (oppure, per visualizzare dati costantemente aggiornati in tempo reale, usare `watch -n1 ./memboxstat.sh /tmp/mbox_stats.txt`):

![screenshot](screenshot/statistiche%20ultimo%20timestamp.png "Statistiche: valore corrente")

È anche possibile visualizzare lo storico dei valori precedenti (per la sintassi, vedere `./memboxstat.sh --help`):

![screenshot](screenshot/statistiche%20tutti%20timestamp.png "Statistiche: storico dei valori precedenti")
