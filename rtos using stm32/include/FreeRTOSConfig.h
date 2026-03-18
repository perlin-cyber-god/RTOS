#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// Core settings
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     1
#define configUSE_TICK_HOOK                     1
#define configCPU_CLOCK_HZ                      ( 72000000UL ) // 72MHz for STM32F103
#define configTICK_RATE_HZ                      ( ( TickType_t ) 1000 )
#define configMAX_PRIORITIES                    ( 5 )
#define configMINIMAL_STACK_SIZE                ( ( unsigned short ) 128 )
#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 10 * 1024 ) )
#define configMAX_TASK_NAME_LEN                 ( 16 )
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1

// Enable the Timer Service Task
#define configUSE_TIMERS                    0

// Give the Timer Task a priority (usually higher than normal tasks)
#define configTIMER_TASK_PRIORITY           ( 2 )

// The Timer Task needs a "To-Do List" length. How many timers can be queued?
#define configTIMER_QUEUE_LENGTH            10

// How much memory (RAM) should the Timer Task get?
#define configTIMER_TASK_STACK_DEPTH        128

// Interrupt priority settings for ARM Cortex-M3
#define configKERNEL_INTERRUPT_PRIORITY         255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    191 // Priority 6 (0xC0 >> 4)

// Set the following definitions to 1 to include the API function, or zero
// to exclude the API function.
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1

// STM32 specific: Map FreeRTOS handlers to STM32 vector table
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */

