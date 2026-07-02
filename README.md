# DAG Task Scheduler

A high-performance, build-system-style task scheduler written in **C++17**.  
Resolves and executes complex multi-stage job dependencies using a Directed Acyclic Graph (DAG).

---

## Features

| Feature | Detail |
|---|---|
| **Topological sort** | Kahn's Algorithm — O(V + E) |
| **Cycle detection** | DFS-based; reports the exact cycle path |
| **Parallel wave grouping** | Tasks at the same dependency level run concurrently |
| **Bottleneck identification** | Waves with a single task are flagged |
| **Critical path** | Longest weighted path through the DAG |
| **Priority scheduling** | Tasks with higher priority run earlier on tie |
| **Persistence** | Save/load graphs to plain-text `.dag` files |
| **Interactive CLI** | Full REPL with colour output and 4 built-in demos |

---

## Prerequisites

- **C++ Compiler**: GCC (MinGW) 6.3+ with `-std=c++17` support
  - Verify: `g++ --version`
- **No external libraries required** — the project is fully self-contained

---

## Build (Windows — PowerShell)

### Compile the main scheduler

```powershell
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/scheduler.cpp src/cli.cpp src/main.cpp -o build/scheduler.exe
```

### Compile the test suite

```powershell
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude tests/test_main.cpp src/scheduler.cpp src/cli.cpp -o build/tests.exe
```

> **Note:** Run all commands from the `task_scheduler/task_scheduler/` directory.

---

## Run

### Start the interactive CLI

```powershell
.\build\scheduler.exe
```

### Run a demo non-interactively

```powershell
.\build\scheduler.exe --demo build --exit
.\build\scheduler.exe --demo basic --exit
.\build\scheduler.exe --demo complex --exit
.\build\scheduler.exe --demo cycle --exit
```

### Run the test suite (13 tests)

```powershell
.\build\tests.exe
```

Expected output:
```
  ✔  add_task
  ✔  add_dependency
  ✔  linear_schedule
  ✔  parallel_tasks
  ✔  cycle_detection
  ✔  bottleneck_detection
  ✔  critical_path
  ✔  remove_task
  ✔  save_load
  ✔  demo_build_schedule
  ✔  demo_complex_schedule
  ✔  demo_cycle_fails
  ✔  priority_ordering

  Results: 13 / 13 passed
```

---

## Quick Start

```
$ .\build\scheduler.exe

  scheduler> demo build      # load the C++ build-pipeline demo
  scheduler> schedule        # run Kahn's algorithm & display results
  scheduler> graph           # print adjacency list
  scheduler> exit
```

---

## Interactive Commands

| Command | Description |
|---|---|
| `add <id> [name] [dur_ms] [priority]` | Add a task |
| `dep <from> <to>` | Add dependency edge (from must finish before to) |
| `remove <id>` | Remove a task and all its edges |
| `schedule` | Run the scheduler and display full output |
| `graph` | Print adjacency list |
| `demo basic` | Simple 4-task chain |
| `demo complex` | 10-task multi-branch DAG |
| `demo cycle` | 3-task cycle (demonstrates error handling) |
| `demo build` | Realistic C++ build-system pipeline |
| `save <file>` | Persist graph to file |
| `load <file>` | Load graph from file |
| `clear` | Reset graph |
| `help` | Show help |
| `exit` | Quit |

---

## Architecture

```
task_scheduler/
├── include/
│   ├── scheduler.h   # TaskScheduler class, Task & ScheduleResult structs
│   ├── cli.h         # Terminal output helpers (ANSI colour codes)
│   └── demos.h       # Built-in demo graphs (basic, complex, cycle, build)
├── src/
│   ├── scheduler.cpp # Kahn's algorithm, cycle detection, critical path, I/O
│   ├── cli.cpp       # ANSI-coloured CLI renderer
│   └── main.cpp      # Interactive REPL
├── tests/
│   └── test_main.cpp # 13 self-contained unit tests
├── build/            # Compiled binaries (scheduler.exe, tests.exe)
├── CMakeLists.txt    # CMake build config (alternative)
├── Makefile          # GNU Make build config (Linux/macOS)
└── README.md
```

### Algorithm Summary

1. **Cycle detection** — DFS with WHITE/GRAY/BLACK colouring; O(V + E)
2. **Wave-level BFS** — assigns each node a level (= earliest wave it can start); O(V + E)
3. **Kahn's sort** — priority queue seeded with zero-in-degree nodes; processes edges once; O((V + E) log V)
4. **Critical path** — DP relaxation in topological order; O(V + E)

---

## License
MIT
