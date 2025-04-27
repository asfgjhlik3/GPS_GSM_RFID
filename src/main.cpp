#include <MFRC522.h>
#include <Wire.h>
#include <DS3231.h>

#define LILYGO_T_A7670
#define TINY_GSM_RX_BUFFER 1024
#include "utilities.h"
#include <TinyGsmClient.h>
#include <TinyGPS++.h>

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

struct Passenger {
  String uid;
  String tapInStop;
  unsigned long tapInTime;
};

#define MAX_PASSENGERS 50
Passenger onboardPassengers[MAX_PASSENGERS];
int passengerCount = 0;

// Fare
const int FARE = 15;

// Dummy recipient number
String phoneNumber = "+639518840538";

int readBalance() { return 100; }
bool writeBalance(int newBalance) { return true; }

int findPassengerIndex(String uid) {
  for (int i = 0; i < passengerCount; i++) {
    if (onboardPassengers[i].uid == uid) return i;
  }
  return -1;
}

void sendSMS(String message) {
  Serial.println("📤 Sending SMS...");
  if (modem.sendSMS(phoneNumber, message)) {
    Serial.println("✅ SMS sent!");
  } else {
    Serial.println("❌ SMS failed.");
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
  Serial.begin(115200);
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS (last one is optional)
  rfid.PCD_Init();
  Wire.begin();
  Serial.println("🚍 RFID Fare System Ready");
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
        onboardPassengers[passengerCount++] = { uid, "N/A", millis() };
        Serial.println("✅ Tap-In recorded");
        Serial.print("🆔 UID: "); Serial.println(uid);
      } else {
        Serial.println("❌ Max passengers reached");
      }

      rfid.PICC_HaltA();
      return;
    }

    int balance = readBalance();

    if (balance >= FARE) {
      int newBalance = balance - FARE;
      if (writeBalance(newBalance)) {
        unsigned long travelTime = (millis() - onboardPassengers[index].tapInTime) / 1000;
        Serial.println("✅ Tap-Out Success");
        Serial.print("💸 ₱"); Serial.print(FARE);
        Serial.print(" deducted | Remaining: ₱"); Serial.println(newBalance);
        Serial.print("🕒 Duration: "); Serial.print(travelTime); Serial.println("s");

        String gpsData = getLocationString();

String smsMsg = "🚌 e-Ticket\nFare: ₱" + String(FARE) + 
                "\nCard: " + uid + 
                "\nBalance: ₱" + newBalance + 
                "\n" + gpsData;

        sendSMS(smsMsg);

        // Remove passenger from list
        for (int i = index; i < passengerCount - 1; i++) {
          onboardPassengers[i] = onboardPassengers[i + 1];
        }
        passengerCount--;
      }
    } else {
      Serial.println("❌ Not enough balance.");
    }

    rfid.PICC_HaltA();  
  }
}
