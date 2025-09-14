# Kernix - Operating System Simulator

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](#-installation)
[![Platform](https://img.shields.io/badge/platform-Unix%2FLinux-lightgrey.svg)](#-requirements)
[![Version](https://img.shields.io/badge/version-2.0-orange.svg)](#-releases)

> **A comprehensive operating system simulator implementing core OS functionalities including process scheduling, memory management, and inter-process communication.**

## Features

### **Process Scheduling**
- **Multiple Scheduling Algorithms**:
  - **Shortest Remaining Time Next (SRTN)** - Preemptive shortest job first
  - **Round Robin (RR)** - Time-sliced fair scheduling  
  - **Non-preemptive Highest Priority First (HPF)** - Priority-based scheduling (0 = highest, 10 = lowest)
- Real-time process state tracking and visualization
- Comprehensive performance metrics and statistics

### **Memory Management (Buddy System)**
- **Total Memory**: 1024 bytes with buddy allocation algorithm
- **Process Size Limit**: Up to 256 bytes per process
- Tree-based memory allocation with efficient splitting and merging
- Automatic fragmentation reduction
- Dynamic memory allocation/deallocation simulation with detailed logging

### **Inter-Process Communication (IPC)**
- **Shared Memory**: High-speed data exchange between Process Generator and Scheduler
- **Message Queues**: Reliable, structured communication for process coordination
- **Clock Synchronization**: All components synchronized through shared clock process
- POSIX IPC mechanisms for safe concurrent access and resource cleanup

## Performance Metrics

The simulator provides detailed analytics including:
- **CPU Utilization** - Percentage of time CPU is actively executing processes
- **Average Weighted Turnaround Time (WTA)** - Time from arrival to completion weighted by runtime
- **Average Waiting Time** - Time processes spend waiting in ready queue  
- **Standard Deviation of WTA** - Measure of scheduling consistency
- **Memory Allocation Efficiency** - Fragmentation analysis and allocation success rates

## 🛠Installation

### Requirements
- Unix/Linux environment
- GCC compiler
- Make utility
- POSIX IPC support

### Quick Start
```bash
# Clone the repository
git clone https://github.com/cmp-2060-sp25/kernel-sim-kernix.git
cd kernel-sim-kernix

# Compile the project
make

# Run the simulator
./os-sim
```

### Manual Compilation
```bash
# Compile individual components
gcc -o process_generator process_generator.c
gcc -o scheduler scheduler.c
gcc -o process process.c
gcc -o clk clk.c

# Run the main simulator
./os-sim
```

## Usage

### Basic Execution
```bash
# Run the simulator (will prompt for scheduling algorithm)
./os-sim

# Available scheduling algorithms:
# 1. HPF (Non-preemptive Highest Priority First)
# 2. SRTN (Shortest Remaining Time Next) 
# 3. RR (Round Robin) - requires time quantum parameter
```

### Input Format
Create a `processes.txt` file with the following format:
```
# id arrival_time runtime priority memsize
1 1 6 5 200
2 3 3 3 170
4 4 8 2 256
```

**Field Descriptions:**
- `id`: Process identifier
- `arrival_time`: When process arrives (integer seconds)
- `runtime`: CPU burst time required 
- `priority`: Priority level (0 = highest, 10 = lowest)
- `memsize`: Memory required in bytes (≤ 256)

## Output Files

| File | Description |
|------|-------------|
| `scheduler.log` | Process state changes (started/stopped/resumed/finished) |
| `scheduler.perf` | CPU utilization, WTA, waiting time, and standard deviation |
| `memory.log` | Memory allocation/deallocation events with addresses |

### Sample Output
```
At time 1 process 1 started arr 1 total 6 remain 6 wait 0
At time 3 process 1 stopped arr 1 total 6 remain 4 wait 0  
At time 3 process 2 started arr 3 total 3 remain 3 wait 0
At time 6 process 2 finished arr 3 total 3 remain 0 wait 0 TA 3 WTA 1.00
At time 6 process 1 resumed arr 1 total 6 remain 4 wait 3
At time 10 process 1 finished arr 1 total 6 remain 0 wait 3 TA 9 WTA 1.50

=== PERFORMANCE STATISTICS ===
CPU utilization = 90%
Avg WTA = 1.25  
Avg Waiting = 1.5
Std WTA = 0.35
```

## Architecture

```
                    ┌─────────────────┐
                    │     Clock       │
                    │ (Time Sync)     │
                    └─────────┬───────┘
                              │
                    ┌─────────▼───────┐
                    │                 │
         ┌──────────▼──────────┐     │     ┌─────────────────┐
         │  Process Generator  │     │     │   Scheduler     │
         │  • Creates processes│◄────┼────►│ (HPF/SRTN/RR)   │
         │  • Manages memory   │     │     │ • Process mgmt  │
         │  • Sends to sched   │     │     │ • PCB tracking  │
         └──────────┬──────────┘     │     └─────────┬───────┘
                    │                │               │
                    ▼                │               ▼
         ┌─────────────────┐         │    ┌─────────────────┐
         │  Buddy System   │         │    │   Process 1     │
         │  (1024 bytes)   │         │    │   Process 2     │
         │  Memory Alloc   │         │    │   Process N     │
         └─────────────────┘         │    └─────────────────┘
                                     │
                    ┌────────────────┘
                    ▼
         ┌─────────────────┐
         │      IPC        │
         │ • Shared Memory │
         │ • Message Queue │
         └─────────────────┘
```

**System Flow:**
1. **Process Generator** reads input file and manages memory allocation via Buddy System
2. **Clock Process** provides time synchronization for all components  
3. **Scheduler** receives process information via IPC and manages scheduling
4. **Processes** execute and communicate completion back to scheduler
5. **Memory** is freed by Process Generator when processes terminate

## Testing

### Run Test Suite
```bash
# Compile test generator
gcc -o test_generator test_generator.c

# Generate test cases
./test_generator 100  # Generate 100 random processes
```

### Test Scenarios
- **Algorithm Comparison**: Compare HPF vs SRTN vs RR performance
- **Priority Testing**: Verify priority handling (0 = highest to 10 = lowest)
- **Memory Pressure**: Test buddy system with various allocation patterns
- **Concurrent Arrivals**: Handle multiple processes arriving simultaneously

## Documentation

- 📖 [Phase 1 Report](Project%20Phase%201.pdf) - Process Scheduling & IPC Implementation
- 📖 [Phase 2 Report](Project%20Phase%202.pdf) - Memory Management & Buddy System

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👥 Team Kernix Members

| Avatar                                                                        | Name                                                          
| ----------------------------------------------------------------------------- | ------------------------------------------------------------- | 
| <img src="https://github.com/OmarGamal10.png" width="50" height="50">         | [**Omar Gamal**](https://github.com/OmarGamal10)    | 
| <img src="https://github.com/AbdallahAyman03.png" width="50" height="50"> | [**Abdallah Ayman**](https://github.com/AbdallahAyman03)   | 
| <img src="https://github.com/MohamedAbdelaiem.png" width="50" height="50">    | [**Mohamed Abdelaziem**](https://github.com/MohamedAbdelaiem) | 
| <img src="https://github.com/OmarHassan2003.png" width="50" height="50">           | [**Omar Hassan**](https://github.com/OmarHassan2003)           |

[Report Bug](https://github.com/MohamedAbdelaiem/Kernix/issues) · [Request Feature](https://github.com/MohamedAbdelaiem/Kernix/issues)

</div>
