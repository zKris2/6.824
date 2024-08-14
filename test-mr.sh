#!/bin/sh

RACE=

(cd master && go build $RACE mrmaster.go) || exit 1
(cd worker && go build $RACE mrworker.go) || exit 1

echo '***' Starting master.
./master/mrmaster ./pg*txt


echo '***' Starting worker.
./worker/mrworker
