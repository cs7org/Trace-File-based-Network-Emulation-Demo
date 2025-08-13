# Emulation Setup

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
cp client server ../emulation/.
```

## Run the Experiments
### Full Normal Tests (UDP Crosstraffic)

```bash
sudo FORWARD_TRACE=trace_normal/forward.csv RETURN_TRACE=trace_normal/return.csv p2t run -p out -e emulation .
p2t export csv -e emulation -o csvs --skip_substitution .
p2t clean -e emulation
mv csvs/vma/a-ping/ping_rtt.csv ../results/ping_emulation.csv
mv out/vmb/tmp/server.csv ../results/speedtest_emulation.csv
rm -rf out/ csvs/
```

### TCP Crosstraffic Tests
```bash
sudo FORWARD_TRACE=trace_tcp_cross_traffic/forward.csv RETURN_TRACE=trace_tcp_cross_traffic/return.csv p2t run -p out -d .
mv out/vmb/tmp/server.csv ../results/crosstraffic/speedtest_emulation.csv
rm -rf out/
```

## Additional Details
### IP Addresses
- Node 1 (vma), eth1: 10.0.0.1
- Node 2 (vmb), eth1: 10.0.1.1
- Router:
   - eth1: 10.0.0.2
   - eth2: 10.0.1.2

### Topology
```
                      +-----------------------+                      
+------------+        |                       |        +------------+
| Node 1   eth1<---->eth1    Replay-Node    eth2<---->eth1   Node 2 |
|  (vma)     |        |       (router)        |        |     (vmb)  |
+------------+        +-----------------------+        +------------+
```
