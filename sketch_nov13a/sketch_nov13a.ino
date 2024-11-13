#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "PubSubClient.h"

// Pin sensor
const int DHTPIN = 15;        // Pin DHT22 untuk suhu dan kelembapan
const int DHTTYPE = DHT22;    // Tipe sensor DHT yang digunakan
const int TURBIDITY_PIN = 34; // Pin analog untuk simulasi sensor kekeruhan
const int PH_PIN = 35;        // Pin analog untuk simulasi sensor pH

// Pin kontrol perangkat
const int LED_GREEN = 5;      // Pin untuk LED Hijau
const int LED_YELLOW = 17;    // Pin untuk LED Kuning
const int LED_RED = 4;        // Pin untuk LED Merah
const int RELAY_PUMP = 27;    // Pin untuk relay pompa
const int BUZZER = 2;         // Pin untuk buzzer

// WiFi settings
const char* ssid = "Femiliz";               // Ganti dengan SSID jaringan Anda
const char* password = "tertibkost";      // Ganti dengan password WiFi Anda

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com";  // Ganti dengan IP atau alamat broker MQTT
const int mqtt_port = 1883;                    // Port broker MQTT
const char* pub_lampu = "status196/lampu";
const char* pub_relay = "status196/relay";
const char* pub_buzzer = "status196/buzzer";
const char* sub_suhu = "sensor196/suhu";
const char* sub_humi = "sensor196/kelembapan";
const char* sub_turbidity = "sensor196/kekeruhan";
const char* sub_ph = "sensor196/ph";

// Alamat URL Flask server
const char* flask_server_url = "http://192.168.18.132:5000/data";  // Ganti <NGROK_URL> dengan URL ngrok yang digunakan

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

// Variabel untuk menyimpan nilai suhu dan kelembapan
float suhu = -1;
float humidity = -1;

void setup() {
  Serial.begin(115200);

  // Setup untuk sensor
  dht.begin();
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(RELAY_PUMP, LOW);
  digitalWrite(BUZZER, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Connect to MQTT Broker
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqtt_callback);

  // Connect to MQTT broker
  connectToMQTT();
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  delay(2000);  // Delay sebelum pembacaan data berikutnya
}

void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT broker");
      // Subscribe to sensor data topics to receive sensor values
      client.subscribe(sub_suhu);
      client.subscribe(sub_humi);
      client.subscribe(sub_turbidity);
      client.subscribe(sub_ph);
    } else {
      Serial.print("Failed to connect, rc=");
      Serial.print(client.state());
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (String(topic) == sub_suhu) {
    Serial.print("Received Suhu: ");
    Serial.println(message);

    suhu = message.toFloat();
    String lampuStatus;

    if (suhu < 30) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, LOW);
      lampuStatus = "{\"green\": \"ON\"}";
    } else if (suhu >= 30 && suhu <= 35) {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_RED, LOW);
      lampuStatus = "{\"yellow\": \"ON\"}";
    } else {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, HIGH);
      digitalWrite(BUZZER, HIGH);
      lampuStatus = "{\"red\": \"ON\"}";
    }
    client.publish(pub_lampu, lampuStatus.c_str());

    // Kirim suhu dan kelembapan ke Flask jika kelembapan telah diterima
    if (humidity != -1) {
      sendToFlask(suhu, humidity);
    }
  }

  if (String(topic) == sub_humi) {
    Serial.print("Received Kelembapan: ");
    Serial.println(message);

    humidity = message.toFloat();

    if (humidity < 50) {
      if (digitalRead(RELAY_PUMP) == LOW) {
        digitalWrite(RELAY_PUMP, HIGH);
        client.publish(pub_relay, "Pompa ON");
      }
      if (digitalRead(BUZZER) == LOW) {
        digitalWrite(BUZZER, HIGH);
        client.publish(pub_buzzer, "Buzzer ON");
      }
    } else {
      if (digitalRead(RELAY_PUMP) == HIGH) {
        digitalWrite(RELAY_PUMP, LOW);
        client.publish(pub_relay, "Pompa OFF");
      }
      if (digitalRead(BUZZER) == HIGH) {
        digitalWrite(BUZZER, LOW);
        client.publish(pub_buzzer, "Buzzer OFF");
      }
    }

    // Kirim suhu dan kelembapan ke Flask jika suhu telah diterima
    if (suhu != -1) {
      sendToFlask(suhu, humidity);
    }
  }
}

void sendToFlask(float suhu, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(flask_server_url);

    // JSON data sesuai dengan yang diberikan
    String jsonData = "{\"suhumax\":34,\"suhumin\":15,\"suhurata2\":25,\"nilaisuhuhumid\":[{\"id\":1,\"suhu\":" 
                      + String(suhu) + ",\"humid\":" + String(humidity) 
                      + ",\"kecerahan\":150,\"timestamp\":\"2024-11-13T18:06:00\"}],\"month_year\":\"11-2024\"}";

    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("Error in WiFi connection");
  }
}