#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* --- Global Variables --- */
UART_HandleTypeDef huart2;
volatile uint8_t button_flag = 0; // <--- Our shared RTOS flag!

/* --- Function Prototypes --- */
void SystemClock_Config(void);
void GPIO_Init(void);           
void USART2_Init(void);
void button_task(void *params); // <--- New Prototype
void led_task(void *params);    // <--- New Prototype

void vApplicationIdleHook(void) {
    // We removed the LED toggle here so it doesn't fight our led_task!
}

void vApplicationTickHook(void) {
    // Still required by FreeRTOSConfig.h, but kept empty.
}

int main(void) {
    /* 1. Reset all peripherals, initialize Flash interface and Systick. */
    HAL_Init();

    /* 2. Configure system clock to 72 MHz */
    SystemClock_Config();

    /* 3. Initialize GPIO (LED and Button) */
    GPIO_Init();                

    /* 4. Initialize UART for printing */
    USART2_Init();
    printf("\r\n--- RTOS Button & LED Exercise Started ---\r\n");

    /* 5. Create Button Task (Priority 2) */
    xTaskCreate(button_task, "Button-Task", 256, NULL, 2, NULL);

    /* 6. Create LED Task (Priority 2) */
    xTaskCreate(led_task, "LED-Task", 256, NULL, 2, NULL);

    /* 7. Start the FreeRTOS Scheduler */
    vTaskStartScheduler();

    /* We should never reach here */
    for(;;);
    
    return 0;
}

/* --- Task 1: Button Poller --- */
void button_task(void *params) {
    (void)params;
    
    while(1) {
        // Read PC13. Nucleo buttons read 0 (RESET) when pressed!
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
            button_flag = 1; // SET the flag
        } else {
            button_flag = 0; // CLEAR the flag
        }
        
        // Block for 10ms so we don't hog the CPU
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

/* --- Task 2: LED Controller --- */
void led_task(void *params) {
    (void)params;
    
    while(1) {
        // Check the shared flag updated by the button task
        if (button_flag == 1) {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);   // Turn LED ON
        } else {
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); // Turn LED OFF
        }
        
        // Block for 10ms
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* --- Retarget printf to UART --- */
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* --- Hardware Initialization --- */
void GPIO_Init(void) {
    // Turn on clocks for Port A (LED) and Port C (Button)
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE(); // <--- ADDED Port C Clock

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure PA5 (Nucleo Green LED) as Push-Pull Output
    GPIO_InitStruct.Pin = GPIO_PIN_5; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure PC13 (Blue User Button) as Input
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // Nucleo board has a physical external pull-up
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
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