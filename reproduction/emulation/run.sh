#!/bin/bash

# Bash strict mode.
set -euo pipefail
IFS=$'\n\t'

mkdir -p ./results

cwd=`pwd`
cd ../../emulation

for run in $(seq 1 100)
do
    rm -rf out
    echo "Starting emulation run $run ..."
    sudo FORWARD_TRACE=trace_normal/forward.csv RETURN_TRACE=trace_normal/return.csv p2t run -d -p out .
    mv out/vmb/tmp/server.csv $cwd/results/${run}.csv
done
