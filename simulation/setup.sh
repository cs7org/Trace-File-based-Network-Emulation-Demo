#!/bin/bash

wget -O ns3-44.tar.gz https://gitlab.com/nsnam/ns-3-dev/-/archive/ns-3.44/ns-3-dev-ns-3.44.tar.gz
tar xzf ns3-44.tar.gz -C ./ns-3 --strip-components=1
cp -avr ./overlay/. ./ns-3
rm ns3-44.tar.gz

cd ./ns-3
./ns3 configure
./ns3 build
