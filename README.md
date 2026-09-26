# Multithreaded-Producer-Consumer-Simulator
A desktop GUI application that simulates automated semiconductor equipment
processing jobs concurrently. Demonstrating
multithreaded C++ software design with a live Qt Widgets dashboard.

![Status](https://img.shields.io/badge/status-working-brightgreen)
![Language](https://img.shields.io/badge/language-C%2B%2B17-blue)
![Framework](https://img.shields.io/badge/framework-Qt%205-41cd52)

---

## Overview

The simulator models a small fab floor:

- A **job generator** (producer) creates wafer-lot jobs at intervals and
  pushes them onto a shared queue.
- Several **equipment workers** (consumers), each running on its own
  thread, pull jobs from the queue and process them independently and
  concurrently.
- Each piece of equipment moves through a small state machine —
  `IDLE → RUNNING → (occasionally ALARM) → IDLE`, and finally `DOWN` once
  there's no more work — and the GUI reflects that state live with a
  colored indicator per worker, plus a scrolling, timestamped event log.
  
## Features

- Configurable number of equipment workers (1–10) and jobs (1–100)
- Live color-coded status per worker: 🟡 Idle · 🟢 Running · 🔴 Alarm · ⚫ Down
- Scrolling, timestamped activity log
- Fully responsive UI — the simulation runs on background threads, so the
  window never freezes while a run is in progress
- Simulated random equipment faults (~5% chance per job) with automatic
  recovery, to make the state transitions worth watching

## How it works (architecture)

| Concept | Where | Why it matters |
|---|---|---|
| Producer–consumer pattern | `MainWindow::onStartClicked` (producer) + `EquipmentWorker::run` (consumers) | Core concurrency pattern behind most equipment/job-processing software |
| Thread-safe queue | `ThreadSafeQueue<Job>` (`include/ThreadSafeQueue.h`) | Generic blocking queue built on `std::mutex` + `std::condition_variable`; no busy-waiting |
| Signals across threads | `EquipmentWorker` (`QObject` with `stateChanged`/`logMessage` signals) | Workers run on plain `std::thread`, not `QThread` — Qt automatically delivers their signals to the GUI thread as **queued connections** since the emitting thread differs from the receiver's (GUI) thread, so no manual locking is needed on the UI side |
| Non-blocking orchestration | One background thread runs the producer loop, then joins all workers, then calls back via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` | Keeps the GUI thread free for the entire run instead of blocking on `.join()` |
| Graceful shutdown | `ThreadSafeQueue::shutdown()` | Wakes every worker blocked in `pop()` so they drain remaining jobs and exit cleanly instead of hanging |

## Project layout

```
Multithreaded-Producer-Consumer-Simulator/
├── equipment-sim-qt.pro      # qmake project file — open this in Qt Creator
├── README.md
├── include/
│   ├── Job.h                 # data model for one unit of work
│   ├── ThreadSafeQueue.h     # generic blocking thread-safe queue
│   ├── EquipmentWorker.h     # consumer thread logic + state machine (QObject)
│   └── MainWindow.h          # GUI + simulation orchestration
└── src/
    ├── main.cpp              # QApplication entry point
    ├── MainWindow.cpp
    └── EquipmentWorker.cpp
```

## Requirements

- Qt 5 (Widgets module) — Qt Creator is the easiest way to get everything
  needed in one install
- A C++17 compiler (MinGW on Windows, GCC/Clang on Linux/Mac)


## Build & run
 
### 1. Clone the repo
 
```bash
git clone https://github.com/sakiiif/Multithreaded-Producer-Consumer-Simulator.git
cd Multithreaded-Producer-Consumer-Simulator
```

### Option A — Qt Creator (recommended)

1. Open Qt Creator.
2. **File → Open File or Project...** → select `equipment-sim-qt.pro`.
   (inside the cloned `Multithreaded-Producer-Consumer-Simulator` folder).
4. Choose a Kit (e.g. *Desktop Qt 5.15.x MinGW 64-bit*).
5. Click the green **Run** button (or `Ctrl+R`).

### Option B — command line (qmake + make)

Run these from inside the cloned `Multithreaded-Producer-Consumer-Simulator`
folder:

```bash
qmake equipment-sim-qt.pro
make            # on Windows with MinGW: mingw32-make
./equipment_sim_qt      # Windows: equipment_sim_qt.exe
```

## Usage

1. Set **Workers** (number of equipment units) and **Jobs** (how many
   jobs to process).
2. Click **Start Simulation**.
3. Watch each equipment row change color as it works through jobs, and
   read the play-by-play in the log panel below.
4. Controls re-enable automatically once every job has been processed.
