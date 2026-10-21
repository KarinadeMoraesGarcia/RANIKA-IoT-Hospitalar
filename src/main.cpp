#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const int PIN_TEMP = 34;
const int PIN_BPM = 35;
const int PIN_BTN = 12;
const int PIN_BUZZER = 26;
const int PIN_LED_RED = 27;
const int PIN_LED_GREEN = 14;

WiFiClient espClient;
PubSubClient client(espClient);

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
}

void reconnectMQTT() {
  while (!client.connected()) {
    String clientId = "ESP32_UTI_Ranika_" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe("ranika/leito01/atuadores/#");
    } else { delay(5000); }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  connectWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  int rawTemp = analogRead(PIN_TEMP);
  int rawBpm  = analogRead(PIN_BPM);
  float temp  = map(rawTemp, 0, 4095, 350, 420) / 10.0;
  int bpm     = map(rawBpm, 0, 4095, 40, 160);
  bool emergencia = (digitalRead(PIN_BTN) == LOW);

  StaticJsonDocument<200> doc;
  doc["paciente"] = "Ranika (Leito 01)";
  doc["temperatura"] = temp;
  doc["bpm"] = bpm;
  doc["emergencia"] = emergencia;

  char jsonBuffer[256];
  serializeJson(doc, jsonBuffer);
  client.publish("ranika/leito01/telemetria", jsonBuffer);

  static bool buzzerState = false;
  if (emergencia || temp > 38.5 || bpm < 50 || bpm > 120) {
    digitalWrite(PIN_LED_RED, HIGH);
    digitalWrite(PIN_LED_GREEN, LOW);
    
    // Alarme sonoro pulsado de UTI hospitalar
    tone(PIN_BUZZER, 1800);
    delay(150);
    noTone(PIN_BUZZER);
    buzzerState = true;
  } else {
    digitalWrite(PIN_LED_RED, LOW);
    digitalWrite(PIN_LED_GREEN, HIGH);
    if (buzzerState) {
      noTone(PIN_BUZZER);
      buzzerState = false;
    }
  }
  delay(1850);
}