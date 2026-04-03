#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_rcc.h"

/* Private variables ---------------------------------------------------------*/
// Global handle so the EXTI hardware interrupt can target Task 2
TaskHandle_t xTask2Handle = NULL;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
void vTask1_Worker(void *pvParameters);
void vTask2_VIP(void *pvParameters);

/* USER CODE BEGIN 0 */

/*
 * Hardware Interrupt Service Routine Callback
 * This fires instantly when the physical button (PC13) is pressed.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Check if the interrupt came from the User Button on PC13
    if(GPIO_Pin == GPIO_PIN_13) 
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Ensure Task 2 actually exists before trying to notify it
        if(xTask2Handle != NULL) 
        {
            // Send the notification from the ISR safely
            vTaskNotifyGiveFromISR(xTask2Handle, &xHigherPriorityTaskWoken);
            
            // Force an immediate context switch if waking Task 2 preempts Task 1
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();

  /* USER CODE BEGIN 2 */
  
  // Create the Tasks. 
  // Task 2 has Priority 2 (Higher). Task 1 has Priority 1 (Lower).
  xTaskCreate(vTask2_VIP,    "Task2", 128, NULL, 2, &xTask2Handle);
  xTaskCreate(vTask1_Worker, "Task1", 128, NULL, 1, NULL);

  // Start the FreeRTOS Scheduler
  vTaskStartScheduler();

  /* USER CODE END 2 */

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
  }
}

/* USER CODE BEGIN 4 */

/*
 * Task 2: The VIP (Priority 2)
 * Toggles the LED every 1000ms. Deletes itself if notified by the Button ISR.
 */
void vTask2_VIP(void *pvParameters)
{
    uint32_t ulNotificationValue;

    for(;;)
    {
        ulNotificationValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

        if(ulNotificationValue > 0)
        {
            // BUTTON PRESSED! 
            // 1. Immediately disable the button's hardware interrupt to ignore bounces!
            HAL_NVIC_DisableIRQ(EXTI15_10_IRQn); 
            
            // 2. Safely clear the handle
            xTask2Handle = NULL; 
            
            // 3. Safely commit suicide
            vTaskDelete(NULL);   
        }
        else
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        }
    }
}

/*
 * Task 1: The Worker (Priority 1)
 * Toggles the LED every 200ms. Only gets to run when Task 2 is asleep or deleted.
 */
void vTask1_Worker(void *pvParameters)
{
    for(;;)
    {
        // Toggle the exact same LED
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        
        // Go to sleep for 200ms, handing CPU control back to the RTOS
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* USER CODE END 4 */

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 (User Button) */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Trigger on press
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 (User LED) */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0); // Ensure priority is safe for FreeRTOS
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void SystemClock_Config(void)
{
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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    // Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    // Error_Handler();
  }
}

void vApplicationIdleHook(void)
{
    // Idle hook - called when no tasks are running
}

void vApplicationTickHook(void)
{
    // Tick hook - called every tick
}