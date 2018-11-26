#!/usr/bin/env bash
EXN=$1

LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 4 | cut -d')' -f1`

while :
do
    PID=${LPID}

    sleep 5
    LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 4 | cut -d')' -f1`
    if [ "$PID" == "$LPID" ]; then
        echo "killing $PID";
        kill $PID;
    fi
done
