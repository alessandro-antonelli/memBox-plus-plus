#!/bin/bash

# messaggio di errore che mi aspetto
OP_FAIL=12
OP_PUT_TOOMANY=14
OP_PUT_SIZE=15
OP_PUT_REPOSIZE=16
OP_UPDATE_SIZE=19
MAX_CONNECTIONS=2

./client -l $1 -c 0:0:50000
if [[ $((256-$?)) != $OP_PUT_SIZE ]]; then
    exit 1
fi

# inserisco 2 oggetti di size massima
./client -l $1 -c 0:0:32768 -c 1:0:32768
if [[ $? != 0 ]]; then
    exit 1
fi

./client -l $1 -c 2:0:1
if [[ $((256-$?)) != $OP_PUT_REPOSIZE ]]; then
    exit 1
fi

# rimuovo gli oggetti inseriti 
./client -l $1 -c 0:3:0 -c 1:3:0
if [[ $? != 0 ]]; then
    exit 1
fi

# PUT
for ((i=0;i<1024;++i)); do 
    ./client -l $1 -c $i:0:63
    if [[ $? != 0 ]]; then
	exit 1
    fi
done

./client -l $1 -c 1024:0:1
if [[ $((256-$?)) != $OP_PUT_TOOMANY ]]; then
    exit 1
fi

# GET
for ((i=0;i<1024;++i)); do 
    ./client -l $1 -c $i:2:0
    if [[ $? != 0 ]]; then
	exit 1
    fi
done

# UPDATE
for ((i=0;i<1024;++i)); do 
    ./client -l $1 -c $i:1:63
    if [[ $? != 0 ]]; then
	exit 1
    fi
done

./client -l $1 -c 0:1:62
if [[ $((256-$?)) != $OP_UPDATE_SIZE ]]; then
    exit 1
fi


# REMOVE
for ((i=0;i<1024;++i)); do 
    ./client -l $1 -c $i:3:0
    if [[ $? != 0 ]]; then
	exit 1
    fi
done

# testing MaxConnections
./client -l $1 -s 1000 -c 0:0:1024 -c 1:0:1024 -c 2:0:1024 -c 0:2:0 -c 1:2:0 -c 2:2:0  &
./client -l $1 -s 1000 -c 3:0:1024 -c 4:0:1024 -c 5:0:1024 -c 3:2:0 -c 4:2:0 -c 5:2:0  &  
./client -l $1 -s 1000 -c 6:0:1024 -c 7:0:1024 -c 8:0:1024 -c 6:2:0 -c 7:2:0 -c 8:2:0  &
./client -l $1 -s 1000 -c 9:0:1024 -c 10:0:1024 -c 11:0:1024 -c 9:2:0 -c 10:2:0 -c 11:2:0  &

# invio SIGUSR1 in modo che il server faccia il dump delle statistiche
killall -USR1 membox

sleep 1
./client -l $1 -c 12:0:1024
e=$?
if [[ $((256-e)) != $OP_FAIL ]]; then
    echo "Errore non corrispondente $e" 
    exit 1
fi
# controllo il numero di connessioni concorrenti nel file delle statistiche 
conn=$(tail -1 $2 | cut -d\  -f 13)
if ! (($conn <= $MAX_CONNECTIONS)); then
    echo "Numero massimo di connessioni non esatto!!!"
    exit 1
fi
killall -QUIT -w membox
exit 0


