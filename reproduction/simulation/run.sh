#!/bin/bash

# Bash strict mode.
set -euo pipefail
IFS=$'\n\t'

mkdir -p ./results

for run in $(seq 1 100)
do
    echo "Starting simulation run $run ..."
    bash -c '../../simulation/ns3 run simulate -- --seed="$1"' _ "$run"
    mv ../../simulation/speedtest_0.csv ./results/${run}.csv
done
