# Plotting Tools
## Installation
Use the `requirements.txt` file to install the required dependencies, e.g., using a `venv`.

## Scripts
- **compare-cwnd.py**: Compare the TCP congestion windows reported by the socket and recorded during the TCP tests.
- **compare-delay.py**: Compare the end-2-end Layer 5/7 delay (timediff between packet send via sender socket and reception in receiver socket) *(not used in paper)*
- **compare-goodput.py**: Compare the goodput of the TCP-Test. Script can be modified to test the goodput in the TCP crosstraffic test case (see comments in script)
- **compare-rtt.py**: Compare the TCP socket RTT reported by the socket statistics and recorded during the TCP tests.
- **plot-ping.py**: Plot the results of the ICMP ping tests *(not used in paper)*
- **plot-speedtest.py**: Visualize the goodput of a single TCP test result file *(not used in paper)*
- **plot-trace-paper.py**: Plot selected details from a Trace File
- **plot-trace.py**: Plot more details from a Trace File *(not used in paper)*

