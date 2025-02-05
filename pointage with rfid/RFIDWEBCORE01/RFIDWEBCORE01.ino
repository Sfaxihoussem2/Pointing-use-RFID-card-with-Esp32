#include <WebSocketsClient.h>
#include <SPI.h>
#include <ESP32Time.h>
#include "time.h"
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include "FS.h"

File myFile;

#define SCK 18
#define MISO 19
#define MOSI 23
#define CS 5

SoftwareSerial RFID(33, 4);    // RX and TX for entering RFID
SoftwareSerial RFID2(32, 22);  // RX and TX for exit RFID

WebSocketsClient webSocket;

TaskHandle_t taskWebSocket, taskRFID;

const char* ssid2 = "RD-Team";
const char* password2 = "R&D-T3@m";

const char* ssid1 = "RD-Team";
const char* password1 = "R&D-T3@m";

const char* ssid = "RD-Team";
const char* password = "R&D-T3@m";
const char* serverAddress = "ws.elastic-watch.elastic-solutions.com";
const int serverPort = 14000;

const int MAX_WIFI_RETRIES = 2;
const int WIFI_RETRY_INTERVAL = 500;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

ESP32Time rtc(3600);
String text;
String CardNumber = "";

const char* serverName = "https://back.el-erp.saas.elastic-erp.com/api/pointing/storeOffline";
const char* loginServer = "https://back.el-erp.saas.elastic-erp.com/api/pointageLogin";
const char* username = "superadmin@tac-tic.net";
const char* user_password = "147258";
String authToken;

const int MAX_STRING_COUNT = 200;
String cards[MAX_STRING_COUNT];
int lastIndex;
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
const long interval = 900000;
const long intervalsend = 300000;
int counter = 0;
char c;
int pin_number = 8;
int startIndex;
struct tm timeinfo;
SPIClass spi = SPIClass(VSPI);
unsigned long lastMessageTime = 0;
const unsigned long messageInterval = 120000;
void webSocketTask(void* pvParameters) {
  while (1) {
    checkWebSocketConnection();
    webSocket.loop();
    vTaskDelay(10);
  }
}

void RFIDTask(void* pvParameters) {
  while (1) {
    // ... (Your existing RFID-related code)
    vTaskDelay(10);
  }
}
void sendPeriodicMessage() {
  unsigned long currentTime = millis();
  if (currentTime - lastMessageTime >= messageInterval) {
    // Send your message to the WebSocket server
    webSocket.sendTXT("{\"id\":\"1\",\"type\":\"device\",\"key\":\"key1\"}");

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
      webSocket.sendTXT("{\"id\":\"1\",\"type\":\"device\",\"key\":\"key1\"}");
    }
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
      webSocket.sendTXT("{\"id\":\"1\",\"type\":\"device\",\"key\":\"key1\"}");

      break;
    case WStype_TEXT:
      {
        String message = String((char*)(payload));
        Serial.println(message);

        if (message == "offlock") {
          digitalWrite(17,HIGH);
        }

        if (message == "onlock") {
          digitalWrite(17,LOW);
          delay(3000);                  // 3-second delay
          digitalWrite(17,HIGH);  // Turn off the lock after the delay
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
  // Initialize WebSocket
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  delay(2000);

  webSocket.begin(serverAddress, serverPort, "/");
  webSocket.onEvent(webSocketEvent);
  xTaskCreatePinnedToCore(webSocketTask, "WebSocketTask", 8192, NULL, 1, &taskWebSocket, 0);
  xTaskCreatePinnedToCore(RFIDTask, "RFIDTask", 8192, NULL, 1, &taskRFID, 1);
  spi.begin(SCK, MISO, MOSI, CS);

  Serial.print("Initializing SD card...");

  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    while (1)
      ;
  }
  Serial.println("initialization done.");

  // define the relay pin to switch the
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  // define the the red and the green led
  pinMode(14, OUTPUT);  // the green led for the enter rfid R1
  pinMode(13, OUTPUT);  // the red led for the enter rfid V1
  pinMode(4, OUTPUT);  // the buzzer pin B1
  // define the   led pins for the exit module
  pinMode(21, OUTPUT);  // the green led for the enter module R2
  pinMode(15, OUTPUT);  // the red led for the exit module V2
  pinMode(32, OUTPUT);  // the buzzer pin B2
  // set the red led to high
  digitalWrite(14, HIGH);  // set the enter red led to HIGH
  digitalWrite(21, HIGH);  // set the exit red led to HIGH

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  int WIFI_RETRY = 500;
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 5) {
    delay(WIFI_RETRY);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to Wi-Fi!");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect to Wi-Fi!");
    WiFi.begin(ssid1, password1);
    Serial.print("Connecting to Wi-Fi...");
    int WIFI_RETRY = 500;
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 5) {
      delay(WIFI_RETRY);
      Serial.print(".");
      retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected to Wi-Fi!");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }
  // try to connect for the third time
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid2, password2);
    Serial.print("Connecting to Wi-Fi...");
    int WIFI_RETRY = 500;
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 5) {
      delay(WIFI_RETRY);
      Serial.print(".");
      retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected to Wi-Fi!");
      Serial.println("WiFi connected.");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }
  }

  const size_t capaciter = JSON_OBJECT_SIZE(2);  // deffine the size of json object
  DynamicJsonDocument doc2(capaciter);           // create a doc
  JsonObject obj2 = doc2.to<JsonObject>();       // create the json object
  // fill the json object with username and password
  obj2["email"] = username;
  obj2["password"] = user_password;
  // create string var to prepare it to serialize the json data
  String jsonStr2;
  serializeJson(doc2, jsonStr2);
  // initialize the rfid modules
  RFID.begin(9600);
  RFID2.begin(9600);
  // this section is for reading the rfid code of all users in database and store them inside array
  WiFiClient client;
  HTTPClient http;
  // login to the server
  http.begin(loginServer);
  http.addHeader("Content-Type", "application/json");
  //https.addHeader("Authorization", "Bearer " + String(authToken));
  int authResponseCode = http.POST(jsonStr2);
  Serial.print("HTTP response code: ");
  Serial.println(authResponseCode);
  Serial.print("HTTP response : ");
  authToken = http.getString();
  //Serial.println(authToken);
  http.end();
  // gsdf
  String AllUserserver = "https://back.el-erp.saas.elastic-erp.com/api/allusers";
  http.begin(AllUserserver);
  http.addHeader("Authorization", "Bearer " + String(authToken));
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  String response;
  // If the GET request succeeds
  if (httpResponseCode == 200) {
    // Get the response body as a String
    response = http.getString();

    // Parse the response to extract the array of strings
    const int maxStringLength = 11;  // Maximum length of each string
    int index = 1;                   // Current index in the payload string
    int count = 0;                   // Counter for the number of strings read
    while (count < MAX_STRING_COUNT && index < response.length()) {
      String currentString = response.substring(index + 1, index + maxStringLength);  // Extract the string from the payload, excluding the quotation marks
      cards[count] = currentString;                                                   // Assign the string to the array
      count++;                                                                        // Increment the counter
      index += maxStringLength + 2;                                                   // Move the index to the next string, considering the quotation marks and comma
    }
    startIndex = count;  // the start index is the lenght of the cards array initially
    // Print the incoming array of strings
    Serial.println("Incoming array:");
    for (int i = 0; i < count; i++) {
      Serial.println(cards[i]);
    }

    // TODO: Copy the incoming array to your own array and save it if necessary
  } else {
    Serial.print("HTTP GET request failed, error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  delay(200);
  // save the incomming data to text file which contain the cards number
  // Save the incoming array to a file

  // open the file for saving the new incomming array if the wifi is connected:
  if (WiFi.status() == WL_CONNECTED) {

    deleteFile(SD, "/users.txt");
    // save users new list in the file
    appendFile(SD, "/users.txt", response);
    // if the wifi is not connected read cards array from the saved file

  } else {
    String usersList;
    readFileoffline(SD, "/users.txt", usersList);
    const int maxStringLength = 11;  // Maximum length of each string
    int index = 1;                   // Current index in the payload string
    int count = 0;                   // Counter for the number of strings read
    while (count < MAX_STRING_COUNT && index < usersList.length()) {
      String currentString = usersList.substring(index + 1, index + maxStringLength);  // Extract the string from the payload, excluding the quotation marks
      cards[count] = currentString;                                                    // Assign the string to the array
      count++;                                                                         // Increment the counter
      index += maxStringLength + 2;                                                    // Move the index to the next string, considering the quotation marks and comma
    }
    Serial.println("read users list for users file");
    for (int i = 0; i < count; i++) {
      Serial.println(cards[i]);
    }
    startIndex = count;
  }

  // configure time of esp32
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
  // route ,to handle entrer button which will open the door to the user

  Serial.println("Approximate your card to the reader...");
  Serial.println();
}


void loop() {
  // put your main code here, to run repeatedly:
  checkWebSocketConnection();
  webSocket.loop();
  static unsigned long lastPingTime = 0;
  if (millis() - lastPingTime > 20000) {
    webSocket.sendTXT("{\"id\":\"1\",\"type\": \"ping\"}");
    Serial.println("ping");

    lastPingTime = millis();
  }

  bool access = false;
  bool access2 = false;
  WiFiClient client;
  HTTPClient https;
  // define and store date and time
  //struct tm timeinfo = rtc.getTimeStruct();
  // this if connect to wifi if esp32 lost connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost Wi-Fi connection, reconnecting...");
    connectToWiFi();
    // configure time of esp32
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    if (getLocalTime(&timeinfo)) {
      rtc.setTimeStruct(timeinfo);
    }
  }

  // this is the part to command the machine with RFID and push buttons
  // first read the card id if  there is a card
  while (RFID.available() > 0) {
    delay(5);
    c = RFID.read();
    text += c;
  }
  if (text.length() > 20) {
    text = text.substring(1, 11);
    access = false;
    // this is the modification that i add to transform the readed rfid card
    long decimalValue = strtoul(text.substring(2).c_str(), NULL, 16);
    String decimalString = String(decimalValue, DEC);
    while (decimalString.length() < 10) {
      decimalString = "0" + decimalString;
    }
    Serial.println(decimalString);

    for (int i = 0; i < startIndex; i++) {
      if (decimalString == cards[i]) {
        Serial.println("Card ID : " + text);
        Serial.println("Access accepted");
        CardNumber = cards[i];
        access = true;
      }
    }

    if ((access == true)) {
      Serial.println("accepted");
      digitalWrite(14, LOW);   // red led is off
      digitalWrite(13, HIGH);  // green led is on
      digitalWrite(27, LOW);   // open the door
      tone(12, 1200, 500);     // buzzer sound to say that access is authorized
      digitalWrite(14, HIGH);  // return the red led on again
      delay(2000);
      digitalWrite(13, LOW);   // green led is off
      digitalWrite(27, HIGH);  // close the door
      char time_string[20];    // allocate a buffer for the time string
      char date_string[11];    // allocate a buffer for the date string (YYYY-MM-DD)
      getLocalTime(&timeinfo);
      strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &timeinfo);
      // get and store the date in a file
      strftime(date_string, sizeof(date_string), "%Y-%m-%d", &timeinfo);
      const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(3);
      DynamicJsonDocument doc(capacity);
      JsonArray data1 = doc.to<JsonArray>();
      JsonObject obj1 = data1.createNestedObject();
      obj1["rfid"] = CardNumber;
      obj1["sens"] = "in";
      obj1["date"] = time_string;
      String jsonStr1;
      serializeJson(doc, jsonStr1);
      //Serial.println(jsonStr1);
      String name = String("/backup-") + String(date_string) + String(".txt");
      const char* path = name.c_str();
      Serial.println(path);
      appendFile(SD, path, jsonStr1);
      String data;
      readFileoffline(SD, "/data.txt", data);
      if (data == "") {
        appendFile(SD, "/data.txt", jsonStr1);
        //Serial.println(data);
      } else {
        // remove the first element of the array [
        jsonStr1 = jsonStr1.substring(1);
        // remove the last element ]
        jsonStr1 = jsonStr1.substring(0, jsonStr1.length() - 1);

        int pos = data.indexOf("]");
        if (pos != -1) {
          data = data.substring(0, pos) + "," + jsonStr1 + data.substring(pos);
        }
        //mouvement.insert(mouvement.length() - 2,"," + jsonStr);
        deleteFile(SD, "/data.txt");
        // append the new object to the array in the file
        appendFile(SD, "/data.txt", data);
        //Serial.println(data);
      }
    }
    if (access == false) {
      Serial.println("Card ID : " + text);
      Serial.println("Access denied");
      //end of the non authorized access code
      for (int i = 0; i < 3; i++) {
        digitalWrite(14, LOW);
        tone(12, 500, 500);
        delay(500);
        digitalWrite(14, HIGH);
        noTone(12);
        delay(500);
      }
    }
    text = "";
  }

  /************************* this is the code for the second rfid module **********************/
  // first read the card id if  there is a card
  while (RFID2.available() > 0) {
    delay(5);
    c = RFID2.read();
    text += c;
  }

  if (text.length() > 20) {
    text = text.substring(1, 11);
    access2 = false;
    // this is the modification that i add to transform the readed rfid card
    long decimalValue = strtoul(text.substring(2).c_str(), NULL, 16);
    String decimalString = String(decimalValue, DEC);
    while (decimalString.length() < 10) {
      decimalString = "0" + decimalString;
    }
    Serial.println(decimalString);

    for (int i = 0; i < startIndex; i++) {
      if (decimalString == cards[i]) {
        Serial.println("Card ID : " + text);
        Serial.println("Access accepted");
        CardNumber = cards[i];
        access2 = true;
      }
    }

    if ((access2 == true)) {
      Serial.println("accepted");

      digitalWrite(21, LOW);   // red led is off
      digitalWrite(15, HIGH);  // green led is on
      digitalWrite(27, LOW);   //open the door
      tone(26, 1200, 500);     // buzzer sound to say that access is authorized
      digitalWrite(21, HIGH);  // return the red led on again
      delay(2000);
      digitalWrite(15, LOW);  // green led is off
      digitalWrite(27, HIGH);
      // then open the door and wait for some time until the user enter
      // prepare the data to publish in json format
      /* const size_t capacity2 = JSON_OBJECT_SIZE(4);
      DynamicJsonDocument doc1(capacity2);
      JsonObject obj1 = doc1.to<JsonObject>(); */
      char time_string[20];  // allocate a buffer for the time string
      char date_string[11];  // allocate a buffer for the date string (YYYY-MM-DD)
      getLocalTime(&timeinfo);
      strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &timeinfo);
      // get and store the date in a file
      strftime(date_string, sizeof(date_string), "%Y-%m-%d", &timeinfo);
      const size_t capacity = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(3);
      DynamicJsonDocument doc(capacity);
      JsonArray data1 = doc.to<JsonArray>();
      JsonObject obj1 = data1.createNestedObject();
      obj1["rfid"] = CardNumber;
      obj1["sens"] = "out";
      obj1["date"] = time_string;
      String jsonStr1;
      serializeJson(doc, jsonStr1);
      Serial.println(jsonStr1);
      String name = String("/backup-") + String(date_string) + String(".txt");
      const char* path = name.c_str();
      Serial.println(path);
      appendFile(SD, path, jsonStr1);
      String data;
      readFileoffline(SD, "/data.txt", data);
      if (data == "") {
        appendFile(SD, "/data.txt", jsonStr1);
        //Serial.println(data);
      } else {
        // remove the first element of the array [
        jsonStr1 = jsonStr1.substring(1);
        // remove the last element ]
        jsonStr1 = jsonStr1.substring(0, jsonStr1.length() - 1);

        int pos = data.indexOf("]");
        if (pos != -1) {
          data = data.substring(0, pos) + "," + jsonStr1 + data.substring(pos);
        }
        //mouvement.insert(mouvement.length() - 2,"," + jsonStr);
        deleteFile(SD, "/data.txt");
        // append the new object to the array in the file
        appendFile(SD, "/data.txt", data);
        //Serial.println(data);
      }


      // close the door
    }
    if (access2 == false) {
      Serial.println("Card ID : " + text);
      Serial.println("Access denied");
      // send the rfid code to the server
      //end of the non authorized access code

      for (int i = 0; i < 3; i++) {
        digitalWrite(21, LOW);
        tone(26, 500, 500);
        delay(500);
        digitalWrite(21, HIGH);
        noTone(26);
        delay(500);
      }
    }
    text = "";
  }
  // check for new users
  unsigned long currentMillis = millis();
  // check if it's time to send data of pointing if there is data
  if (currentMillis - previousMillis >= intervalsend) {
    // save the last time you called the function
    previousMillis = currentMillis;
    // WiFiClient client;
    HTTPClient https;
    String mouvement;
    readFileoffline(SD, "/data.txt", mouvement);
    if (mouvement != "") {
      https.begin(serverName);
      https.addHeader("Authorization", "Bearer " + String(authToken));
      https.addHeader("Content-Type", "application/json");
      int httpResponseCode = https.POST(mouvement);
      //Serial.println(http.POST(jsonStr));
      //Serial.println(mouvement);
      // Check for errors
      if (httpResponseCode == HTTP_CODE_OK) {  //HTTP_CODE_OK
        String response = https.getString();
        Serial.println("HTTP Response: " + response);
        deleteFile(SD, "/data.txt");
      } else {
        Serial.print("HTTP Error code: ");
        Serial.println(httpResponseCode);
        //Serial.println(https.getString());
      }
      https.end();
    } else {
      Serial.println("there is no data to be send");
    }
  }
  unsigned long currentMillis1 = millis();
  // check if it's time to call the function to check for update in users list
  if (currentMillis1 - previousMillis1 >= interval) {
    // save the last time you called the function
    previousMillis1 = currentMillis1;
    // WiFiClient client;
    HTTPClient http;
    // call your function here
    String AllUserserver = "https://back.el-erp.saas.elastic-erp.com/api/allusers";
    http.begin(AllUserserver);
    http.addHeader("Authorization", "Bearer " + String(authToken));
    //String serverdata = "http://13.37.220.40/api/userdata";
    //http.begin(client, serverdata);

    // Send HTTP GET request
    int httpResponseCode = http.GET();
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    // If the GET request succeeds
    if (httpResponseCode == 200) {
      // Get the response body as a String
      String response = http.getString();
      String usersList;
      readFileoffline(SD, "/users.txt", usersList);
      //usersList.replace(" ","");
      //response.replace(" ","");
      int response_length = response.length();
      int users_length = usersList.length();

      Serial.print(" response.length(): ");
      Serial.println(response_length);
      Serial.print("users.length(): ");
      Serial.println(users_length);
      // Compare the two strings using strcmp()
      if (usersList != response) {
        const int maxStringLength = 11;  // Maximum length of each string
        int index = 1;                   // Current index in the payload string
        int count = 0;                   // Counter for the number of strings read
        while (count < MAX_STRING_COUNT && index < response.length()) {
          String currentString = response.substring(index + 1, index + maxStringLength);  // Extract the string from the payload, excluding the quotation marks
          cards[count] = currentString;                                                   // Assign the string to the array
          count++;                                                                        // Increment the counter
          index += maxStringLength + 2;                                                   // Move the index to the next string, considering the quotation marks and comma
        }
        deleteFile(SD, "/users.txt");
        // save users new list in the file
        appendFile(SD, "/users.txt", response);
        startIndex = count;
      } else {
        Serial.println("the list is the same nothing changes");
      }
      // Parse the response to extract the array of strings
      // TODO: Copy the incoming array to your own array and save it if necessary
    } else {
      Serial.print("HTTP GET request failed, error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
    delay(200);
  }
}

/** reconnect  to wifi fucntion */
void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < MAX_WIFI_RETRIES) {
    delay(WIFI_RETRY_INTERVAL);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to Wi-Fi!");
  } else {
    Serial.println();
    Serial.println("Failed to connect to Wi-Fi!");
  }
}



void deleteFile(fs::FS& fs, const char* path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
// this is the function for reading the data saved offline
void readFile(fs::FS& fs, const char* path, String tab[], int numCards) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  int i = 0;
  while (file.available() && i < numCards) {
    tab[i] = file.readStringUntil('\n');
    Serial.println(tab[i]);
    i++;
  }
  file.close();
}
////*******************//////

void readFileoffline(fs::FS& fs, const char* path, String& data) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");
  data = file.readString();
  data.trim();
  //Serial.println(data);

  file.close();
}


void writeFile(fs::FS& fs, const char* path, const String message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS& fs, const char* path, const String message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.println(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
