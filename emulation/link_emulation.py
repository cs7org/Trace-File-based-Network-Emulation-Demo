#!/usr/bin/python3

import argparse
import psutil
import sys
import time
import subprocess
import datetime
import json

from typing import Optional, Dict, List
from dataclasses import dataclass

"""
time             = time to apply entry [ms]
rate             = interface outgoing bandwidth limit [bps]
delay            = netem delay [ms]
jitter           = netem delay jitter [ms]
loss             = outgoing packet loss [%]
queue_capacity   = capacity of the queue between both endpoints [packets]
hops             = number of hops emulated [hops]
"""

MIN_UPDATE_MS = 100
DEFAULT_HEADER = "at,delay,stddev,min_link_cap,max_link_cap,queue_capacity,hops,dropratio"
TC_EXEC = "/usr/sbin/tc"
IPTABLES_EXEC = "/usr/sbin/iptables"
IP_EXEC = "/usr/sbin/ip"
DEFAULT_INTERFACE_MTU = 1500

###### MTU (Always set for forward and return link)
# After interface setup:
#   iptables -A FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss <MTU>
# Cleanup (flush all):
#   iptables -F FORWARD
# Check rule:
#   iptables -L FORWARD ---line-numbers

##### TTL
# Initial setup (always rule 1 -> 2):
#   iptables -t mangle -A FORWARD -i <INTERFACE> -j TTL --ttl-dec/--ttl-inc <HOPS>
#   iptables -A FORWARD -m ttl --ttl-eq 0 -j REJECT --reject-with icmp-net-unreachable
# Update:
#   iptables -t mangle -R FORWARD <INDEX> -i <INTERFACE> -j TTL --ttl-dec/--ttl-inc <HOPS>
# Cleanup (flush all mangle chain):
#   iptables -t mangle -F FORWARD
# Check rule (get INDEX, check for correct interface!):
#   iptables -t mangle -L FORWARD -v --line-numbers

@dataclass
class TraceFileEntry:
    def __init__(self, interface: str) -> None:
        self.time: int
        self.rate: int
        self.delay: float
        self.jitter: float
        self.loss: float
        self.hops: int
        self.queue_cap: int
        self.interface: str = interface

    def __str__(self) -> str:
        return f"At t={self.time}: Rate={self.rate}, Delay={self.delay}, Hops={self.hops}, Queue={self.queue_cap}"

    def parse_line(self, line: str) -> bool:
        parts = line.split(",")
        if len(parts) != 8:
            return False

        at, delay, stddev, min_link_cap, _, queue_cap, hops, drop = parts
        min_link_cap = int(float(min_link_cap))

        try:
            self.time = int(at) // 1000 # µs to ms
            self.rate = min_link_cap
            self.delay = int(float(delay) // 1000) # µs to ms
            self.jitter = int(float(stddev) // 1000) # µs to ms
            self.loss = float(drop) * 100 # 0-1 to %
            self.hops = int(hops)
            self.queue_cap = int(queue_cap)
        except Exception as ex:
            print(ex)
            return False

        return True


def check_interface_exists(interface_name: str) -> bool:
    interfaces = psutil.net_if_addrs()
    return interface_name in interfaces


def load_and_parse_trace(trace_file_name: str, iface_name: str) -> Optional[Dict[int, TraceFileEntry]]:
    result = {}
    try:
        with open(trace_file_name, "r") as handle:
            lines = handle.readlines()

            linecount = 0
            for line in lines:
                linecount += 1
                line = ''.join(line.split())
                if line == DEFAULT_HEADER:
                    continue

                if line == "":
                    continue
                
                elem = TraceFileEntry(iface_name)
                if not elem.parse_line(line):
                    print(f"Unable to parse file '{trace_file_name}': Parsing error in line {linecount}.", file=sys.stderr)
                    return None
                
                result[elem.time] = elem
    except Exception as ex:
        print(f"Unable to read trace file '{trace_file_name}': {ex}", file=sys.stderr)
        return None
    
    return result


def subprocess_wrapper(command: List[str]) -> None:
    proc = None
    try:
        proc = subprocess.run(command, capture_output=True, shell=False)
    except Exception as ex:
        print(ex)
        raise Exception(f"Unable to run command '{command[0]}'.") from ex

    if proc is not None and proc.returncode != 0:
        raise Exception(f"Command '{command[0]}' failed with exit {proc.returncode} and stderr: {proc.stderr.decode('utf-8')}")


def set_mtu(interface: str, mtu: int = DEFAULT_INTERFACE_MTU, clear: bool = False) -> bool:
    try:
        subprocess_wrapper([IP_EXEC, "link", "set", "dev", interface, "mtu", str(mtu)])

        if clear:
            subprocess_wrapper([IPTABLES_EXEC, "-F", "FORWARD"]) # Also deletes rtt rule
        elif mtu != DEFAULT_INTERFACE_MTU:
            subprocess_wrapper([IPTABLES_EXEC, "-A", "FORWARD", "-p", "tcp", "--tcp-flags", "SYN,RST", "SYN", 
                                "-j", "TCPMSS", "--set-mss", str(mtu)])
        return True
    except Exception as ex:
        print(f"WARNING: Unable to set MTU of interface '{interface}': {ex}")
        return False

def get_netem_limit(values: TraceFileEntry) -> int:
    # Calculate BDP and offset with link queue sum
    if values.rate == 0 or values.delay == 0:
        return values.queue_cap

    return int(((values.rate / 8) * (values.delay / 1000)) / 1024 + values.queue_cap)


def check_reordering_condition(t: int, delay: int, jitter: int):
    if 2 * jitter > delay:
        print(f"WARNING: At t={t}: Jitter is to high to prevent reordering!", file=sys.stderr)


def clear_qdisc(interface: str, print_error: bool = False) -> bool:
    try:
        subprocess_wrapper([TC_EXEC, "qdisc", "del", "root", "dev", interface])
        return True
    except Exception as ex:
        if print_error:
            print(f"WARNING: Error during interface cleanup of '{interface}': {ex}")
        return False


def check_qdisc(interface: str) -> bool:
    proc = None
    try:
        proc = subprocess.run([TC_EXEC, "-j", "qdisc", "sh", "dev", interface], 
                              shell=False, capture_output=True)
    except Exception as ex:
        print(f"WARNING: Unable to query qdiscs for interface '{interface}': {ex}")
        return False

    if proc is not None and proc.returncode != 0:
        print(f"WARNING: Unable to query qdiscs for interface '{interface}': {proc.stderr.decode('utf-8')}")
        return False

    try:
        data = json.loads(proc.stdout)
        
        if len(data) != 1 or data[0]["kind"] != "netem":
            print(f"WARNING: Interface '{interface}' has no correctly applied qdisc.")
            return False
    except Exception as ex:
        print(f"WARNING: Unable to parse qdisc information for interface '{interface}': {ex}")
        return False

    return True


def install_ttl_mangle(interface: str, hops: int) -> bool:
    mode = "--ttl-dec" if hops >= 0 else "--ttl-inc" 
    hops = max(hops, 1)
    try:
        subprocess_wrapper([IPTABLES_EXEC, "-t", "mangle", "-A", "FORWARD", "-i", 
                            interface, "-j", "TTL", mode, str(hops)])
        return True
    except Exception as ex:
        print(f"WARNING: Unable to install ttl iptables rule for interface '{interface}': {ex}")
        return False


def index_ttl_mangle(interface: str) -> Optional[int]:
    proc = None
    try:
        proc = subprocess.run([IPTABLES_EXEC, "-t", "mangle", "-L", "FORWARD", "-v", "--line-numbers"], 
                              shell=False, capture_output=True)
    except Exception as ex:
        print(f"WARNING: Unable to query iptables rules for interface '{interface}': {ex}")
        return False

    if proc is not None and proc.returncode != 0:
        print(f"WARNING: Unable to query iptables rules for interface '{interface}': {proc.stderr.decode('utf-8')}")
        return False

    for line in proc.stdout.decode('utf-8').splitlines():
        if "TTL" in line and interface in line:
            line_no, _ = line.split(maxsplit=1)
            line_no = int(line_no.strip())
            return line_no

    return None


def update_ttl_mangle(interface: str, hops: int, rule_no: int):
    mode = "--ttl-dec" if hops >= 0 else "--ttl-inc" 
    hops = max(hops, 1)
    try:
        subprocess_wrapper([IPTABLES_EXEC, "-t", "mangle", "-R", "FORWARD", str(rule_no), 
                            "-i", interface, "-j", "TTL", mode, str(hops)])
        return True
    except Exception as ex:
        print(f"WARNING: Unable to update ttl iptables rule for interface '{interface}': {ex}")
        return False


def remove_ttl_mangle():
    try:
        subprocess_wrapper([IPTABLES_EXEC, "-t", "mangle", "-F", "FORWARD"])
        return True
    except Exception as ex:
        print(f"WARNING: Unable to clear ttl iptables rules: {ex}")
        return False


def initiate_qdisc(values: TraceFileEntry) -> bool:
    try:
        check_reordering_condition(values.time, values.delay, values.jitter)
        subprocess_wrapper([TC_EXEC, "qdisc", "add", "root", "handle", "1:", 
                            "dev", values.interface, "netem",
                            "delay", f"{values.delay}ms", f"{values.jitter}ms",
                            "loss", "random", f"{values.loss}",
                            "rate", f"{values.rate}bit",
                            "limit", str(get_netem_limit(values))])
        return True
    except Exception as ex:
        print(f"Error during initiation of netem qdisc: {ex}", file=sys.stderr)
        return False


def update_qdisc(values: TraceFileEntry) -> bool:
    try:
        check_reordering_condition(values.time, values.delay, values.jitter)
        subprocess_wrapper([TC_EXEC, "qdisc", "change", "root", "handle", "1:", 
                            "dev", values.interface, "netem",
                            "delay", f"{values.delay}ms", f"{values.jitter}ms",
                            "loss", "random", f"{values.loss}",
                            "rate", f"{values.rate}bit",
                            "limit", str(int(get_netem_limit(values)))])
        return True
    except Exception as ex:
        print(f"Error during initiation of netem qdisc: {ex}", file=sys.stderr)
        return False


def init(fwd_if_trace: Dict[int, TraceFileEntry], rtn_if_trace: Dict[int, TraceFileEntry], 
         debug: bool, mtu: Optional[int] = None) -> int:
    update_zero_forward = fwd_if_trace.get(0, None)
    update_zero_return = rtn_if_trace.get(0, None)

    if update_zero_forward is None:
        print(f"Forward link trace file does not contain information for t=0!", file=sys.stderr)
        return 1

    if update_zero_return is None:
        print(f"Return link trace file does not contain information for t=0!", file=sys.stderr)
        return 1

    if debug:
        print(f"Init Forward Link with -> {update_zero_forward}", file=sys.stderr)
        print(f"Init Return Link with -> {update_zero_return}", file=sys.stderr)

    if not initiate_qdisc(update_zero_forward) or not initiate_qdisc(update_zero_return):
        print(f"Initial installation of qdisc for trace file playback failed.", file=sys.stderr)
        return 1

    set_mtu(update_zero_forward.interface, mtu)
    set_mtu(update_zero_return.interface, mtu)

    if not install_ttl_mangle(update_zero_forward.interface, update_zero_forward.hops):
        print(f"Initial installation of ttl mangle rule failed for forward link.", file=sys.stderr)
        return 1
    
    if not install_ttl_mangle(update_zero_return.interface, update_zero_return.hops):
        print(f"Initial installation of ttl mangle rule failed for return link.", file=sys.stderr)
        return 1

    try:
        subprocess_wrapper([IPTABLES_EXEC, "-A", "FORWARD", "-m", "ttl", "--ttl-eq", 
                            "0", "-j", "REJECT", "--reject-with", "icmp-net-unreachable"])
    except Exception as ex:
        print(f"Unable to install ttl iptables filter rule: {ex}")
        return 1

    print(f"Emulation environment initialized, use 'run' to start playback.", file=sys.stderr)
    return 0


def run(fwd_if_trace: Dict[int, TraceFileEntry], rtn_if_trace: Dict[int, TraceFileEntry], debug: bool) -> int:
    forward_if_name = fwd_if_trace[0].interface
    return_if_name = rtn_if_trace[0].interface
    forward_rtt_rule_index = index_ttl_mangle(forward_if_name)
    return_rtt_rule_index = index_ttl_mangle(return_if_name)

    if forward_rtt_rule_index is None or return_rtt_rule_index is None:
        print(f"WARNING: Cannot obtain indexes for rtt iptables rules.", file=sys.stderr)
        return 1

    if debug:
        print(f"TTL rules: {forward_if_name}={forward_rtt_rule_index}, {return_if_name}={return_rtt_rule_index}", file=sys.stderr)

    forward_hops = fwd_if_trace[0].hops
    return_hops = rtn_if_trace[0].hops

    update_times = list(fwd_if_trace.keys())
    update_times.extend(list(rtn_if_trace.keys()))
    update_times = set(update_times)
    update_times.remove(0)

    print(f"Starting link Emulation ...", file=sys.stderr)
    current_time = 0
    took = 0
    for update_time in sorted(update_times):
        if (update_time - current_time) < MIN_UPDATE_MS:
            print(f"WARNING: Update time interval is less than {MIN_UPDATE_MS}ms!", file=sys.stderr)

        sleep_for = (update_time - current_time - took) / 1000
        time.sleep(sleep_for)
        if debug:
            print(f"Running t={update_time}ms at {datetime.datetime.now().isoformat()} (slept for {sleep_for:.2f}s).", 
                  file=sys.stderr)
        current_time = update_time
        started = time.time() * 1000

        fwd_entry = fwd_if_trace.get(update_time)
        rtn_entry = rtn_if_trace.get(update_time)

        if debug:
            print(f"Update Forward Link with -> {fwd_entry}", file=sys.stderr)
            print(f"Update Return Link with -> {rtn_entry}", file=sys.stderr)

        if fwd_entry is not None and not update_qdisc(fwd_entry):
            print(f"Unable to apply changes to forward link interface {forward_if_name} at time {update_time}", 
                  file=sys.stderr)

        if fwd_entry.hops != forward_hops:
            if not update_ttl_mangle(forward_if_name, fwd_entry.hops, forward_rtt_rule_index):
                print(f"Unable to apply changes to ttl at link interface {forward_if_name} at time {update_time}", 
                      file=sys.stderr)
            else:
                forward_hops = fwd_entry.hops

        if rtn_entry is not None and not update_qdisc(rtn_entry):
            print(f"Unable to apply changes to return link interface {return_if_name} at time {update_time}", 
                  file=sys.stderr)

        if rtn_entry.hops != return_hops:
            if not update_ttl_mangle(return_if_name, rtn_entry.hops, return_rtt_rule_index):
                print(f"Unable to apply changes to ttl at link interface {return_if_name} at time {update_time}", 
                      file=sys.stderr)
            else:
                return_hops = rtn_entry.hops

        took = (time.time() * 1000) - started

    print(f"Link Emulation completed.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Hypatia Trace File Replay Tool")

    common_parser = argparse.ArgumentParser(add_help=False)
    common_parser.add_argument(
        "forward_interface_name",
        type=str,
        help="Name of the forward interface")
    common_parser.add_argument(
        "forward_interface_trace_file",
        type=str,
        help="Trace file for the forward interface")
    common_parser.add_argument(
        "return_interface_name",
        type=str,
        help="Name of the return interface")
    common_parser.add_argument(
        "return_interface_trace_file",
        type=str,
        help="Trace file for the return interface")
    common_parser.add_argument(
        "-d", "--debug", required=False, action="store_true", default=False, 
        help="Show debug output")

    subparsers = parser.add_subparsers(
        title="Operation Mode", dest="command", required=True,
        description="Operational mode for the link emulation replay tool")

    init_parser = subparsers.add_parser(
        "init",
        parents=[common_parser],
        help="Initialize Link Emulation Interfaces")
    init_parser.add_argument(
        "--mtu",
        type=int,
        default=DEFAULT_INTERFACE_MTU,
        help="Define a path MTU for emulation (default: Ethernet Frame, 1500 bytes)")

    run_parser = subparsers.add_parser(
        "run",
        parents=[common_parser],
        help="Run Link Emulation Setup")

    clean_additional_parser = argparse.ArgumentParser(add_help=False)
    clean_additional_parser.add_argument(
        "forward_interface_name",
        type=str,
        help="Name of the forward interface")
    clean_additional_parser.add_argument(
        "return_interface_name",
        type=str,
        help="Name of the return interface")
    
    clean_parser = subparsers.add_parser(
        "clean",
        parents=[clean_additional_parser],
        help="Clean up or reset link emulation")

    args = parser.parse_args()

    if args.command == "clean":
        clear_qdisc(args.forward_interface_name, True)
        clear_qdisc(args.return_interface_name, True)
        set_mtu(args.forward_interface_name, DEFAULT_INTERFACE_MTU, clear=True)
        set_mtu(args.return_interface_name, DEFAULT_INTERFACE_MTU, clear=True)
        remove_ttl_mangle()
        print("Emulation configuration cleaned.", file=sys.stderr)
        sys.exit(0)

    for iface in [args.forward_interface_name, args.return_interface_name]:
        if not check_interface_exists(iface):
            print(f"No interface with name '{iface}' on system!", file=sys.stderr)
            sys.exit(1)

    trace_forward_link = load_and_parse_trace(args.forward_interface_trace_file, args.forward_interface_name)
    trace_return_link = load_and_parse_trace(args.return_interface_trace_file, args.return_interface_name)

    if trace_forward_link is None or trace_return_link is None:
        sys.exit(1)

    if args.command == "init":
        clear_qdisc(args.forward_interface_name, args.debug)
        clear_qdisc(args.return_interface_name, args.debug)

        status = init(trace_forward_link, trace_return_link, args.debug, args.mtu)
        sys.exit(status)
    elif args.command == "run":
        for iface in [args.forward_interface_name, args.return_interface_name]:
            if not check_qdisc(iface):
                print(f"Interface '{iface}' was not intialized, cannot proceed.", file=sys.stderr)
                sys.exit(1)

        status = run(trace_forward_link, trace_return_link, args.debug)
        sys.exit(status)
    else:
        print(f"Unknown subcommand: '{args.command}'", file=sys.stderr)
        sys.exit(1)
