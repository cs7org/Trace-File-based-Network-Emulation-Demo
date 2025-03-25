# Reproduction
Scripts to automatically run repeated tests to check the reproducibility in all setups. Runs for multiple hours.

Preconditions: Follow the installation instructions of `../simulation`, `../emulation` and `../testbed`.

```bash
cd simulation && ./run.sh
cd ../emulation && ./run.sh
cd ../testbed && ./run.sh
cd ..
python3 analyze.py # Show the plot
```
