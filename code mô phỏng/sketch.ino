#include <WiFi.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST"; // WiFi SSID
const char* password = ""; // WiFi password

#define FIREBASE_HOST "https://iots37-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "Qco7jxQBPlImtSoYRdpyFN5Rh9dW0wExw0xChigV"

#define DHTPIN 13 // DHT sensor pin
#define DHTTYPE DHT22 // DHT sensor type
#define GAS_PIN 32 // Gas sensor pin
#define BUZZER_PIN 14 // Buzzer pin
#define LED1_PIN 5 // LED1 pin
#define LED3_PIN 18 // LED3 pin

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  
  dht.begin();
  wifiConnection();
}

void wifiConnection() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("WiFi Connected");
}

void updateDataToFirebase(float humidity, float temperature, int gas) {
  HTTPClient http;
  String url = String(FIREBASE_HOST) + "/data_s.json?auth=" + FIREBASE_AUTH;

  String jsonPayload = String("{\"humidity\":") + humidity +
                       String(",\"temperature\":") + temperature +
                       String(",\"gas\":") + gas + "}";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PUT(jsonPayload);
  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
  } else {
    Serial.printf("Error sending data: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

void readSettings() {
  HTTPClient http;
  String settingsUrl = String(FIREBASE_HOST) + "/.json?auth=" + FIREBASE_AUTH;

  http.begin(settingsUrl);
  int settingsResponseCode = http.GET();
  
  if (settingsResponseCode > 0) {
    String settingsResponse = http.getString();
    Serial.println("Settings Response: " + settingsResponse);

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, settingsResponse);
    if (!error) {
      bool buzzerStatus = doc["buzzer"];
      bool led1Status = doc["fan"];
      bool led3Status = doc["led3"];

      digitalWrite(BUZZER_PIN, buzzerStatus ? HIGH : LOW);
      digitalWrite(LED1_PIN, led1Status ? HIGH : LOW);
      digitalWrite(LED3_PIN, led3Status ? HIGH : LOW);

      Serial.print("Buzzer: ");
      Serial.println(buzzerStatus);
      Serial.print("LED1: ");
      Serial.println(led1Status);
      Serial.print("LED3: ");
      Serial.println(led3Status);
    } else {
      Serial.println("Failed to parse settings JSON");
    }
  } else {
    Serial.printf("Error fetching settings: %s\n", http.errorToString(settingsResponseCode).c_str());
  }
  
  http.end();
}

void loop() {
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int16_t g = analogRead(GAS_PIN) * 100 / 4095;

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  updateDataToFirebase(h, t, g);
  readSettings();
}
