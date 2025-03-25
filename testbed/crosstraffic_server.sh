#!/bin/bash

# Server recives
/opt/iperf-3.18/src/iperf3 --server --bind $IPERF_ADDRESS --port $IPERF_PORT --one-off --logfile /tmp/server.log
