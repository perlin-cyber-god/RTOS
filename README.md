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
1. **Priority-Based Preemptive Scheduling:** The scheduler ensures the highest-priority "Ready" task always owns the CPU.
2. **Minimized Critical Sections:** RTOS kernels are designed with no or very short critical sections where preemption is disabled. This ensures that a high-priority task is never "locked out" of the CPU by a lower-priority task running in kernel space for too long.
3. **Bounded Interrupt Latency:** The maximum time between an interrupt trigger and the start of the Interrupt Service Routine (ISR) is strictly defined and guaranteed.
4. **Bounded Scheduling Latency:** The time taken to switch context from one task to another (after an event or interrupt) is constant ($O(1)$) and has a guaranteed upper bound.
5. **Priority Inversion Avoidance:** Built-in mechanisms like Priority Inheritance or Priority Ceiling protocols prevent lower-priority tasks from indirectly blocking higher-priority ones.

## Task States in an RTOS

In an RTOS, a task is always in one of the following four states:

1.  **Running:** The task is currently being executed by the CPU. In a single-core system, only one task can be in the Running state at any given time.
2.  **Ready:** The task is prepared to run and is waiting for the scheduler to allocate CPU time. It has all the resources it needs but is not the highest-priority "Ready" task at the moment.
3.  **Blocked:** The task is waiting for a specific event to occur before it can continue. This could be waiting for a mutex to be released, a semaphore to be signaled, a message to arrive in a queue, or a specific time delay to expire. A blocked task does not consume any CPU time.
4.  **Suspended:** The task has been explicitly moved out of the scheduling loop, often by another task or the kernel itself (e.g., using a `vTaskSuspend()` call). It will not run until it is explicitly "Resumed."

### State Transitions
- **Ready → Running:** The scheduler selects the task because it is the highest-priority task in the Ready list.
- **Running → Ready:** A higher-priority task becomes Ready (preemption), or the task's time slice expires (if round-robin is used for equal priorities).
- **Running → Blocked:** The task requests a resource that isn't available or waits for an event/delay.
- **Blocked → Ready:** The event the task was waiting for occurs (e.g., a mutex is released).
- **Any State → Suspended:** A task is explicitly suspended via a system call.
- **Suspended → Ready:** A task is explicitly resumed and is now eligible to be scheduled again.

## The Idle Task

The Idle Task is a special task created automatically by the RTOS kernel when the scheduler is started. It ensures that there is always at least one task capable of running on the CPU.

### Key Characteristics:
- **Automatic Creation:** It is initialized by the kernel during the scheduling startup phase.
- **Lowest Priority:** It is assigned the lowest possible priority (typically 0) to ensure it never consumes CPU time if any application task is in the **Ready** state.
- **Always Available:** When no other task is in the **Running** or **Ready** state, the Idle Task becomes the **Running** task.

### Responsibilities:
1. **Memory Management (The "Housekeeper"):** When a task deletes itself (e.g., `vTaskDelete(NULL)`), it cannot free its own memory because it is still using its stack to execute the deletion code. The RTOS marks the task as deleted, and the **Idle Task** is responsible for actually freeing the memory (Stack and TCB) allocated to that deleted task. 
   - *Note:* If Task A deletes Task B, the memory can often be freed immediately; the Idle Task is specifically required for *self-deleting* tasks.
2. **Low Power Management (Idle Hook):** RTOS allows the use of an **Application Idle Hook**—a callback function within the Idle Task. This is often used to put the CPU into a low-power or sleep mode when no useful application tasks are executing, significantly reducing power consumption.
3. **Background Processing:** It can be used to perform background activities like system telemetry or watchdog "kicking" without interfering with time-critical tasks.

### Implementing the Idle Hook (FreeRTOS Example)
To use the idle hook in FreeRTOS:
1.  **Enable the Feature:** Set `configUSE_IDLE_HOOK` to `1` in your `FreeRTOSConfig.h`.
2.  **Define the Callback:** Implement the following function in your application code:
    ```c
    void vApplicationIdleHook( void ) {
        /* Application specific code here */
        // Example: Put MCU in sleep mode
        __WFI(); // Wait For Interrupt
    }
    ```
3.  **Critical Rule:** The idle hook function **must not** call any API functions that could cause the idle task to **Block** (e.g., `vTaskDelay()` or waiting for a semaphore). The idle task must always be ready to run.

---

## The Tick Hook Function

The **Tick Hook** is an optional callback function called by the RTOS kernel during each **Tick Interrupt**. It is often used to implement software timers or periodic checks that must happen with high frequency and precision.

### Key Characteristics:
- **High Frequency:** It executes as part of the RTOS tick interrupt (e.g., every 1ms if the tick rate is 1000Hz).
- **ISR Context:** Unlike the Idle Hook, the Tick Hook executes from within an **Interrupt Service Routine (ISR)**.

### Implementing the Tick Hook (FreeRTOS Example)
1.  **Enable the Feature:** Set `configUSE_TICK_HOOK` to `1` in your `FreeRTOSConfig.h`.
2.  **Define the Callback:** Implement the following function in your application:
    ```c
    void vApplicationTickHook( void ) {
        /* High-precision periodic code here */
    }
    ```

### Critical Rules for the Tick Hook:
- **Keep it Short:** Since it runs inside an ISR, it must execute extremely quickly to avoid delaying other interrupts or the scheduler itself.
- **Minimal Stack Usage:** It uses the system's interrupt stack, which is often very limited.
- **API Restrictions:** It **must not** call any RTOS API functions that do not end in `FromISR` (e.g., use `xQueueSendToBackFromISR` instead of `xQueueSend`).
- **No Blocking:** It cannot call any function that might cause the task to block or yield.

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
