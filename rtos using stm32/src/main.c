#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "timers.h"  

/* Global Handlers */
UART_HandleTypeDef huart2;

/* Function Prototypes */
void SystemClock_Config(void);
void USART2_Init(void);
void vTask1_Handler(void *params);
void vTask2_Handler(void *params);

void vApplicationIdleHook(void) {
    // This runs ONLY when Task 1 and Task 2 are in vTaskDelay
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // Blink the LED super fast
}

void myTimerCallback(TimerHandle_t xTimer) {
    printf("Timer Fired! No task needed.\n");
}

int main(void) {
    /* 1. Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* 2. Configure the system clock to 72 MHz */
    SystemClock_Config();

    /* 3. Initialize the UART for printing (Case 2) */
    USART2_Init();

    /* 4. Create Task 1 (Priority 2) */
    xTaskCreate(vTask1_Handler, "Task-1", 256, NULL, 2, NULL);

    /* 5. Create Task 2 (Priority 2) - Same priority to force Round-Robin! */
    xTaskCreate(vTask2_Handler, "Task-2", 256, NULL, 2, NULL);

    TimerHandle_t myTimer = xTimerCreate("Blinker", pdMS_TO_TICKS(500), pdTRUE, 0, myTimerCallback);
xTimerStart(myTimer, 0);

    /* 6. Start the FreeRTOS Scheduler */
    vTaskStartScheduler();

    /* We should never reach here as control is now taken by the scheduler */
    for(;;);
    
    return 0;
}

/* --- Task 1 --- */
void vTask1_Handler(void *params) {
    (void)params;
    
    while(1) {
        printf("Hello World from Task-1\r\n");
        // Block for 1 second (1000 ticks) so Task 2 can have a turn
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* --- Task 2 --- */
void vTask2_Handler(void *params) {
    (void)params;
    
    while(1) {
        printf("Hello World from Task-2\r\n");
        // Block for 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* --- Retarget printf to UART --- */
// When you call printf, it secretly calls _write. We are hijacking it here!
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* --- Hardware Initialization --- */
void USART2_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    // PA2 is TX (Transmit), PA3 is RX (Receive)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; // Alternate Function Push Pull
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure UART parameters (115200 baud is standard)
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

    // 8MHz HSE to 72MHz SYSCLK using PLL x9
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}