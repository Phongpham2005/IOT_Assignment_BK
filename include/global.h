#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

extern float glob_temperature;
extern float glob_humidity;
extern bool humidity_red_zone;
extern bool temperature_red_zone;
extern bool neo_enabled;
extern bool blinky_enabled;

extern bool fan_auto_mode;
extern bool fan_state;
#define FAN_PIN 6

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

// Semaphores cho LED tasks
extern SemaphoreHandle_t ledSemaphore;   // Sensor -> led_blinky
extern SemaphoreHandle_t neoSemaphore;   // Sensor -> neo_blinky
extern SemaphoreHandle_t dataMutex;      // Bảo vệ glob_temperature & glob_humidity
#endif