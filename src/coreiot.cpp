#include "coreiot.h"

// ----------- CONFIGURE THESE! -----------
const char* coreIOT_Server = "app.coreiot.io";
const char* coreIOT_Token = "mioermbgdwqftctiyr8c";   // Device Access Token
const int   mqttPort = 1883;
// ----------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect (username=token, password=empty)
    //if (client.connect("ESP32Client", coreIOT_Token, NULL)) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    // Dùng Token do người dùng cài đặt hoặc mặc định nếu trống
    String useToken = CORE_IOT_TOKEN.isEmpty() ? String(coreIOT_Token) : CORE_IOT_TOKEN;

    if (client.connect(clientId.c_str(), useToken.c_str(), NULL)) {
        
      Serial.println("connected to CoreIOT Server!");
      client.subscribe("v1/devices/me/rpc/request/+");
      Serial.println("Subscribed to v1/devices/me/rpc/request/+");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  // Allocate a temporary buffer for the message
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  Serial.print("Payload: ");
  Serial.println(message);

  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* method = doc["method"];
  // App.coreiot (ThingsBoard) switches truyền kiểu bool: {"method":"setNeoLED","params":true}
  bool params = doc["params"] | false; 

  if (strcmp(method, "setNeoLED") == 0) {
    neo_enabled = params;
    Serial.printf("RPC setNeoLED: %s\n", params ? "ON" : "OFF");
  } 
  else if (strcmp(method, "setBlinkyLED") == 0) {
    blinky_enabled = params;
    Serial.printf("RPC setBlinkyLED: %s\n", params ? "ON" : "OFF");
  } 
  else if (strcmp(method, "setFanAuto") == 0) {
    fan_auto_mode = params;
    Serial.printf("RPC setFanAuto: %s\n", params ? "AUTO" : "MANUAL");
  } 
  else if (strcmp(method, "setFanState") == 0) {
    if (!fan_auto_mode) {
      fan_state = params;
      digitalWrite(FAN_PIN, fan_state ? HIGH : LOW);
      Serial.printf("RPC setFanState (Manual): %s\n", params ? "ON" : "OFF");
    } else {
      Serial.println("RPC setFanState: Ignored (System is in Auto mode)");
    }
  } 
  else {
    Serial.print("Unknown method: ");
    Serial.println(method);
    return;
  }

  // Cần publish response ngược lại để công tắc trên CoreIoT app không bị treo (Time out)
  // Request dạng: v1/devices/me/rpc/request/{request_id}
  String topicStr = String(topic);
  int lastSlash = topicStr.lastIndexOf("/");
  if (lastSlash > 0) {
    String reqId = topicStr.substring(lastSlash + 1);
    String respTopic = "v1/devices/me/rpc/response/" + reqId;
    client.publish(respTopic.c_str(), "{\"status\":\"ok\"}");
  }
}


void setup_coreiot() {
  // Đợi cho đến khi WiFi kết nối thành công (semaphore từ mainserver)
  while (1) {
    if (xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY)) {
      break;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println(" WiFi Connected! Setting up MQTT...");

  static char mqttServer[128];
  if (CORE_IOT_SERVER.isEmpty()) {
    strncpy(mqttServer, coreIOT_Server, sizeof(mqttServer) - 1);
  } else {
    strncpy(mqttServer, CORE_IOT_SERVER.c_str(), sizeof(mqttServer) - 1);
  }
  mqttServer[sizeof(mqttServer) - 1] = '\0';

  int usePort = CORE_IOT_PORT.isEmpty() ? mqttPort : CORE_IOT_PORT.toInt();

  Serial.print("MQTT Connect to: ");
  Serial.print(mqttServer);
  Serial.print(":");
  Serial.println(usePort);

  client.setServer(mqttServer, usePort);
  client.setCallback(callback);
}

void coreiot_task(void *pvParameters){

    setup_coreiot();

    while(1){

        if (!client.connected()) {
            reconnect();
        }
        client.loop();

        // Thu thập toàn bộ telemetry
        String payload = "{";
        payload += "\"temperature\":" + String(glob_temperature) + ",";
        payload += "\"humidity\":" + String(glob_humidity) + ",";
        payload += "\"neo\":" + String(neo_enabled ? "true" : "false") + ",";
        payload += "\"blinky\":" + String(blinky_enabled ? "true" : "false") + ",";
        payload += "\"fanAuto\":" + String(fan_auto_mode ? "true" : "false") + ",";
        payload += "\"fanOn\":" + String(fan_state ? "true" : "false");
        payload += "}";
        
        client.publish("v1/devices/me/telemetry", payload.c_str());
        
        Serial.println("Published telemetry: " + payload);
        vTaskDelay(6000 / portTICK_PERIOD_MS);  // Publish every 6 seconds
    }
}