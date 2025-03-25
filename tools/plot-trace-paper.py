#!/usr/bin/python3

import matplotlib.pyplot as plt
import pandas as pd
import sys

def main(file):
    data = pd.read_csv(file)

    data['at'] /= 1e6 # µs -> s
    data['queue'] =  ((data['min_link_cap'] / 8) * (data['delay'] / 1e6) // 1024) + data['queue_capacity']
    data['delay'] /= 1e3 # µs -> ms
    data['stddev'] /= 1e3 # µs -> ms
    data['min_link_cap'] /= 1e6 # bps -> Mbps

    plt.style.use('ggplot')
    fig, ax1 = plt.subplots(figsize=(8, 4.2))

    ax1.set_xlabel("Time (s)", fontsize=16, color='black')
    ax1.set_ylabel("Path Delay (ms)", color='tab:blue', fontsize=16)
    line1, = ax1.plot(data['at'], data['delay'], label='Delay', color='tab:blue', linewidth=2)
    ax1.tick_params(axis='y', labelcolor='tab:blue', labelsize=16)
    ax1.tick_params(axis='x', which='both', labelsize=16, color='black')
    ax1.grid(color='white', linestyle='-', linewidth=1.5)
    ax1.set_ylim(9.5, 10.2)
    ax1.set_xlim(0, 120)

    ax2 = ax1.twinx()
    ax2.set_ylabel("Available Bandwidth (Mbps)", color='tab:red', fontsize=16)
    line2, = ax2.plot(data['at'], data['min_link_cap'], label='Available Bandwidth', color='tab:red', linewidth=2)
    ax2.tick_params(axis='y', labelcolor='tab:red', labelsize=16)
    ax2.set_ylim(0, 35)
    ax2.grid(None)

    ax3 = ax1.twinx()
    ax3.spines['right'].set_position(('outward', 60))
    ax3.spines['right'].set_color('gray')
    ax3.spines['right'].set_linewidth(1)
    ax3.set_ylabel("Queue Capacity (Packets)", color='tab:green', fontsize=16)
    line3, = ax3.plot(data['at'], data['queue'], label='Queue Capacity', color='tab:green', linewidth=2)
    ax3.tick_params(axis='y', labelcolor='tab:green', labelsize=16)
    ax3.set_ylim(0, 175)
    ax3.grid(None)

    [t.set_color('black') for t in ax1.xaxis.get_ticklabels()]

    lines = [line1, line2, line3]
    labels = [l.get_label() for l in lines]
    fig.legend(lines, labels, loc='lower center', ncol=4, fontsize=14)

    plt.xticks(fontsize=16)
    plt.yticks(fontsize=16)
    plt.tight_layout()
    plt.subplots_adjust(bottom=0.27)
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file>")
        sys.exit(1)

    main(sys.argv[1])