#!/bin/bash

ethtool -K eth1 tso off gso off gro off
ethtool -K eth2 tso off gso off gro off

ip a a 10.0.0.2/24 dev eth1
ip a a 10.0.1.2/24 dev eth2
ip l s up dev eth1
ip l s up dev eth2

sysctl -w net.ipv4.ip_forward=1

python3 $TESTBED_PACKAGE/link_emulation.py init -d $FORWARD_LINK $FORWARD_TRACE $RETURN_LINK $RETURN_TRACE > /tmp/trace.log 2>&1
