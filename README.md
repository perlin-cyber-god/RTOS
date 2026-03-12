# RTOS (Real-Time Operating System)

This repository explores the fundamentals and implementation of Real-Time Operating Systems.

## GPOS vs. RTOS: Key Differences

### General Purpose Operating System (GPOS)
**Examples:** Windows, Linux, macOS.
- **Focus:** High throughput and fairness. A GPOS aims to execute as many tasks as possible over time, ensuring every process gets a "fair" share of CPU resources.
- **Latency:** Non-deterministic. Interrupt latency and scheduling delays can vary significantly depending on the system load.
- **Scheduling Complexity:** Typically uses complex algorithms (like CFS in Linux) where scheduling overhead often increases with the number of tasks ($O(\log n)$ or similar). As CPU load increases, the time taken to decide "who runs next" follows an increasing curve.

### Real-Time Operating System (RTOS)
**Examples:** FreeRTOS, QNX, VxWorks.
- **Focus:** Determinism and predictability. In an RTOS, "logical correctness is based on both the correctness of the outputs and their timeliness."
- **Latency:** Deterministic. It guarantees that a high-priority task will be serviced within a strictly defined time limit (jitter is minimized).
- **Scheduling Complexity:** Designed for $O(1)$ (constant time) scheduling. Whether there are 10 tasks or 1000, the time it takes for the scheduler to switch to the highest priority task remains constant.

## Task Scheduling and Latency

| Feature | GPOS | RTOS |
|---------|------|------|
| **Primary Goal** | Maximize Throughput | Minimize Response Time |
| **Scheduling** | Fairness-based (Time-slicing) | Priority-based (Preemptive) |
| **Scaling** | Overhead increases with load | Constant scheduling time ($O(1)$) |
| **Interrupts** | Can be masked for long periods | Low and predictable latency |

### How RTOS Achieves Determinism
1. **Preemptive Kernel:** The RTOS kernel can be interrupted at almost any point. If a higher-priority task becomes ready, the current task is immediately suspended.
2. **Fixed-Priority Scheduling:** Tasks are assigned a priority, and the scheduler always runs the highest-priority "Ready" task.
3. **Efficient Data Structures:** Uses bitmasks and priority-grouped ready lists to identify the highest priority task in a single CPU instruction or a few cycles, regardless of the total task count.

---

## Priority Inversion

Priority inversion is a problematic scenario in real-time systems where a high-priority task is indirectly preempted by a lower-priority task, effectively "inverting" their assigned priorities.

### The Scenario:
1. **Low Priority (L)** holds a shared resource (e.g., a Mutex).
2. **High Priority (H)** wants the resource and enters a "Blocked" state waiting for **L**.
3. **Medium Priority (M)** becomes ready and preempts **L** (since $M > L$).
4. **H** is now waiting for **L**, but **L** cannot run because **M** is hogging the CPU.
   - Result: **M** is running while **H** waits, even though **H** has higher priority.

#### Concrete Example: I2C Bus Writing
Imagine a system where multiple tasks share an **I2C bus**, which can only handle one electrical signal/transaction at a time.
1. **Task Low** starts writing a large data packet to the I2C bus and locks the I2C mutex.
2. **Task High** (e.g., a critical sensor task) wakes up and needs to read from the I2C bus. It finds the bus locked and enters a **Blocked** state, waiting for Task Low to finish.
3. **Task Medium** (e.g., a UI refresh task) becomes ready. Since it has a higher priority than Task Low, the RTOS preempts Task Low so Task Medium can use the CPU.
4. **The Inversion:** Task Medium is now using the CPU for UI updates, preventing Task Low from finishing its I2C write. Consequently, Task High remains blocked. Task Medium has effectively bypassed the priority hierarchy, gaining CPU time over Task High, which is stuck waiting for a resource held by a preempted task.

### Solutions:
- **Priority Inheritance Protocol (PIP):** When **H** blocks on a resource held by **L**, **L**'s priority is temporarily boosted to match **H**. This prevents **M** from preempting **L**, allowing **L** to finish and release the resource quickly.
- **Priority Ceiling Protocol (PCP):** Each resource is assigned a priority ceiling (the highest priority of any task that may lock it).
