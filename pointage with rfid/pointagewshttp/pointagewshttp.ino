#include <WiFi.h>
#include <WebSocketsClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

TaskHandle_t webSocketTask;
TaskHandle_t httpTask;

const int ledPin = 33;
#define DHT_PIN 4
DHT dht(DHT_PIN, DHT22);
#define SOUND 2
#define lockPin 32
const int pirPin = 25;
int dust = 34;
const int finDeCourse = 5;

const char* ssid = "ssid";
const char* password = "password";
const char* serverAddress = "serverAddress";
const int serverPort = port;
WebSocketsClient webSocket;
unsigned long previousMillis = 0;
const long interval = 300000;
const char* serverUrl = "serverUrl";
const char* serverData = "serverData";
unsigned long doorOpenTime = 0;
const char* Sound = "";
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 30000;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

String jsonStr;
String tokenString;

unsigned long lastMessageTime = 0;
const unsigned long messageInterval = 120000;  // 2 minutes in milliseconds

void sendPeriodicMessage() {
  unsigned long currentTime = millis();
  if (currentTime - lastMessageTime >= messageInterval) {
    // Send your message to the WebSocket server
    webSocket.sendTXT("{\"device_id\":\"3\",\"type\":\"device\",\"key\":\"key3\"}");

    // Update the last message time
    lastMessageTime = currentTime;
  }
}



const long webSocketReconnectInterval = 60000;  // Reconnect every 1 minute
unsigned long lastWebSocketConnection = 0;

void checkWebSocketConnection() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastWebSocketConnection >= webSocketReconnectInterval) {
    lastWebSocketConnection = currentMillis;

    if (!webSocket.isConnected()) {
      Serial.println("[WebSocket] Reconnecting...");
      webSocket.begin(serverAddress, serverPort, "/");
      webSocket.onEvent(webSocketEvent);
      webSocket.setReconnectInterval(5000);
      webSocket.sendTXT("{\"device_id\":\"3\",\"type\":\"device\",\"key\":\"key3\"}");
    }
  }
}

void webSocketTaskFunction(void* pvParameters) {
  for (;;) {
    checkWebSocketConnection();
    webSocket.loop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void httpTaskFunction(void* pvParameters) {
  for (;;) {
    // HTTP-related code here
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
unsigned long lockOffTime = 0;  // Variable to store the time when the lock should be turned off

void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("[WebSocket] Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("[WebSocket] Connected");
      webSocket.sendTXT("{\"id\":\"3\",\"type\":\"device\",\"key\":\"key3\"}");

      break;
    case WStype_TEXT:
      {
        String message = String((char*)(payload));
        Serial.println(message);

        if (message == "offlock") {
          digitalWrite(lockPin, HIGH);
        }

        if (message == "onlock") {
          digitalWrite(lockPin, LOW);
          delay(3000);                  // 3-second delay
          digitalWrite(lockPin, HIGH);  // Turn off the lock after the delay
        }

        break;
      }
    case WStype_BIN:
    case WStype_ERROR:
      Serial.println("[WebSocket] Error");
      break;
  }
  sendPeriodicMessage();
}

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(dust, INPUT);
  pinMode(lockPin, OUTPUT);
  pinMode(SOUND, INPUT);
  dht.begin();
  digitalWrite(lockPin, HIGH);

  starttime = millis();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  delay(2000);

  // Initialize WebSocket
  webSocket.begin(serverAddress, serverPort, "/");
  webSocket.onEvent(webSocketEvent);



  // Create tasks for WebSocket and HTTP clients
  xTaskCreatePinnedToCore(
    webSocketTaskFunction,
    "WebSocketTask",
    10000,
    NULL,
    1,
    &webSocketTask,
    0);

  xTaskCreatePinnedToCore(
    httpTaskFunction,
    "HTTPTask",
    10000,
    NULL,
    1,
    &httpTask,
    1);

  doorOpenTime = millis();

  DynamicJsonDocument jsonDoc(128);
  JsonObject data = jsonDoc.to<JsonObject>();

  data["device_id"] = 3;
  data["key"] = "key3";
  serializeJson(data, jsonStr);

  // Send the POST request
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonStr);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    DynamicJsonDocument doc(256);

    // Deserialize the JSON data
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
      Serial.println("Failed to parse JSON");
    }

    // Extract the token
    const char* token = doc["token"];

    // Save the token in a string variable
    tokenString = String(token);

    // Print the extracted token
    Serial.print("Token: ");
    Serial.println(tokenString);
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  delay(3000);

  DynamicJsonDocument jsonDoc1(250);
  JsonObject root = jsonDoc1.to<JsonObject>();
  tokenString.trim();
  root["device_id"] = 3;
  root["token"] = String(tokenString);

  // Create the 'valeur' nested object and add all the key-value pairs within it
  JsonObject valeur = root.createNestedObject("valeur");
  valeur["temperature"] = 25;
  valeur["humidity"] = 60;
  valeur["msg"] = "";
  valeur["boolean"] = "";
  valeur["sound"] = "";
  valeur["lowpulseoccupancy"] = 0;
  valeur["ratio"] = 0;
  valeur["concentration"] = 0;
  valeur["status"] = "";

  String jsonStr1;
  serializeJson(root, jsonStr1);
  Serial.println(jsonStr1);
  Serial.println(tokenString);

  HTTPClient http1;
  http1.begin(serverData);
  http1.addHeader("Content-Type", "application/json");
  int httpResponseCode1 = http1.POST(jsonStr1);

  if (httpResponseCode1 > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode1);
    String response2 = http1.getString();
    Serial.println(response2);
  } else {
    Serial.print("Error on sending POST request: ");
    Serial.println(httpResponseCode1);
  }

  http1.end();
}
void loop() {
  webSocket.loop();  // Handle WebSocket events
  static unsigned long lastPingTime = 0;
  if (millis() - lastPingTime > 20000) {
    webSocket.sendTXT("{\"device_id\":\"3\",\"type\": \"ping\"}");
    Serial.println("ping");
    lastPingTime = millis();
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int soundValue = digitalRead(SOUND);
  duration = pulseIn(dust, LOW);

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.println(temperature);
    }

    DynamicJsonDocument jsonDoc1(250);
    JsonObject root = jsonDoc1.to<JsonObject>();
    tokenString.trim();
    root["device_id"] = 3;
    root["token"] = String(tokenString);

    JsonObject valeur = root.createNestedObject("valeur");
    valeur["temperature"] = temperature;
    valeur["humidity"] = humidity;

    int switchState = digitalRead(finDeCourse);

    if (switchState == LOW) {
      valeur["status"] = "open";
      Serial.println("Door open");
    } else {
      valeur["status"] = "closed";
      Serial.println("Door closed");
    }

    if (soundValue == HIGH) {
      Sound = "Sound is detected";
      delay(1000);  // Keep the buzzer on for 1 second
    } else {
      Sound = "Sound not detected";
    }

    valeur["sound"] = Sound;

    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - starttime) > sampletime_ms) {
      ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
      concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
      Serial.print("lowpulseoccupancy:");
      Serial.print(ratio);
      Serial.print(" ratio:");
      Serial.println(concentration);
      lowpulseoccupancy = 0;
      starttime = millis();
    }

    valeur["lowpulseoccupancy"] = lowpulseoccupancy;
    valeur["ratio"] = ratio;
    valeur["concentration"] = concentration;

    String jsonStr1;
    serializeJson(root, jsonStr1);
    Serial.println(jsonStr1);
    Serial.println(tokenString);

    HTTPClient http;
    http.begin(serverData);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode1 = http.POST(jsonStr1);

    if (httpResponseCode1 > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode1);
      String response2 = http.getString();
      Serial.println(response2);
    } else {
      Serial.print("Error on sending POST request: ");
      Serial.println(httpResponseCode1);
    }

    http.end();
  }

  DynamicJsonDocument jsonDoc1(250);
  JsonObject root = jsonDoc1.to<JsonObject>();
  tokenString.trim();
  root["device_id"] = 1;
  root["token"] = String(tokenString);
  JsonObject valeur = root.createNestedObject("valeur");

  int switchState = digitalRead(finDeCourse);

  if (switchState == LOW) {
    valeur["status"] = "open";
    Serial.println("Door open");

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.println(temperature);
    }

    valeur["temperature"] = temperature;
    valeur["humidity"] = humidity;

    if (soundValue == HIGH) {
      Sound = "Sound is detected";
      delay(1000);  // Keep the buzzer on for 1 second
    } else {
      Sound = "Sound not detected";
    }

    valeur["sound"] = Sound;

    lowpulseoccupancy = lowpulseoccupancy + duration;

    if ((millis() - starttime) > sampletime_ms) {
      ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
      concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
      Serial.print("lowpulseoccupancy:");
      Serial.print(ratio);
      Serial.print(" ratio:");
      Serial.println(concentration);
      lowpulseoccupancy = 0;
      starttime = millis();
    }

    valeur["lowpulseoccupancy"] = lowpulseoccupancy;
    valeur["ratio"] = ratio;
    valeur["concentration"] = concentration;

    String jsonStr2;
    serializeJson(root, jsonStr2);
    Serial.println(jsonStr2);
    Serial.println(tokenString);

    HTTPClient http1;
    http1.begin(serverData);
    http1.addHeader("Content-Type", "application/json");
    int httpResponseCode2 = http1.POST(jsonStr2);

    if (httpResponseCode2 > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode2);
      String response2 = http1.getString();
      Serial.println(response2);
    } else {
      Serial.print("Error on sending POST request: ");
      Serial.println(httpResponseCode2);
    }

    http1.end();

    while (digitalRead(finDeCourse) == LOW) {
      delay(10);
    }

    valeur["status"] = "closed";
    Serial.println("Door closed");

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      Serial.print("Humidity: ");
      Serial.print(humidity);
      Serial.print(" %\t");
      Serial.print("Temperature: ");
      Serial.println(temperature);
    }

    valeur["temperature"] = temperature;
    valeur["humidity"] = humidity;

    if (soundValue == HIGH) {
      Sound = "Sound is detected";
      delay(1000);
    } else {
      Sound = "Sound not detected";
    }

    valeur["sound"] = Sound;

    lowpulseoccupancy = lowpulseoccupancy + duration;

if ((millis() - starttime) > sampletime_ms) {
      ratio = lowpulseoccupancy / (sampletime_ms * 10.0);
      concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62;
      Serial.print("lowpulseoccupancy:");
      Serial.print(ratio);
      Serial.print(" ratio:");
      Serial.println(concentration);
      lowpulseoccupancy = 0;
      starttime = millis();
    }

    valeur["lowpulseoccupancy"] = lowpulseoccupancy;
    valeur["ratio"] = ratio;
    valeur["concentration"] = concentration;

    doorOpenTime = millis();

    String jsonStr1;
    serializeJson(root, jsonStr1);
    Serial.println(jsonStr1);
    Serial.println(tokenString);

    HTTPClient http;
    http.begin(serverData);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode1 = http.POST(jsonStr1);

    if (httpResponseCode1 > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode1);
      String response2 = http.getString();
      Serial.println(response2);
    } else {
      Serial.print("Error on sending POST request: ");
      Serial.println(httpResponseCode1);
    }

    http.end();
  }
}
