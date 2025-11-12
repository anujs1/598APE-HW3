# 598APE-HW3

## INSTRUCTIONS ON RUNNING OPTIMIZED CODE:
(Do this once) Build a container from the image from the project root:
```bash
docker build -t anujs1/598ape docker/
```

Select an optimization from a given branch (ex: `dev-optimization-1`), or run our final version with all optimizations combined from the main branch:
```bash
git checkout dev-optimization-1
```

Run the container using the script:
```bash
./dockerrun.sh
```

Make and test the executable (replace with your desired test/command):
```bash
make clean && make -j && ./main.exe 64 2.269 100000
```

Or, use `benchmark.sh` to test all configurations (in the main branch):
```bash
make clean && make -j && ./benchmark.sh
```

---

This repository contains code for homework 3 of 598APE.

This assignment is relatively simple in comparison to HW1 and HW2 to ensure you have enough time to work on the course project.

In particular, this repository implements a 2D Ising model Monte Carlo simulator (with Metropolis–Hastings algorithm) on an L×L lattice with periodic boundary conditions.

To compile the program run:
```bash
make -j
```

To clean existing build artifacts run:
```bash
make clean
```

This program assumes the following are installed on your machine:
* A working C compiler (gcc is assumed in the Makefile)
* make
* ImageMagick `convert` for PNG output

Usage (after compiling):
```bash
./main.exe <L> <temperature> <steps>
```

In particular, consider speeding up simple run like the following (which runs ~5 seconds on my local laptop under the default setup):
```bash
./main.exe 100 2.269 100000
```

Exact bitwise reproducibility is not required; sanity checks on energy/magnetization bounds must pass. In addition, at the critical temperature (T ≈ 2.269) the energy per spin should approach -1.414 in equilibrium.
