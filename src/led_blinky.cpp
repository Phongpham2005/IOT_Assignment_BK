#include "led_blinky.h"

void led_blinky(void *pvParameters) {
    SystemData_t *sysData = (SystemData_t *)pvParameters;
    pinMode(LED_GPIO, OUTPUT);
    uint32_t blinkDelay = 1000; // Default

    while (1) {
        // Non-blocking semaphore take (Timeout = 0). 
        // If Sensor Task gave the semaphore, update the blink speed.
        if (xSemaphoreTake(sysData->ledSemaphore, 0) == pdTRUE) {
            float temp;
            if (xSemaphoreTake(sysData->dataMutex, portMAX_DELAY)) {
                temp = sysData->temperature;
                xSemaphoreGive(sysData->dataMutex);
            }

            // TASK 1: 3 different behaviors based on temperature conditions
            if (temp < 25.0) {
                blinkDelay = 2000; // Cold: Slow blink
            } else if (temp >= 25.0 && temp < 30.0) {
                blinkDelay = 500;  // Optimal: Fast blink
            } else {
                blinkDelay = 100;  // Hot: Very fast blink
            }
        }

        // Execute the blink behavior
        digitalWrite(LED_GPIO, HIGH);
        vTaskDelay(pdMS_TO_TICKS(blinkDelay));
        digitalWrite(LED_GPIO, LOW);
        vTaskDelay(pdMS_TO_TICKS(blinkDelay));
    }
}