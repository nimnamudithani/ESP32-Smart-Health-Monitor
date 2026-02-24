#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>

// --- Pin Definitions ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define ONE_WIRE_BUS 15
#define TILT_PIN 14
#define LED_RED 13
#define LED_GREEN 12
#define BUZZER 18

// --- Sensor Objects ---
Adafruit_MPU6050 mpu;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature bodySensor(&oneWire);
U8X8_SH1106_128X64_NONAME_HW_I2C u8g2(U8X8_PIN_NONE); 

// --- Variables for Logic ---
float accel_offset_z = -0.45; // Your calibrated offset
unsigned long fallTime = 0;
bool potentialFall = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize Components
  dht.begin();
  bodySensor.begin();
  u8g2.begin();
  
  pinMode(TILT_PIN, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  
  Serial.println("System Initialized...");
}

void loop() {
  // 1. DATA ACQUISITION
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  float h = dht.readHumidity();
  float t_env = dht.readTemperature();
  bodySensor.requestTemperatures();
  float t_body = bodySensor.getTempCByIndex(0);
  
  // 2. FORMULAS
  // Vector Magnitude for Fall Detection
  float totalAcc = sqrt(sq(a.acceleration.x) + sq(a.acceleration.y) + sq(a.acceleration.z));
  // Heat Stress Index
  float heatIndex = dht.computeHeatIndex(t_env, h, false);

  // 3. FALL DETECTION LOGIC (Accurate Strategy)
  String statusMsg = "NORMAL";
  bool isEmergency = false;

  if (totalAcc > 28.0) { // Step 1: Impact Spike
    potentialFall = true;
    fallTime = millis();
  }

  if (potentialFall && (millis() - fallTime > 2000)) {
    int tiltState = digitalRead(TILT_PIN); // Step 2: Tilt Confirmation
    if (tiltState == HIGH) {
      isEmergency = true;
      statusMsg = "FALL!";
    } else {
      potentialFall = false; // Reset if user is upright
    }
  }

  // 4. MEDICAL LOGIC (HSI & FEVER)
  if (t_body > 38.0) {
    statusMsg = "FEVER!";
    isEmergency = true;
  } else if (heatIndex > 40.0) {
    statusMsg = "HEAT DANGER";
  }

  // 5. OUTPUTS (Physical)
  if (isEmergency) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER, HIGH);
  } else {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(BUZZER, LOW);
  }

  // 6. OLED DISPLAY (Professional UI)
  u8g2.clear();
  u8g2.setFont(u8x8_font_art8_f);
  
  // Environment Row
  u8g2.drawString(0, 0, "ENV:");
  u8g2.setCursor(5, 0); u8g2.print(t_env, 1); u8g2.print("C");
  
  // Body Temp Row
  u8g2.drawString(0, 2, "BODY:");
  u8g2.setCursor(6, 2); u8g2.print(t_body, 1); u8g2.print("C");

  // HSI / Status Row
  u8g2.drawString(0, 4, "FEELS:");
  u8g2.setCursor(7, 4); u8g2.print(heatIndex, 1); u8g2.print("C");

  // Emergency Banner
  u8g2.drawString(0, 7, "STATUS: ");
  u8g2.drawString(8, 7, statusMsg.c_str());

  delay(500);
}
