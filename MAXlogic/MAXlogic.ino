//updated by rivini
#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

// --- Pin Definitions ---
#define BUZZER_PIN 19
#define RED_LED_PIN 13
#define BLUE_LED_PIN 14
#define GREEN_LED_PIN 25

// --- Display & Sensor Setup ---
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
MAX30105 particleSensor;

const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg = 0;

void setup() {
  Serial.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  // Force all off at start
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(33, 32);
  Wire.setClock(100000);

  u8g2.begin();
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    while(1);
  }
  particleSensor.setup(0x1F, 4, 2, 100, 411, 4096);
}

void loop() {
  long irValue = particleSensor.getIR();

  if (irValue > 50000) {
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);

      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
    updateSystem(true);
  } else {
    beatAvg = 0;
    updateSystem(false);
  }
}

void updateSystem(bool fingerDetected) {
  u8g2.clearBuffer();
  
  // 1. TITLE
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "HEALTH MONITOR");
  u8g2.drawHLine(0, 12, 128);

  if (fingerDetected) {
    // 2. HEART ANIMATION (Placed in front of BPM)
    if ((millis() / 500) % 2 == 0) {
      u8g2.setFont(u8g2_font_open_iconic_human_2x_t);
      u8g2.drawGlyph(5, 45, 66); // X=5
    }

    // 3. BPM RATE
    u8g2.setFont(u8g2_font_logisoso24_tf); 
    u8g2.setCursor(30, 45); // Shifted right (X=30) to make room for heart
    if(beatAvg > 0) u8g2.print(beatAvg);
    else u8g2.print("--");

    // 4. STATE & LED LOGIC
    u8g2.setFont(u8g2_font_7x14_tr);
    u8g2.setCursor(0, 62);

    // Boolean checks for stability
    bool isDanger = (beatAvg > 0 && (beatAvg < 40 || beatAvg > 140));
    bool isWarning = (beatAvg > 100 && beatAvg <= 140);
    bool isHealthy = (beatAvg >= 40 && beatAvg <= 100);

    if (isDanger) {
      u8g2.print("!! DANGER !!");
      digitalWrite(RED_LED_PIN, HIGH);
      digitalWrite(BLUE_LED_PIN, LOW);
      digitalWrite(GREEN_LED_PIN, LOW);
      
      // Pulse Buzzer ONLY in Danger
      if ((millis() / 250) % 2 == 0) tone(BUZZER_PIN, 1500); 
      else noTone(BUZZER_PIN);
    } 
    else if (isWarning) {
      u8g2.print("STATE: WARNING");
      // Force Mute
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      
      digitalWrite(BLUE_LED_PIN, HIGH);
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
    } 
    else if (isHealthy) {
      u8g2.print("STATE: HEALTHY");
      // Force Mute
      noTone(BUZZER_PIN);
      digitalWrite(BUZZER_PIN, LOW);
      
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(BLUE_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
    }
  } else {
    // IDLE
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.drawStr(10, 45, "PLACE FINGER");
    
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);
  }

  u8g2.sendBuffer();
}