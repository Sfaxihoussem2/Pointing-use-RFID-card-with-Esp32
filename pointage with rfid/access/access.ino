
#include <SPI.h>
#include <ESP32Time.h>
#include "time.h"
// Load Wi-Fi library
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiegandMulti.h>
#include <Arduino.h>

//#include <WiFiMulti.h>
#include <SD.h>
#include "FS.h"
File myFile;

#define SCK  18
#define MISO  19
#define MOSI  23
#define CS  5



// Replace with your network credentials

const char* ssid = "ssid";
const char* password = "password";

//const char* ssid = "ssid"; 
//const char* password = "password";
//const char* ssid = "ssid"; 
//const char* password = "password"; 
const int MAX_WIFI_RETRIES = 2; // maximum number of Wi-Fi connection retries
const int WIFI_RETRY_INTERVAL = 500; // time interval between Wi-Fi connection retries in milliseconds

// ntp server configuration 
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

ESP32Time rtc(3600);  // offset in seconds GMT+1
String text;
String CardNumber = "";
//Your Domain name with URL path or IP address with path

const char* serverName = "server";
// define the login server that will give me the token
const char* loginServer = "loginServer";

const char* username = "superadmin@tac-tic.net"; // replace with your username
const char* user_password = "147258"; // replace with your password
// define the variable that will query the token value
String authToken;

const int MAX_STRING_COUNT = 200;
String cards[MAX_STRING_COUNT];
int lastIndex;
unsigned long previousMillis = 0;  // will store last time the function was called
unsigned long previousMillis1 = 0;  // will store last time the function was called
//const long interval = 300000;      // interval at which to call the function (5 minutes)
const long interval = 90000;      // interval at which to call the function (5 minutes)900000
const long  intervalsend = 30000; // interval for sending the data of pointing

//define a counter that count the number of data stored in the local storage
int counter = 0;
char c;
int pin_number=8;// this the pin of the buzzer
int startIndex; // define the start index if you want to add new user it means the length of the allusers table
// define gloable variable for date and time
struct tm timeinfo;
SPIClass spi = SPIClass(VSPI);
//RFIDdata rfidArray[100];

WIEGANDMULTI wg;
WIEGANDMULTI wg2;
void Reader1D0Interrupt(void)
{
  wg.ReadD0();
}

void Reader1D1Interrupt(void)
{
  wg.ReadD1();
}

void Reader2D0Interrupt(void)
{
  wg2.ReadD0();
}

void Reader2D1Interrupt(void)
{
  wg2.ReadD1();
}

void setup()
{

  
  Serial.begin(9600);
  
  spi.begin(SCK, MISO, MOSI, CS);

  Serial.print("Initializing SD card...");

  if (!SD.begin(5)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  // define the relay pin to switch the 
  Serial.begin(9600);
  pinMode(17,OUTPUT);
  digitalWrite(17,HIGH);
  // define the the red and the green led 
 // define the the red and the green led 
  pinMode(15,OUTPUT);// the green led for the enter rfid R1
  pinMode(0,OUTPUT); // the buzzer pin B1
  // define the   led pins for the exit module 
  pinMode(33,OUTPUT); // the red led for the exit module V2
  pinMode(32,OUTPUT); // the buzzer pin B2
  // set the red led to high
  digitalWrite(15,LOW); // set the enter red led to HIGH
  digitalWrite(33,LOW); // set the exit red led to HIGH 
 	wg.begin(2,4,Reader1D0Interrupt,Reader1D1Interrupt);
	wg2.begin(34,35,Reader2D0Interrupt,Reader2D1Interrupt);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  int WIFI_RETRY=500;
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
    int WIFI_RETRY=500;
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
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid2, password2);
  Serial.print("Connecting to Wi-Fi...");
  int WIFI_RETRY=500;
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
    
  const size_t capaciter = JSON_OBJECT_SIZE(2); // deffine the size of json object
  DynamicJsonDocument doc2(capaciter); // create a doc
  JsonObject obj2 = doc2.to<JsonObject>(); // create the json object 
  // fill the json object with username and password
  obj2["email"] = username;
  obj2["password"] = user_password;  
  // create string var to prepare it to serialize the json data
  String jsonStr2;
  serializeJson(doc2, jsonStr2);
  // initialize the rfid modules

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
  String AllUserserver = "https://back.el-erp.saas.elastic-erp.com/api/allusers/2";
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
    const int maxStringLength = 11; // Maximum length of each string
    int index = 1; // Current index in the payload string
    int count = 0; // Counter for the number of strings read
    while (count < MAX_STRING_COUNT && index < response.length()) {
      String currentString = response.substring(index + 1, index + maxStringLength ); // Extract the string from the payload, excluding the quotation marks
      cards[count] = currentString; // Assign the string to the array
      count++; // Increment the counter
      index += maxStringLength + 2; // Move the index to the next string, considering the quotation marks and comma
    
    }
    startIndex = count; // the start index is the lenght of the cards array initially    
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
  if (WiFi.status() == WL_CONNECTED)
  {
    
    deleteFile(SD, "/users.txt");
    // save users new list in the file 
    appendFile(SD, "/users.txt", response);
  // if the wifi is not connected read cards array from the saved file
 
  } else {
    String usersList; 
    readFileoffline(SD, "/users.txt",usersList);
    const int maxStringLength = 11; // Maximum length of each string
    int index = 1; // Current index in the payload string
    int count = 0; // Counter for the number of strings read
    while (count < MAX_STRING_COUNT && index < usersList.length()) {
      String currentString = usersList.substring(index + 1, index + maxStringLength ); // Extract the string from the payload, excluding the quotation marks
      cards[count] = currentString; // Assign the string to the array
      count++; // Increment the counter
      index += maxStringLength + 2; // Move the index to the next string, considering the quotation marks and comma
    
    }
    Serial.println("read users list for users file");
    for (int i = 0; i < count; i++) {
      Serial.println(cards[i]);
    }
    startIndex = count;
  }       
 
    // configure time of esp32 
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    if (getLocalTime(&timeinfo)){
      rtc.setTimeStruct(timeinfo); 
    }
    // route ,to handle entrer button which will open the door to the user
  
    Serial.println("Approximate your card to the reader...");
    Serial.println();
  
}


void loop() {
  // put your main code here, to run repeatedly:
  bool access=false;  
  bool access2=false;
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
    if (getLocalTime(&timeinfo)){
      rtc.setTimeStruct(timeinfo); 
    }    
  }
 
  // this is the part to command the machine with RFID and push buttons
   // first read the card id if  there is a card
  while (wg.available()) {
    unsigned long decimalCode = wg.getCode();  // Obtenez le code décimal
    String decimalString = String(decimalCode);  

    while (decimalString.length() < 10) {
       decimalString = "0" + decimalString;
     }
    Serial.println(decimalString);
          
       for(int i =0;i<startIndex;i++)
      {         
        if(decimalString == cards[i]){
        Serial.println("Card ID : " + text);
        Serial.println("Access accepted");
        CardNumber = cards[i];
        access = true;
        }   
      } 

    if ((access==true)) { 
      Serial.println("accepted");      
         digitalWrite(15,HIGH); // red led is off
      digitalWrite(17,LOW); // open the door
      tone(0,1200,500); // buzzer sound to say that access is authorized
      delay(2000);
      digitalWrite(15,LOW); // green led is on
      digitalWrite(17,HIGH); // close the door
      char time_string[20]; // allocate a buffer for the time string
      char date_string[11]; // allocate a buffer for the date string (YYYY-MM-DD)      
      getLocalTime(&timeinfo);
      strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &timeinfo);
      // get and store the date in a file
      strftime(date_string, sizeof(date_string), "%Y-%m-%d", &timeinfo);
      const size_t capacity = JSON_ARRAY_SIZE(3) + 3*JSON_OBJECT_SIZE(3);
      DynamicJsonDocument doc(capacity);
      JsonArray data1 = doc.to<JsonArray>();
      JsonObject obj1 = data1.createNestedObject();
      obj1["rfid"] = CardNumber;
      obj1["sens"] = "in";
      obj1["date"] = time_string;
      obj1["porte"] = 2;
      String jsonStr1;
      serializeJson(doc, jsonStr1);
      //Serial.println(jsonStr1);
      String name = String("/backup-") + String(date_string) + String(".txt");
      const char* path = name.c_str();
      Serial.println(path);
      appendFile(SD, path, jsonStr1);
      String data; 
      readFileoffline(SD, "/data.txt",data);
        if (data == "")
        {
          appendFile(SD, "/data.txt", jsonStr1);
          //Serial.println(data);
        }
        else 
        {
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
      for(int i=0; i<3;i++){
        digitalWrite(15,HIGH);
        tone(0,500,500);
        delay(500);
        digitalWrite(15,HIGH);
        noTone(0); 
        delay(500);
      }
       
    }
    text = ""; 
  }

  /************************* this is the code for the second rfid module **********************/
   // first read the card id if  there is a card
  
 while (wg2.available()) {
    unsigned long decimalCode = wg2.getCode();  // Obtenez le code décimal
    String decimalString = String(decimalCode);  

    while (decimalString.length() < 10) {
       decimalString = "0" + decimalString;
     }
    Serial.println(decimalString);
          
       for(int i =0;i<startIndex;i++)
      {         
        if(decimalString == cards[i]){
        Serial.println("Card ID : " + text);
        Serial.println("Access accepted");
        CardNumber = cards[i];
        access2 = true;
        }   
      } 
    if ((access2==true)) { 
      Serial.println("accepted");

      digitalWrite(33,HIGH); // red led is off
      digitalWrite(17,LOW); //open the door
      tone(32,1200,500); // buzzer sound to say that access is authorized
      delay(2000);
      digitalWrite(33,LOW); // green led is off
      digitalWrite(17,HIGH);
      // then open the door and wait for some time until the user enter
       // prepare the data to publish in json format
      /* const size_t capacity2 = JSON_OBJECT_SIZE(4);
      DynamicJsonDocument doc1(capacity2);
      JsonObject obj1 = doc1.to<JsonObject>(); */
      char time_string[20]; // allocate a buffer for the time string
      char date_string[11]; // allocate a buffer for the date string (YYYY-MM-DD)      
      getLocalTime(&timeinfo);
      strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &timeinfo);
      // get and store the date in a file
      strftime(date_string, sizeof(date_string), "%Y-%m-%d", &timeinfo);
      const size_t capacity = JSON_ARRAY_SIZE(3) + 3*JSON_OBJECT_SIZE(3);
      DynamicJsonDocument doc(capacity);
      JsonArray data1 = doc.to<JsonArray>();
      JsonObject obj1 = data1.createNestedObject();
      obj1["rfid"] = CardNumber;
      obj1["sens"] = "out";
      obj1["date"] = time_string;
      obj1["porte"] = 2;
      String jsonStr1;
      serializeJson(doc, jsonStr1);
      Serial.println(jsonStr1);
      String name = String("/backup-") + String(date_string) + String(".txt");
      const char* path = name.c_str();
      Serial.println(path);
      appendFile(SD, path, jsonStr1);
      String data; 
      readFileoffline(SD, "/data.txt",data);
        if (data == "")
        {
          appendFile(SD, "/data.txt", jsonStr1);
          //Serial.println(data);
        }
        else 
        {
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
               
      for(int i=0; i<3;i++){
     digitalWrite(33,HIGH);
      tone(32,500,500);
      delay(500);
      digitalWrite(33,HIGH);
      noTone(32); 
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
    readFileoffline(SD, "/data.txt",mouvement);    
    if (mouvement != "")
    {
      https.begin(serverName);
      https.addHeader("Authorization", "Bearer " + String(authToken));
      https.addHeader("Content-Type", "application/json");
      int httpResponseCode = https.POST(mouvement);
      //Serial.println(http.POST(jsonStr));
      //Serial.println(mouvement);
      // Check for errors
      if (httpResponseCode == HTTP_CODE_OK) { //HTTP_CODE_OK
        String response = https.getString();
        Serial.println("HTTP Response: " + response);
        deleteFile(SD, "/data.txt");
      } else {
        Serial.print("HTTP Error code: ");
        Serial.println(httpResponseCode);
        //Serial.println(https.getString());
      }
      https.end();
      }
      else {Serial.println("there is no data to be send");}
  }
  unsigned long currentMillis1 = millis();
  // check if it's time to call the function to check for update in users list
  if (currentMillis1 - previousMillis1 >= interval) {
    // save the last time you called the function
    previousMillis1 = currentMillis1;
   // WiFiClient client;    
    HTTPClient http;
    // call your function here
     String AllUserserver = "https://back.el-erp.saas.elastic-erp.com/api/allusers/2";
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
    readFileoffline(SD, "/users.txt",usersList);
    //usersList.replace(" ","");
    //response.replace(" ","");
    int response_length = response.length();
    int users_length = usersList.length();
    
    Serial.print(" response.length(): ");
    Serial.println(response_length);
    Serial.print("users.length(): ");
    Serial.println(users_length);
    // Compare the two strings using strcmp()
    if(usersList != response)
    {
      const int maxStringLength = 11; // Maximum length of each string
      int index = 1; // Current index in the payload string
      int count = 0; // Counter for the number of strings read
      while (count < MAX_STRING_COUNT && index < response.length()) {
        String currentString = response.substring(index + 1, index + maxStringLength ); // Extract the string from the payload, excluding the quotation marks
        cards[count] = currentString; // Assign the string to the array
        count++; // Increment the counter
        index += maxStringLength + 2; // Move the index to the next string, considering the quotation marks and comma
        
      }
      deleteFile(SD, "/users.txt");
      // save users new list in the file 
      appendFile(SD, "/users.txt", response);
      startIndex = count;
    }
    else {Serial.println("the list is the same nothing changes");}
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



void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
// this is the function for reading the data saved offline
void readFile(fs::FS &fs, const char * path, String tab[],int numCards){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  int i = 0;
  while(file.available() && i < numCards){
    tab[i] = file.readStringUntil('\n');
    Serial.println(tab[i]);
    i++;
  }
  file.close();
}
////*******************//////

void readFileoffline(fs::FS &fs, const char * path, String &data){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");
  data = file.readString();
  data.trim();
  //Serial.println(data);

  file.close();
}


void writeFile(fs::FS &fs, const char * path, const String message){
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const String message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.println(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

