#!/bin/bash
make
for var in 1 2 3 4 5 6 7 8 9 10 11 12
do
echo $var
time ./run $var
done
