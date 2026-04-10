#include "neo_blinky.h"


void neo_blinky(void *pvParameters){
    SystemData_t *sysData = (SystemData_t *)pvParameters;
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set all pixels to off to start
    strip.clear();
    strip.show();

    uint32_t neoColor = strip.Color(0, 255, 0); 
    uint32_t blinkDelay = 0;
    bool isLedOn = true;

    while (1) {
        // Block task until the Sensor Task notifies a humidity change
        if (xSemaphoreTake(sysData->neoSemaphore, portMAX_DELAY)) {
            float humi;
            if (xSemaphoreTake(sysData->dataMutex, portMAX_DELAY)) {
                humi = sysData->humidity;
                xSemaphoreGive(sysData->dataMutex);
            }

            // TASK 2: Mapping humidity levels to 3 colors
            if (humi < 40.0) {
                // Level 1: Dry -> Orange
                neoColor = strip.Color(255, 100, 0);
                blinkDelay = 200; 
            } else if (humi >= 40.0 && humi <= 70.0) {
                // Level 2: Optimal -> Green
                neoColor = strip.Color(0, 255, 0);
                blinkDelay = 0;   
            } else {
                // Level 3: Humid -> Blue
                neoColor = strip.Color(0, 0, 255);
                blinkDelay = 1000;  
            }
            if (blinkDelay == 0) {
                strip.setPixelColor(0, neoColor);
                strip.show();
                vTaskDelay(pdMS_TO_TICKS(100)); 
            } else {
                if (isLedOn) {
                    strip.setPixelColor(0, 0); 
                } else {
                    strip.setPixelColor(0, neoColor); 
                }
                strip.show();
                isLedOn = !isLedOn;
                vTaskDelay(pdMS_TO_TICKS(blinkDelay));
            }
        }
    }
}