#!/bin/bash

rm -rf logs
mkdir logs

for i in $(seq 4 11); do
    timeout 60 ./build/modus-ponens ./examples/axioms3.txt $@ < "./examples/a$i.txt" 1> "logs/a$i.log" 2>&1
    retval=$?
    if [ $retval -ne 0 ]; then
        echo "a$i.txt: FAILED"
    else
        echo "a$i.txt: SUCCESS"
    fi
done