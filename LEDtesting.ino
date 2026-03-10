#include <WiFi.h>
#include <Firebase_ESP_Client.h>

//wifi credentials
#define WIFI_SSID "HONOR X6c"
#define WIFI_PASSWORD "nimnaroshan1031"

//Define firebase credentials
#define API_KEY "AIzaSyDYidr3gIRkORrdDe5MgTTEqKppWmBWjWo"
#define DATABASE_URL "https://sampleproject1-5e9be-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "nimnamudithani0318@gmail.com"
#define USER_PASSWORD "NIMNAnimna0318"

//firebase objects
FirebaseData fbdo; //send and take data- firebase data object
FirebaseAuth auth;
FirebaseConfig config;

//time tracking
unsigned long sendDataPreMills = 0;

//Hardware pins
const int ledPin = 2;

void setup(){
  pinMode(ledPin, OUTPUT);
  digitalWRite(ledPin, LOW);
  //pinMode

  Serial.login(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() !=WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //firebase configuration
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectNetwork(true);
  fbdo.setResponseSize(2840);
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
  config.timeout.serverResponse = 10 * 1000;
}

void loop(){
  if (Firebase.ready() && (mills() - sendDataPreMills > 1000 || sendDataPreMills == 0)){
    sendDataPreMills = mills();

    int ledState;

    if(Firebase.RTDB.getInt(&fbdo, "/led/state", &ledState)){
      digitalWrite(ledPin, ledState);
      Serial.print("LED state updated: ");
      Serial.println(ledState);
    } else(
      Serial.print("Failed to read LED state: ");
      Serial.println(fbdo.errorReason());
    )
  }
}