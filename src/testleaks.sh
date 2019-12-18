#!/bin/bash

# testa se ci sono memory leaks nel server mbox

if [[ $# != 1 ]]; then
    echo "usa $0 unix_path"
    exit 1
fi

rm -f valgrind_out
/usr/bin/valgrind --leak-check=full ./membox -f DATA/membox.conf1 >& ./valgrind_out &
pid=$!

# aspetto un po' per far partire valgrind
sleep 5

# put
for((i=1;i<=128;++i)); do 
    ./client -l $1 -c $i:0:$((i*16))
done 

# invio il segnale per generare le statistiche
kill -USR1 $pid

# update and get
for((i=1;i<=128;++i)); do 
    ./client -l $1 -c $i:1:$((i*16)) -c $i:2:0
done 

# invio il segnale per generare le statistiche
kill -USR1 $pid

# remove
for((i=1;i<=128;++i)); do 
    ./client -l $1 -c $i:3:0
done 

# invio il segnale per far terminare il server
kill -USR2 $pid

sleep 2

r=$(tail -10 ./valgrind_out | grep "ERROR SUMMARY" | cut -d: -f 2 | cut -d" " -f 2)

if [[ $r != 0 ]]; then
    exit 1
fi

exit 0


