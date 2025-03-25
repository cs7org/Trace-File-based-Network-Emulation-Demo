#!/bin/bash

# Client sends
if [ "$IPERF_TCP" == "true" ]; then
    im log Starting $IPERF_BW TCP flow at `hostname`
    /opt/iperf-3.18/src/iperf3 --client $IPERF_ADDRESS --port $IPERF_PORT --bandwidth $IPERF_BW --time $IPERF_TIME --logfile /tmp/client.log
    im log Stopping $IPERF_BW TCP flow at `hostname`
else
    im log Starting $IPERF_BW UDP flow at `hostname`
    /opt/iperf-3.18/src/iperf3 --client $IPERF_ADDRESS --port $IPERF_PORT --udp --length 1024 --bandwidth $IPERF_BW --time $IPERF_TIME --logfile /tmp/client.log
    im log Stopping $IPERF_BW UDP flow at `hostname`
fi
