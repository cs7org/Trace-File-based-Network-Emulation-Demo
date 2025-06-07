#!/usr/bin/python3

import os
import math

import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt

base_simulation = "./simulation/results/"
base_emulation = "./emulation/results/"
base_testbed = "./testbed/results/"

def load_sinle_series(basepath,  offset = 0):
    print(f"Loading series from {basepath} ...")
    series = []

    for file in os.listdir(basepath):
        if not file.endswith(".csv"):
            continue
        
        df = pd.read_csv(os.path.join(basepath, file))
        df = df[df["receive_time"] != 0]
        df = df.sort_values(by="receive_time")

        df["receive_time"] = (df["receive_time"] // (250 * 1e6)) # ns -> s (250ms segments)
        packet_counts = df.groupby('receive_time').size().reset_index(name="packet_count")
        packet_size_bits = 1024 * 8
        packet_counts["bitrate_ma"] = (packet_counts["packet_count"] * packet_size_bits) / (250 * 1e3)  # Mbps (based on 250ms segments)
        packet_counts["receive_time"] = packet_counts["receive_time"] * 0.25 + offset

        packet_counts = packet_counts.dropna(subset=["bitrate_ma"])
        packet_counts = packet_counts[packet_counts["receive_time"] > 50]
        packet_counts = packet_counts[packet_counts["receive_time"] < 70]
        mean = packet_counts["bitrate_ma"].mean()

        if math.isnan(mean):
            print(f"Warning: File {mean} contains NaN bitrate mean!")
        else:
            series.append(packet_counts["bitrate_ma"].mean())

    return series

def main():
    simulation = load_sinle_series(base_simulation)
    emulation = load_sinle_series(base_emulation)
    testbed = load_sinle_series(base_testbed)

    plt.style.use('ggplot')
    plt.figure(figsize=(7, 3))
    sns.stripplot(data=[simulation, emulation, testbed], palette=["m", "tab:green", "tab:blue"], orient='h', jitter=0.4, size=5)
    plt.yticks(ticks=[0,1,2], labels=["ns-3 Simulation", "Emulation", "Real Testbed"])
    plt.xlim(8.2, 9)
    plt.xlabel("Mean Goodput (Mbit/s)", fontsize=14, color='black')
    plt.grid(color='white', linestyle='-', linewidth=1.5)
    plt.tick_params(axis='y', which='both', labelsize=14, color='black')
    plt.tick_params(axis='x', which='both', labelsize=14, color='black')
    plt.xticks(fontsize=14, color='black')
    plt.yticks(fontsize=14, color='black')
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()