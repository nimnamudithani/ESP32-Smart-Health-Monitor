#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// OLED Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// DHT Setup
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Initialize OLED (Address 0x3C is common for these screens)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED failed to start. Check wiring!"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("System Initializing...");
  display.display();
  delay(2000);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  display.clearDisplay();
  
  if (isnan(h) || isnan(t)) {
    display.setCursor(0, 10);
    display.println("Sensor Error!");
  } else {
    // Show Temperature
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ENVIRONMENT:");
    
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(t);
    display.print(" C");

    // Show Humidity
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print("Humidity: ");
    display.print(h);
    display.print("%");
  }

  display.display(); // Actually push data to the screen
  delay(2000);
}