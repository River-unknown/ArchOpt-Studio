# ArchOpt-VP: Architectural Optimizer - Visual Profiler

This project is a prototype tool to analyze the architectural power and performance of C-based Machine Learning algorithms. It is based on research that uncovered a "performance anomaly" where naive **Array-of-Structs (AoS)** layouts can outperform cache-optimized **Struct-of-Arrays (SoA)** layouts for algorithms with high randomness (like Random Forest), likely due to hardware prefetcher interactions.

This tool compiles and runs C code against a hardware simulator (`memory.h`) to extract key performance metrics and visualize them.

## Sprint 1: Single-File Analyzer

The goal of Sprint 1 is to create a core pipeline that can:
1.  Compile and run a single `.c` file.
2.  Parse the `memory.h` simulator output from the console.
3.  Generate and save a bar chart of the key performance metrics.

### Folder Structure

ArchOpt-VP/\
├── memory.h\
├── benchmark_cmp.h\
├── benchmark.h\
├── power.h\
├── .gitignore\
├── README.md\
│\
├── src/\
│   ├── run_sim.py        # Compiles & runs the C code \
│   ├── parser.py        # Parses the console output \
│   ├── visualizer.py     # Generates charts with Matplotlib \
│   └── main.py          # (Sprint 1) Runs the pipeline for one file \
│\
└── projects/\
    ├── DecisionTree/\
    │   ├── AoS.c\
    │   ├── Reordered.c\
    │   └── SoA.c\
    └── RandomForest/\
        ├── AoS.c      # (Example input file) \
        ├── SoA.c\
        ├── Reordered.c\
        └── AoS_performance.png # (Example output chart) 

### How to Run (Sprint 1)

This prototype requires Python 3 and `gcc`.

1.  **Install dependencies:**
    ```bash
    pip install matplotlib
    ```

2.  **Ensure your C code is ready:**
    * Place your algorithm's `.c` file (e.g., `AoS.c`) inside a `projects/` subfolder.
    * Ensure `memory.h` and any other required headers (like `benchmark_cmp.h`) are in the root directory.

3.  **Run the analysis:**
    From the root `ArchOpt-VP/` directory, run `main.py` and point it to the C file you want to analyze.

    ```bash
    python src/main.py projects/RandomForest/AoS.c
    ```

### Expected Output

The script will compile and run the C file, then print the parsed metrics to the console.

--- Compiling projects/RandomForest/AoS.c --- --- Running AoS.exe --- --- Simulation Complete --- --- Parsed Metrics --- {'cpu_cycles_m': 191.34, 'cache_misses': 866, 'energy_1e13_nj': 1.7} --- Chart saved to projects/RandomForest/AoS_performance.png ---

Sprint 1 Pipeline Complete!


A new image file (`AoS_performance.png`) will be saved in the `projects/RandomForest/` folder, showing a bar chart of these metrics.
