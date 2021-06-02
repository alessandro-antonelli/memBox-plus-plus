#!/bin/bash

./client -l $1 -c 100000:0:1024
# la unlock viene fatta automaticamente alla chiusura della connessione
./client -l $1 -s 1000 -c 100000:6:1024 -c 100000:1:1024 &
sleep 1
./client -l $1 -c 100000:6:0 -c 100000:1:1024 -c 100000:7:0
if [[ $? != 0 ]]; then
    exit 1
fi

sleep 1
# stattistiche 
killall -USR1 membox
sleep 1

read update lockobj <<< $(tail -1 $2 | cut -d\  -f 5,13 )

if [[ $update != 2 || $lockobj != 2 ]]; then 
    echo "Test fallito"
    exit 1
fi


# piu' lock sullo stesso oggetto
for((i=0;i<5;++i)); do 
    ./client -l $1 -c 100000:6:0  -c 100000:1:1024 -c 100000:7:0 &
    PID[i]=$!
done

for((i=0;i<5;++i)); do
    wait ${PID[i]}
done

# messaggi di errore che mi aspetto
OP_LOCK_OBJ_NONE=23
OP_LOCK_OBJ_DLK=24


# controllo condizione di deadlock
./client -l $1 -c 100000:6:0  -c 100000:6:1024
if [[ $((256-$?)) != $OP_LOCK_OBJ_DLK ]]; then
    echo "Test fallito"
    exit 1
fi


# mentre un client e' in attesa della lock l'oggetto su cui e' stata
# fatta la lock viene cancellato
./client -l $1 -c 200000:0:100
./client -l $1 -s 2000 -c 200000:6:0 -c 200000:1:100 -c 200000:3:0  &
sleep 1
./client -l $1 -c 200000:6:0 -c 200000:1:1024 -c 200000:7:0
if [[ $((256-$?)) != $OP_LOCK_OBJ_NONE ]]; then
    echo "Test fallito"
    exit 1
fi

echo "Test OK!"
exit 0

