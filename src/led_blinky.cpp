#include "led_blinky.h"

void led_blinky(void *pvParameters) {
  pinMode(LED_GPIO, OUTPUT);

  // Mặc định: giả sử bình thường (< 35°C)
  uint32_t onDelay  = 500;
  uint32_t offDelay = 0;   // 0 = không tắt (sáng liên tục trong khoảng delay)

  while (1) {
    if (!blinky_enabled) {
      digitalWrite(LED_GPIO, LOW);
      vTaskDelay(pdMS_TO_TICKS(500));
      continue;
    }

    // Non-blocking semaphore take (timeout = 0).
    // Nếu Sensor Task đã give semaphore -> cập nhật tốc độ nhấp nháy.
    if (xSemaphoreTake(ledSemaphore, 0) == pdTRUE) {
      float temp = 0;

      // Đọc nhiệt độ có bảo vệ bằng Mutex
      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        temp = glob_temperature;
        xSemaphoreGive(dataMutex);
      }

      if (temp < 0 || isnan(temp)) {
        // Dữ liệu không hợp lệ: tắt đèn tạm thời
        onDelay  = 0;
        offDelay = 500;
      } else if (temp < 35.0) {
        // Bình thường: sáng liên tục (nhấp nháy chậm 500ms ON)
        onDelay  = 500;
        offDelay = 0;
      } else if (temp <= 45.0) {
        // Cảnh báo vàng: chớp chậm 2 giây
        onDelay  = 1000;
        offDelay = 1000;
      } else {
        // Nguy hiểm đỏ: chớp nhanh 250ms
        onDelay  = 250;
        offDelay = 250;
      }
    }

    // Thực thi hành vi nhấp nháy hiện tại
    if (onDelay > 0) {
      digitalWrite(LED_GPIO, HIGH);
      vTaskDelay(pdMS_TO_TICKS(onDelay));
    }
    if (offDelay > 0) {
      digitalWrite(LED_GPIO, LOW);
      vTaskDelay(pdMS_TO_TICKS(offDelay));
    }
  }
}