#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "config.h"

// --- Ultrasonic Sensor Pins ---
#define TRIG_PIN1 17
#define ECHO_PIN1 4
#define TRIG_PIN2 27
#define ECHO_PIN2 26

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

// --- Function: Measure one distance for given pins ---
float singleDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseInLong(echoPin, HIGH, MEASURE_TIMEOUT);
  if (duration == 0) return -1.0;

  float dist = (duration * 0.0343) / 2.0;
  if (dist < 2.0 || dist > MAX_DISTANCE) return -1.0;
  return dist;
}

// --- Function: Median Filter per sensor ---
float medianDistance(int trigPin, int echoPin, int samples = 7) {
  float readings[15];
  int valid = 0;

  for (int i = 0; i < samples; i++) {
    float d = singleDistance(trigPin, echoPin);
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
  Serial.println("\n=== Compost Bin Sensor Starting (Dual Ultrasonic) ===");

  pinMode(TRIG_PIN1, OUTPUT);
  pinMode(ECHO_PIN1, INPUT);
  pinMode(TRIG_PIN2, OUTPUT);
  pinMode(ECHO_PIN2, INPUT);
  digitalWrite(TRIG_PIN1, LOW);
  digitalWrite(TRIG_PIN2, LOW);

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

  // --- Measure both sensors ---
  float d1 = medianDistance(TRIG_PIN1, ECHO_PIN1, 7);
  float d2 = medianDistance(TRIG_PIN2, ECHO_PIN2, 7);

  // --- Average distances ---
  float avgDistance = -1.0;
  if (d1 > 0 && d2 > 0)
    avgDistance = (d1 + d2) / 2.0;
  else if (d1 > 0)
    avgDistance = d1;
  else if (d2 > 0)
    avgDistance = d2;

  // --- Compute fill percentage ---
  float fill_percent = calculateFillPercent(avgDistance);

  // --- Debug output ---
  Serial.printf("Sensor1: %.2f cm | Sensor2: %.2f cm | Avg: %.2f cm | Fill: %.1f%%\n",
                d1, d2, avgDistance, fill_percent);

  // --- Send data to Firebase ---
  if (Firebase.ready() && signupOK && (millis() - sendDataPrev >= SEND_INTERVAL)) {
    sendDataPrev = millis();

    FirebaseJson json;
    json.set("sensor1_distance", d1);
    json.set("sensor2_distance", d2);
    json.set("average_distance", avgDistance);
    json.set("fill_percent", fill_percent);
    json.set("timestamp", millis());
    json.set("status", avgDistance > 0 ? "active" : "error");

    Serial.println("Uploading JSON →");
    Serial.println(json.raw());

    if (Firebase.RTDB.setJSON(&fbdo, binPath.c_str(), &json))
      Serial.printf("✅ Data saved → Avg: %.2f cm | %.1f%% full\n", avgDistance, fill_percent);
    else
      Serial.printf("❌ Firebase error: %s\n", fbdo.errorReason().c_str());
  }

  delay(500);
}
