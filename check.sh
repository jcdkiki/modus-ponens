#!/bin/bash

rm -rf logs
mkdir logs

for i in $(seq 4 11); do
    timeout 2 ./build/modus-ponens ./examples/axioms3.txt < "./examples/a$i.txt" > "logs/a$i.log"
    retval=$?
    if [ $retval -ne 0 ]; then
        echo "a$i.txt: FAILED"
    else
        echo "a$i.txt: SUCCESS"
    fi
done