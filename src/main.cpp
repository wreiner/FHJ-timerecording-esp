// -- Arduino and std libraries
#include <Arduino.h>

// I2C
#include <Wire.h>
#include <SPI.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
// -- Hardware specific libraries
// Wifi
#include <NTPClient.h>
#include <DNSServer.h>
#include <ESP8266WiFi.h>
#include <MFRC522.h>
#include <WiFiManager.h>
#include "SSD1306Wire.h"
#include <Firebase.h>

constexpr uint8_t RST_PIN = D3;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = D4;     // Configurable, see typical pin layout above
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;
String tag;

/* 2. Define the API Key */
#define API_KEY "plsset"

/* 3. Define the RTDB URL */
#define DATABASE_URL "plsset"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "plsset"
#define USER_PASSWORD "plsset"


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;

// put function declarations here:
void init_bus_systems();
void init_ssd1306();
void initWiFi();
void print_ssd1306(const char* msg);
void print_ssd1306(String msg);
void handleRFID();
bool update_user_presence();
bool add_time_record(bool presence);

// -- WIFI
#define WIFI_SSID "plsset"
#define WIFI_KEY  "plsset"
// #define BUZZER_PIN D0

/*
 * -- GLOBAL VARS --
 */
WiFiClient espClient;
bool buttonArrivePressed = false;
bool buttonLeavePressed = false;

SSD1306Wire display(0x3c, SDA, SCL);

void setup() {
  // UART and I2C
  init_bus_systems();
  SPI.begin();

  init_ssd1306();
  Serial.println("initializing ..");
  print_ssd1306("initializing ..");

  // pinMode(BUZZER_PIN, OUTPUT);

  // Init MFRC522
  rfid.PCD_Init();

  initWiFi();
  timeClient.begin();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  Firebase.begin(&config, &auth);
}

int j = 0;
void loop() {
  if (j % 100 == 0) {
    Serial.println("looping ..");
    print_ssd1306("Please scan card");
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
  }
  j++;

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    tag = "";

    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    
    Serial.println("Read tag " + tag);
    print_ssd1306("Read tag " + tag);
    
    update_user_presence();
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}


void rcloop() {
  if (j % 100 == 0) {
    Serial.println("looping ..");
    print_ssd1306("Pls scan card!");
    tag = "";
  }
  j++;

  if (tag != "") {
    Serial.println("tag is something");
  } 

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (rfid.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    
    Serial.println(tag);
    
    print_ssd1306(tag);
    
    // tag = "";
    
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

// put function definitions here:
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_KEY);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void init_bus_systems()
{
  Serial.begin(115200);
  Wire.begin(D2, D1);
}

void handleRFID() {
  return;
}


/********************************************************
 * -- SSD1306 --
 ********************************************************/
void init_ssd1306()
{
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  // display.setFont(ArialMT_Plain_10);
}

void print_ssd1306(const char* msg)
{
  // clear the display
  display.clear();

  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  String displine1 = String(msg);
  // String displine2 = "Temp: " + String(temp);

  display.drawString(0, 10, displine1);
  // display.drawString(0, 26, displine2);
  
  // write the buffer to the display
  display.display();
}

void print_ssd1306(String msg)
{
  // clear the display
  display.clear();

  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  String displine1 = msg;
  // String displine2 = "Temp: " + String(temp);

  display.drawString(0, 10, displine1);
  // display.drawString(0, 26, displine2);
  
  // write the buffer to the display
  display.display();
}

/********************************************************
 * -- FIREBASE FUNCTIONS --
 ********************************************************/
bool update_user_presence() {
  Serial.println("will update presence for tag " + tag);

  bool current_presence_status = true;
  if (Firebase.ready()) {
    Serial.println("firebase ready");

    String employee_path = "/employees/" + tag + "/presence";

    Firebase.getBool(fbdo, employee_path, &current_presence_status);
    Serial.println("read current presence" + current_presence_status);
    Firebase.setBool(fbdo, employee_path, !current_presence_status);
    
    add_time_record(!current_presence_status);

    // Serial.printf("Get shallow data... %s\n", Firebase.getShallowData(fbdo, "/") ? "ok" : fbdo.errorReason().c_str());

    // if (fbdo.httpCode() == FIREBASE_ERROR_HTTP_CODE_OK) {
    //   printResult(fbdo);
    // }
  }

  String msg;
  if (current_presence_status) {
    msg = " left";
  } else {
    msg = " arrived";
  }

  print_ssd1306(tag + msg);

  return true;
}


bool add_time_record(bool presence) {
  Serial.println("will add time record for tag " + tag + " with presence " + presence);

  if (Firebase.ready()) {
    FirebaseJson jsonData;

    timeClient.update();

    // Get the current time as a tm structure
    struct tm timeinfo;
    time_t rawtime = timeClient.getEpochTime();
    localtime_r(&rawtime, &timeinfo);

    // Format and print the date and time
    char date[11];
    sprintf(date, "%04d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    
    char time[9];
    sprintf(time, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    jsonData.add("date", date);
    jsonData.add("time", time);
    jsonData.add("type", presence ? "arrive" : "leave");

    // Add the JSON object to the array under the specified path
    String path = "/timerecords/" + tag;
    if (Firebase.pushJSON(fbdo, path, jsonData)) {
      Serial.println("Data pushed to Firebase successfully!");
    } else {
      Serial.println("Failed to push data to Firebase.");
      // Serial.println("Error: " + firebaseData.errorReason());
    }
  }

  return true;
}
