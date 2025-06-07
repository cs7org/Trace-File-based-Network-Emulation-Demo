#!/bin/bash

ethtool -K eth1 tso off gso off gro off

ip a a $IP_ADDRESS dev eth1
ip l s up dev eth1

sysctl -w net.core.rmem_max=67108864
sysctl -w net.core.wmem_max=67108864
sysctl -w net.ipv4.tcp_rmem="4096 1391456 6291456"
sysctl -w net.ipv4.tcp_wmem="4096 1391456 6291456"

/usr/sbin/tc qdisc add root handle 1: dev eth1 netem rate $LINK_RATE limit $LINK_LIMIT

ip r del default
ip r a $ROUTE dev eth1
