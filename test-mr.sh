#!/bin/bash

RACE=

(cd master && go build $RACE mrmaster.go) || exit 1
(cd worker && go build $RACE mrworker.go) || exit 1

echo '***' Starting master.
timeout -k 2s 180s ./master/mrmaster ../pg*txt &

# give the master time to create the sockets.
sleep 1

echo '***' Starting worker.

# start multiple workers
for i in {1..3}; do
    echo "Starting worker $i"
    timeout -k 2s 180s ./worker/mrworker ../../mrapps/wc.so &
done
# wait for one of the processes to exit.
# under bash, this waits for all processes,
# including the master.
wait
echo "All workers completed"


