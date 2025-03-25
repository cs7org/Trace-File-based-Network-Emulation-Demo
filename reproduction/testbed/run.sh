#!/bin/bash

# Bash strict mode.
set -euo pipefail
IFS=$'\n\t'

mkdir -p ./results

cwd=`pwd`
cd ../../testbed

for run in $(seq 1 100)
do
    rm -rf out
    echo "Starting testbed run $run ..."
    sudo USE_TCP=false p2t run -d -p out .
    mv out/node3/tmp/server.csv $cwd/results/${run}.csv
done
