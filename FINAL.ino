#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "DHT.h" // Added DHT Library

// --- PIN MAPPING ---
#define DHTPIN 4       // DHT22 Data Pin
#define DHTTYPE DHT22
#define RED_LED 13
#define BLUE_LED 14
#define GREEN_LED 25
#define TEMP_PIN 26 
#define TILT_PIN 27
#define BUZZER 18

// --- OBJECTS ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MAX30105 particleSensor;
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
Adafruit_MPU6050 mpu;
DHT dht(DHTPIN, DHTTYPE); // Added DHT Object

// --- VARIABLES ---
unsigned long lastTempRequest = 0;
const int tempInterval = 2000; 
int beatAvg = 0;
long lastBeat = 0; 
float bodyTemp = 0;
float envTemp = 0; // Environment Temp
bool heartLarge = false;

// Fall Detection Variables
bool mpuActive = false;
bool potentialFall = false;
bool isFallEmergency = false;
unsigned long fallTime = 0;
const float impactThreshold = 10.0; 

// Heart Bitmap
static const unsigned char heart_bits[] = { 0x0c, 0x30, 0x1e, 0x78, 0x3f, 0xfc, 0x3f, 0xfc, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80 };

void setup() {
  Serial.begin(115200);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TILT_PIN, INPUT_PULLUP);

  Wire.begin(33, 32);
  u8g2.begin();
  dht.begin(); // Start DHT22

  if (mpu.begin()) {
    mpuActive = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  }

  sensors.begin();
  sensors.setWaitForConversion(false); 
  sensors.requestTemperatures(); 

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) while(1);
  particleSensor.setup(0x1F, 4, 2, 400, 411, 4096); 
}

void loop() {
  // 1. MOTION SCAN (Fall Detection)
  if (mpuActive) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float totalAcc = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

    if (totalAcc > impactThreshold && !potentialFall && !isFallEmergency) {
      potentialFall = true;
      fallTime = millis();
    }

    if (potentialFall) {
      if (millis() - fallTime < 5000) {
        if (digitalRead(TILT_PIN) == LOW) potentialFall = false; 
      } else {
        if (digitalRead(TILT_PIN) == HIGH) isFallEmergency = true;
        potentialFall = false;
      }
    }
  }

  // 2. VITALS SCAN (BPM & Temp)
  long irValue = particleSensor.getIR();
  if (irValue > 50000) {
    if (checkForBeat(irValue) == true) {
      heartLarge = true;
      long delta = millis() - lastBeat;
      lastBeat = millis();
      float bpm = 60 / (delta / 1000.0);
      if (bpm < 220 && bpm > 30) beatAvg = (int)bpm;
    }
  } else {
    beatAvg = 0;
  }

  if (millis() - lastTempRequest >= tempInterval) {
    bodyTemp = sensors.getTempCByIndex(0);
    if (bodyTemp < -50) bodyTemp = 0; 
    
    envTemp = dht.readTemperature(); // Read Environment Temperature
    
    sensors.requestTemperatures(); 
    lastTempRequest = millis();
  }

  // 3. MASTER DISPLAY & LED TALLY
  updateDisplay(irValue > 50000);
  
  if (millis() - lastBeat > 250) heartLarge = false; 
}

void updateDisplay(bool fingerDetected) {
  u8g2.clearBuffer();
  
  // --- HEADER ---
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "HEALTH MONITOR");
  u8g2.setCursor(95, 10);
  u8g2.print(bodyTemp, 1); u8g2.print("C");
  u8g2.drawHLine(0, 13, 128);

  // --- PRIORITY 1: FALL EMERGENCY ---
  if (isFallEmergency) {
    // Beautiful Alert: Inverse screen flashing
    if ((millis() / 500) % 2 == 0) {
      u8g2.drawBox(0, 14, 128, 50);
      u8g2.setDrawColor(0); // Draw black text on white box
    }
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(30, 45, "FALL!");
    u8g2.setDrawColor(1); // Reset to white text
    
    digitalWrite(RED_LED, HIGH); digitalWrite(BUZZER, HIGH);
    digitalWrite(GREEN_LED, LOW); digitalWrite(BLUE_LED, LOW);
  } 

  // --- PRIORITY 2: FALL WAITING ---
  else if (potentialFall) {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(25, 40, "IMPACT!");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 60, "STAY UPRIGHT (5s)");
    
    digitalWrite(BLUE_LED, (millis() % 400 < 200)); 
    digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW); digitalWrite(BUZZER, LOW);
  }

  // --- PRIORITY 3: NORMAL MONITORING ---
  else if (fingerDetected) {
    // Main BPM Display
    u8g2.setFont(u8g2_font_logisoso24_tf); 
    u8g2.setCursor(15, 48);
    if(beatAvg > 0) u8g2.print(beatAvg); else u8g2.print("---");
    
    // Beautiful Pulsing Heart
    if(heartLarge) {
        u8g2.drawDisc(110, 35, 7); // Large heart pulse
    } else {
        u8g2.drawXBM(102, 30, 16, 10, heart_bits);
    }

    // Environment Data Footer
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setCursor(0, 63);
    u8g2.print("ROOM TEMP: "); u8g2.print(envTemp, 1); u8g2.print("C");

    // Danger/State Logic
    u8g2.setFont(u8g2_font_6x10_tf);
    if (bodyTemp > 38.5 || (beatAvg > 0 && (beatAvg > 140 || beatAvg < 40))) {
      u8g2.drawStr(75, 52, "DANGER");
      digitalWrite(RED_LED, HIGH); digitalWrite(BUZZER, HIGH);
      digitalWrite(GREEN_LED, LOW); digitalWrite(BLUE_LED, LOW);
    } else if (bodyTemp >= 24.0 && beatAvg >= 40) {
      u8g2.drawStr(75, 52, "HEALTHY");
      digitalWrite(GREEN_LED, HIGH); 
      digitalWrite(RED_LED, LOW); digitalWrite(BLUE_LED, LOW); digitalWrite(BUZZER, LOW);
    } else {
      u8g2.drawStr(75, 52, "SYNC...");
      digitalWrite(BLUE_LED, HIGH); 
      digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW); digitalWrite(BUZZER, LOW);
    }
  } 

  // --- IDLE ---
  else {
    // Breathing Animation for Idle
    int yOffset = (sin(millis() / 400.0) * 3); 
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(20, 45 + yOffset, "READY SCAN");
    
    digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW); digitalWrite(BLUE_LED, LOW); digitalWrite(BUZZER, LOW);
  }
  u8g2.sendBuffer();
}