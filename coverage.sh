#!/bin/bash
make
./run 4
lcov -t "run" -o run.info -c -d .
genhtml -o report run.info 
