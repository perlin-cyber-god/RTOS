#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

// --- UART Handle for Printing ---
UART_HandleTypeDef huart2;

// --- Task Handle ---
TaskHandle_t TaskLEDHandle = NULL;

// --- Function Prototypes ---
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void taskButton(void *pvParameters);
void taskLED(void *pvParameters);

// --- Printf Redirect for UART ---
// This allows you to use standard printf() to send data over the ST-Link USB
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

// --- Main Application ---
int main(void) {
    // 1. Reset of all peripherals, Initializes the Flash interface and the Systick.
    HAL_Init();

    // 2. Configure the system clock to 72 MHz
    SystemClock_Config();

    // 3. Initialize all configured peripherals
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    // 4. Create the FreeRTOS Tasks
    // Note: Stack size is in words (e.g., 256 words = 1024 bytes)
    xTaskCreate(taskLED, "TaskLED", 256, NULL, 1, &TaskLEDHandle);
    xTaskCreate(taskButton, "TaskButton", 256, NULL, 1, NULL);

    // 5. Start the RTOS scheduler
    vTaskStartScheduler();

    // We should never get here as control is now taken by the scheduler
    while (1) {
    }
}

// --- Task 1: The Button Task ---
void taskButton(void *pvParameters) {
    // Nucleo User Button on PC13 is usually pulled HIGH by default, goes LOW when pressed.
    GPIO_PinState lastButtonState = GPIO_PIN_SET; 
    
    for (;;) {
        GPIO_PinState currentButtonState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
        
        // Check for a falling edge (button was just pressed down)
        if (lastButtonState == GPIO_PIN_SET && currentButtonState == GPIO_PIN_RESET) {
            
            // Notify the LED task
            xTaskNotifyGive(TaskLEDHandle);
            
            // Debounce delay
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }
        
        lastButtonState = currentButtonState;
        vTaskDelay(pdMS_TO_TICKS(10)); // Yield CPU
    }
}

// --- Task 2: The LED Task ---
void taskLED(void *pvParameters) {
    uint32_t pressCount = 0;
    
    for (;;) {
        // Block indefinitely until a notification arrives from the Button Task
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // A notification was received!
        pressCount++;
        
        // Toggle the Nucleo User LED on PA5
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        
        // Print to the serial monitor
        printf("Button pressed! Total times so far: %lu\r\n", pressCount);
    }
}

// --- FreeRTOS Hooks ---
// The FreeRTOSConfig.h file expects these to exist. 
// We are leaving them empty since we don't need them for this project.

void vApplicationIdleHook(void) {
    // This runs when the CPU has absolutely nothing else to do.
}

void vApplicationTickHook(void) {
    // This tells the native STM32 HAL that time is still moving forward
    HAL_IncTick(); 
}


// ============================================================================
// STM32 HARDWARE INITIALIZATION BOILERPLATE BELOW
// ============================================================================

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Initializes the RCC Oscillators according to the specified parameters
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    // Initializes the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_USART2_UART_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200; // Matches your platformio.ini monitor_speed
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configure GPIO pin Output Level for LED
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    // Configure GPIO pin : PC13 (User Button)
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // Nucleo board has an external pull-up for B1
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure GPIO pins : PA2 (TX) and PA3 (RX) for USART2
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // Configure GPIO pin : PA5 (User LED)
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}