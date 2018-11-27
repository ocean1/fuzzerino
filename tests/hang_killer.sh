#!/usr/bin/env bash
EXN=$1
SLEEP=$2

LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 4 | cut -d')' -f1`

while :
do
    PID=${LPID}

    sleep ${SLEEP}
    LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 4 | cut -d')' -f1`

    if [ -z "$LPID" ]; then
        echo "meh";
    elif [ "$PID" == "$LPID" ]; then
        echo "killing $PID";
        kill $PID;
    fi
done
