#!/usr/bin/python3

import pandas as pd
import matplotlib.pyplot as plt

file_simulation = "../results/speedtest_simulation.csv"
file_emulation = "../results/speedtest_emulation.csv"
file_testbed = "../results/speedtest_testbed.csv"

def load_series(file, scale = 1, offset = 0):
    df = pd.read_csv(file)
    df = df[df["receive_time"] != 0]
    df = df.sort_values(by="receive_time")

    df["receive_time"] = (df["receive_time"] / 1e9) + offset # ns -> s
    packet_size_bits = 1024 * 8
    df["time_diff"] = df["receive_time"].diff()
    df["bitrate"] = packet_size_bits / df["time_diff"]
    df = df.dropna(subset=["bitrate"])

    df["sock_cwnd"] = ((df["sock_cwnd"]) * scale) / 1024

    return df

def main():
    series_simulation = load_series(file_simulation, scale=1)
    series_emulation = load_series(file_emulation, scale=1024, offset=5)
    series_testbed = load_series(file_testbed, scale=1024)

    plt.style.use('ggplot')
    fig, ax = plt.subplots(figsize=(7, 4))

    ax.set_xlabel("Time (s)", fontsize=16, color='black')
    ax.set_xlim(10, 110)
    ax.set_ylim(0, 189)
    ax.set_ylabel("Congestion Window (KB)", fontsize=16, color='black')
    ax.plot(series_simulation["receive_time"], series_simulation["sock_cwnd"], label="ns-3 Simulation", color="tab:red", linewidth=2)
    ax.plot(series_emulation["receive_time"], series_emulation["sock_cwnd"], label="Emulation", color="tab:orange", linewidth=2)
    ax.plot(series_testbed["receive_time"], series_testbed["sock_cwnd"], label="Real Testbed", color="tab:blue", linewidth=2)
    ax.legend(loc="upper center", fontsize=14)
    ax.grid(color='white', linestyle='-', linewidth=1.5)
    ax.tick_params(axis='y', which='both', labelsize=16, color='black')
    ax.tick_params(axis='x', which='both', labelsize=16, color='black')

    plt.xticks(fontsize=16, color='black')
    plt.yticks(fontsize=16, color='black')
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
