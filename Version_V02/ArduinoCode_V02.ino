#include <DHT.h>
#include <Wire.h>

// DHT11 setup
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Gas sensors
#define MQ135_PIN A0
#define MQ4_PIN A1

// RTC setup
#define RTC_ADDRESS 0x51  // Likely PCF8563

// Function declarations
void setRTC();
void readRTC();
byte decToBcd(byte val);
byte bcdToDec(byte val);

void setup() {
  Wire.begin();
  Serial.begin(115200);  // Match with ESP32 Serial2 baud rate
  dht.begin();

  Serial.println("System Initializing...");
  setRTC();  // Optional: Uncomment if you want to set RTC manually
}

void loop() {
  // Read from RTC
  readRTC();

  // Read DHT11 values
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  // Read gas sensor values
  int mq135_raw = analogRead(MQ135_PIN);
  int mq4_raw = analogRead(MQ4_PIN);

  int cal_mq135 = 0;
  int cal_mq4 = 0;

  if (mq135_raw < 200){
    cal_mq135 = 0;
  }
  else{
    cal_mq135 = mq135_raw - 200;
  }

  if (mq4_raw < 260){
    cal_mq4 = 0;
  }
  else{
    cal_mq4 = mq4_raw - 260;
  }

  delay(100);  // Stabilize readings

  // Display sensor values
  Serial.print("Temp: ");
  Serial.print(temp, 1);
  Serial.print(" °C, ");

  Serial.print("Humidity: ");
  Serial.print(hum, 1);
  Serial.print(" %, ");

  Serial.print("AQI (MQ135): ");
  Serial.print(cal_mq135);
  Serial.print(", ");

  Serial.print("Methane (MQ4 ppm): ");  
  Serial.println(cal_mq4);

  delay(3000); // Delay between readings
}

// Placeholder to manually set RTC time (customize this function)
void setRTC() {
  // // Example: Set to 11 June 2025, 15:30:00
  // Wire.beginTransmission(RTC_ADDRESS);
  // Wire.write(0x04); // Set starting register (time)
  // Wire.write(decToBcd(00));    // Seconds
  // Wire.write(decToBcd(59));   // Minutes
  // Wire.write(decToBcd(12));   // Hours
  // Wire.write(decToBcd(12));   // Day
  // Wire.write(decToBcd(3));    // Weekday (0-6)
  // Wire.write(decToBcd(6));    // Month
  // Wire.write(decToBcd(25));   // Year (last two digits of 2025)
  // Wire.endTransmission();
}

void readRTC() {
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x04); // Start reading from time registers
  Wire.endTransmission();

  Wire.requestFrom(RTC_ADDRESS, 7);

  if (Wire.available() == 7) {
    int sec = bcdToDec(Wire.read() & 0x7F);
    int min = bcdToDec(Wire.read());
    int hour = bcdToDec(Wire.read());
    int day = bcdToDec(Wire.read());
    int weekday = bcdToDec(Wire.read());
    int month = bcdToDec(Wire.read() & 0x1F);
    int year = bcdToDec(Wire.read()) + 2000;

    Serial.print("Time: ");
    if (hour < 10) Serial.print('0');
    Serial.print(hour);
    Serial.print(":");
    if (min < 10) Serial.print('0');
    Serial.print(min);
    Serial.print(":");
    if (sec < 10) Serial.print('0');
    Serial.println(sec);

    Serial.print("Date: ");
    Serial.print(day);
    Serial.print("/");
    Serial.print(month);
    Serial.print("/");
    Serial.println(year);
  } else {
    Serial.println("RTC read error!");
  }
}

byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

byte bcdToDec(byte val) {
  return ((val / 16 * 10) + (val % 16));
}
