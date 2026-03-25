#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

// ------------------- PINS -------------------
#define TRIG_PIN 5
#define ECHO_PIN 18
#define TANK_HEIGHT 100  // cm

WiFiClient espClient;
PubSubClient client(espClient);

// ------------------- WIFI -------------------
void setupWiFi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
}

// ------------------- MQTT -------------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    
    if (client.connect("ESP32_Basic_Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, retrying...");
      delay(3000);
    }
  }
}

// ------------------- SENSOR -------------------
float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);

  if (duration == 0) return -1;

  float distance = duration * 0.034 / 2;
  float level = ((TANK_HEIGHT - distance) / TANK_HEIGHT) * 100;

  // Clamp values
  if (level > 100) level = 100;
  if (level < 0) level = 0;

  return level;
}

// ------------------- SETUP -------------------
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  setupWiFi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

// ------------------- LOOP -------------------
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  float level = readWaterLevel();

  if (level >= 0) {
    Serial.print("Water Level: ");
    Serial.print(level);
    Serial.println(" %");

    char payload[50];
    snprintf(payload, sizeof(payload), "{\"level\": %.2f}", level);

    client.publish(MQTT_TOPIC, payload);
  } else {
    Serial.println("Sensor error");
  }

  delay(5000); // send every 5 sec
}
