#include <MFRC522.h>
#include <Wire.h>
#include <DS3231.h>

#define LILYGO_T_A7670
#define TINY_GSM_RX_BUFFER 1024
#include "utilities.h"
#include <iostream>
#include "stops.h"
#include "fare.h"
#include <TinyGsmClient.h>
#include <TinyGPS++.h>
#include <ArduinoHttpClient.h>  // For HTTP requests
#include <WiFi.h>
#include <esp_now.h>

#define RST_PIN 4
#define SS_PIN 5

#define SerialMon Serial
#define TINY_GSM_DEBUG SerialMon

MFRC522 rfid(SS_PIN, RST_PIN);
DS3231 rtc;

// Setup modem
#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TinyGsmClient client(modem);
HttpClient http(client, server, port);

typedef struct struct_message {
  char uid[20];
} struct_message;

uint8_t driverMAC[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};  // Replace with your Driver ESP32 MAC address


struct Passenger {
  String uid;
  String tapInStop;
  unsigned long tapInTime;
};

#define MAX_PASSENGERS 50
Passenger onboardPassengers[MAX_PASSENGERS];
int passengerCount = 0;

const char server[] = "192.168.1.100";  // Replace with your local server IP
const int port = 80;

int readBalance() { return 100; }
bool writeBalance(int newBalance) { return true; }

int findPassengerIndex(String uid) {
  for (int i = 0; i < passengerCount; i++) {
    if (onboardPassengers[i].uid == uid) return i;
  }
  return -1;
}

void sendSMS(const String& uid, const String& message) {
  String phoneNumber = getPhoneNumberByUID(uid);
  if (phoneNumber == "") {
    Serial.println("❌ No phone number found for UID: " + uid);
    return;
  }

  Serial.println("📤 Sending SMS to " + phoneNumber);
  if (modem.sendSMS(phoneNumber.c_str(), message)) {
    Serial.println("✅ SMS sent!");
  } else {
    Serial.println("❌ SMS failed.");
  }
}


String getPhoneNumberByUID(const String& uid) {
  String path = "/get-phone?uid=" + uid;
  http.get(path);

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  if (statusCode == 200) {
    response.trim();
    return response;  // Assuming the response is just the phone number
  } else {
    Serial.println("❌ Failed to retrieve phone number. Status: " + String(statusCode));
    return "";
  }
}


String getLocationString() {
  float lat = 0, lon = 0, speed = 0, alt = 0, acc = 0;
  int vsat = 0, usat = 0, yr = 0, mon = 0, day = 0, hr = 0, min = 0, sec = 0;
  uint8_t fixMode = 0;

  for (int i = 10; i > 0; i--) {
    Serial.println("📡 Getting GPS...");
    if (modem.getGPS(&fixMode, &lat, &lon, &speed, &alt, &vsat, &usat, &acc, &yr, &mon, &day, &hr, &min, &sec)) {
      String location = "📍 Location:\nLat: " + String(lat, 6) + "\nLon: " + String(lon, 6);
      location += "\nDate: " + String(yr) + "-" + String(mon) + "-" + String(day);
      location += "\nTime: " + String(hr) + ":" + String(min) + ":" + String(sec);
      return location;
    }
    delay(3000);
  }
  return "❌ GPS unavailable.";
}

void setup() {
  Serial.begin(115200);

#ifdef BOARD_POWERON_PIN
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL); delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL); delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW); delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH); delay(1000);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

  Serial.println("🔄 Initializing modem...");
  int retry = 0;
  while (!modem.testAT(1000)) {
    Serial.print(".");
    if (++retry > 10) {
      digitalWrite(BOARD_PWRKEY_PIN, LOW); delay(100);
      digitalWrite(BOARD_PWRKEY_PIN, HIGH); delay(1000);
      digitalWrite(BOARD_PWRKEY_PIN, LOW);
      retry = 0;
    }
  }
  modem.sendAT("+CMGF=1");  // Set SMS mode to text
  if (modem.waitResponse(1000) == 1) {
    Serial.println("✅ Text Mode set.");
  } else {
    Serial.println("❌ Failed to set Text Mode");
  }

  // Enable GPS
  Serial.println("\n🛰️ Enabling GPS...");
  while (!modem.enableGPS(MODEM_GPS_ENABLE_GPIO)) {
    Serial.print(".");
  }
  Serial.println("\n✅ GPS Enabled");
  
  // ✅ Configure SMS Mode
  modem.sendAT("+CMGF=1");  // Set SMS to text mode
  if (modem.waitResponse(1000) == 1) {
    modem.sendAT("+CSCS=\"GSM\"");  // Use GSM character set
    if (modem.waitResponse(1000) == 1) {
      Serial.println("✅ Modem ready for SMS");
    } else {
      Serial.println("❌ Failed to set character set");
    }
  } else {
    Serial.println("❌ Failed to set SMS text mode");
  }
  WiFi.mode(WIFI_STA);  // Required for ESP-NOW
WiFi.disconnect();

if (esp_now_init() != ESP_OK) {
  Serial.println("❌ Error initializing ESP-NOW");
  return;
}

esp_now_peer_info_t peerInfo = {};
memcpy(peerInfo.peer_addr, driverMAC, 6);
peerInfo.channel = 0;  
peerInfo.encrypt = false;

if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  Serial.println("❌ Failed to add ESP-NOW peer");
}

  
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS (last one is optional)
  rfid.PCD_Init();
  Wire.begin();
  Serial.println("🚍 RFID Fare System Ready");
}
extern float fareMatrix[21][21];  // Your fare matrix, assuming it's 21x21
extern String busStops[21];  

int findStopIndex(String stopName) {
  for (int i = 0; i < 21; i++) {  // Assuming 21 bus stops
    if (busStops[i] == stopName) {
      return i;  // Return the index if the stop name matches
    }
  }
  return -1;  // Return -1 if the stop name was not found
}

void loop() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Convert UID to string
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      uid += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
      uid += String(rfid.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    Serial.println("✅ UID: " + uid);

    int index = findPassengerIndex(uid);

    if (index == -1) {
      if (passengerCount < MAX_PASSENGERS) {
        float lat, lon, speed, alt, acc;
        uint8_t fixMode;
        int vsat, usat, yr, mon, day, hr, min, sec;
    
        String tapInStop = "N/A";
        if (modem.getGPS(&fixMode, &lat, &lon, &speed, &alt, &vsat, &usat, &acc, &yr, &mon, &day, &hr, &min, &sec)) {
          tapInStop = getNearestStopFromGPS(lat, lon);
        }
    
        onboardPassengers[passengerCount++] = { uid, tapInStop, millis() };
        Serial.println("✅ Tap-In recorded");
        // Send UID via ESP-NOW
struct_message message;
uid.toCharArray(message.uid, sizeof(message.uid));
esp_err_t result = esp_now_send(driverMAC, (uint8_t *) &message, sizeof(message));

if (result == ESP_OK) {
  Serial.println("📤 UID sent via ESP-NOW: " + uid);
} else {
  Serial.println("❌ Failed to send UID via ESP-NOW");
}

        
        Serial.print("🆔 UID: "); Serial.println(uid);
      } else {
        Serial.println("❌ Max passengers reached");
      }
    
      rfid.PICC_HaltA();
      return;
    }    

    int balance = readBalance();

    if (balance >= getFare) {
      // Get current GPS location
float lat = 0, lon = 0;
uint8_t fixMode = 0;
int vsat = 0, usat = 0, yr = 0, mon = 0, day = 0, hr = 0, min = 0, sec = 0;
float speed, alt, acc;

if (modem.getGPS(&fixMode, &lat, &lon, &speed, &alt, &vsat, &usat, &acc, &yr, &mon, &day, &hr, &min, &sec)) {
    String tapOutStop = getNearestStopFromGPS(lat, lon);
    String tapInStop = onboardPassengers[index].tapInStop;

    int tapInIndex = findStopIndex(tapInStop);
        int tapOutIndex = findStopIndex(tapOutStop);

        // Calculate the fare using the fare matrix
        if (tapInIndex >= 0 && tapOutIndex >= 0) {
          float fare = getFare(tapInIndex, tapOutIndex, false);  // Assuming false for non-reverse direction
          if (fare < 0) {
            Serial.println("❌ Unable to calculate fare.");
            return;
          }

    int newBalance = balance - (int)fare;
      if (writeBalance(newBalance)) {
        unsigned long travelTime = (millis() - onboardPassengers[index].tapInTime) / 1000;
        Serial.println("✅ Tap-Out Success");
        Serial.print("💸 ₱"); Serial.print(fare);
        Serial.print(" deducted | Remaining: ₱"); Serial.println(newBalance);
        Serial.print("🕒 Duration: "); Serial.print(travelTime); Serial.println("s");

        String gpsData = getLocationString();

String smsMsg = "🚌 e-Ticket\nFare: ₱" + String(fare) + 
                "\nCard: " + uid + 
                "\nBalance: ₱" + newBalance + 
                "\n" + gpsData;

                sendSMS(uid, smsMsg);

        // Remove passenger from list
        for (int i = index; i < passengerCount - 1; i++) {
          onboardPassengers[i] = onboardPassengers[i + 1];
        }
        passengerCount--;
      }
    } else {
      Serial.println("❌ Invalid bus stop indices.");
    }
  }else {
      Serial.println("❌ Not enough balance.");
    }

    rfid.PICC_HaltA();  
  }
}
