#!/bin/bash

# server on vmb (receives)
#/opt/iperf-3.18/src/iperf3 -s -B $ADDRESS -p $PORT -1 -J --logfile /tmp/server.log
$TESTBED_PACKAGE/server --port $PORT --file /tmp/server.csv --verbose >> /tmp/status.log 2>&1
