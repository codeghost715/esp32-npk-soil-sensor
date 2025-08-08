
// main.ino - ESP32 Soil Sensor with RS485 NPK/Moisture, AHT20, LoRa (RYLR998), Google Sheets Logging

#include <Wire.h>
#include <HardwareSerial.h>
#include <Adafruit_AHTX0.h>
#include "TRIGGER_WIFI.h"
#include "TRIGGER_GOOGLESHEETS.h"

// RS485 Pins
#define RE_PIN 4
#define DE_PIN 5

// RS485 Command Set
const byte N_CMD[]    = {0x01, 0x03, 0x00, 0x1E, 0x00, 0x01, 0xE4, 0x0C};
const byte P_CMD[]    = {0x01, 0x03, 0x00, 0x1F, 0x00, 0x01, 0xB5, 0xCC};
const byte K_CMD[]    = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xC0};
const byte MOIST_CMD[]= {0x01, 0x03, 0x00, 0x23, 0x00, 0x01, 0x25, 0xCD};

HardwareSerial rs485Serial(1);   // UART1 for RS485 (NPK)
HardwareSerial loraSerial(2);    // UART2 for RYLR998 (LoRa)
Adafruit_AHTX0 aht;

byte values[8];
float valN, valP, valK, valMoisture;
float airTemp, airHumidity;

// Google Sheets Setup
char column_name_in_sheets[][8] = {"N", "P", "K", "Moist", "AirT", "AirH"};
String Sheets_GAS_ID = "YOUR_GAS_ID_HERE";  // Replace with your real GAS ID
int No_of_Parameters = 6;

void setup() {
  Serial.begin(115200);

  pinMode(RE_PIN, OUTPUT);
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(RE_PIN, LOW);
  digitalWrite(DE_PIN, LOW);

  rs485Serial.begin(9600, SERIAL_8N1, 16, 17);  // RX, TX for RS485
  loraSerial.begin(115200, SERIAL_8N1, 26, 25); // RX, TX for LoRa RYLR998
  Wire.begin();

  if (!aht.begin()) {
    Serial.println("AHT20 not detected");
  } else {
    Serial.println("AHT20 ready");
  }

  WIFI_Connect("yourSSID", "yourPASSWORD");
  Google_Sheets_Init(column_name_in_sheets, Sheets_GAS_ID, No_of_Parameters);
}

byte readSensor(const byte *cmd) {
  digitalWrite(RE_PIN, HIGH);
  digitalWrite(DE_PIN, HIGH);
  delay(10);
  rs485Serial.write(cmd, 8);
  rs485Serial.flush();
  digitalWrite(RE_PIN, LOW);
  digitalWrite(DE_PIN, LOW);
  delay(300);

  int i = 0;
  while (rs485Serial.available() < 7 && i < 1000) { delay(1); i++; }
  for (i = 0; i < 7; i++) {
    if (rs485Serial.available()) values[i] = rs485Serial.read();
  }
  return values[4];
}

void loop() {
  // Read RS485 Values
  valN = readSensor(N_CMD);
  valP = readSensor(P_CMD);
  valK = readSensor(K_CMD);
  valMoisture = readSensor(MOIST_CMD);

  // Read AHT20
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  airTemp = temp.temperature;
  airHumidity = humidity.relative_humidity;

  Serial.println("=================");
  Serial.print("N: "); Serial.println(valN);
  Serial.print("P: "); Serial.println(valP);
  Serial.print("K: "); Serial.println(valK);
  Serial.print("Moisture: "); Serial.println(valMoisture);
  Serial.print("Air Temp: "); Serial.println(airTemp);
  Serial.print("Air Humidity: "); Serial.println(airHumidity);

  // Send to Google Sheets
  Data_to_Sheets(No_of_Parameters, valN, valP, valK, valMoisture, airTemp, airHumidity);

  // Send to LoRa
  String payload = String("N:") + valN + ",P:" + valP + ",K:" + valK + ",Moist:" + valMoisture + ",T:" + airTemp + ",H:" + airHumidity;
  loraSerial.print("AT+SEND=0," + String(payload.length()) + "," + payload + "\r\n");
  delay(300);
  Serial.println(loraSerial.readString());

  delay(10000);  // Delay 10s between cycles
}
