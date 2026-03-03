#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

// --- DISPLAY CONFIGURATION ---
// This specific "Constructor" works for both SSD1306 and SH1106 (Blue Glitch fix)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

MAX30105 particleSensor;

// --- BPM VARIABLES ---
const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- INITIALIZING SYSTEM ---");

  // I2C Setup on Pins 33 and 32
  Wire.begin(33, 32);
  Wire.setClock(100000); // Standard speed for better stability

  // Initialize OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr); 
  u8g2.drawStr(0, 30, "SYSTEM READY");
  u8g2.sendBuffer();

  // Initialize Sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Sensor not found. Check wiring/power.");
    while(1);
  }

  // Your specific working settings
  particleSensor.setup(0x1F, 4, 2, 100, 411, 4096);
  particleSensor.setPulseAmplitudeRed(0x1F); 
  particleSensor.setPulseAmplitudeIR(0x1F); 
  
  Serial.println("Ready. Place finger on sensor.");
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
  if (millis() - lastUpdate < 300) return; // Fast refresh
  lastUpdate = millis();

  u8g2.clearBuffer();
  
  // Header
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "HEALTH MONITOR");
  u8g2.drawHLine(0, 12, 128);

  if (fingerDetected) {
    // BPM Number
    u8g2.setFont(u8g2_font_logisoso24_tf); 
    u8g2.setCursor(0, 50);
    u8g2.print("BPM: ");
    if(beatAvg > 0) u8g2.print(beatAvg);
    else u8g2.print("--");

    // Heartbeat Animation
    if ((millis() / 500) % 2 == 0) {
      u8g2.setFont(u8g2_font_open_iconic_human_2x_t);
      u8g2.drawGlyph(110, 50, 66); // Heart icon
    }
  } else {
    u8g2.setFont(u8g2_font_ncenB12_tr);
    u8g2.drawStr(10, 45, "PLACE FINGER");
  }

  u8g2.sendBuffer();
  
  // Also keep serial detail for debugging
  Serial.print("IR: "); Serial.print(ir);
  Serial.print(" | AVG BPM: "); Serial.println(beatAvg);
}