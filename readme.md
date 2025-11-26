# ArchOpt-VP: Architectural Optimizer - Visual Profiler

This project is a prototype tool to analyze and visualize the complex architectural power-performance of C-based Machine Learning algorithms.

## The Core Problem: The "AoS Anomaly"

The conventional wisdom in performance engineering is that "fewer cache misses = faster code."

Our research (based on the "Architectural Power-Performance Analysis for AI/ML Algorithms" report) found this is **not always true** for modern ML algorithms. We discovered a non-obvious "AoS Anomaly":

* For algorithms with high randomness (like a true Random Forest), a "naive" **Array-of-Structs (AoS)** data layout can be significantly **faster** and more **energy-efficient** than a cache-optimized **Struct-of-Arrays (SoA)** layout.
* This occurs *despite* the AoS layout having *more* L1/L2 cache misses.
* **Hypothesis:** This is due to a complex, non-obvious interaction between the algorithm's random memory access patterns and the CPU's hardware prefetcher. The prefetcher can predict the simple AoS strides, effectively hiding the latency of its misses, but fails to predict the chaotic access patterns of the "optimized" SoA layout.

This tool is being built to help developers, students, and researchers *find* and *visualize* this counter-intuitive trade-off, preventing them from making "obvious" optimizations that actually slow down their code.

## Project Status & Roadmap

This document tracks our progress.

* [x] **Sprint 1: Single-File Analyzer (Complete)**
    * Build the core pipeline to compile, run, and parse the output of a *single* C file.
    * Generate a basic bar chart of the results.

* [x] **Sprint 2: Comparative Engine (Complete)**
    * Upgrade the tool to profile an entire project folder (AoS, SoA, etc.).
    * Generate summary tables and *grouped bar charts* that visually compare all implementations side-by-side.

* [ ] **Sprint 3: Interactive UI Shell (Next)**
    * Wrap the Python tool in a simple Streamlit web interface.
    * Allow users to select a project and run the analysis from their browser.

### Folder Structure

ArchOpt-VP/
├── memory.h
├── benchmark_cmp.h
├── benchmark.h
├── power.h
├── .gitignore
├── README.md
│
├── src/
│   ├── run_sim.py 
│   ├── profile.py        #Comparative engine & table generator
│   ├── parser.py         # Parses the console output
│   ├── visualizer.py     # Generates charts with Matplotlib
│   └── main.py           # main file runner
│
└── projects/
    ├── DecisionTree/
    │   ├── AoS.c
    │   ├── Reordered.c
    │   └── SoA.c
    └── RandomForest/
        ├── AoS.c         # (Example input file)
        ├── SoA.c
        ├── Reordered.c
        └──Generated Charts...

### ## Usage Guide

This prototype requires Python 3, `gcc` (with math library support), and `matplotlib`.

### 1. Run Comparative Analysis (Sprint 2)

This is the main feature. It analyzes all `.c` files in a specific project folder and compares them.

**Command:**
```bash
    python src/profile.py projects/RandomForest
```

**Expected Console Output:** The tool will run simulations silently and present a clean summary table:

=== Profiling Project: RandomForest ===
Running simulations, please wait...
  > Processing AoS... [Done]
  > Processing Reordered... [Done]
  > Processing SoA... [Done]

=================================================================
Implementation  | CPU Cycles (M)  | Misses     | Energy    
-----------------------------------------------------------------
AoS             | 169.79          | 836        | 1.5       
Reordered       | 192.32          | 685        | 1.7       
SoA             | 172.93          | 670        | 1.5       
=================================================================

Generating charts in projects/RandomForest...
Done! Charts generated successfully.

### Generated Visuals:

The script saves three comparison charts in the project folder:

- RandomForest_cpu_cycles_m_comparison.png
- RandomForest_cache_misses_comparison.png
- RandomForest_energy_1e13_nj_comparison.png

These charts allow you to instantly see the "AoS Anomaly" (lower cycles despite higher misses).

### 2. Run Single-File Analysis (Sprint 1)  

If you only want to check one specific implementation:

 **Command:**

```bash
    python src/main.py projects/RandomForest/AoS.c
```
 **Output:**

* Parses metrics to console.
* Generates `AoS_performance.png`.

## Future Plans

### Sprint 3: Interactive UI

* **Goal:** Make the tool accessible to non-developers.
* **Action:** We will use `Streamlit` to build a simple web application (`app.py`).
* **Result:** A user will be able to:
    1.  Open the app in their browser.
    2.  Select a project (e.g., "RandomForest") from a dropdown menu.
    3.  Click a "Run Profile" button.
    4.  See the comparative charts and a table of the results directly on the web page.