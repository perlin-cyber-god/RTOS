#include <Arduino.h>

// --- Hardware Pin Definitions ---
const int BUTTON_PIN = 2; // Connect button between Pin 2 and Ground
const int LED_PIN = 13;   // Built-in LED pin on many boards

// --- Task Handle ---
// We need this handle so the Button Task knows exactly who to notify
TaskHandle_t TaskLEDHandle = NULL;

// --- Task 1: The Button Task ---
void taskButton(void *pvParameters) {
    bool lastButtonState = HIGH; // Assuming INPUT_PULLUP (HIGH = unpressed)
    
    for (;;) {
        bool currentButtonState = digitalRead(BUTTON_PIN);
        
        // Check for a falling edge (button was just pressed down)
        if (lastButtonState == HIGH && currentButtonState == LOW) {
            
            // 1. Notify the LED task that a press happened
            xTaskNotifyGive(TaskLEDHandle);
            
            // 2. Simple debounce delay to prevent multi-counting a single physical press
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }
        
        lastButtonState = currentButtonState;
        
        // Yield the CPU briefly so the watchdog timer doesn't crash the system
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

// --- Task 2: The LED Task ---
void taskLED(void *pvParameters) {
    uint32_t pressCount = 0;
    bool ledState = LOW;
    
    for (;;) {
        // This is the magic line. It places the task in the BLOCKED state 
        // indefinitely (portMAX_DELAY) until a notification arrives. 
        // It consumes zero CPU cycles while waiting.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // --- If the code reaches here, a notification was received! ---
        
        // 1. Increment the press counter
        pressCount++;
        
        // 2. Toggle the LED
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        
        // 3. Print the results
        Serial.print("Button pressed! Total times so far: ");
        Serial.println(pressCount);
    }
}

// --- Main Setup ---
void setup() {
    Serial.begin(115200);
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    // Create the LED Task
    // Priority is set to 1. The handle is saved to TaskLEDHandle.
    xTaskCreate(
        taskLED,          // Function that implements the task
        "TaskLED",        // Text name for the task
        2048,             // Stack size (in words)
        NULL,             // Task input parameter
        1,                // Priority (1 is equal to Button Task)
        &TaskLEDHandle    // Task handle
    );

    // Create the Button Task
    // Priority is also set to 1. No handle needed since nobody notifies this task.
    xTaskCreate(
        taskButton,       // Function that implements the task
        "TaskButton",     // Text name for the task
        2048,             // Stack size (in words)
        NULL,             // Task input parameter
        1,                // Priority (1 is equal to LED Task)
        NULL              // Task handle
    );
}

void loop() {
    // In a pure FreeRTOS environment, the loop is empty.
    // The RTOS scheduler takes over entirely.
}