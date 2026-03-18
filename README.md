# RTOS (Real-Time Operating System)

This repository explores the fundamentals and implementation of Real-Time Operating Systems.

## Hardware Platforms: Choosing Your RTOS Target

Before diving into the code, it's important to understand the hardware platforms commonly used in the industry and how they differ in their real-time capabilities.

### 1. STM32 (The Surgeon)
The STM32 is built for **hard real-time precision**. In the industry, this is what you use for motor control, medical devices, or flight controllers (like the ones used in drone projects).

- **Real-time Determinism:** When you tell an STM32 to run a task every 1ms, it does it with microsecond accuracy.
- **Peripherals:** It has "beast" hardware timers and ADCs. If you need to read a sensor at exactly 10,000 times per second, STM32 is the king.
- **The RTOS Experience:** This is the best board for learning FreeRTOS because it doesn't have a "hidden" operating system running in the background. It's just your code and the hardware.
- **Why it's better than ESP32 here:** ESP32 is great, but its internal Wi-Fi/Bluetooth stack often "interrupts" the CPU, which can mess with extreme precision timing.

### 2. ESP32 (The Socialite)
The ESP32 is the **IoT Workhorse**. If your project needs to talk to the internet, you use an ESP32.

- **Integrated Wireless:** It has Wi-Fi and Bluetooth baked into the silicon. To do that on an STM32, you'd need extra modules and messy wiring.
- **Dual Core:** Most ESP32s have two cores. You can run your "FreeRTOS task" on Core 1 and let the "Wi-Fi stuff" hang out on Core 0 so they don't fight.
- **The "Hidden" RTOS:** Interestingly, the ESP32 already runs FreeRTOS by default. Even a simple "Arduino" sketch on an ESP32 is actually a task running inside FreeRTOS.

### 3. Raspberry Pi 5 (The General)
The RPi 5 isn't a microcontroller; it's a **Full Computer (SBC)**. It runs Linux.

- **Processing Power:** The RPi 5 is roughly 100x to 500x more powerful than your STM32. It can handle 4K video, AI object detection, and web servers.
- **The "Non-Real-Time" Problem:** Because it runs Linux, it is **not deterministic**. If you ask Linux to toggle a pin in 1ms, it might take 1.2ms or 2.0ms because it was busy updating the clock or checking for an email in the background.
- **The "Combo" Move:** In professional robotics, engineers often use both:
  - **RPi 5:** Does the "heavy lifting" (Camera vision, Path planning, GUI).
  - **STM32:** Connected to the RPi via UART/SPI to handle the "real-time" stuff (Motor PWM, Sensor timing).

### Comparison at a Glance

| Feature | STM32 (Nucleo) | ESP32 | Raspberry Pi 5 |
| :--- | :--- | :--- | :--- |
| **Primary Use** | Industrial/Real-Time | IoT/Wireless | High-level Apps/AI |
| **OS** | None / RTOS | FreeRTOS (Native) | Full Linux |
| **Power Draw** | Very Low (< 50mA) | Medium (~100-200mA) | High (> 2A) |
| **Timing** | Microsecond Precision | Millisecond Precision | Varies (Not precise) |

---

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

---

## FreeRTOS Scheduling: The Core Engine

The **Scheduler** is a piece of kernel code responsible for deciding which task should be executing at any particular time on the CPU. It is the "brain" of FreeRTOS.

The **Scheduling Policy** is the algorithm used by the scheduler to decide which task to execute at any point in time. In FreeRTOS, this policy is primarily configured via `configUSE_PREEMPTION` in the `FreeRTOSConfig.h` file.

### 1. Priority-Based Pre-emptive Scheduling
**Configuration:** `configUSE_PREEMPTION = 1`

This is the industry standard for real-time systems. The scheduler ensures that the **highest priority task** that is in the **Ready** state is always the one currently **Running**.

- **The Preemption Event:** If Task A (Priority 2) is running and Task B (Priority 3) suddenly becomes **Ready**, the scheduler immediately interrupts Task A.
- **Context Switch:** The RTOS saves Task A's registers and loads Task B's context.
- **Determinism:** High-priority tasks are never delayed by lower-priority application logic.

### 2. Co-operative Scheduling
**Configuration:** `configUSE_PREEMPTION = 0`

This is a more "polite" but less deterministic approach.
- **The Process:** A task continues to run until it **explicitly** decides to give up the CPU (by calling `vTaskDelay` or `taskYIELD`).
- **No Preemption:** Even if a high-priority task becomes ready, it will **not** interrupt the current task until that task voluntarily yields.
- **The Danger:** If a task gets stuck in an infinite loop, the entire system freezes.

---

## Scheduling Mechanisms: Preemption and Time Slicing

The RTOS scheduler determines which task should be in the **Running** state based on their assigned priorities and the current scheduling policy.

### 1. Handling Equal Priority Tasks (Time Slicing)
When multiple tasks share the **same priority** and are all in the **Ready** state, the RTOS typically employs **Round Robin Time Slicing**.

- **The Tick Interrupt:** The RTOS kernel uses a hardware timer to generate a periodic "Tick Interrupt" (e.g., every 1ms).
- **Time Slices:** Each task is given a "time slice" equal to one tick. When the tick interrupt occurs, the scheduler checks if there are other tasks of the same priority in the Ready list.
- **Switching:** If other tasks exist at that priority, the scheduler performs a context switch to the next task in the sequence.
- **Configuration:** In FreeRTOS, this behavior is controlled by `configUSE_TIME_SLICING`. If disabled, a task of equal priority will only yield the CPU if it explicitly blocks or is preempted by a higher-priority task.

#### Visualization of Equal Priority Switching:
| Tick | Running Task (Priority 2) | Ready List (Priority 2) |
| :--- | :--- | :--- |
| T0 | Task A | [Task B, Task C] |
| T1 (Tick) | Task B | [Task C, Task A] |
| T2 (Tick) | Task C | [Task A, Task B] |

---

## The Scheduler: Why Can't We Just Hardcode Priorities?

A common question is: *"If we already know the priorities (Task A=3, Task B=2), why do we need a complex Scheduler? Can't we just hardcode the CPU to always run the highest number?"*

The answer lies in the difference between a **Static Strategy** and a **Dynamic Reality**.

### 1. High Priority $\neq$ Always Running
A high-priority task (like a safety-critical sensor check) often spends **99% of its time doing nothing**. It might be waiting for:
- A specific 10ms delay to pass.
- A message to arrive from a UART port.
- A user to press a button.

If you "hardcoded" the execution based strictly on priority, the CPU would stay stuck on the high-priority task forever, even if that task was just "waiting." The **Scheduler** is needed to realize: *"Task A is high priority, but it's currently **Blocked**. Therefore, I will let Task B (Lower Priority) run for now."*

### 2. The "Event" Problem (Asynchronicity)
In embedded systems, things happen at random times.
- An interrupt fires because a packet arrived.
- A timer expires.
- A hardware error occurs.

You cannot hardcode the "termination" or "switching" points because you don't know **when** these events will happen. The Scheduler acts as an **Event Manager** that constantly re-evaluates the "Ready List" every time a system event occurs.

### 3. Context Management
When you switch from Task A to Task B, you have to save the "State" (CPU registers, Program Counter, Stack Pointer).
- If you tried to hardcode this, your code would be a mess of `if-else` statements and manual register saving at every possible line of code.
- The **Scheduler** automates this by performing a **Context Switch** during the Tick Interrupt or a System Call, making it invisible and "safe" for the developer.

### Summary: Strategy vs. Engine
| Component | Analogy | Role |
| :--- | :--- | :--- |
| **Preemptive Strategy** | The Rulebook | The logic that says: "Higher numbers win the CPU." |
| **The Scheduler** | The Referee | The actual code that watches the clock, checks who is "Ready," and physically swaps the tasks in RAM. |

**Without the Scheduler, your "Preemptive Strategy" is just a theory. The Scheduler is the engine that makes that theory a reality in a world of unpredictable events.**

---

### Configuring the Scheduler in PlatformIO (`FreeRTOSConfig.h`)

In your PlatformIO project, you can easily toggle between these policies by modifying the `include/FreeRTOSConfig.h` file:

```c
// For Priority-Based Pre-emptive Scheduling:
#define configUSE_PREEMPTION                    1

// For Co-operative Scheduling:
#define configUSE_PREEMPTION                    0
```

---

## Task States: The Scheduler as a "State Machine Manager"

In an RTOS, the Scheduler doesn't just decide who runs; it actively manages the **lifecycle** of every task. It maintains internal "Ready," "Blocked," and "Suspended" lists (sets of TCBs) and moves tasks between them based on system events.

### The Four States
1.  **Running:** The task currently owns the CPU. In a single-core system, only one task can be in this state.
2.  **Ready:** The task has everything it needs to run but isn't the highest priority currently. It is sitting in the Scheduler's **Ready List**.
3.  **Blocked:** The task is waiting for an external event (a delay, a semaphore, or a queue). It is sitting in the Scheduler's **Blocked List** and consumes **zero CPU time**.
4.  **Suspended:** The task has been "put to sleep" by an explicit API call (e.g., `vTaskSuspend`). It won't run until explicitly resumed.

### How the Scheduler Pulls the Strings
The Scheduler acts as the "Engine" for these state transitions. It reacts to three main triggers:

| Trigger | Event | Scheduler's Action | Resulting State |
| :--- | :--- | :--- | :--- |
| **API Call** | `vTaskDelay(100)` | Moves the Running task to the **Blocked List**. | **Running → Blocked** |
| **Tick Interrupt** | 100ms have passed | Moves the task from the **Blocked List** back to the **Ready List**. | **Blocked → Ready** |
| **Resource Event** | A Semaphore is released | Moves the highest-priority task waiting for that semaphore to the **Ready List**. | **Blocked → Ready** |
| **Preemption** | A high-priority task wakes up | Moves the current Running task to the **Ready List** and loads the new task. | **Running → Ready** |
| **Manual Suspend** | `vTaskSuspend()` | Moves the task to the **Suspended List**, removing it from the scheduling loop. | **Any → Suspended** |

### Visualization of State Transitions
- **Ready → Running:** The Scheduler selects the task because it is the highest-priority "Ready" task.
- **Running → Ready:** A higher-priority task becomes "Ready" (Preemption), or the task's time slice expires.
- **Running → Blocked:** The task requests a resource (Mutex/Queue) that isn't available.
- **Blocked → Ready:** The event the task was waiting for finally occurs.

**Crucial Point:** A task **never** moves itself. It "requests" a state change by calling an API, but it's the **Scheduler** that physically moves the Task Control Block (TCB) between lists and performs the context switch.

---

## Under the Hood: Kernel Architecture and Boot Sequence

Ever wondered how your Nucleo board's "brain" actually functions at the electrical and assembly level? Here is exactly how the FreeRTOS kernel operates under the hood.

### 1. The "Brain" vs. The "Translator"
FreeRTOS is deliberately split into two distinct pieces so it can be ported to almost any microcontroller:

*   **task.c (Generic Code):** This is the high-level logic. It manages lists of tasks, states (Ready, Blocked, Suspended), and priorities. It is hardware-agnostic and has no idea what an ARM Cortex-M processor is.
*   **port.c (Architecture Specific Code):** This is the **Hardware Translator**. It takes the generic concepts from `task.c` and turns them into physical register writes and assembly instructions specifically for your STM32 chip.

### 2. The Boot-Up Sequence
When you call `vTaskStartScheduler()` in `main.c`, the following sequence occurs:

1.  **Internal Setup:** The generic brain sets up its internal lists and kernel objects.
2.  **The Handover:** It hands control to the hardware translator by calling `xPortStartScheduler()`.
3.  **Hardware Wiring:** The translator wires up the physical hardware:
    *   **Configures SysTick:** It programs the internal hardware timer to fire an interrupt exactly every 1 millisecond.
    *   **Configures Priorities:** It sets the hardware interrupt priority levels for the kernel interrupts.
    *   **Executes SVC:** It fires a **Supervisor Call (SVC)** assembly instruction to kick off the highest-priority task in the Ready list.

### 3. The Three Kernel Interrupts (ARM Cortex-M)
Your STM32 uses three specific hardware interrupts to make the operating system function:

*   **SVC (Supervisor Call) - The "Starter Motor":** This interrupt is used **exactly once** during boot-up. Its only job is to launch the very first task and get the system engine running.
*   **SysTick - The "Metronome":** This is a hardware timer that interrupts the CPU every 1ms. It updates the system time, checks if any sleeping tasks need to wake up, and if a context switch is needed, it "pends" (requests) a **PendSV** interrupt.
*   **PendSV (Pendable Service Value) - The "Bouncer":** This is the interrupt that actually performs the **Context Switch**. It kicks the current task off the CPU, saves its registers to RAM, and loads the new task's registers into the CPU.

### 4. The "Lowest Priority" Secret
One of the most important safety features in an RTOS is that the **Scheduler (SysTick and PendSV) runs at the lowest possible hardware interrupt priority.**

**Why?** Because hardware always outranks software! The operating system should never delay a real-world hardware emergency. If your board receives a critical byte of data over UART or an ADC finishes a critical reading, those hardware interrupts will instantly preempt the scheduler, handle the emergency, and then return control to the OS.

### 5. The Silent Killer: Why `vTaskStartScheduler()` Fails

There is only one single, catastrophic reason why `vTaskStartScheduler()` will ever give up, return, and trap your code in that infinite `for(;;);` black hole: **It runs out of money (RAM).**

Specifically, it runs out of the **FreeRTOS Heap Memory**. Here is exactly how this silent killer happens:

#### The Hidden Cost of the "Janitor"
As we’ve seen, `vTaskStartScheduler()` has a hidden job: before it opens the office, it must "hire" the **Idle Task** (and the **Timer Task**, if enabled). But hiring tasks isn't free. Every single task requires RAM for two things:
1.  **The TCB (Task Control Block):** The ID card that holds the task's priority, name, and status.
2.  **The Stack:** The physical workspace (memory) the task needs to do its math and store its local variables.

#### The 10KB Budget Constraint
The FreeRTOS Manager doesn't get to use all the RAM on your STM32 chip. It only gets the specific allowance you wrote in your `FreeRTOSConfig.h` file. In many default setups, this budget is exactly **10 Kilobytes**:
```c
#define configTOTAL_HEAP_SIZE ( 10 * 1024 )
```

#### The Crash Scenario
Imagine you get a little carried away in `main()`:
- You create **Task 1** and give it a massive **4KB stack**. (6KB left)
- You create **Task 2** and give it another massive **4KB stack**. (2KB left)
- You create a big **Message Queue** to send data between them, which eats up another **2KB**. (**0KB left**)

Then, you confidently call `vTaskStartScheduler()`. The Manager wakes up, looks at its bank account, and realizes it has **0 bytes left**. It physically cannot allocate the memory required to create the mandatory **Idle Task**.

Because the OS mathematically cannot run without an Idle Task, the Manager throws its hands in the air, aborts the entire boot-up sequence, and returns control back to `main()`. Your code instantly falls into the `for(;;);` loop, and the board sits there frozen like a brick.

#### How to Protect Yourself
When you are building a complex drone or cybersecurity tool, you will be creating lots of tasks. If your board suddenly freezes on boot, 99% of the time, you just need to open `FreeRTOSConfig.h` and increase **`configTOTAL_HEAP_SIZE`**.

---

## The Idle Task: The Kernel's Unsung Hero

The **Idle Task** is a special system task created **automatically** by the RTOS kernel when the scheduler is started. It ensures the CPU is always doing something valid.

### 1. Automatic Creation and Priority
You never need to call `xTaskCreate` for the Idle Task. The kernel initializes it during the startup phase.

- **Priority 0:** It has the lowest possible priority. It only runs when every other task is either **Blocked** or **Suspended**.
- **The "Janitor":** Like a shop staff that sweeps the floors when there are no customers, the Idle Task handles cleanup. If you delete a task, the Idle Task is the one that actually frees up that RAM (stack/TCB).

### 2. Power Management (The Idle Hook)
Modern embedded systems use the Idle Task for **Power Saving**. Instead of wasting battery, you can use an **Application Idle Hook** to put the CPU into a sleep state until the next interrupt occurs.

---

## The Timer Service Task (Timer Daemon)

While the Idle Task handles maintenance, the **Timer Service Task** (or **Timer Daemon**) manages all **Software Timers**.

### 1. The "Alarm Clock"
Imagine you tell a friend to remind you to check the oven in 10 minutes. The Timer Service Task is that friend. It sits in the corner, watches the clock, and "pokes" you when it’s time to execute a function.

### 2. Core Responsibilities
- It handles all software timers (e.g., created via `xTimerCreate`).
- It doesn't run based on code logic; it runs based on the **System Tick**.
- Software timers are often better than tasks for simple, repetitive things (like blinking an LED) because they save RAM.

---

## The RTOS Heartbeat: How FreeRTOS Knows What Time It Is

The RTOS kernel does not have an internal "clock" that inherently knows what a millisecond is. Instead, it relies on a specific piece of hardware inside your STM32 to act as a **Metronome**.

### 1. The SysTick Timer (The Metronome)
Your STM32 chip has a dedicated piece of hardware called the **SysTick Timer**. It is completely separate from the main CPU core. Think of it as a physical metronome sitting on the Office Manager's desk. It swings back and forth, and every time it "clicks," it fires a hardware interrupt. This is the "Heartbeat" of the entire operating system.

### 2. `configTICK_RATE_HZ` (The Speed of the Metronome)
How fast does that metronome click? That is entirely up to you. In your `FreeRTOSConfig.h` file, you have this exact line:
```c
#define configTICK_RATE_HZ  ( ( TickType_t ) 1000 )
```
By setting this to 1000, you told the hardware: *"I want the SysTick metronome to fire an interrupt exactly 1000 times per second."* This means your RTOS has a "Tick" exactly every **1 millisecond**.

### 3. `xTickCount` (The Clock on the Wall)
So, the metronome clicks 1000 times a second. What happens during that click?
The FreeRTOS Manager drops whatever it is doing, runs the **SysTick Interrupt handler**, and simply adds 1 to a massive global variable named **`xTickCount`**.

- **At Boot:** `xTickCount` is 0.
- **1 Second Later:** `xTickCount` is 1,000.
- **1 Minute Later:** `xTickCount` is 60,000.

This single variable is how the entire operating system knows what time it is.

### Why This Matters: The `vTaskDelay` Secret
Think about the code you wrote for Task 1:
```c
vTaskDelay(pdMS_TO_TICKS(1000));
```
When Task 1 hits that line, it tells the Manager: *"I am going to sleep. Wake me up in 1000 milliseconds."*

The Manager doesn't set a separate stopwatch for Task 1. Instead, it looks at the **Wall Clock (`xTickCount`)**. If the clock currently says **500**, the Manager writes a note: *"Wake up Task 1 when the wall clock reaches 1500."* 

Then, the Manager puts Task 1 to sleep, hands the CPU to the Idle Task, and goes back to waiting for the next metronome click. Every millisecond, the SysTick interrupt fires, `xTickCount` goes up by 1, and the Manager checks its notepad to see if any sleeping tasks have hit their wake-up number yet.

**This is why your logic analyzer showed that perfect 1-second gap! The hardware metronome is keeping perfect time.**

### 4. The Math: Cycles per Tick
To generate a 1ms tick at **72 MHz**, the SysTick hardware counts exactly **72,000 cycles** before firing an interrupt.

### 5. The Danger of Misconfiguration
If you tell the RTOS your CPU is at 72 MHz but it's actually running at 8 MHz, every "1 ms tick" will take 9ms of real time. A 1-second delay will actually take **9 seconds**!

---

## Memory Allocation: What Happens in RAM?

When you call `xTaskCreate`, the RTOS kernel carves out two blocks of memory from the **Heap**:
1.  **TCB (Task Control Block):** Stores task metadata (priority, name, state).
2.  **Task Stack:** Stores local variables and CPU registers during context switches.

---

## Debugging and I/O: Semihosting vs. UART

### 1. Semihosting (The Debugger Trick)
The CPU physically **pauses** execution to let the debugger read memory. It is incredibly slow and destroys RTOS timing.

### 2. UART (The Professional Way)
The STM32 uses dedicated hardware (USART2) to send text out over physical wires (**PA2 TX**). It runs independently of the CPU core, keeping the RTOS scheduler ticking perfectly.

---

## Advanced Profiling: Visualizing the "Heartbeat"

### 1. Task Profiling with GPIO (The "D2" Trigger)
We've added a **Profiler Pin** on **PA10**. 
- **Rising Edge:** Task 1 takes the CPU.
- **Falling Edge:** Task 1 finishes and enters `vTaskDelay`.

### 2. Measuring the Idle Gap (The "974ms to 976ms" Window)
By enabling the **Idle Hook**, we observed the Idle Task starting at **974ms** and stopping at **976ms**. This confirms the CPU is "Idle" for ~97.5% of every 1000ms cycle, proving FreeRTOS efficiency.

---

## Conclusion

This repository provides a comprehensive guide to RTOS on STM32, from hardware configuration and UART retargeting to kernel-level interrupt management and advanced profiling. Mastering these concepts is fundamental to building predictable, high-performance embedded systems.
