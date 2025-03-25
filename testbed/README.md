# "Physical" Testbed Setup

## Installation
Install [Proto²Testbed](https://github.com/martin-ottens/proto2testbed) on your system. Follow the instructions in `baseimage-creation` to create a base disk image using a *Debian 12.9* Netinstall ISO.

Create an experiment-specific image by installing the additional dependencies:
```bash
cd <path/to/proto-tetsbed>/instance-manager
make all

cd -
sudo p2t-genimg -i <path/to/baseimage.qcow2> -o <path/to/output.qcow2> -p <path/to/proto-tetsbed>/instance-manager/instance-manager.deb -e extra.commands
```

Change the `settings.diskimage_basepath` and `instances.*.diskimage` values in `./testbed.json`, so that they point to the path of the experiment-specific image created previously.

Build the *tcp-test* applications in `../tcp-test` and copy the binaries:
```bash
cd ../tcp-test
make all
cp client server ../testbed/.
```

## Run the Experiments
### Full Normal Tests (UDP Crosstraffic)

```bash
sudo USE_TCP=false p2t run -p out -e testbed .
p2t export csv -e testbed -o csvs --skip_substitution .
p2t clean -e testbed
mv csvs/node1/a-ping/ping_rtt.csv ../results/ping_testbed.csv
mv out/node3/tmp/server.csv ../results/speedtest_testbed.csv
rm -rf out/ csvs/
```

### TCP Crosstraffic Tests
```bash
sudo USE_TCP=true p2t run -p out -d .
mv out/node3/tmp/server.csv ../results/crosstraffic/speedtest_testbed.csv
rm -rf out/
```

## Additional Details
### IP Addresses
- Node 1, eth1: 10.0.1.1
- Node 2, eth1: 10.0.2.1
- Node 3, eth1; 10.0.3.1
- Node 4, eth1: 10.0.4.1
- Router 1:
   - eth1: 10.0.10.1
   - eth2: 10.0.1.2
   - eth3: 10.0.2.2
- Router 2:
   - eth1: 10.0.10.2
   - eth2: 10.0.3.2
   - eth3: 10.0.4.2

### Topology
```
+-----------+                                                                 +-----------+
| Node 1  eth1<---+                                                     +--->eth1  Node 3 |
+-----------+     |     +---------------+        +----------------+     |     +-----------+
                  +-->eth2              |        |               eth2<--+
                        |  RouterNode eth1<---->eth1  RouterNode  |
                  +-->eth3       1      |        |      2        eth3<--+
+-----------+     |     +---------------+        +----------------+     |     +-----------+
| Node 2  eth1<---+                                                     +--->eth1  Node 4 |
+-----------+                                                                 +-----------+
```
