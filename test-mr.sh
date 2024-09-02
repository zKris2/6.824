#!/bin/bash

RACE=-race

rm -f out/*
rm -f mr-tmp/*

(cd mrapps && go build $RACE -buildmode=plugin wc.go) || exit 1
(cd mrapps && go build $RACE -buildmode=plugin crash.go) || exit 1
(cd main && go build $RACE mrmaster.go) || exit 1
(cd main && go build $RACE mrworker.go) || exit 1

echo '***' Starting master '***'.
timeout -k 2s 30s main/mrmaster main/pg*.txt &
# give the master time to create the sockets.
sleep 3

# echo '***' Starting wc worker  '***'.
# # start multiple workers
# for i in {1..3}; do
#     timeout -k 2s 30s main/mrworker mrapps/wc.so &
# done
# wait
# echo "All workers completed"


echo '***' Starting crash worker  '***'.
# start multiple workers
for i in {1..3}; do
    timeout -k 2s 180s main/mrworker mrapps/crash.so &
done
wait
echo "All workers completed"
echo '***' Crask worker success! '***'.


