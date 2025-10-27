#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "config.h"



// --- Ultrasonic Sensor Pins ---
#define TRIG_PIN 14
#define ECHO_PIN 4

// --- Constants ---
#define SEND_INTERVAL 5000          // ms
#define MEASURE_TIMEOUT 60000       // µs
#define MAX_DISTANCE 400.0          // cm
#define BIN_HEIGHT 60.0             // cm (height of compost bin)

// --- Firebase Objects ---
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// --- Variables ---
unsigned long sendDataPrev = 0;
bool signupOK = false;
float distance = 0;
float fill_percent = 0;
String binPath = "";

// --- Function: Measure one distance ---
float singleDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseInLong(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  if (duration == 0) return -1.0;

  float dist = (duration * 0.0343) / 2.0;
  if (dist < 2.0 || dist > MAX_DISTANCE) return -1.0;
  return dist;
}

// --- Function: Median Filter ---
float medianDistance(int samples = 7) {
  float readings[15];
  int valid = 0;

  for (int i = 0; i < samples; i++) {
    float d = singleDistance();
    if (d > 0) readings[valid++] = d;
    delay(60);
  }

  if (valid == 0) return -1.0;

  // Sort readings (simple bubble sort)
  for (int i = 0; i < valid - 1; i++) {
    for (int j = i + 1; j < valid; j++) {
      if (readings[j] < readings[i]) {
        float temp = readings[i];
        readings[i] = readings[j];
        readings[j] = temp;
      }
    }
  }

  // Median calculation
  if (valid % 2 == 0)
    return (readings[valid / 2 - 1] + readings[valid / 2]) / 2.0;
  else
    return readings[valid / 2];
}

// --- Function: Calculate Fill Percentage ---
float calculateFillPercent(float distance) {
  if (distance <= 0 || distance > BIN_HEIGHT) return 0.0;
  float fill = ((BIN_HEIGHT - distance) / BIN_HEIGHT) * 100.0;
  fill = constrain(fill, 0, 100);
  return fill;
}

// --- Setup Function ---
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Compost Bin Sensor Starting ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // --- WiFi Connection ---
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

  // --- Unique Device ID ---
  uint64_t chipid = ESP.getEfuseMac();
  char macID[13];
  sprintf(macID, "%012llX", chipid);
  binPath = "Sensor/data/bin_" + String(macID);
  Serial.printf("Device ID: %s\nFirebase Path: %s\n", macID, binPath.c_str());

  // --- Firebase Setup ---
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

// --- Loop Function ---
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi disconnected! Reconnecting...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  // --- Measure distance ---
  distance = medianDistance(7);

  // Always compute fill_percent (even if invalid distance)
  fill_percent = calculateFillPercent(distance);

  // Print for debugging
  Serial.printf("DEBUG → Distance: %.2f cm | Fill: %.2f%%\n", distance, fill_percent);

  if (distance > 0)
    Serial.printf("📏 Median Distance: %.2f cm | 🪣 Fill Level: %.1f%%\n", distance, fill_percent);
  else
    Serial.println("⚠️ Sensor reading failed");

  // --- Send data to Firebase ---
  if (Firebase.ready() && signupOK && (millis() - sendDataPrev >= SEND_INTERVAL)) {
    sendDataPrev = millis();

    FirebaseJson json;
    json.set("distance", distance);
    json.set("fill_percent", fill_percent);  // ✅ Always included now
    json.set("timestamp", millis());
    json.set("status", distance > 0 ? "active" : "error");

    Serial.println("Uploading JSON →");
    Serial.println(json.raw());  // Debug line: shows what’s sent

    if (Firebase.RTDB.setJSON(&fbdo, binPath.c_str(), &json))
      Serial.printf("✅ Data saved → %.2f cm | %.1f%% full\n", distance, fill_percent);
    else
      Serial.printf("❌ Firebase error: %s\n", fbdo.errorReason().c_str());
  }

  delay(500);
}
