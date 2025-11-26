# ArchOpt-VP: Architectural Optimizer - Visual Profiler

**ArchOpt-VP** is a research-grade tool designed to analyze, visualize, and optimize the architectural power-performance interactions of C-based Machine Learning algorithms.

It serves as both a **diagnostic profiler** (detecting hardware anomalies) and a **refactoring assistant** (automating Data-Oriented Design).

---

## 🧠 Core Research: The Three Architectural Profiles

Conventional wisdom suggests that "fewer cache misses = faster code." Our research proves this is **not always true** for modern ML workloads.

This tool diagnoses three distinct performance behaviors:

### 1. Standard Cache Optimization (e.g., Decision Tree)
* **Behavior:** The **SoA (Struct-of-Arrays)** layout is significantly faster.
* **Reason:** SoA improves spatial locality, directly reducing L1/L2 cache misses.
* **Verdict:** ✅ **Use SoA.**

### 2. The "AoS Anomaly" (e.g., Random Forest)
* **Behavior:** The naive **AoS (Array-of-Structs)** layout is **faster**, despite having *more* cache misses.
* **Reason:** Hardware Prefetcher Interaction. The CPU prefetcher can predict the linear strides of AoS but fails on the chaotic access patterns of SoA in stochastic algorithms.
* **Verdict:** ⚠️ **Stick with AoS.**

### 3. Memory Traffic Optimization (e.g., KNN)
* **Behavior:** SoA is faster, even though cache misses are **identical** to AoS.
* **Reason:** Bandwidth Reduction. SoA reduces the total *volume* of data transferred from main memory, optimizing bus traffic rather than latency.
* **Verdict:** ✅ **Use SoA.**

---

## 🚀 Features

### 🔍 1. Comparative Profiling Engine
* **Automated Simulation:** Compiles and runs C benchmarks using a hardware simulator (`memory.h`).
* **Diagnostic Engine:** Automatically classifies performance results into one of the three research profiles above.
* **Visualization Matrix:** Generates side-by-side charts for **CPU Cycles**, **Cache Misses**, and **Energy Consumption**.
* **Sci-Fi Dashboard:** A modern, dark-mode UI for clear data visualization.

### 💻 2. Refactoring Playground
* **Automated Translation:** A Regex-based engine that converts legacy Object-Oriented C structs (AoS) into Data-Oriented SoA layouts.
* **Memory Allocator Generation:** Automatically writes the C boilerplate code for allocating and freeing SoA structures.

---

## 📂 Folder Structure

```text
ArchOpt-VP/
├── memory.h              # Hardware Simulator Header
├── src/
│   ├── app.py            # Main Streamlit Web Application
│   ├── profile.py        # Comparative Logic Engine
│   ├── run_sim.py        # Subprocess Simulation Runner
│   ├── parser.py         # Regex Output Parser
│   └── visualizer.py     # Matplotlib Chart Generator
│
└── projects/
    ├── DecisionTree/     # [Profile 1] Deterministic Access
    ├── RandomForest/     # [Profile 2] Stochastic Access (AoS Anomaly)
    └── KNN/              # [Profile 3] Traffic Optimization
```
## Usage Guide

This prototype requires Python 3, `gcc` (with math library support), and `matplotlib`.

### 1. Run Comparative Analysis (Sprint 2)

This is the main feature. It analyzes all `.c` files in a specific project folder and compares them.

**Command:**
```bash
python src/profile.py projects/RandomForest
```
**Expected Console Output:**
The tool will run simulations silently and present a clean summary table:

```text
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
```

## Future Plans

### Sprint 4: Advanced Optimization (Future Work)

* **Hardware Prefetcher Modeling:** Investigate the exact behavior of the hardware prefetcher that causes the "AoS Anomaly" in Random Forests.
* **Broader Algorithm Support:** Extend the tool to support other memory-intensive algorithms like Support Vector Machines (SVMs) and Neural Networks.
* **Cache Parameter Tuning:** Allow users to configure cache size, block size, and associativity within the simulation to find the optimal hardware configuration for their specific algorithm.
* **Alternative Architectures:** Explore the impact of data layouts on emerging architectures like Processing-In-Memory (PIM).
