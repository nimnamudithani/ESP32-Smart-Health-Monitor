#include "DHT.h"

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // We are using the DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println(F("DHT22 Test Starting..."));
  dht.begin();
}

void loop() {
  // Wait a few seconds between measurements
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature(); // Read temperature as Celsius

  // Check if any reads failed
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Error: Failed to read from DHT sensor! Check your wiring."));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  |  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C"));
}