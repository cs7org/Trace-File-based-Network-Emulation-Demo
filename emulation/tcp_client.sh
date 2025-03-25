#!/bin/bash

# client on vma (sends)
#/opt/iperf-3.18/src/iperf3 -c $ADDRESS -p $PORT -t 110 -J --logfile /tmp/client.log
$TESTBED_PACKAGE/client --host $ADDRESS --port $PORT --runfor 110 --verbose >> /tmp/status.log 2>&1
