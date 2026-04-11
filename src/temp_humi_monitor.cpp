#include "temp_humi_monitor.h"
#include <Wire.h>
DHT20 dht20;
LiquidCrystal_I2C lcd(33, 16, 2);

void temp_humi_monitor(void *pvParameters) {

  Wire.begin(11, 12);
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  Serial.begin(115200);
  dht20.begin();

  while (1) {
    dht20.read();
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();

    bool dht_failed = isnan(temperature) || isnan(humidity);
    if (dht_failed) {
      Serial.println("Failed to read from DHT sensor!");
      temperature = humidity = -1;
    }

    // Ghi dữ liệu vào biến toàn cục có bảo vệ bằng Mutex
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      glob_temperature = temperature;
      glob_humidity = humidity;
      humidity_red_zone = (!dht_failed && humidity > 55.0);
      temperature_red_zone = (!dht_failed && temperature > 45.0);
      xSemaphoreGive(dataMutex);
    }
    bool humidity_yellow_zone = (!dht_failed && humidity < 35.0);

    // Thông báo cho LED tasks rằng có dữ liệu mới
    xSemaphoreGive(ledSemaphore);
    xSemaphoreGive(neoSemaphore);

    // Print the results on Serial
    if (dht_failed) {
      Serial.println("Humidity: --%  Temperature: --°C");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print("%  Temperature: ");
      Serial.print(temperature);
      Serial.println("°C");
    }

    lcd.clear();
    static bool alternate_red_message = false;
    if (dht_failed) {
      lcd.setCursor(0, 0);
      lcd.print("DHT20 read error");
      lcd.setCursor(0, 1);
      lcd.print("Check sensor");
    } else if (humidity_red_zone && temperature_red_zone) {
      // Cả 2 đều đỏ — xen kẽ hiển thị
      alternate_red_message = !alternate_red_message;
      if (alternate_red_message) {
        lcd.setCursor(0, 0);
        lcd.print("HUMID TOO HIGH");
        lcd.setCursor(0, 1);
        lcd.print("Hum:");
        lcd.print(humidity, 1);
        lcd.print("%");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("TEMP TOO HOT");
        lcd.setCursor(0, 1);
        lcd.print("Temp:");
        lcd.print(temperature, 1);
        lcd.print((char)223);
        lcd.print("C");
      }
    } else if (humidity_yellow_zone && temperature_red_zone) {
      // Humidity thấp + nhiệt độ cao — xen kẽ hiển thị
      alternate_red_message = !alternate_red_message;
      if (alternate_red_message) {
        lcd.setCursor(0, 0);
        lcd.print("HUMID TOO LOW");
        lcd.setCursor(0, 1);
        lcd.print("Hum:");
        lcd.print(humidity, 1);
        lcd.print("%");
      } else {
        lcd.setCursor(0, 0);
        lcd.print("TEMP TOO HOT");
        lcd.setCursor(0, 1);
        lcd.print("Temp:");
        lcd.print(temperature, 1);
        lcd.print((char)223);
        lcd.print("C");
      }
    } else if (humidity_red_zone) {
      lcd.setCursor(0, 0);
      lcd.print("HUMID TOO HIGH");
      lcd.setCursor(0, 1);
      lcd.print("Hum:");
      lcd.print(humidity, 1);
      lcd.print("%");
    } else if (temperature_red_zone) {
      lcd.setCursor(0, 0);
      lcd.print("TEMP TOO HOT");
      lcd.setCursor(0, 1);
      lcd.print("Temp:");
      lcd.print(temperature, 1);
      lcd.print((char)223);
      lcd.print("C");
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

    vTaskDelay(2415 / portTICK_PERIOD_MS);
  }
}