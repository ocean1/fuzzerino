#!/usr/bin/env bash
EXN=$1
SLEEP=$2

#LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 3 | cut -d')' -f1`

P_PID=`pgrep $EXN | head -n1`
CPID=`ps --ppid $P_PID | cut -d' ' -f1`
echo $P_PID $CPID

while :
do
    PID=${CPID}

    sleep ${SLEEP}
    #LPID=`pstree -s -p | grep $EXN | cut -d'(' -f 3 | cut -d')' -f1`
    CPID=`ps --ppid $P_PID | cut -d' ' -f1`

    if [ -z "$CPID" ]; then
        echo "meh" > /dev/null;
    elif [ "$PID" == "$CPID" ]; then
        echo "killing $PID";
        kill $PID;
    fi
done
