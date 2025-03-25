#!/bin/bash

# Client sends
#/opt/iperf-3.18/src/iperf3 --client $ADDRESS --port $PORT --time 120 --logfile /tmp/status.log
$TESTBED_PACKAGE/client --host $ADDRESS --port $PORT --runfor 120 >> /tmp/status.log 2>&1
