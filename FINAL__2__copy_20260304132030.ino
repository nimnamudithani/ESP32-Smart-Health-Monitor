#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "DHT.h"

// ---------------- WIFI + FIREBASE CREDENTIALS ----------------
#define WIFI_SSID "realme C11 2021"
#define WIFI_PASSWORD "icecream"
#define API_KEY "AIzaSyDJ12bZ1oRZeRHVP7mGljW0ezfUyGPq4zk"
#define DATABASE_URL "https://health-monitor-iot-7eca5-default-rtdb.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ---------------- PIN MAPPING ----------------
#define DHTPIN 4
#define DHTTYPE DHT22
#define RED_LED 13
#define BLUE_LED 14
#define GREEN_LED 25
#define TEMP_PIN 26
#define TILT_PIN 27
#define BUZZER 18

// ---------------- OBJECTS ----------------
// Hardware I2C Constructor for SH1106 OLED
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
MAX30105 particleSensor;
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
Adafruit_MPU6050 mpu;
DHT dht(DHTPIN, DHTTYPE);

// ---------------- VARIABLES ----------------
unsigned long lastTempRequest = 0;
unsigned long lastFirebaseUpdate = 0;
const int tempInterval = 2000;
const int firebaseInterval = 3000;

int beatAvg = 0;
long lastBeat = 0;
float bodyTemp = 0;
float envTemp = 0;

bool mpuActive = false;
bool potentialFall = false;
bool isFallEmergency = false;
unsigned long fallTime = 0;
const float impactThreshold = 10.0; 

String healthState = "BOOTING";

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  // Initialize Pin Modes
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TILT_PIN, INPUT_PULLUP);

  // Initialize I2C Bus on pins 33 (SDA) and 32 (SCL)
  Wire.begin(33, 32);
  
  // Initialize OLED Display
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 30, "SYSTEM STARTING...");
  u8g2.sendBuffer();

  dht.begin();

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected");

  // Firebase Initialization
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();

  // MPU6050 Initialization
  if (mpu.begin()) {
    mpuActive = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  }

  // MAX30105 Pulse Sensor Initialization
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30105 not found");
  }
  particleSensor.setup();
}

// ---------------- LOOP ----------------
void loop() {

  // ----- 1. FALL DETECTION -----
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

  // ----- 2. HEART RATE SCANNING -----
  long irValue = particleSensor.getIR();
  bool fingerDetected = (irValue > 50000); 

  if (fingerDetected) {
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      float bpm = 60 / (delta / 1000.0);
      if (bpm > 30 && bpm < 220) beatAvg = (int)bpm;
    }
  } else {
    beatAvg = 0; 
  }

  // ----- 3. TEMPERATURE UPDATES -----
  if (millis() - lastTempRequest >= tempInterval) {
    bodyTemp = sensors.getTempCByIndex(0);
    if (bodyTemp < -50) bodyTemp = 0;

    envTemp = dht.readTemperature();
    if (isnan(envTemp)) envTemp = 0;

    sensors.requestTemperatures();
    lastTempRequest = millis();
  }

  // ----- 4. HEALTH STATE & LED INDICATORS -----
  if (isFallEmergency) {
    healthState = "FALL EMERGENCY";
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
  } else if (bodyTemp > 38.5 || (fingerDetected && (beatAvg > 140 || beatAvg < 40))) {
    healthState = "DANGER";
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(GREEN_LED, LOW);
  } else if (fingerDetected && beatAvg >= 40) {
    healthState = "STABLE";
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
  } else {
    healthState = fingerDetected ? "SYNCING..." : "NO FINGER";
    digitalWrite(BLUE_LED, (millis() / 500) % 2); 
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER, LOW);
  }

  // ----- 5. OLED DISPLAY UPDATE -----
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "IOT HEALTH MONITOR");
  u8g2.drawHLine(0, 13, 128);

  if (isFallEmergency) {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(25, 45, "FALL!");
  } else {
    u8g2.setCursor(0, 30);
    u8g2.print("HR: "); 
    u8g2.print(beatAvg > 0 ? String(beatAvg) : "--"); u8g2.print(" BPM");

    u8g2.setCursor(0, 45);
    u8g2.print("Temp: "); 
    u8g2.print(bodyTemp, 1); u8g2.print(" C");

    u8g2.setCursor(0, 60);
    u8g2.print("State: "); u8g2.print(healthState);
  }
  u8g2.sendBuffer();

  // ----- 6. FIREBASE CLOUD SYNC -----
  if (Firebase.ready() && millis() - lastFirebaseUpdate > firebaseInterval) {
    FirebaseJson json;
    json.set("bodyTemp", bodyTemp);
    json.set("heartRate", beatAvg);
    json.set("fallDetected", isFallEmergency);
    json.set("status", healthState);
    json.set("roomTemp", envTemp);

    if (Firebase.RTDB.updateNode(&fbdo, "/health", &json)) {
      Serial.println("Firebase Updated Successfully");
    } else {
      Serial.println("Firebase Sync Failed: " + fbdo.errorReason());
    }
    lastFirebaseUpdate = millis();
  }

  delay(20);
}