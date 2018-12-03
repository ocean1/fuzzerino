#!/usr/bin/env bash
EXN=$1
SLEEP=$2

#LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 3 | cut -d')' -f1`

CPID=`ps h --ppid $EXN | cut -d' ' -f1`
echo $CPID

while :
do

    PID=$CPID
    sleep ${SLEEP}
    #LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 3 | cut -d')' -f1`
    CPID=`ps h --ppid $EXN | cut -d' ' -f1`

    ps h --ppid $EXN
    if [ -z "$CPID" ]; then
        echo "meh"
    elif [ "$PID" == "$CPID" ]; then
        echo "killing $PID";
        kill -9 $PID;
    fi
done
