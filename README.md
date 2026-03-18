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

## Scheduling Mechanisms: Preemption and Time Slicing

The RTOS scheduler determines which task should be in the **Running** state based on their assigned priorities and the current scheduling policy.

### 1. Preemptive Scheduling (Fixed Priority)
In a preemptive system, the scheduler ensures that the **highest priority task** that is in the **Ready** state is always the one currently **Running**.

- **The Preemption Event:** If Task A (Priority 2) is running and Task B (Priority 3) suddenly becomes **Ready** (e.g., a timer expires or an interrupt signals a semaphore), the scheduler immediately interrupts Task A.
- **Context Switch:** The RTOS saves Task A's registers to its stack and loads Task B's context. Task B begins running instantly.
- **Determinism:** This ensures that high-priority, time-critical tasks are never delayed by lower-priority application logic.

### 2. Handling Equal Priority Tasks (Time Slicing)
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

## Scheduling Policies: The Scheduler's Decision Algorithm

The **Scheduling Policy** is the specific algorithm the Scheduler uses to decide which task should be in the **Running** state at any point in time. While there are several options, most real-time systems (like FreeRTOS) default to a specific type for maximum reliability.

### 1. Simple Pre-emptive Scheduling (Round Robin)
In this policy, all tasks are treated equally. There are no priorities.
- **The Process:** Each task is given a fixed "time slice" (a tick). When the tick interrupt occurs, the Scheduler simply moves to the next task in the list.
- **Use Case:** Systems where no task is more important than another. It ensures "fairness" but provides zero real-time guarantees for critical events.

### 2. Priority-Based Pre-emptive Scheduling (The RTOS Standard)
This is the **default** for FreeRTOS and almost all professional RTOSs.
- **The Process:** Every task is assigned a priority number. The Scheduler **must** always run the "Ready" task with the highest priority.
- **Preemption:** If a high-priority task suddenly becomes ready (even in the middle of a low-priority task's time slice), the Scheduler **immediately** stops the low-priority task and switches.
- **Why it's the King:** This ensures that time-critical tasks (like safety sensors or motor controls) are serviced with microsecond precision, regardless of what else the CPU is doing.

### 3. Co-operative Scheduling
This is a more "polite" but less deterministic approach.
- **The Process:** A task continues to run until it **explicitly** decides to give up the CPU (by calling `vTaskDelay` or `taskYIELD`).
- **No Preemption:** Even if a high-priority task becomes ready, it will **not** interrupt the current task until that task finishes its work or voluntarily yields.
- **The Danger:** If a task gets stuck in an infinite loop without yielding, the entire system (including all other tasks) will freeze forever.

### Comparison Table

| Policy | Decision Logic | Preemption? | Best For... |
| :--- | :--- | :--- | :--- |
| **Round Robin** | Fairness (Equal slices) | Yes (on Tick) | Simple, non-critical multitasking. |
| **Priority-Based** | Importance (Highest number) | **Yes (Immediate)** | **Real-time systems / FreeRTOS default.** |
| **Co-operative** | Manual (Wait for Yield) | **No** | Very simple systems with trusted tasks. |

---

### Configuring the Scheduler in `FreeRTOSConfig.h`

In a PlatformIO or any FreeRTOS project, you select the scheduling policy by modifying specific **#define** macros in the `FreeRTOSConfig.h` file (usually found in the `include` folder).

#### 1. To enable Priority-Based Pre-emptive Scheduling (Standard):
```c
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
```
*   `configUSE_PREEMPTION = 1`: Allows higher-priority tasks to interrupt lower-priority ones.
*   `configUSE_TIME_SLICING = 1`: Enables Round Robin for tasks sharing the **same** priority level.

#### 2. To enable Simple Pre-emptive Scheduling (Round Robin):
*Set all task priorities to the same value (e.g., 1) and keep the above settings.*

#### 3. To enable Co-operative Scheduling:
```c
#define configUSE_PREEMPTION                    0
```
*When this is 0, the scheduler will **never** interrupt a task. It will only switch when the current task calls a function like `taskYIELD()` or `vTaskDelay()`.*

#### 4. To disable Round Robin (Equal Priority):
```c
#define configUSE_TIME_SLICING                  0
```
*With this setting, if two tasks have the same priority, the first one to start running will stay running until it blocks or is preempted by a **higher** priority task. They won't "share" time.*

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

## Under the Hood: Architecture Specifics (ARM Cortex-M)

If you look into your FreeRTOS library files, you will see two files that contain the "glue" code between the RTOS kernel and your STM32's hardware: `port.c` and `portmacro.h`. These are architecture-specific (e.g., for ARM Cortex-M3 on the Nucleo-F103RB).

The Scheduler relies on three critical ARM hardware exceptions to function:

### 1. `vPortSVCHandler` (Supervisor Call - SVC)
- **Triggered By:** The `SVC` instruction.
- **Role:** It is used primarily to **launch the very first task** when you call `vTaskStartScheduler()`. 
- **The Scene:** It forces the CPU into a privileged state and sets up the stack for the first task so it can start running safely.

### 2. `xPortPendSVHandler` (Pendable Service Call - PendSV)
- **Triggered By:** Setting the `PendSV` bit in the CPU's Interrupt Control State Register (ICSR).
- **Role:** This is the heart of **Context Switching**.
- **The Scene:** Unlike a normal interrupt, `PendSV` is designed to be delayed until no other high-priority interrupts are running. When it finally fires, it saves the current task's registers (R0-R15), finds the next highest-priority task, and loads its registers.

### 3. `xPortSysTickHandler` (SysTick)
- **Triggered By:** The internal ARM **SysTick Timer** (hardware timer).
- **Role:** It implements the **RTOS Tick management**.
- **The Scene:** Every 1ms (or whatever your tick rate is), this interrupt fires. It increments the system's "Tick Count," checks if any tasks in the Blocked list have timed out, and then "pends" a `PendSV` to check if a context switch is needed.

### Summary: The Context Switching Workflow
1.  **SysTick** fires every 1ms.
2.  It realizes a higher-priority task has finished its `vTaskDelay`.
3.  It triggers a **PendSV** interrupt.
4.  **PendSV** saves the current task, loads the new one, and the Scheduler has successfully performed its job.

---

## The Idle Task: The Kernel's Unsung Hero

The **Idle Task** is a special system task created **automatically** by the RTOS kernel when the scheduler is started (e.g., when calling `vTaskStartScheduler()` in FreeRTOS). It is the kernel's "fallback" mechanism to ensure the CPU is always doing something valid.

### 1. Automatic Creation and Priority
You never need to call `xTaskCreate` for the Idle Task. The kernel initializes it during the startup phase to ensure there is **always** at least one task in the **Ready** or **Running** state.

- **Priority 0:** It is assigned the lowest possible priority. This ensures it never "steals" time from your application tasks. It only runs when every other task in the system is either **Blocked** or **Suspended**.
- **The "Infinite Loop":** Like all RTOS tasks, it contains an infinite loop. If no other work is available, the CPU just cycles through this loop.

### 2. Core Responsibilities: The "Housekeeper"
The Idle Task performs essential maintenance that application tasks cannot do for themselves:

- **Cleaning Up After Self-Deletion:** When a task calls `vTaskDelete(NULL)`, it is asking to be killed. However, a task cannot free its own stack and TCB memory while it is still using that stack to execute the deletion code! 
- **The Handover:** The kernel marks the task as "Ready to be deleted" and moves it to a termination list. The **Idle Task** then scans this list and actually frees the memory (RAM) back to the heap.
- **Critical Note:** If your application creates and deletes tasks frequently but **never** allows the Idle Task to run (because higher-priority tasks are hogging the CPU), your system will eventually run out of memory (Heap Exhaustion).

### 3. Power Management (The Idle Hook)
In modern embedded systems, the Idle Task is where **Power Saving** happens. Instead of just "spinning" in a loop and wasting battery, you can use an **Application Idle Hook**.

| Feature | Description |
| :--- | :--- |
| **Low Power Mode** | You can use the `__WFI()` (Wait For Interrupt) instruction inside the hook to put the CPU into a sleep state. |
| **Background Processing** | Use it to calculate CPU load statistics or "kick" a watchdog timer. |
| **Continuous Availability** | The hook must **never** block. It must be ready to yield the CPU the microsecond a higher-priority task wakes up. |

### Summary of Idle Task Properties

| Property | Value/Behavior |
| :--- | :--- |
| **Created By** | RTOS Kernel (Automatic) |
| **Priority** | 0 (Lowest) |
| **Stack Size** | Defined by `configMINIMAL_STACK_SIZE` |
| **Can it Block?** | **No.** It must always be in the Ready/Running state. |
| **Can it be Deleted?** | **No.** The system would crash if no tasks were ready to run. |

---

## The Timer Service Task (Timer Daemon)

While the **Idle Task** handles system maintenance, the **Timer Service Task** (also known as the **Timer Daemon**) is responsible for managing all **Software Timers** in your application.

### 1. Automatic Creation
This task is created automatically by the RTOS kernel during the scheduling startup phase, but **only if** you have enabled software timers in your configuration:
- `configUSE_TIMERS = 1` in `FreeRTOSConfig.h`.
- If you don't need software timers, setting this to `0` will save RAM by not creating this task or its associated command queue.

### 2. Core Responsibilities
The Timer Daemon is the "brain" behind every software timer you create (e.g., using `xTimerCreate`). Its primary duties include:
- **Tracking Timeouts:** It keeps track of when a timer is set to expire.
- **Executing Callbacks:** When a timer expires, the Timer Daemon is the task that actually calls the **Timer Callback Function**.
- **Command Processing:** It listens to a "Timer Command Queue." When your application tasks start, stop, or change a timer, they send a message to this queue, which the Timer Daemon then processes.

### 3. The "Execution Context" Warning
A very common mistake for beginners is forgetting **which task** is running their timer code.
- **Context:** All software timer callback functions execute in the **context of the Timer Daemon task**.
- **The Rule:** You must **never** call an RTOS API function that could cause the task to **Block** (like `vTaskDelay()` or waiting for a semaphore) inside a timer callback.
- **The Consequence:** If you block the Timer Daemon, **all other software timers in your system will stop working**, as the daemon won't be able to process the next expiration or command.

### 4. Configuration Impact
Because the Timer Daemon is a task itself, it has its own settings in `FreeRTOSConfig.h`:
- `configTIMER_TASK_PRIORITY`: Usually set to a high priority to ensure timers expire with high precision.
- `configTIMER_TASK_STACK_DEPTH`: Must be large enough to handle the logic inside all your timer callback functions.

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

## The RTOS Heartbeat: CPU Clock vs. RTOS Tick

The RTOS kernel does not have an internal "clock" that inherently knows what a millisecond is. Instead, it relies on a hardware timer (typically the **SysTick** timer in ARM Cortex-M processors) to "pulse" the CPU at regular intervals. This pulse is called the **RTOS Tick**.

### 1. The Relationship
- **CPU Clock (High Frequency):** This is the speed at which the processor executes instructions (e.g., 72 MHz).
- **RTOS Tick (Low Frequency):** This is the frequency at which the scheduler wakes up to manage tasks (e.g., 1 kHz or 1 ms per tick).

### 2. The Math: Cycles per Tick
To generate a 1ms tick, the hardware timer must count a specific number of CPU clock cycles before triggering an interrupt.

**Formula:**
$$\text{Cycles per Tick} = \frac{\text{CPU Frequency (Hz)}}{\text{Target Tick Rate (Hz)}}$$

**Example (72 MHz CPU):**
If your CPU is at 72 MHz and you want a 1 ms tick (1,000 Hz):
$$\frac{72,000,000 \text{ cycles/sec}}{1,000 \text{ ticks/sec}} = 72,000 \text{ cycles per tick}$$
The SysTick hardware is programmed to count 72,000 cycles and then fire an interrupt.

### 3. The Danger of Misconfiguration
The RTOS kernel calculates the "Cycles per Tick" based on a constant you provide in your configuration (e.g., `configCPU_CLOCK_HZ` in FreeRTOS). If this value does not match the **actual** physical speed of your hardware, all time-based functions (`vTaskDelay`, timeouts, software timers) will be incorrect.

#### Scenario: The 9-Second Delay
If you tell the RTOS your CPU is at **72 MHz** but the hardware is actually running at **8 MHz**:
1.  **The RTOS expects** 72,000 cycles to equal 1 ms.
2.  **The 8 MHz Hardware** takes much longer to reach 72,000 cycles:
    $$\frac{72,000 \text{ cycles}}{8,000,000 \text{ cycles/sec}} = 0.009 \text{ seconds (9 ms)}$$
3.  **The Result:** Every "1 ms tick" in your code actually takes 9 ms of real time. A `vTaskDelay(1000)` (intended for 1 second) will actually take **9 seconds** to complete!

---

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

---

## Task Creation: `xTaskCreate`

In an RTOS like FreeRTOS, the `xTaskCreate` function is used to create a new task and add it to the list of tasks that are ready to run. It allocates a **Task Control Block (TCB)** and a dedicated **Stack** for the task from the heap.

### Function Signature
```c
BaseType_t xTaskCreate(
    TaskFunction_t pvTaskCode,
    const char * const pcName,
    unsigned short usStackDepth,
    void *pvParameters,
    UBaseType_t uxPriority,
    TaskHandle_t *pxCreatedTask
);
```

### Parameter Breakdown

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `pvTaskCode` | `TaskFunction_t` | Pointer to the task entry function. This function must contain an infinite loop and should never return. |
| `pcName` | `const char * const` | A descriptive name for the task. This is primarily used for debugging and is not used by the kernel for scheduling. |
| `usStackDepth` | `unsigned short` | The number of **words** (not bytes) to allocate for the task's stack. For example, on a 32-bit architecture, a stack depth of 100 equates to 400 bytes. |
| `pvParameters` | `void *` | A pointer to any data you want to pass into the task function when it starts. |
| `uxPriority` | `UBaseType_t` | The priority at which the task will run. Systems typically use 0 for the lowest priority (Idle Task). Higher numbers represent higher priority. |
| `pxCreatedTask` | `TaskHandle_t *` | An optional output parameter. It stores a "handle" to the task, which can be used later to delete or change the task's priority. |

### Key Considerations for the User
1. **Memory Allocation:** Ensure the `usStackDepth` is sufficient for the task's local variables and function call overhead. If the stack is too small, it will lead to a **Stack Overflow**, often crashing the system.
2. **Priority Assignment:** RTOS scheduling is priority-based. A task with priority `5` will always preempt a task with priority `4` if both are in the **Ready** state.
3. **The Task Function:** The function pointed to by `pvTaskCode` must look like this:
   ```c
   void vTaskFunction( void * pvParameters ) {
       for( ;; ) {
           // Task logic goes here
       }
   }
   ```
4. **Return Value:** The function returns `pdPASS` if the task was created successfully. If it returns `errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY`, it means the heap is full and the task could not be initialized.

---

## Debugging and I/O: Semihosting vs. UART

Normally, a microcontroller has no idea what a "screen" or a "console" is. To see what's happening inside, we have two primary methods: **Semihosting** and **UART**.

### 1. Semihosting (The Debugger Trick)
Semihosting is a clever trick designed by ARM to let the microcontroller borrow (host) the screen and keyboard of your laptop.

- **How it works on your Nucleo:**
    1. You call `printf("Hello");` in your code.
    2. The compiler translates this into a special software interrupt (a breakpoint).
    3. When the STM32F103 reaches this instruction, the CPU core physically halts execution.
    4. The ST-LINK debugger (the top half of your Nucleo board) notices the CPU has halted.
    5. The ST-LINK reads the word "Hello" directly out of the STM32's RAM.
    6. The ST-LINK sends that word over the mini-USB cable to PlatformIO.
    7. The ST-LINK tells the CPU to un-pause and keep running.

- **The Pros & Cons:**
    - **Pros:** Zero hardware setup. You don't have to configure any pins, baud rates, or peripheral clocks.
    - **Cons:** It is **devastatingly slow**. Because the CPU pauses every single time it prints a character, it destroys the precise timing required by FreeRTOS. If Task 1 is paused while the debugger reads memory, Task 2 misses its deadline.

### 2. UART (The Professional Way)
This is the standard way to do it. Instead of pausing the CPU to let the debugger read memory, the STM32 uses a dedicated piece of hardware (the UART peripheral) to shove the text out through physical wires.

- **How it works on your Nucleo F103RB:**
    1. You configure USART2 on your STM32 (setting the baud rate to 115200, for example).
    2. You map `printf()` so that every character goes into the USART2 data register.
    3. The USART2 hardware automatically shifts the bits out over pin **PA2 (TX)**.
    4. **The Nucleo Magic:** On your specific board, STMicroelectronics permanently wired pin PA2 directly into the ST-LINK chip on the top of the board.
    5. The ST-LINK chip acts as a "Virtual COM Port" (USB-to-Serial converter) and sends it up the mini-USB cable to your terminal.

- **The Pros & Cons:**
    - **Pros:** It runs independently of the CPU core. The RTOS scheduler keeps ticking perfectly while the UART hardware handles the data transfer in the background. It is incredibly fast.
    - **Cons:** You have to write the setup code (enabling the GPIO clock, configuring PA2 as an alternate function, and setting up the USART2 registers).

---

## The Heartbeat: System Clock (SYSCLK), HSE, and PLL

Imagine an orchestra. Without a conductor waving a baton at a specific tempo, the components would play out of sync. The System Clock is the metronome for the entire chip.

### 1. The Motive: Why Speed Matters
Every time the clock "ticks" (a microscopic electrical pulse), the CPU executes an instruction, memory is read, or data is moved. At **72MHz** (72 million ticks per second), your STM32 can execute roughly 72 million simple instructions every second.

### 2. What is HSE? (High-Speed External)
Microcontrollers need a physical source to generate that electrical "tick." While they have internal sources (HSI), these fluctuate with temperature. If the chip gets warm, the clock might drift, ruining precise timings like UART or RTOS delays.

- **Your Board:** On the Nucleo-F103RB, the ST-Link debugger generates a perfect **8MHz signal** and feeds it into the **HSE pin** of your STM32 chip. This provides a rock-solid, unchangeable frequency.

### 3. What is the PLL? (Phase-Locked Loop)
If our external crystal (HSE) only ticks at 8MHz, how do we get the chip to 72MHz? You can't just buy a 72MHz crystal easily—they are fragile and expensive. This is where the **PLL** comes in.

- **The Electronic Multiplier:** The PLL is a multiplier circuit inside the STM32. You feed the stable 8MHz wave from the HSE into the PLL and tell it: "Multiply this by 9."
- **The Math:** `8 MHz (HSE) x 9 (PLL Multiplier) = 72 MHz (SYSCLK)`.

This is exactly what the `SystemClock_Config()` function in our code handles, ensuring the chip runs at its maximum advertised speed for peak RTOS performance.

---

## Advanced Debugging: SEGGER J-Link & SystemView

As your RTOS projects grow in complexity (e.g., dealing with deadlocks or complex priority inversion), standard text-based logging via UART might not be enough. This is where the **SEGGER ecosystem** provides a massive advantage.

### 1. Why Switch to SEGGER?
While the Nucleo boards come with an ST-Link, you can "reflash" them to act as a **SEGGER J-Link**. This unlocks professional-grade tools:

- **SystemView (The "X-Ray"):** Instead of reading text, you see a live visual timeline of your OS. It shows exactly when Task A preempted Task B, how long an ISR took to execute, and if any tasks are missing their deadlines.
- **RTT (Real-Time Transfer):** A high-speed replacement for UART (up to 3.5 MB/s) that sends data through the debug cable **without** stopping the CPU. No pins or baud rates required.
- **Unlimited Breakpoints:** Standard STM32 chips are limited to ~6 hardware breakpoints. SEGGER software allows you to set unlimited breakpoints in Flash memory.

### 2. Setting Up J-Link on Nucleo (F103RB)

If you are using a Nucleo-F103RB, you can convert your on-board ST-Link into a J-Link OB (On-Board) using these steps:

1. **Download the Reflash Tool:** Get the [ST-Link Reflash Utility](https://www.segger.com/products/debug-probes/j-link/models/other-j-links/st-link-on-board/) from SEGGER.
2. **Upgrade:** Plug in your Nucleo, run the tool, and select **"Upgrade to J-Link."** Your board will now be recognized as a J-Link.
3. **Install SystemView:** Download [SEGGER SystemView](https://www.segger.com/products/development-tools/systemview/) to begin visualizing your FreeRTOS schedule.

### 3. Integration with PlatformIO
To use your newly converted J-Link in a PlatformIO project, update your `platformio.ini` file:

```ini
[env:nucleo_f103rb]
platform = ststm32
board = nucleo_f103rb
framework = arduino ; or stm32cube

; Change upload and debug protocol to jlink
upload_protocol = jlink
debug_tool = jlink

; Optional: Monitor via RTT instead of UART
monitor_port = socket://localhost:19021
```

### 4. Recommendation: When to Switch?
- **Stick with UART for now:** It is simpler for "Hello World" and basic task logic.
- **Switch to SEGGER/SystemView:** When you start dealing with **Priority Inversion**, **Deadlocks**, or timing-critical bugs that require seeing the exact moment a context switch occurs.

---

## ST-Link vs. SEGGER J-Link: A Comparison

When working with STM32, you will encounter two main debug probes. Understanding their differences is key to choosing the right tool for your development stage.

| Feature | ST-Link (STMicroelectronics) | SEGGER J-Link |
| :--- | :--- | :--- |
| **Compatibility** | **Locked:** Only works with STM8 and STM32 chips. | **Universal:** Supports ARM Cortex-M/R/A, RISC-V, and chips from almost all vendors (NXP, TI, Microchip, etc.). |
| **Speed** | **Standard:** SWD speeds usually max out around 4-10 MHz. | **High-Speed:** Professional versions reach up to 50 MHz. Much faster firmware uploads. |
| **Breakpoints** | **Limited:** Restricted to the hardware breakpoints built into the MCU (usually 6-8). | **Unlimited:** Can set unlimited breakpoints even in Flash memory using SEGGER software. |
| **Real-Time Data** | **Basic:** Relies on UART or ITM (Instruction Trace Macrocell). | **Advanced (RTT):** Real-Time Transfer allows massive data throughput without halting the CPU. |
| **Visualization** | **Minimal:** Basic variable watching and simple graphing in STM32CubeIDE. | **Professional (SystemView):** Provides a full visual timeline of OS tasks, interrupts, and CPU load. |
| **Cost** | **Cheap/Free:** Often built into Nucleo boards for $10-$20. | **Expensive:** Professional versions cost hundreds of dollars (though EDU and "OB" versions are affordable). |

### Summary: Which one should you use?

1.  **Use ST-Link if:** You are a beginner, working exclusively with STM32, and just need to upload code and do basic step-through debugging.
2. **Use J-Link if:**
    - You need to debug complex timing issues (using SystemView).
    - You are working with non-ST microcontrollers (like Nordic or NXP).
    - You are tired of hitting the "6 breakpoint limit" in large projects.
    - You want the fastest possible upload speeds during a rapid "code-flash-test" cycle.

---

## How it Works: SEGGER RTT & SystemView

This is the "Grand Finale" that explains how **SEGGER SystemView** actually works in real-time without crashing your RTOS timing. While Semihosting stops the CPU and UART is slow, this method uses **RTT (Real-Time Transfer)**—a high-speed data highway between your STM32 and your PC.

### The Data Flow: From Target to Host

#### 1. The Target System (Your STM32)
*   **Application & RTOS:** Your code runs normally.
*   **SystemView API:** Instead of a slow `printf`, you use this lightweight API. It simply records events like "Task A started" or "Interrupt X occurred."
*   **RTT Buffer (Software FIFO):** This is the **"secret sauce."** The data is written into a small "First-In-First-Out" circle of memory inside your **Target RAM**. The CPU doesn't wait for the data to be sent; it just drops it in the buffer and moves on (taking only a few microseconds).

#### 2. The J-Link (The Bridge)
*   The **J-Link** debugger (or your "Reflashed" Nucleo) reaches into the STM32's RAM while the CPU is still running. It "steals" the data from the RTT Buffer and sends it to the PC.

#### 3. The Host System (Your PC)
*   **J-Link DLL:** The driver that manages the data stream.
*   **SystemViewer:** The application that takes those raw timestamps and events and turns them into a beautiful, moving visual graph.

### Why This Matters for Your Project
This architecture provides high **Observability** with:
*   **Minimal Overhead:** Because of the "RTT Buffer" in RAM, the impact on your FreeRTOS task timing is almost zero.
*   **No Extra Pins:** Unlike UART (which needs PA2/PA3), this uses the standard debug pins already connected to your mini-USB.

### Summary: The "Big Three" Logging Methods

| Method | Speed | CPU Impact | Best For... |
| :--- | :--- | :--- | :--- |
| **Semihosting** | Very Slow | **High** (Stops CPU) | Simple setup, no extra hardware. |
| **UART** | Medium | Low | Basic text logging. |
| **SEGGER RTT** | **Very High** | **Very Low** | Professional RTOS debugging / SystemView. |

---


---

## Memory Allocation: What Happens in RAM?

When you call `xTaskCreate`, the RTOS kernel performs specific memory operations within the system's RAM. Understanding how memory is partitioned is crucial for avoiding crashes like stack overflows or heap exhaustion.

### 1. The RAM Layout
The system's RAM is generally divided into two main regions:
- **Static Memory:** Used for global data, static variables, and arrays defined at compile-time.
- **The Heap:** A pool of memory (defined by `configTOTAL_HEAP_SIZE` in FreeRTOS) used for **dynamically created** kernel objects.

### 2. Task Creation in the Heap
As soon as `xTaskCreate` is called, the kernel carves out two distinct blocks of memory from the Heap for the new task:

| Component | Purpose | Memory Type |
| :--- | :--- | :--- |
| **TCB (Task Control Block)** | A small structure containing the task's metadata (priority, state, name, stack pointer). | Allocated from Heap |
| **Task Stack** | A dedicated area where the task stores its local variables, function return addresses, and CPU register states during a context switch. | Allocated from Heap |

### 3. Other Dynamic Kernel Objects
The same Heap space is used whenever you create other RTOS primitives. Each has its own "Control Block":

- **Semaphores:** The kernel allocates a **Semaphore Control Block (SCB)**.
- **Queues:** The kernel allocates a **Queue Control Block (QCB)** and a separate **Item List** to store the actual data packets.

### Visual Representation of the RTOS Heap

```text
Low Address                                               High Address
+--------------------------------------------------------------------+
|                      SYSTEM RAM (TOTAL)                            |
+----------------------+---------------------------------------------+
|     Static Data      |            RTOS HEAP (Dynamic)              |
| (Globals, Statics)   |        (configTOTAL_HEAP_SIZE)              |
+----------------------+---------------------------------------------+
                       |                                             |
                       |  [Task 1]  TCB1 | STACK 1                   |
                       |  [Task 2]  TCB2 | STACK 2                   |
                       |  [Sem]     SCB                              |
                       |  [Queue]   QCB  | ITEM LIST                 |
                       |  [Free]    Unallocated Memory               |
                       |                                             |
                       +---------------------------------------------+
```

### Key Takeaway
Every time you create a Task, Semaphore, or Queue, the "Free" space in the RTOS Heap shrinks. If the Heap is too small, `xTaskCreate` will fail and return an error code, even if there is plenty of "Static Data" space available.

### Deep Dive: Learning UART through Electricity

This project provides a perfect opportunity to connect the C code in `main.c` to the physical signals observed on a logic analyzer. UART (Universal Asynchronous Receiver-Transmitter) is the "bridge" between our STM32 and the laptop.

#### 1. The Asynchronous Secret: Baud Rate
Unlike protocols like SPI, UART has no "Clock" wire to tell the receiver when to read. Instead, both devices agree on a **Baud Rate**. In our project, we use `115200`.
- **Timing is Everything:** At 115200 bits per second, each single bit lasts exactly **8.68µs**. 
- **Synchronization:** When you tell your logic analyzer the baud rate is 115200, it "knows" to wait for the signal to drop (Start Bit) and then sample the line every 8.68µs to determine if the bit is a 1 (High) or 0 (Low).

#### 2. How "Hello World" Becomes Electricity
When our FreeRTOS tasks call `printf("H")`, the USART2 hardware performs a **Parallel to Serial** conversion:
- **Idle State:** The line sits at Logic HIGH (3.3V). This is the "silence" before the data starts.
- **The Start Bit:** To signal the start of a byte, the STM32 pulls the line LOW for exactly 1 bit-period (8.68µs). This is the "Wake Up" call for the ST-LINK.
- **The Data Bits (The Payload):** The letter 'H' in ASCII is `0x48` (`01001000` in binary). The STM32 flips its internal switches to represent these 1s and 0s as square waves between 0V and 3.3V.
- **The Stop Bit:** After 8 bits, the line returns to HIGH, ensuring it is ready for the next Start Bit.

#### 3. Why CN3 RX? (The "Ear" on the Wire)
This is a common point of confusion:
- The **STM32** is the Transmitter (TX). It "talks" on its TX pin (**PA2**).
- The **ST-LINK** chip (the debugger) is the Receiver (RX). It "listens" on its RX pin.
Because we want to "hear" what the STM32 is saying using the logic analyzer, we must place our probe on the wire the ST-LINK is listening to. That is why we tap the **RX** pin on the **CN3** jumper.

#### 4. UART in the FreeRTOS Lifecycle
In our code, Task-1 and Task-2 blast their messages and then call `vTaskDelay(pdMS_TO_TICKS(1000))`. 
- **On the Analyzer:** You will see a **cluster of waves** (the "Hello World" string) followed by a **long, flat HIGH line**. 
- **The "Silence":** That flat line is the electrical representation of the task's 1-second sleep. While the task is in the **Blocked** state, the UART hardware is idle, and the line stays at 3.3V.

---

## Conclusion

This repository serves as a comprehensive guide to understanding and implementing Real-Time Operating Systems. By exploring hardware platforms like STM32, ESP32, and Raspberry Pi, we've highlighted the critical balance between determinism, processing power, and connectivity.

Through the study of scheduling mechanisms—from preemptive and time-slicing to co-operative policies—we've seen how an RTOS ensures that high-priority tasks meet their deadlines with microsecond precision. We've also delved into the internal mechanics of the kernel, including task state management, memory allocation, and the vital roles of the Idle and Timer Service tasks.

Furthermore, we've compared debugging methodologies, demonstrating why professional tools like SEGGER J-Link and SystemView are essential for visualizing complex system behaviors and resolving issues like priority inversion.

Ultimately, mastering these concepts is fundamental to building reliable, predictable, and high-performance embedded systems. Whether you are controlling a medical device or orchestrating an industrial robot, the principles of RTOS provide the foundation for success in the world of real-time computing.

---

## Project 1: STM32 Nucleo-F103RB RTOS Deployment
**Status:** Tested and Deployed.

This project implements a dual-task FreeRTOS system on the STM32 Nucleo-F103RB, demonstrating real-time multitasking and precise hardware control.

### Accomplishments
- **Environment Setup:** Configured PlatformIO to target the Nucleo-F103RB board with the STM32Cube framework.
- **Hardware Integration:** Initialized USART2 on pins **PA2 (TX)** and **PA3 (RX)** for terminal output.
- **Clock Configuration:** Successfully configured the system to **72MHz** using the HSE (External Crystal) and PLL.
- **Multitasking:** 
    - **Task 1:** Prints "Hello World from Task-1" every 1000ms (Priority 2).
    - **Task 2:** Prints "Hello World from Task-2" every 1000ms (Priority 2).
- **Verification:** Observed Round-Robin scheduling and verified timing via logic analyzer tapping CN3 RX.

### Implementation: Round-Robin Scheduling in Action

We have successfully implemented a textbook example of **Round-Robin Scheduling** by creating two tasks with the exact same priority.

#### The Setup:
- **Task 1 (Priority 2):** Prints "Hello World from Task-1" via USART2.
- **Task 2 (Priority 2):** Prints "Hello World from Task-2" via USART2.
- Both tasks use `vTaskDelay(pdMS_TO_TICKS(1000))` to yield the CPU.

#### Why it Works:
Because both tasks share **Priority 2**, FreeRTOS acts as the ultimate fair referee. Here is exactly what is happening in the terminal:
1. **Task-1** wakes up, prints its message, and tells the OS: *"I'm going to block (sleep) for 1000 ticks. Let someone else use the CPU."*
2. The OS instantly saves Task-1's state, switches context, and hands the CPU over to **Task-2**.
3. **Task-2** prints its message and also goes to sleep for 1000 ticks.
4. The OS idles in the background until those timers expire, and the cycle repeats flawlessly.

This rapid and reliable switching—**the context switch**—is the fundamental heartbeat of advanced embedded engineering. It is the same underlying architecture used in multi-drone flight controllers to juggle sensor reading, spatial positioning, and motor adjustments simultaneously without failure.

### How the Programming Works
1.  **PlatformIO Workflow:** I used the `pio run --target upload` command to automate the compilation and flashing process.
2.  **ST-Link Communication:** The board was programmed via the integrated ST-Link debugger using the **SWD** (Serial Wire Debug) protocol.
3.  **OpenOCD Integration:** PlatformIO invoked OpenOCD to bridge the gap between the binary `firmware.elf` and the STM32's flash memory.

### Behaviour Explanation: The "Slow Blink" Debug
Initially, the LED blinked every ~4.5 seconds instead of 0.5 seconds.
- **Cause:** The RTOS kernel calculated its 1ms "Tick" based on an expected **72MHz** clock (`configCPU_CLOCK_HZ`). However, the hardware was actually running on its internal **8MHz** HSI oscillator.
- **Math:** $\frac{72MHz}{8MHz} = 9$. Thus, every requested delay was 9x slower than intended.
- **Fix:** We implemented `SystemClock_Config()` to explicitly engage the PLL and set the CPU to 72MHz, restoring microsecond-level precision.

1. What is "Flashing"? (And why it’s not like a Pi)
  On a Raspberry Pi 5, you have an SD card or an SSD. When you want to run a program, the
  Linux operating system "loads" that program from the SD card into RAM (temporary memory),
  and then the CPU runs it from there. If you turn off the power, the RAM clears, but the file
  stays on the SD card.


  The STM32 is much simpler. It doesn't have an SD card or a big OS like Linux. Instead, it
  has Internal Flash Memory.
   * Flashing is the process of "burning" your code directly into the chip's permanent memory.
   * Unlike RAM, Flash doesn't disappear when you unplug the power.
   * The moment the STM32 gets power, it looks at a specific spot in that Flash memory and
     starts running whatever is there instantly. There is no "loading" phase.


  2. What is a "Debugger" actually?
  Think of a debugger (like the ST-Link chip on your Nucleo board) as a Security Camera +
  Remote Control for your chip.
   * Without a debugger: You'd just upload code and "hope" it works. If it crashes, you have
     no idea why.
   * With a debugger: You can "pause" the CPU while it's running, look inside to see what the
     variables are currently equal to, and even run the code one single line at a time to see
     exactly where it fails. It is the "bridge" between your computer and the tiny brain of
     the STM32.


  3. What is SWD (Serial Wire Debug)?
  In the old days, debugging required a "JTAG" connection which used 10+ wires. SWD is a
  modern, "diet" version of that created by ARM.
   * It only uses 2 wires (plus ground): SWDIO (Data) and SWCLK (Clock).
   * It is the "language" the debugger uses to talk to the STM32. It’s like a very fast,
     private conversation where the debugger says "Hey, stop the CPU!" and the STM32 obeys.


  4. What is OpenOCD? (The Translator)
  Your computer speaks "USB," and the STM32 speaks "SWD." They don't understand each other
  directly.
   * OpenOCD (Open On-Chip Debugger) is a piece of software running on your PC.
   * It acts as a Translator. It takes the commands from your IDE (like VS Code/PlatformIO)
     and translates them into the specific USB packets that the ST-Link understands, which
     then gets turned into SWD signals for the chip.


  5. What is firmware.elf?
  When you click "Build," the compiler takes your C code and turns it into machine code (1s
  and 0s).
   * The .elf file is like a Grand Blueprint. It contains the actual code, but it also
     contains "extra info" (metadata) that tells the debugger: "This piece of code belongs to
     line 45 of main.c" or "This variable is stored at this specific memory address."
   * The debugger uses the .elf file to show you your C code while the chip is actually just
     running raw numbers.

  6. The "0.5s and 4.5s" Issue (Clock Confusion)
  This is the most common "Welcome to Embedded" headache!


  The Setup:
  Your code told FreeRTOS: "I want to blink the LED every 500ms (0.5 seconds)."
  FreeRTOS looked at its settings and saw a note saying: "The CPU is running at 72MHz (72
  million beats per second)."
  So, FreeRTOS calculated: "Okay, to wait 0.5s, I need to count exactly 36 million beats."

  The Problem:
  However, the chip's "heart" (the clock) was actually beating at its default "safe mode"
  speed of only 8MHz (8 million beats per second).


  The Result:
  The chip started counting those 36 million beats... but because it was beating 9 times
  slower than FreeRTOS thought ($72 \div 8 = 9$), it took 9 times longer to reach that number.
   * $0.5 \text{ seconds} \times 9 = \mathbf{4.5 \text{ seconds}}$.


  The Fix:
  You had to write a piece of code (SystemClock_Config) that basically tells the STM32: "Hey,
  stop being lazy! Turn on your high-speed oscillators and run at 72MHz like I told you." Once
  the heart started beating at the speed the software expected, the 0.5s delay became a real
  0.5s delay.

---

## Idle Task and Time Service Task

Perfect. Let’s shift from the "wiring and drivers" nightmare to the actual "brain" of FreeRTOS. Switching to A Plague Tale is a great move—Hugo and Amicia definitely deal with better scheduling than we just did with those USB drivers!

Think of FreeRTOS as a Manager in a busy office. You've already met the workers (Task 1 and Task 2), but the Manager has two "Secret Assistants" that keep the office from collapsing.

### 1. The Idle Task (The "Janitor")

**In Layman's Terms:**
Imagine a shop where customers (Tasks) come and go. When there are no customers, the staff doesn't just vanish into thin air. Someone has to stay behind to sweep the floors, lock the back door, and wait for the next customer.
The Idle Task is the Janitor. It only works when everyone else is "Sleeping" or "Waiting."

**Going Deeper:**
The Idle Task is automatically created by the kernel when you call `vTaskStartScheduler()`. It has the lowest possible priority (0).
- **Memory Cleanup:** If you delete a task, the Idle Task is the one that actually goes and frees up that "RAM" (stack/TCB) so it can be used again.
- **Power Saving:** This is where the magic happens for battery life. You can tell the Idle Task to put the CPU into "Sleep Mode" until the next interrupt happens.

**Practical Experiment:**
You can actually "catch" the Idle Task working! FreeRTOS has an Idle Hook function. If you enable it, the CPU will jump into this function every time it has nothing else to do.

Try this in your code:
1. Set `configUSE_IDLE_HOOK` to 1 in `FreeRTOSConfig.h`.
2. Add this to your `main.c`:

```c
void vApplicationIdleHook(void) {
    // This runs ONLY when Task 1 and Task 2 are in vTaskDelay
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // Blink the LED super fast
}
```

If you look at your logic analyzer now, you’ll see the LED toggling like crazy during those 1000ms gaps between your "Hello World" prints!

### 2. The Timer Service Task (The "Alarm Clock")

**In Layman's Terms:**
Imagine you tell a friend, "Hey, remind me to check the oven in 10 minutes." You don't want to sit there staring at your watch for 10 minutes—you want to go watch TV.
The Timer Service Task is that friend. It sits in the corner, watches the clock, and "pokes" you when it’s time to do something.

**Going Deeper:**
Also known as the Daemon Task. It handles "Software Timers."
- It doesn't run based on code logic; it runs based on the System Tick.
- When you create a software timer (e.g., `xTimerCreate`), you aren't creating a new task. You are just adding a note to the Timer Service Task’s "To-Do List."

**Practical Experiment:**
Software timers are better than tasks for simple, repetitive things (like blinking an LED or checking a sensor) because they save RAM.

Try replacing one of your tasks with a Timer:
```c
// 1. Define the callback
void myTimerCallback(TimerHandle_t xTimer) {
    printf("Timer Fired! No task needed.\n");
}

// 2. In main(), before starting scheduler:
TimerHandle_t myTimer = xTimerCreate("Blinker", pdMS_TO_TICKS(500), pdTRUE, 0, myTimerCallback);
xTimerStart(myTimer, 0);
```

### The "Practical" Difference

| Feature | Idle Task | Timer Task |
| :--- | :--- | :--- |
| **When does it run?** | When NOTHING else can run. | When a specific TIME has passed. |
| **Priority** | Always 0 (Lowest). | Configurable (usually high). |
| **Who creates it?** | The Kernel (Automatic). | The Kernel (if `configUSE_TIMERS` is 1). |
| **Main Job** | Cleanup & Power Saving. | Executing "Delayed" functions. |

---

## Advanced Profiling: Visualizing the "Heartbeat"

We have recently "powered up" the STM32 to go beyond simple text logs. By using a Logic Analyzer and custom GPIO triggers, we can now see the exact microsecond when tasks hand over the CPU to each other.

### 1. Task Profiling with GPIO (The "D2" Trigger)
In `main.c`, we've added a **Profiler Pin** on **PA10**. This acts as a physical "flag" for Task 1:
- **Rising Edge:** Task 1 takes control of the CPU.
- **Falling Edge:** Task 1 finishes its work (printing "Hello World") and enters the `vTaskDelay` state.

```c
void vTask1_Handler(void *params) {
    while(1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET); // START PROFILING
        printf("Hello World from Task-1\r\n");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET); // STOP PROFILING
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

### 2. Measuring the Idle Gap (The "974ms to 976ms" Window)
By enabling the **Idle Hook** (`configUSE_IDLE_HOOK 1`), we can observe exactly how much "free time" the CPU has between tasks. We've configured the Idle Hook to toggle the Nucleo's Green LED (**PA5**) as fast as possible.

**Observed Timing on Logic Analyzer:**
- **Task Execution Time:** When Task 1 and Task 2 are printing to UART, they hog the CPU for roughly **2-4ms**.
- **The Idle Recovery:** You observed the Idle Task "opening" (starting its toggle) at **974ms** and "closing" (preempted by the next tick) at **976ms**. 
- **The Verdict:** This confirms that out of every 1000ms cycle, the CPU is effectively "Idle" for ~97.5% of the time, proving that FreeRTOS is successfully managing the power and processing efficiency of the chip.

### 3. Summary of Profiling Pins
| Hardware Pin | Function | Logic High | Logic Low |
| :--- | :--- | :--- | :--- |
| **PA10 (Profiler)** | Task 1 State | Task 1 is **Running** | Task 1 is **Blocked/Delayed** |
| **PA5 (Green LED)** | CPU Idle State | CPU is **Free** (Idle Hook) | CPU is **Busy** (Task 1 or 2) |

This setup allows us to "see" the RTOS scheduler working in real-time, providing empirical proof of the timing math we discussed earlier!
