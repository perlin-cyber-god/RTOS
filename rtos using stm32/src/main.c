#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

/* Task handlers */
void vTask1_Handler(void *params);
void vTask2_Handler(void *params);
void GPIO_Init(void);
void SystemClock_Config(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    
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

void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// THE INSTRUCTOR'S GOAL
void vTask1_Handler(void *params) {
    while(1) {
        printf("Hello World from Task-1\r\n"); // Printing to UART console
        vTaskDelay(pdMS_TO_TICKS(1000));       // Same delay for both tasks
    }
}

void vTask2_Handler(void *params) {
    (void)params;
    
    while(1) {
        /* Task 2 Logic */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}