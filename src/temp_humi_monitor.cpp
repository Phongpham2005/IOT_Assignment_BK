#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void temp_humi_monitor(void *pvParameters){
    SystemData_t *sysData = (SystemData_t *)pvParameters;
    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();
    lcd.begin(); 
    lcd.backlight();

    float lastTemp = -100.0;
    float lastHumi = -100.0;
    while (1){
        /* code */
        
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        bool dht_failed = isnan(temperature) || isnan(humidity);
        bool temperature_red_zone = (temperature > 30.0);
        bool humidity_blue_zone = (humidity > 70.0);
        bool humidity_yellow_zone = (humidity < 40.0);
        

        // Check if any reads failed and exit early
        if (dht_failed) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            xSemaphoreGive(sysData->lcdSemaphore);
            //return;
        } else {
            if (xSemaphoreTake(sysData->dataMutex, portMAX_DELAY)) {
                sysData->temperature = temperature;
                sysData->humidity = humidity;
                xSemaphoreGive(sysData->dataMutex);
            }
            if (abs(temperature - lastTemp) > 0.2) {
                xSemaphoreGive(sysData->ledSemaphore);
                lastTemp = temperature;
            }
            if (abs(humidity - lastHumi) > 1.0) {
                xSemaphoreGive(sysData->neoSemaphore);
                lastHumi = humidity;
            }
            xSemaphoreGive(sysData->lcdSemaphore);
        }

        // //Update global variables for temperature and humidity
        // glob_temperature = temperature;
        // glob_humidity = humidity;

        // xSemaphoreGive(xHumiditySemaphore);

        //Print the results
        if (dht_failed) {
            Serial.println("Humidity: --%  Temperature: --°C");
        } else {
            Serial.print("Humidity: ");
            Serial.print(humidity);
            Serial.print("%  Temperature: ");
            Serial.print(temperature);
            Serial.println("°C");
        }

        
        if (xSemaphoreTake(sysData->lcdSemaphore, 0) == pdTRUE) {
            lcd.clear();
            static bool alternate_message = false;
            
            if (dht_failed) {
                lcd.setCursor(0, 0);
                lcd.print("DHT20 read error");
                lcd.setCursor(0, 1);
                lcd.print("Check sensor");
            } else if (humidity_blue_zone && temperature_red_zone) {
                alternate_message = !alternate_message;
                if (alternate_message) {
                    lcd.setCursor(0, 0);
                    lcd.print("HUMID TOO HIGH");
                } else {
                    lcd.setCursor(0, 0);
                    lcd.print("TEMP TOO HOT");
                }
            } else if (humidity_blue_zone) {
                lcd.setCursor(0, 0);
                lcd.print("HUMID TOO HIGH");
            } else if (temperature_red_zone) {
                lcd.setCursor(0, 0);
                lcd.print("TEMP TOO HOT");
            } else if (humidity_yellow_zone) {
                lcd.setCursor(0, 0);
                lcd.print("HUMID TOO LOW");
                lcd.setCursor(0, 1);
                lcd.print("Hum:");
                lcd.print(humidity, 1);
                lcd.print("%");
            } else {
                lcd.setCursor(0, 0);
                lcd.print("Temp:");
                lcd.print(temperature, 1);
                lcd.print((char)223);
                lcd.print("C");
                lcd.setCursor(0, 1);
                lcd.print("Hum:");
                lcd.print(humidity, 1);
                lcd.print("%");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}