#!/bin/bash

for((i=0;i<500;++i)); do ./client -l $1 -c $i:0:16; done
for((i=0;i<200;++i)); do ./client -l $1 -c $i:2:0;  done
for((i=0;i<300;++i)); do ./client -l $1 -c $i:1:16; done
for((i=0;i<500;++i)); do ./client -l $1 -c $i:3:0;  done

killall -USR1 membox
sleep 1

read put putfailed update updatefailed get getfailed remove removefailed <<< $(tail -1 $2 | cut -d\  -f 3,4,5,6,7,8,9,10 )

if [[ $put != 500 || $putfailed != 0 || $get != 200 || $getfailed != 0 || $update != 300 || $updatefailed != 0 || $remove != 500 || $removefailed != 0 ]]; then 
    echo "Test fallito"
    exit 1
fi
echo "Test OK!"
exit 0


