
#include <WiFi.h>
#include <PubSubClient.h>

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT
const char* mqtt_server = "broker.hivemq.com";

const char* topic_control = "trafficlight/alen/control";
const char* topic_config = "trafficlight/alen/config";
const char* topic_status = "trafficlight/alen/status";

WiFiClient espClient;
PubSubClient client(espClient);

// Pin LED
const int redLED = 18;
const int yellowLED = 19;
const int greenLED = 21;

// Durasi lampu (milidetik)
unsigned long redDuration = 5000;
unsigned long yellowDuration = 2000;
unsigned long greenDuration = 5000;

// Status lampu
enum LightState {
  RED,
  YELLOW,
  GREEN,
  OFF
};

LightState currentState = OFF;

bool trafficRunning = false;
unsigned long previousMillis = 0;

void setLight(LightState state) {
  currentState = state;

  digitalWrite(redLED, state == RED ? HIGH : LOW);
  digitalWrite(yellowLED, state == YELLOW ? HIGH : LOW);
  digitalWrite(greenLED, state == GREEN ? HIGH : LOW);

  const char* status;

  switch (state) {
    case RED:
      status = "RED";
      break;
    case YELLOW:
      status = "YELLOW";
      break;
    case GREEN:
      status = "GREEN";
      break;
    default:
      status = "OFF";
      break;
  }

  Serial.print("Status lampu: ");
  Serial.println(status);

  if (client.connected()) {
    client.publish(topic_status, status);
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String payload = "";

  for (unsigned int i = 0; i < length; i++) {
    payload += (char)message[i];
  }

  payload.trim();

  Serial.print("Pesan diterima: ");
  Serial.println(payload);

  if (String(topic) == topic_control) {

    if (payload == "START") {
      trafficRunning = true;
      previousMillis = millis();
      setLight(RED);
    }

    else if (payload == "STOP") {
      trafficRunning = false;
      setLight(OFF);
    }

    else if (payload == "RED" && !trafficRunning) {
      setLight(RED);
    }

    else if (payload == "YELLOW" && !trafficRunning) {
      setLight(YELLOW);
    }

    else if (payload == "GREEN" && !trafficRunning) {
      setLight(GREEN);
    }

    else if (payload == "OFF") {
      trafficRunning = false;
      setLight(OFF);
    }
  }

  else if (String(topic) == topic_config) {
    unsigned long r, y, g;

    if (sscanf(payload.c_str(), "%lu,%lu,%lu", &r, &y, &g) == 3) {
      if (r > 0 && y > 0 && g > 0) {
        redDuration = r;
        yellowDuration = y;
        greenDuration = g;

        Serial.println("Konfigurasi durasi berhasil diperbarui!");
        Serial.print("Merah: ");
        Serial.println(redDuration);
        Serial.print("Kuning: ");
        Serial.println(yellowDuration);
        Serial.print("Hijau: ");
        Serial.println(greenDuration);
      }
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Menghubungkan ke HiveMQ...");

    String clientId = "TrafficLightESP32-" +
      String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Terhubung ke HiveMQ!");

      client.subscribe(topic_control);
      client.subscribe(topic_config);

      Serial.println("Berhasil subscribe ke topic!");
    } else {
      Serial.print("Gagal, kode: ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);

  setLight(OFF);

  WiFi.begin(ssid, password, 6);

  Serial.print("Menghubungkan WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi terhubung!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  if (!trafficRunning) {
    return;
  }

  unsigned long currentMillis = millis();
  unsigned long duration = 0;

  switch (currentState) {
    case RED:
      duration = redDuration;
      break;

    case YELLOW:
      duration = yellowDuration;
      break;

    case GREEN:
      duration = greenDuration;
      break;

    default:
      return;
  }

  if (currentMillis - previousMillis >= duration) {
    previousMillis = currentMillis;

    if (currentState == RED) {
      setLight(YELLOW);
    }
    else if (currentState == YELLOW) {
      setLight(GREEN);
    }
    else if (currentState == GREEN) {
      setLight(RED);
    }
  }
}
