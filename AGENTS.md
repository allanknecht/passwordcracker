## Cursor Cloud specific instructions

This is a self-contained C++ project (OpenMP brute-force password cracker benchmark). No package manager, no external services, no databases.

### System requirements

- `g++` with C++17 support (pre-installed)
- `libgomp` / OpenMP runtime (pre-installed)

### Build

Compile all three binaries from the workspace root (see `README.md` for full commands):

```bash
g++ -std=c++17 -O2 -w sequential_cracker.cpp -o sequential_cracker
g++ -std=c++17 -O2 -fopenmp -w parallel_cracker.cpp -o parallel_cracker
g++ -std=c++17 -O2 -fopenmp -w benchmark.cpp -o benchmark
```

### Run

- `./sequential_cracker` — sequential brute-force (random passwords)
- `./parallel_cracker [num_threads]` — parallel brute-force (random passwords, default: max threads)
- `./benchmark` — deterministic benchmark comparing sequential vs parallel at 1/2/4/8/16/24 threads

### Notes

- There is no lint or test framework; correctness is validated by running the programs.
- The `benchmark` binary uses fixed passwords (max 5 chars) so it completes in a few seconds. The standalone crackers use random passwords up to 6 chars and can take longer.
- Binaries and CSV result files (`*.csv`) are build artifacts; do not commit them.
