/* ====================================================================
 * ZONE 1: INCLUDES
 * ==================================================================== */
#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* ====================================================================
 * ZONE 2: GLOBAL VARIABLES & PROTOTYPES
 * ==================================================================== */
UART_HandleTypeDef huart2;

// Hardware Setup Prototypes
void SystemClock_Config(void);
void GPIO_Init(void);           
void USART2_Init(void);

// Task Prototypes (The Instruction Manuals)
void task_A(void *params);
void task_B(void *params);
void task_C(void *params);

/* ====================================================================
 * ZONE 3: THE FREE-RTOS HOOKS & HANDLERS (The Traffic Cops)
 * ==================================================================== */
extern void xPortSysTickHandler(void);

// Our custom SysTick handler that prevents the 1ms startup crash
void SysTick_Handler(void) {
    HAL_IncTick(); 
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
    }
}

// The Janitor: Runs ONLY when all other tasks are asleep (Priority 0)
void vApplicationIdleHook(void) {
    // Pulse PA8 HIGH then LOW instantly. 
    // This creates a dense "comb" on the logic analyzer to prove the CPU is idling!
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

// Required by FreeRTOSConfig.h, but we leave it empty
void vApplicationTickHook(void) {
}

/* ====================================================================
 * ZONE 4: THE MAIN FUNCTION (The Manager's Office)
 * ==================================================================== */
int main(void) {
    /* --- 1. Boot up the hardware --- */
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();                
    USART2_Init();
    
    printf("\r\n--- Logic Analyzer Profiling Test Starting ---\r\n");

    /* --- 2. Hire the Workers --- */
    // All workers get the exact same Priority (2)
    xTaskCreate(task_A, "TaskA", 128, NULL, 2, NULL);
    xTaskCreate(task_B, "TaskB", 128, NULL, 2, NULL);
    xTaskCreate(task_C, "TaskC", 128, NULL, 2, NULL);

    /* --- 3. Lock the doors and start the OS --- */
    vTaskStartScheduler();

    /* --- 4. The Black Hole --- */
    while(1);
    
    return 0;
}

/* ====================================================================
 * ZONE 5: THE TASK FUNCTIONS (The Workers)
 * ==================================================================== */

void task_A(void *params) {
    while(1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   // D0 High
        printf("Task A is working...\r\n");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // D0 Low
        
        vTaskDelay(pdMS_TO_TICKS(2000));                      // Sleep 2000ms
    }
}

void task_B(void *params) {
    while(1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);   // D1 High
        printf("Task B is working...\r\n");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); // D1 Low
        
        vTaskDelay(pdMS_TO_TICKS(2000));                      // Sleep 2000ms
    }
}

void task_C(void *params) {
    while(1) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);   // D2 High
        printf("Task C is working...\r\n");
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET); // D2 Low
        
        vTaskDelay(pdMS_TO_TICKS(2000));                      // Sleep 2000ms
    }
}

/* ====================================================================
 * ZONE 6: HELPER FUNCTIONS
 * ==================================================================== */
// This links 'printf' directly to our UART cable
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* ====================================================================
 * ZONE 7: HARDWARE INITIALIZATION (The Boring Silicon Stuff)
 * ==================================================================== */

void GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure PA5 (D0), PA6 (D1), PA7 (D2), and PA8 (D3)
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void USART2_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Using the internal HSI clock (The fix we applied earlier!)
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16; 
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}