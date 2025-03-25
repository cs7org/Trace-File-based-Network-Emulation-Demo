#!/bin/bash

# Server receives
#/opt/iperf-3.18/src/iperf3 --server --bind $ADDRESS --port $PORT --one-off --logfile /tmp/status.log
$TESTBED_PACKAGE/server --port $PORT --file /tmp/server.csv >> /tmp/status.log 2>&1
