#include <Wire.h>

void setup() {
  Wire.begin(21, 22);
  Serial.begin(115200);
  while (!Serial);
  Serial.println("\nI2C Scanner - Finding your sensors...");
}

void loop() {
  byte error, address;
  int nDevices = 0;

  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);

      if (address == 0x3C || address == 0x3D) Serial.println(" (OLED Screen)");
      else if (address == 0x68) Serial.println(" (MPU6050 Gyro)");
      else if (address == 0x57) Serial.println(" (MAX30102 Heart Rate)");
      else Serial.println(" (Unknown Device)");

      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("No I2C devices found. Check SDA/SCL wiring!");
  else Serial.println("Scan complete.\n");
  delay(5000);
}