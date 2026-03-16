#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* Task handlers */
void vTask1_Handler(void *params);
void vTask2_Handler(void *params);

int main(void) {
    HAL_Init();
    
    /* Create Task 1 (Priority 2) */
    xTaskCreate(vTask1_Handler, "Task-1", 128, NULL, 2, NULL);
    
    /* Create Task 2 (Priority 2) */
    xTaskCreate(vTask2_Handler, "Task-2", 128, NULL, 2, NULL);
    
    /* Start the FreeRTOS Scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    for(;;);
    
    return 0;
}

void vTask1_Handler(void *params) {
    (void)params;
    
    while(1) {
        /* Task 1 Logic */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTask2_Handler(void *params) {
    (void)params;
    
    while(1) {
        /* Task 2 Logic */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}