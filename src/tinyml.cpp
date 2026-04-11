#include "tinyml.h"
#include <Arduino.h>

// Nhóm chân màu trắng (D10-D9, D8-D7, D6-D5, D4-D3)
// Chân FAN_PIN 6 đã được đưa vào global.h để dùng chung

// Globals, for the convenience of one-shot setup.
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;
    constexpr int kTensorArenaSize = 8 * 1024; // Adjust size based on your model
    uint8_t tensor_arena[kTensorArenaSize];
} // namespace

void setupTinyML()
{
    Serial.println("TensorFlow Lite Init....");

    // Khởi tạo chân Quạt
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW); // Tắt quạt mặc định
    
    // --- Bật quạt 2 giây lúc khởi động để TEST PHẦN CỨNG ---
    Serial.println("Testing FAN Hardware for 2 seconds...");
    digitalWrite(FAN_PIN, HIGH);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    digitalWrite(FAN_PIN, LOW);
    Serial.println("FAN Hardware test completed.");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(dht_anomaly_model_tflite); // g_model_data is from model_data.h
    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model provided is schema version %d, not equal to supported version %d.",
                               model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TensorFlow Lite Micro initialized on ESP32.");
}

void tiny_ml_task(void *pvParameters)
{

    setupTinyML();

    while (1)
    {

        // Prepare input data (e.g., sensor readings)
        // For a simple example, let's assume a single float input
        input->data.f[0] = glob_temperature;
        input->data.f[1] = glob_humidity;

        // Run inference
        TfLiteStatus invoke_status = interpreter->Invoke();
        if (invoke_status != kTfLiteOk)
        {
            error_reporter->Report("Invoke failed");
            return;
        }

        // Get and process output
        float result = output->data.f[0];
        Serial.print("Inference result: ");
        Serial.println(result);

        // --- ĐIỀU KHIỂN QUẠT DỰA VÀO KẾT QUẢ INFERENCE (CHỈ CHẠY KHI ĐANG Ở CHẾ ĐỘ AUTO) ---
        if (fan_auto_mode) {
            if (result > 0.8) {
                fan_state = true;
                digitalWrite(FAN_PIN, HIGH); // Bật quạt
                Serial.println("Auto Mode: Result > 0.8 -> Fan ON");
            } else if (result < 0.5) {
                fan_state = false;
                digitalWrite(FAN_PIN, LOW);  // Tắt quạt khi chỉ số xuống thấp hẳn
            }
        }

        vTaskDelay(8745 / portTICK_PERIOD_MS);
    }
}