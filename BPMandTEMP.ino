#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30105.h"
#include "heartRate.h"

// --- PIN DEFINITIONS ---
#define RED_LED 13
#define BLUE_LED 14
#define GREEN_LED 25
#define TEMP_PIN 26 

// --- OBJECTS ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MAX30105 particleSensor;
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// --- VARIABLES ---
unsigned long lastTempRequest = 0;
const int tempInterval = 2000; 
const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
int beatAvg = 0;
float bodyTemp = 0;
bool heartLarge = false; 

// Small Heart Bitmap for static state
static const unsigned char heart_bits[] = {
  0x0c, 0x30, 0x1e, 0x78, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 
  0x03, 0xc0, 0x01, 0x80
};

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // Non-blocking Temp Setup
  sensors.begin();
  sensors.setWaitForConversion(false); 
  sensors.requestTemperatures(); 

  Wire.begin(33, 32);
  u8g2.begin();
  
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) while(1);
  
  // Saturation Prevention
  particleSensor.setup(0x1F, 4, 2, 400, 411, 4096); 
}

void loop() {
  long irValue = particleSensor.getIR();

  if (irValue > 50000) {
    if (checkForBeat(irValue) == true) {
      heartLarge = true; // Trigger the animation
      long delta = millis() - lastBeat;
      lastBeat = millis();
      float bpm = 60 / (delta / 1000.0);

      if (bpm < 255 && bpm > 20) {
        rates[rateSpot++] = (byte)bpm;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  } else {
    beatAvg = 0;
  }

  // Update Temp every 2 seconds
  if (millis() - lastTempRequest >= tempInterval) {
    bodyTemp = sensors.getTempCByIndex(0);
    sensors.requestTemperatures(); 
    lastTempRequest = millis();
  }

  updateDisplay(irValue > 50000);
  
  // Heart shrink timing
  if (millis() - lastBeat > 250) heartLarge = false; 
}

// Function to handle the heart animation
void drawAnimatedHeart(int x, int y, bool large) {
  if (large) {
    u8g2.drawDisc(x, y, 6); // Left lobe
    u8g2.drawDisc(x+7, y, 6); // Right lobe
    u8g2.drawTriangle(x-5, y+2, x+12, y+2, x+3, y+11); // Bottom point
  } else {
    u8g2.drawXBM(x, y, 16, 10, heart_bits); 
  }
}

void updateDisplay(bool fingerDetected) {
  u8g2.clearBuffer();
  
  // --- HEADER ---
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "HEALTH MONITOR"); // Title updated
  u8g2.setCursor(95, 10);
  u8g2.print(bodyTemp, 1); u8g2.print("C");
  u8g2.drawHLine(0, 13, 128);

  if (fingerDetected) {
    // --- BPM DISPLAY ---
    u8g2.setFont(u8g2_font_logisoso32_tf); 
    u8g2.setCursor(20, 52);
    if(beatAvg > 0) u8g2.print(beatAvg); else u8g2.print("---");
    
    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.drawStr(90, 48, "BPM");

    // --- HEART ANIMATION ---
    drawAnimatedHeart(100, 22, heartLarge);

    // --- STATUS BOX ---
    u8g2.drawRFrame(0, 54, 128, 10, 2); 
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Logic Hierarchy
    if (bodyTemp > 38.5 || (beatAvg > 0 && (beatAvg > 140 || beatAvg < 40))) {
      u8g2.drawStr(25, 63, "STATE: DANGER");
      digitalWrite(RED_LED, HIGH); digitalWrite(BLUE_LED, LOW); digitalWrite(GREEN_LED, LOW);
    } 
    else if (bodyTemp >= 24.0 && bodyTemp <= 37.5 && beatAvg >= 40 && beatAvg <= 100) {
      u8g2.drawStr(22, 63, "STATE: HEALTHY");
      digitalWrite(GREEN_LED, HIGH); digitalWrite(RED_LED, LOW); digitalWrite(BLUE_LED, LOW);
    }
    else {
      u8g2.drawStr(20, 63, "STATE: READING...");
      digitalWrite(BLUE_LED, HIGH); digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW);
    }
  } else {
    // IDLE
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(25, 40, "READY");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 55, "Place finger to start");
    digitalWrite(RED_LED, LOW); digitalWrite(BLUE_LED, LOW); digitalWrite(GREEN_LED, LOW);
  }
  u8g2.sendBuffer();
}