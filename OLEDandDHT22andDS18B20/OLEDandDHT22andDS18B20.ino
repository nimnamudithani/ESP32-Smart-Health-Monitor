#include <Arduino.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 1. OLED Setup (SH1106 1.3")
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 22, /* data=*/ 21, /* reset=*/ U8X8_PIN_NONE);

// 2. DHT22 Setup (Environment)
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// 3. DS18B20 Setup (Body Temp)
#define ONE_WIRE_BUS 5
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature bodySensor(&oneWire);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  u8g2.begin();
  dht.begin();
  bodySensor.begin();
  
  Serial.println("All Sensors Initialized.");
}

void loop() {
  // Read DHT22
  float airTemp = dht.readTemperature();
  
  // Read DS18B20
  bodySensor.requestTemperatures(); 
  float bodyTemp = bodySensor.getTempCByIndex(0);

  u8g2.clearBuffer();
  
  // Top Section: Environment
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 10, "ENV TEMP:");
  u8g2.setCursor(70, 10);
  u8g2.print(airTemp, 1); u8g2.print(" C");

  // Middle Section: Body Temp
  u8g2.drawHLine(0, 15, 128);
  u8g2.drawStr(0, 30, "BODY TEMP:");
  
  u8g2.setFont(u8g2_font_logisoso24_tf); 
  u8g2.setCursor(0, 60);
  
  if(bodyTemp == -127.00) {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.print("Check Wiring!");
  } else {
    u8g2.print(bodyTemp, 1);
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.print(" C");
  }

  u8g2.sendBuffer();
  delay(1000);
}