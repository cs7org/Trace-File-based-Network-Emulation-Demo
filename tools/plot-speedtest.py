#!/usr/bin/python3

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import math
import statistics

def main(file: str):
    df = pd.read_csv(file)

    df = df[df["receive_time"] != 0]
    
    packet_size_bits = 1024 * 8
    df["time"] = df["send_time"] / 1e9 # ns -> s

    df["delay_real"] = (df["receive_time"] - df["send_time"]) / 1e6 # ns -> ms
    df["sock_rtt"] = df["sock_rtt"] / 1e6 # ns -> ms

    df["receive_time_sec"] = df["receive_time"] / 1e9 # ns -> s
    window_size_sec = 1
    data_rates = np.zeros(len(df))

    receive_times = df["receive_time_sec"].to_numpy()

    for i in range(len(df)):
        window_start = receive_times[i] - window_size_sec
        num_packets_in_window = np.searchsorted(receive_times, receive_times[i], side="right") - \
                                np.searchsorted(receive_times, window_start, side="left")

        data_rates[i] = (num_packets_in_window * packet_size_bits) / window_size_sec

    df["data_rates"] = data_rates
    df["data_rates"] = df["data_rates"] / (1024 * 1024)

    fig, ax1 = plt.subplots(figsize=(20,8))

    color = 'tab:blue'
    ax1.set_xlabel("Experiment Time")
    ax1.set_ylabel("Data Rate (Mbit/s)", color=color)
    ax1.set_ylim(0, statistics.fmean(df["data_rates"]) * 5)
    ax1.plot(df["time"][:-1], df["data_rates"][:-1], linestyle="-", color=color)
    ax1.tick_params(axis='y', labelcolor=color)

    ax2 = ax1.twinx()
    color = 'tab:red'
    ax2.set_ylabel("Delay (ms)", color=color)
    ax2.set_ylim(0, statistics.fmean(df["delay_real"]) * 2)
    ax2.plot(df["time"], df["delay_real"], linestyle="-", color=color)
    ax2.tick_params(axis='y', labelcolor=color)

    ax3 = ax1.twinx()
    ax3.spines['right'].set_position(('outward', 60))
    color = 'tab:green'
    ax3.set_ylabel("RTT (ms)", color=color)
    ax3.set_ylim(0, statistics.fmean(df["sock_rtt"]) * 2)
    ax3.plot(df["time"], df["sock_rtt"], linestyle="-", color=color)
    ax3.tick_params(axis='y', labelcolor=color)

    ax4 = ax1.twinx()
    ax4.spines['right'].set_position(('outward', 120))
    color = 'tab:purple'
    ax4.set_ylabel("Congestion Window (Packets)", color=color)
    ax4.set_ylim(0, max(df["sock_cwnd"]) * 1.1)
    ax4.plot(df["time"], df["sock_cwnd"], linestyle="-", color=color)
    ax4.tick_params(axis='y', labelcolor=color)

    plt.title("Data Rate and Delay Visualization")
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="Speedtest result visualizer")
    parser.add_argument("FILE", type=str, help="Input file")
    args = parser.parse_args()

    main(args.FILE)
