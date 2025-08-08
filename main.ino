// main.ino
// ESP32-based Soil Sensor (NPK + Moisture + Temp + Optional LoRa)

#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define RE_PIN 4
#define DE_PIN 5

HardwareSerial rs485Serial(1); // UART1
Adafruit_AHTX0 aht;

// NPK Modbus commands
const byte N_CMD[]  = {0x01, 0x03, 0x00, 0x1E, 0x00, 0x01, 0xE4, 0x0C};
const byte P_CMD[]  = {0x01, 0x03, 0x00, 0x1F, 0x00, 0x01, 0xB5, 0xCC};
const byte K_CMD[]  = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xC0};

byte values[8];

void setup() {
  Serial.begin(115200);
  pinMode(RE_PIN, OUTPUT);
  pinMode(DE_PIN, OUTPUT);
  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);

  rs485Serial.begin(9600, SERIAL_8N1, 16, 17); // RX, TX (adjust as needed)
  Wire.begin();

  if (!aht.begin()) {
    Serial.println("AHT not detected");
  } else {
    Serial.println("AHT sensor online");
  }

  Serial.println("Soil sensor node ready\n");
}

byte readSensor(const byte *cmd) {
  digitalWrite(DE_PIN, HIGH);
  digitalWrite(RE_PIN, HIGH);
  delay(5);

  rs485Serial.write(cmd, 8);
  rs485Serial.flush();
  delay(100);

  digitalWrite(DE_PIN, LOW);
  digitalWrite(RE_PIN, LOW);

  int i = 0;
  unsigned long timeout = millis();
  while (rs485Serial.available() < 7 && millis() - timeout < 1000);

  while (rs485Serial.available() && i < 7) {
    values[i++] = rs485Serial.read();
  }

  return values[4];
}

void loop() {
  byte n = readSensor(N_CMD);
  byte p = readSensor(P_CMD);
  byte k = readSensor(K_CMD);

  Serial.println("\n--- Soil NPK Readings ---");
  Serial.print("N: "); Serial.print(n); Serial.println(" mg/kg");
  Serial.print("P: "); Serial.print(p); Serial.println(" mg/kg");
  Serial.print("K: "); Serial.print(k); Serial.println(" mg/kg");

  sensors_event_t humidity, temp;
  if (aht.getEvent(&humidity, &temp)) {
    Serial.println("--- Ambient Conditions ---");
    Serial.print("Temp: "); Serial.print(temp.temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humidity.relative_humidity); Serial.println(" %");
  }

  delay(5000);
} 