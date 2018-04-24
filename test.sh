#!/bin/bash
make
perf stat ./run
perf stat ./run 2
perf stat ./run 3
perf stat ./run 4
#perf stat ./run 8
#perf stat ./run 12

