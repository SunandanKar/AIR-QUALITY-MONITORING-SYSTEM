#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define MQ135_PIN A0
#define MQ4_PIN A1

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);  // Match with ESP32 Serial2 baud rate
  dht.begin();
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  int mq135_raw = analogRead(MQ135_PIN);
  int mq4_raw = analogRead(MQ4_PIN);

  // Optional: Basic smoothing/filtering
  delay(100); // Short delay for stable readings

  // Formatting and printing the data string
  Serial.print("Temp: ");
  Serial.print(temp, 1);
  Serial.print(" °C, ");

  Serial.print("Humidity: ");
  Serial.print(hum, 1);
  Serial.print(" %, ");

  Serial.print("AQI (MQ135): ");
  Serial.print(mq135_raw);
  Serial.print(", ");

  Serial.print("Methane (MQ4 ppm): ");
  Serial.println(mq4_raw);

  delay(3000); // Send data every 3 seconds
}
