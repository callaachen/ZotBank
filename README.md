
# ZotBank – Banker's Algorithm Simulator
**Calla Chen**  
EECS 111 – System Software – Spring 2025

---

## Description

**ZotBank** is an interactive, modular C++98 simulator for the **Banker's Algorithm**, supporting robust safe-state checks, logging, rollback, and detailed analysis tools. Designed for 5 customers and 4 resource types, the system guards against deadlocks and tracks system state over time.

It features:
- Live safe sequence verification and denial reasoning
- Logging to terminal, per-customer files, and CSV
- Undo/snapshot, state diff, and auto-savepoints before unsafe/deadlock
- Command history, aliases, verbosity toggling, and help system
- Preview of potential requests without altering state
- Deadlock explanation with affected processes and resources
- Python visualization of session metrics (bonus)

---

## Development & Testing Environment

- Primary development done on **CLion (Windows & macOS)** with C++98 toolchain
- Verified compatibility and correctness on:
    - **EECS Linux servers**: `laguna.eecs.uci.edu`
    - **Local Ubuntu terminal** (Ubuntu 24.04)
- Project compiled and tested with GNU Make and g++
---

## File Structure

```
project3/
├── src/
│   ├── banker.cpp / .h
│   ├── command_handler.cpp / .h
│   ├── logger.cpp / .h
│   ├── log_global.cpp / .h
│   ├── validator.cpp / .h
│   ├── utility.cpp / .h
│   └── main.cpp
├── tests/                # 10+ test cases (safe, unsafe, edge cases)
├── logs/
│   ├── full_session.txt
│   ├── customer_P*.txt
│   ├── history.txt
│   ├── save.txt
│   ├── deadlock_log.csv
│   ├── per_customer_log.csv
│   └── report.csv
├── zotbank               # Compiled binary
├── maximum.txt           # Max demand input file (custom - will explain in the report)
├── Makefile
├── analysis.py
└── README.md
```

---

## How to Build and Run

```bash
make clean && make
./zotbank maximum.txt 10 5 7 8
```

---

## Supported Commands

```
  RQ <cust> r0 r1 r2 r3       - Request resources
  RL <cust> r0 r1 r2 r3       - Release resources
  *                           - Display matrices (available, max, alloc, need)
  safety                      - Toggle safe sequence output
  preview <cust> r0..r3       - Preview request without committing
  snapshot                    - Save a snapshot
  undo                        - Restore last snapshot
  save                        - Save system to logs/save.txt
  load                        - Load from logs/save.txt
  summary                     - Print command usage breakdown
  explain                     - Show reason for last denial
  report                      - Save usage summary to CSV
  history                     - Show past commands
  !N                          - Replay history command N
  diff <savepoint>            - Show differences vs savepoint
  compare <savepoint>         - Compare current to savepoint
  test <file>                 - Run command script
  help [cmd]                  - Show help (or help <cmd>)
  verbose [on/off]            - Toggle detailed logging
  color [on/off]              - Toggle ANSI terminal coloring
  exit                        - End session and print summary
```

---

## Bonus Features Implemented

| #  | Feature                                 | Description |
|----|-----------------------------------------|-------------|
| 1  | History Recall (`!N`)                   | Replays past commands |
| 2  | Per-Customer Logs                       | Detailed logs per customer |
| 3  | Snapshots / Undo                        | Manual save and rollback |
| 4  | Deadlock Detection & Explanation        | Blocked customers and missing resources |
| 5  | Auto Savepoints on Unsafe/Deadlock      | Saves state as `auto_P3_deadlock` |
| 6  | Compare / Diff Savepoints               | Track state differences |
| 7  | Preview Mode                            | Shows if a request would succeed |
| 8  | Verbose and Color Output Modes          | Toggle live output formatting |
| 9  | Command Aliases                         | Short forms like `req`, `rel`, etc. |
| 10 | History Persistence                     | Command history saved |
| 11 | Command Usage Summary (`summary`)       | Tracks command usage count |
| 12 | Python Visualizer (`analysis.py`)       | Plots resource usage |
| 13 | Session Duration & Timestamps           | Included in all logs |
| 14 | Input Validation & Denial Categorization| Separates reasons: unsafe, availability, need |
| 15 | Save / Load Support                     | Persist and restore system state |

---

## Log Files

- `logs/full_session.txt` – Complete log
- `logs/customer_PX.txt` – Logs for each customer
- `logs/save.txt` – Saved state for `load`
- `logs/per_customer_log.csv` – Metrics per customer
- `logs/report.csv` – Session resource usage for plotting
- `logs/deadlock_log.csv` – Records of deadlock events
- `logs/history.txt` – Persistent command history

---

## Sample Test

```bash
./zotbank maximum.txt 10 5 7 8
> RQ 0 3 2 1 1
> snapshot
> RL 0 1 1 0 0
> undo
> preview 1 2 2 0 0
> compare alpha
> save
> load
> summary
```

---

## Python Analysis (Bonus)

```bash
pip3 install matplotlib pandas
python3 analysis.py
```
This plots graphs from `logs/report.csv`.

---

## Execution Behavior

- `explain` provides cause for last denial: unsafe, over-need, or unavailable.
- `undo` only works after a `snapshot`.
- `diff` and `compare` require existing named savepoints.
- All timestamps and logs are updated live.
---

## Notes for the Grader

- All core requirements of Part 2 are fully implemented, including interactive mode, logging, and test file support.
- All extra credit features listed in the bonus table above are completed and documented.
- The system automatically creates a savepoint before unsafe or deadlock-causing requests, labeled as `auto_P3_deadlock`.
- Deadlock logs are written to `logs/deadlock_log.csv`, including the blocked processes and missing resources.
- The `logs/` directory contains clean logs from recent runs, including `per_customer_log.csv`, `report.csv`, and `history.txt`.
- Test coverage includes 10+ `.txt` scripts in the `tests/` folder, verified using manual and scripted batch execution.
- Python analysis script (`analysis.py`) generates visualizations and summaries from CSV logs.
- All logs and outputs have been verified for consistency between the EECS server (`laguna.eecs.uci.edu`), Ubuntu, macOS and Windows environments.
- You may test manually by typing in commands in the terminal or do the auto test using the `test` command (e.g., `test tests/test_basic.txt`)
- Please refer to `README.md` or type `help` during execution for usage instructions and command reference.
---

## Last Updated

June 5, 2025 — Final version with all extra credit features implemented and documented.

---