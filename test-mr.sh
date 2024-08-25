#!/bin/bash

RACE=

(cd main && go build $RACE mrmaster.go) || exit 1
(cd main && go build $RACE mrworker.go) || exit 1

echo '***' Starting master.
timeout -k 2s 30s main/mrmaster main/pg*.txt &

# give the master time to create the sockets.
sleep 3

echo '***' Starting worker.

# start multiple workers
for i in {1..3}; do
    echo "Starting worker $i"
    timeout -k 2s 30s main/mrworker ../../mrapps/wc.so &
done
# wait for one of the processes to exit.
# under bash, this waits for all processes,
# including the master.
wait
echo "All workers completed"


