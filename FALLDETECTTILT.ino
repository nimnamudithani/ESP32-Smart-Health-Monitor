#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>

// --- PIN MAPPING ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define ONE_WIRE_BUS 26  
#define TILT_PIN 27    
#define LED_RED 13       
#define LED_GREEN 25     
#define LED_BLUE 14      
#define BUZZER 18

// --- OBJECTS ---
Adafruit_MPU6050 mpu;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature bodySensor(&oneWire);
U8X8_SH1106_128X64_NONAME_HW_I2C u8g2(U8X8_PIN_NONE); 

// --- VARIABLES ---
bool mpuActive = false;
bool potentialFall = false;
unsigned long fallTime = 0;
const float impactThreshold = 40.0; // Lowered for easier testing

void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("\n>>> STARTING SYSTEM...");

  // 1. Initialize I2C with Timeout
  Wire.begin(33, 32);
  Wire.setClock(100000); 
  Wire.setTimeOut(50); 

  // 2. Initialize OLED
  u8g2.begin();
  u8g2.setFont(u8x8_font_chroma48medium8_r);
  u8g2.drawString(0, 0, "SYSTEM ONLINE");

  // 3. Initialize Pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TILT_PIN, INPUT_PULLUP);

  // 4. Initialize MPU6050 Safely
  if (mpu.begin()) {
    mpuActive = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    Serial.println(">>> MPU6050: ONLINE");
  } else {
    Serial.println(">>> MPU6050: NOT FOUND (Skipped)");
  }

  // 5. Initialize Temps
  dht.begin();
  bodySensor.begin();
  bodySensor.setWaitForConversion(false); // CRITICAL: Stops the code from hanging
  
  Serial.println(">>> SETUP COMPLETE - STARTING MONITOR...");
}

void loop() {
  // --- 1. SENSOR READS (With fail-safes) ---
  float t_env = dht.readTemperature();
  bodySensor.requestTemperatures();
  float t_body = bodySensor.getTempCByIndex(0);

  // Filter out the common "-127" error from loose wires
  if (t_body < -50) t_body = 0; 

  String statusMsg = "NORMAL";
  bool isEmergency = false;

  // --- 2. FEVER LOGIC ---
  if (t_body > 38.0 && t_body < 50.0) {
    isEmergency = true;
    statusMsg = "FEVER!";
  }

  // --- 3. FALL DETECTION LOGIC ---
  if (mpuActive) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float totalAcc = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));

    if (totalAcc > impactThreshold && !potentialFall) {
      potentialFall = true;
      fallTime = millis();
      Serial.println("IMPACT DETECTED!");
    }

    if (potentialFall) {
      unsigned long elapsed = millis() - fallTime;
      if (elapsed < 5000) { // 5 second window
        statusMsg = "WAITING";
        digitalWrite(LED_BLUE, (millis() % 400 < 200)); 
        if (digitalRead(TILT_PIN) == LOW) potentialFall = false;
      } else {
        if (digitalRead(TILT_PIN) == HIGH) {
          isEmergency = true;
          statusMsg = "FALL!";
        } else {
          potentialFall = false;
        }
      }
    }
  }

  // --- 4. HARDWARE OUTPUTS ---
  if (isEmergency) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(BUZZER, LOW);
    if (!potentialFall) digitalWrite(LED_BLUE, LOW);
  }

  // --- 5. OLED DISPLAY ---
  u8g2.clearLine(0);
  u8g2.drawString(0, 0, "BODY:"); u8g2.setCursor(7, 0); 
  if (t_body == 0) u8g2.print("ERR"); else u8g2.print(t_body, 1);
  
  u8g2.clearLine(6);
  u8g2.drawString(0, 6, "STAT:"); u8g2.drawString(6, 6, statusMsg.c_str());

  // --- 6. SERIAL DEBUG (To confirm it's not frozen) ---
  Serial.print("Body: "); Serial.print(t_body);
  Serial.print(" | Tilt: "); Serial.print(digitalRead(TILT_PIN));
  Serial.print(" | Accel: "); Serial.print(mpuActive ? "OK" : "NO");
  Serial.print(" | Status: "); Serial.println(statusMsg);

  delay(200); 
}