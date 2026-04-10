#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// extern float glob_temperature;
// extern float glob_humidity;

typedef struct {
    float temperature;
    float humidity;
    // Mutex to protect concurrent read/write access to temp/humidity
    SemaphoreHandle_t dataMutex;         
    // Binary Semaphores for Task Synchronization
    SemaphoreHandle_t ledSemaphore;      // Triggered on Temperature changes
    SemaphoreHandle_t neoSemaphore;      // Triggered on Humidity changes
    SemaphoreHandle_t lcdSemaphore;      // Triggered on Display State changes
} SystemData_t;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
#endif