#!/bin/bash

# controllo 

(for((i=0;i<500;++i)); do ./client -l $1 -c $i:0:16; done)&
(for((i=0;i<500;++i)); do ./client -l $1 -c $i:0:16; done)&
wait

killall -USR1 membox
sleep 1
read put putfailed <<< $(tail -1 $2 | cut -d\  -f 3,4 )

if [[ $put != 1000 || $putfailed != 500 ]]; then 
    echo "Test fallito"
    exit 1
fi
echo "Test OK!"
exit 0


