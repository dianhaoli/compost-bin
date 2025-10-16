#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "iPhone"
#define WIFI_PASSWORD "12345678"

#define API_KEY "AIzaSyB5GKSdrHjKjUwsTYt_ksmvrINWjS7xYHg"
#define DATABASE_URL "https://compost-bin-alert-syste-default-rtdb.firebaseio.com/"

#define TRIG_PIN 14
#define ECHO_PIN 4

#define SEND_INTERVAL 5000
#define MEASURE_TIMEOUT 30000
#define MAX_DISTANCE 400.0

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrev = 0;
bool signupOK = false;
float distance = 0;
String binPath = "";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Compost Bin Sensor Starting ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
    Serial.print(".");
    delay(500);
    wifiAttempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n❌ WiFi connection failed! Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println();
  Serial.print("✅ Connected with IP: ");
  Serial.println(WiFi.localIP());

  // --- Unique ID ---
  uint64_t chipid = ESP.getEfuseMac();  
  char macID[13];
  sprintf(macID, "%012llX", chipid);
  binPath = "Sensor/data/bin_" + String(macID);
  Serial.printf("Device ID: %s\nFirebase Path: %s\n", macID, binPath.c_str());

  // --- Firebase setup ---
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  Serial.println("Signing up to Firebase...");
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✅ Firebase SignUp OK");
    signupOK = true;
  } else {
    Serial.printf("❌ SignUp Error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("=== Setup Complete ===\n");
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(100);

  long duration = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  if (duration == 0) return -1.0;

  float dist = (duration * 0.0343) / 2.0;
  return (dist < 2.0 || dist > MAX_DISTANCE) ? -1.0 : dist;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi disconnected! Reconnecting...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  distance = measureDistance();
  if (distance > 0) Serial.printf("📏 Distance: %.2f cm\n", distance);
  else Serial.println("⚠️ Sensor reading failed");

  if (Firebase.ready() && signupOK && (millis() - sendDataPrev >= SEND_INTERVAL)) {
    sendDataPrev = millis();

    FirebaseJson json;
    json.set("distance", distance);
    json.set("timestamp", millis());
    json.set("status", distance > 0 ? "active" : "error");

    if (Firebase.RTDB.setJSON(&fbdo, binPath.c_str(), &json))
      Serial.printf("✅ Data saved: %.2f cm → %s\n", distance, binPath.c_str());
    else
      Serial.printf("❌ Firebase error: %s\n", fbdo.errorReason().c_str());
  }

  delay(500);
}
