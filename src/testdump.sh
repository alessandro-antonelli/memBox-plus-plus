#!/bin/bash

if [[ $# != 3 ]]; then
    echo "usa $0 unix_path stat_path dump_path"
    exit 1
fi

./membox -f DATA/membox.conf3 &
sleep 1

for((i=1;i<=128;++i)); do 
    ./client -l $1 -c $i:0:$i
done 

# rimuovo un elemento
./client -l $1 -c 100:3:0

# acquisisco la lock e faccio il dump
./client -l $1 -c 0:4:0 -c 0:8:0

# invio il segnale per far terminare il server
killall -USR2 -w membox

if [[ ! -f $3 ]]; then
    echo "Test fallito"
    exit 1
else 
    size=$(wc -c < "$3")
    if [[ $size != 9696 ]]; then
	echo "Test fallito"
	exit 1
    fi
fi

./membox -f DATA/membox.conf3 -d "$3" &
sleep 1

for((i=1;i<100;++i)); do 
    ./client -l $1 -c $i:3:0
done 
for((i=101;i<=128;++i)); do 
    ./client -l $1 -c $i:3:0
done 

# stattistiche 
killall -USR1 membox
sleep 1
read remove removefailed dump dumpfailed obj maxobj <<< $(tail -1 $2 | cut -d\  -f 9,10,15,16,20,21 )

if [[ $remove != 127 || $removefailed != 0 ]]; then 
    echo "Test fallito"
    exit 1
fi

if [[ $obj != 0 || $maxobj != 127 ]]; then 
    echo "Test fallito"
    exit 1
fi

if [[ ! -f "$3.bak" ]]; then
    echo "Test fallito"
    exit 1
fi



# invio il segnale per far terminare il server
killall -USR2 -w membox

echo "Test OK!"
exit 0
