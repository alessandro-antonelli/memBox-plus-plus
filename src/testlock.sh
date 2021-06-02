#!/bin/bash

./client -l $1 -c 0:4:0 -c 10:0:1024 -c 10:1:1024 -c 10:2:0 -c 10:3:0 -c 0:5:0
if [[ $? != 0 ]]; then
    exit 1
fi

# messaggio di errore che mi aspetto
OP_LOCKED=21

# eseguo il comando in background
./client -l $1 -s 1000 -c 0:4:0 -c 10:0:1024 -c 10:1:1024 -c 10:2:0 -c 10:3:0 -c 0:5:0 &

# mi ricordo il pid
pid1=$!

# aspetto un po'
sleep 1

# lancio un comando di PUT ma il sistema e' in stato di lock... deve fallire
./client -l $1 -c 20:0:1024

# e' l'errore che mi aspetto ?
if [[ $((256-$?)) == $OP_LOCKED ]]; then
    wait $pid1
    exit 0
fi
wait $pid1
exit 1
