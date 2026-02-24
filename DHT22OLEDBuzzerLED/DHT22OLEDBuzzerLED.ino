#include <Arduino.h>
#include <U8g2lib.h>
#include <DHT.h>

// OLED & DHT Setup
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 22, 21, U8X8_PIN_NONE);
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Alert Pins
const int BUZZER_PIN = 18;
const int RED_LED = 12;
const int GREEN_LED = 13;
const int BLUE_LED = 14;

void setup() {
  Serial.begin(115200);
  dht.begin();
  u8g2.begin();
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
}

void loop() {
  float temp = dht.readTemperature();
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  
  if (isnan(temp)) {
    u8g2.drawStr(0, 30, "Sensor Error");
  } else {
    u8g2.setCursor(0, 30);
    u8g2.print("Temp: "); u8g2.print(temp); u8g2.print("C");

    // SAFETY LOGIC
    if (temp > 30.0) { // Threshold for "Too Hot"
      u8g2.drawStr(0, 55, "!!! DANGER !!!");
      
      // Buzzer Beep
      digitalWrite(BUZZER_PIN, HIGH);
      // Red LED "Soft Glow" (Safety without resistors)
      analogWrite(RED_LED, 50); 
      digitalWrite(GREEN_LED, LOW);
      
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      analogWrite(RED_LED, 0);
    } else {
      u8g2.drawStr(0, 55, "Status: Safe");
      // Green LED "Soft Glow"
      analogWrite(GREEN_LED, 50);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }
  }
  
  u8g2.sendBuffer();
  delay(1000);
}