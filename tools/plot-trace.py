#!/usr/bin/python3

import matplotlib.pyplot as plt
import pandas as pd
import sys

def main(file):
    data = pd.read_csv(file)

    data['at'] /= 1e6 # µs -> s
    data['delay'] /= 1e3 # µs -> ms
    data['stddev'] /= 1e3 # µs -> ms
    data['min_link_cap'] /= 1e6 # bps -> Mbps
    data['max_link_cap'] /= 1e6 # bps -> Mbps

    fig, ax1 = plt.subplots(figsize=(12, 6))

    ax1.set_xlabel("Simulation Time (s)")
    ax1.set_ylabel("Delay / Stddev (ms)", color='tab:blue')
    line1, = ax1.plot(data['at'], data['delay'], label='Delay', color='blue')
    line2, = ax1.plot(data['at'], data['stddev'], label='Stddev', color='cyan')
    ax1.tick_params(axis='y', labelcolor='tab:blue')

    ax2 = ax1.twinx()
    ax2.set_ylabel("Path Capacity (Mbps)", color='tab:red')
    ax2.set_ylim(0, max(data['max_link_cap']) * 1.5)
    line3, = ax2.plot(data['at'], data['min_link_cap'], label='Minimum Path Capacity', color='red')
    line4, = ax2.plot(data['at'], data['max_link_cap'], label='Maximum Path Capacity', color='darkred')
    ax2.fill_between(data['at'], data['min_link_cap'], data['max_link_cap'], color='red', alpha=0.2)
    ax2.tick_params(axis='y', labelcolor='tab:red')

    ax3 = ax1.twinx()
    ax3.spines['right'].set_position(('outward', 60))
    ax3.set_ylabel("Queue free", color='tab:green')
    line5, = ax3.plot(data['at'], data['queue_capacity'], label='Queue Capacity', color='tab:green')
    ax3.tick_params(axis='y', labelcolor='tab:green')
    ax3.set_ylim(0)

    ax4 = ax1.twinx()
    ax4.spines['right'].set_position(('outward', 120))
    ax4.set_ylabel("Drop Ratio", color='tab:purple')
    line6, = ax4.plot(data['at'], data['dropratio'], label='Drop Ratio', color='purple', linestyle='dashed')
    ax4.tick_params(axis='y', labelcolor='tab:purple')

    lines = [line1, line2, line3, line4, line5, line6]
    labels = [l.get_label() for l in lines]
    fig.legend(lines, labels, loc='lower center', ncol=6)

    plt.title(f"Trace File")
    plt.tight_layout()
    plt.subplots_adjust(bottom=0.15)
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file>")
        sys.exit(1)

    main(sys.argv[1])