# XinuOS-Deadlocks

## Introduction
This repository contains my implementation for Project 3 of the ECE465/565 Operating Systems Design course. The project focuses on understanding and developing different synchronization mechanisms in Xinu, including spinlocks, locks with sleep mechanisms, deadlock detection, and addressing priority inversion for graduate-level students.

## Project Objectives
- **Assembly Implementation**: Create a `test_and_set` function in assembly to support lock mechanisms.
- **Spinlocks**: Implement spinlocks and related functions for lock initialization, acquisition, and release.
- **Sleep-Based Locks**: Design locks that minimize busy-waiting by putting processes to sleep when appropriate.
- **Deadlock Detection**: Enhance locks to detect and report deadlocks involving circular dependencies.
- **Priority Inversion Prevention (Graduate Level)**: Implement the Priority Inheritance Protocol to prevent priority inversion issues.
- **Testing**: Develop test cases to validate the implementations and explore deadlock prevention strategies.

## Key Implementation Details
### 1. Assembly: `test_and_set` Function
- Implemented using the `XCHG` x86 instruction in AT&T syntax to ensure atomicity.
- Code carefully commented to explain each line and maintain clarity.

### 2. Spinlock (`system/spinlock.c`)
- A basic lock that uses `test_and_set` for locking and unlocking mechanisms.
- Includes safety checks for process ownership during unlocks.

### 3. Sleep-Based Lock (`system/lock.c`)
- Reduces CPU usage by putting processes to sleep while waiting for the lock.
- Implemented `park`, `unpark`, and `setpark` primitives with appropriate interrupt management.

### 4. Deadlock Detection (`system/active_lock.c`)
- Integrated circular dependency detection to identify and report deadlocks.
- Outputs a list of processes involved in the deadlock when detected.

### 5. Priority Inversion Prevention (`system/pi_lock.c`)
- Implemented for graduate students as a mechanism to handle priority inversion.
- Includes real-time tracking and adjustment of process priorities, with output logs showing priority changes.

## Test Cases
The `main-deadlock.c` file contains test cases to:
- Trigger and detect deadlocks involving multiple processes.
- Demonstrate deadlock prevention techniques:
  - Avoiding hold-and-wait.
  - Allowing preemption of a lock-holding thread.
  - Preventing circular wait.
- Validate priority changes in cases of priority inversion (graduate level).

## Challenges and Learnings
- Understanding the intricacies of Xinu and working within its limitations.
- Implementing low-level synchronization primitives in assembly for a deeper grasp of hardware interactions.
- Handling the edge cases in deadlock detection and ensuring efficient lock operations under various conditions.
- Managing priority changes dynamically for priority inversion prevention without affecting system stability.

## How to Use This Repository
1. Clone the repository and navigate to the `xinu/compile` directory.
2. Run `make clean` before compiling.
3. Test the implementation using provided or custom test cases to ensure expected behavior.
4. Modify the `main-deadlock.c` or other test files to explore different synchronization scenarios.

## Future Improvements
- Further optimize the deadlock detection algorithm to reduce overhead.
- Extend test cases to cover more complex multi-threaded scenarios.
- Integrate additional synchronization techniques as an extension to the project.

---


