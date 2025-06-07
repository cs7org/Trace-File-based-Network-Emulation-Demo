#!/usr/bin/python3

import pandas as pd
import matplotlib.pyplot as plt

from color import *

file_simulation = "../results/ping_simulation.csv"
file_emulation = "../results/ping_emulation.csv"
file_testbed = "../results/ping_testbed.csv"

def load_series(file, offset = 0):
    return pd.read_csv(file)

def main():
    series_simulation = load_series(file_simulation)
    series_emulation = load_series(file_emulation, offset=5)
    series_testbed = load_series(file_testbed)

    plt.style.use('ggplot')
    fig, ax = plt.subplots(figsize=(7, 4))

    ax.set_xlabel("Time (s)", fontsize=16, color='black')
    ax.set_xlim(10, 110)
    ax.set_ylabel("IMCP Ping RTT (ms)", fontsize=16, color='black')
    ax.plot(series_simulation["time"], series_simulation["rtt"], label="ns-3 Simulation", color=SIMULATION, linewidth=2)
    ax.plot(series_emulation["time"], series_emulation["rtt"], label="Emulation", color=EMULATION, linewidth=2)
    ax.plot(series_testbed["time"], series_testbed["rtt"], label="Real Testbed", color=TESTBED, linewidth=2)
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
