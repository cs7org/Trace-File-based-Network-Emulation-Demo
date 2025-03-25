#!/bin/bash

ethtool -K eth1 tso off gso off gro off
ethtool -K eth2 tso off gso off gro off
ethtool -K eth3 tso off gso off gro off

sysctl -w net.ipv4.ip_forward=1

ip a a $ROUTER_ADDRESS dev eth1
ip l s up dev eth1

ip a a $NODE_A_ADDRESS dev eth2
ip l s up dev eth2

ip a a $NODE_B_ADDRESS dev eth3
ip l s up dev eth3

/usr/sbin/tc qdisc add root handle 1: dev eth1 netem rate $ROUTER_RATE delay $ROUTER_DELAY limit $ROUTER_LIMIT
/usr/sbin/tc qdisc add root handle 1: dev eth2 netem rate $LINK_RATE limit $LINK_LIMIT
/usr/sbin/tc qdisc add root handle 1: dev eth3 netem rate $LINK_RATE limit $LINK_LIMIT

ip r del default
ip r a $ROUTE dev eth1
