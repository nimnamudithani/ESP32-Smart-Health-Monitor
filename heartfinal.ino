#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

// --- PIN DEFINITIONS ---
#define RED_LED 13
#define BLUE_LED 14
#define GREEN_LED 25

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
  
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  Wire.begin(33, 32);
  Wire.setClock(100000);

  u8g2.begin();
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    while(1);
  }

  particleSensor.setup(0x1F, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x1F); 
  particleSensor.setPulseAmplitudeIR(0x1F); 
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
    updateOLED(true, irValue);
  } else {
    beatAvg = 0;
    updateOLED(false, irValue);
  }
}

void updateOLED(bool fingerDetected, long ir) {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 300) return; 
  lastUpdate = millis();

  u8g2.clearBuffer();
  
  // 1. TITLE
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "HEALTH MONITOR");
  u8g2.drawHLine(0, 12, 128);

  if (fingerDetected) {
    // 2. HEART ANIMATION (Placed in front of BPM)
    if ((millis() / 500) % 2 == 0) {
      u8g2.setFont(u8g2_font_open_iconic_human_2x_t);
      u8g2.drawGlyph(5, 45, 66); 
    }

    // 3. BPM NUMBER (Shifted right for the heart)
    u8g2.setFont(u8g2_font_logisoso24_tf); 
    u8g2.setCursor(30, 45);
    if(beatAvg > 0) u8g2.print(beatAvg);
    else u8g2.print("--");

    // 4. MEDICAL STATE LOGIC
    u8g2.setFont(u8g2_font_7x14_tr);
    u8g2.setCursor(0, 62);

    if (beatAvg > 140 || (beatAvg > 0 && beatAvg < 40)) {
      u8g2.print("STATE: DANGER");
      digitalWrite(RED_LED, HIGH); digitalWrite(BLUE_LED, LOW); digitalWrite(GREEN_LED, LOW);
    } 
    else if (beatAvg > 100) {
      u8g2.print("STATE: WARNING");
      digitalWrite(BLUE_LED, HIGH); digitalWrite(RED_LED, LOW); digitalWrite(GREEN_LED, LOW);
    } 
    else if (beatAvg >= 40) {
      u8g2.print("STATE: HEALTHY");
      digitalWrite(GREEN_LED, HIGH); digitalWrite(RED_LED, LOW); digitalWrite(BLUE_LED, LOW);
    }
  } else {
    // 5. IDLE STATE
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.drawStr(10, 45, "PLACE FINGER");
    digitalWrite(RED_LED, LOW); digitalWrite(BLUE_LED, LOW); digitalWrite(GREEN_LED, LOW);
  }

  u8g2.sendBuffer();
}