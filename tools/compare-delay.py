#!/usr/bin/python3

import pandas as pd
import matplotlib.pyplot as plt

file_simulation = "../results/speedtest_simulation.csv"
file_emulation = "../results/speedtest_emulation.csv"
file_testbed = "../results/speedtest_testbed.csv"

def load_series(file, offset = 0):
    df = pd.read_csv(file)
    df = df[df["receive_time"] != 0]
    df = df.sort_values(by="receive_time")

    df["delay"] = df["receive_time"] - df["send_time"]
    df["receive_time"] = (df["receive_time"] / 1e9) + offset # ns -> s
    df["delay"] = df["delay"] / 1e6 # ns -> ms

    return df

def main():
    series_simulation = load_series(file_simulation)
    series_emulation = load_series(file_emulation, offset=5)
    series_testbed = load_series(file_testbed)

    plt.style.use('ggplot')
    fig, ax = plt.subplots(figsize=(7, 4))

    ax.set_xlabel("Time (s)", fontsize=16, color='black')
    ax.set_xlim(10, 110)
    ax.set_ylabel("TCP End-2-End Delay (ms)", fontsize=16, color='black')
    ax.plot(series_simulation["receive_time"], series_simulation["delay"], label="ns-3 Simulation", color="tab:red", linewidth=2)
    ax.plot(series_emulation["receive_time"], series_emulation["delay"], label="Emulation", color="tab:orange", linewidth=2)
    ax.plot(series_testbed["receive_time"], series_testbed["delay"], label="Real Testbed", color="tab:blue", linewidth=2)
    ax.legend(loc="upper left", fontsize=14)
    ax.grid(color='white', linestyle='-', linewidth=1.5)
    ax.tick_params(axis='y', which='both', labelsize=16, color='black')
    ax.tick_params(axis='x', which='both', labelsize=16, color='black')

    plt.xticks(fontsize=16, color='black')
    plt.yticks(fontsize=16, color='black')
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
