#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>

// WiFi Credentials
#define WIFI_SSID "HONOR X6c"
#define WIFI_PASSWORD "nimnaroshan1031"

// Firebase Credentials
#define API_KEY "AIzaSyDG5JUkUF3IeE9mkE_zAbCBxi6kEq4IqRU"
#define DATABASE_URL "https://sampleproject2-cb92d-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "nimnamudithani0318@gmail.com"
#define USER_PASSWORD "NIMNAnimna0318"

// DHT Sensor Settings
#define DHTPIN 4         
#define DHTTYPE DHT22     
DHT dht(DHTPIN, DHTTYPE);

// Firebase Objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//time tracking
unsigned long sendDataPrevMillis = 0;
const long interval = 5000; // Send data every 5 seconds

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConnected!");

  // Firebase configuration
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);
}

void loop() {
  // Check if it's time to send data and if Firebase is ready
  if (Firebase.ready() && (millis() - sendDataPrevMillis > interval || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    float h = dht.readHumidity();
    float t = dht.readTemperature(); //celcius

    // Check if reading failed
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Sending Temperature to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", t)) {
      Serial.print("Temp sent: "); Serial.println(t);
    } else {
      Serial.println("Error: " + fbdo.errorReason());
    }

    // Sending Humidity to Firebase
    if (Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", h)) {
      Serial.print("Hum sent: "); Serial.println(h);
    } else {
      Serial.println("Error: " + fbdo.errorReason());
    }
  }
}