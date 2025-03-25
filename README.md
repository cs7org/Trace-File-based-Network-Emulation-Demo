# From Simulation to Emulation: A System Design for Real-Time Replay of Simulated Network Path Characteristics

This repository contains the sources, scripts and result artifacts used during the work on the paper "From Simulation to Emulation: A System Design for Real-Time Replay of Simulated Network Path Characteristics".
All result files and scripts used for the figures in the paper are also included in this repo.

## Step-by-Step Guide
> Start here to reproduce _all_ results yourself.
0. **Installation**: Follow the installation instructions in `./simulation`, `./emulation`, `./testbed` and `./tools`.
1. **Generate Trace Files**: Run the commands described in both *Generate Trace Files* Sections in `./simulation/README.md` to create your own Trace Files. It is possible to change the simulators settings, see `./simulation/ns-3/scratch/trace.cc`.
2. **Plot Trace Files**: Use `./tools/plot-trace{-paper}.py` to visualize the generated Trace Files.

> Start here to skip the trace file generation. The initial installation is still required.
3. **Simulation Setup**: Run the commands in both *Run Pure Simulation* Sections in `./simulation/README.md` to export results from the pure simulation setup.
4. **Emulation Setup:**: Run all commands in the *Run the Experiments* Section in `./emulation/README.md` to export results from the emulation environment using previously generated Trace Files (or, if step 2 was skipped: The Trace Files used in the paper are supplied in this repo).
5. **Testbed Setup**: Run all commands in the *Run the Experiments* Section in `./testbed/README.md` to export results from the real testbed.

> Start here to just plot pre-generated results. Only the *tools* dependencies must be installed.
6. **Visualize Results**: See `./tools/README.md` for further details. The results used to obtain the figures in the paper are available in `results` (using git LFS).
7. **Optional: Reproducibility Tests**: See `./reproduction/README.md` for further details.

## License
This project is licensed under the [GNU General Public License v3.0](LICENSE). For more details, see the `LICENSE` file or see https://www.gnu.org/licenses/.
