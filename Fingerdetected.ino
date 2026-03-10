#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"

MAX30105 particleSensor;

const byte RATE_SIZE = 4; // Increase this to 10 for even smoother readings
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;

void setup() {
  Serial.begin(115200);
  Wire.begin(33, 32);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 not found!");
    while (1);
  }

  // --- LOW POWER MODE TO PREVENT SATURATION ---
  byte ledBrightness = 0x1F; // Lowered to 31 (Was likely 255)
  byte sampleAverage = 4;    
  byte ledMode = 2;          
  int sampleRate = 100;      
  int pulseWidth = 411;      
  int adcRange = 4096;       

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  
  // Set IR and Red to a lower, stable amplitude
  particleSensor.setPulseAmplitudeRed(0x1F); 
  particleSensor.setPulseAmplitudeIR(0x1F); 
}

void loop() {
  long irValue = particleSensor.getIR();

  // We lowered the "Gate" to 10,000 so it detects your finger sooner
  if (irValue > 10000) { 
    if (checkForBeat(irValue) == true) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      float bpm = 60 / (delta / 1000.0);
      
      if (bpm < 160 && bpm > 45) {
        beatAvg = (beatAvg * 3 + (int)bpm) / 4; 
      }
    }
    Serial.print("FINGER DETECTED! IR: ");
    Serial.print(irValue);
    Serial.print(" | BPM: ");
    Serial.println(beatAvg);
  } else {
    // This tells us the sensor is idling
    Serial.print("Waiting for finger... Current IR: ");
    Serial.println(irValue);
    beatAvg = 0;
  }
  
  delay(20); 
}