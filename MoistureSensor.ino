// TO DO:
// [X] Add reconnect for wifi and mqtt
// [X] Move credentials
// [ ] Add display
// [ ] Add battery measurments
// [ ] Calibrate soil sensor with values for 25 50 75 % (! not gonna work with map())

#include <WiFi.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include "config.h"
// #include "heltec.h"

// WiFi credentials
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;

// Pin setup
const int SensorPin = 36; // A0 36
const int DrySoilValue = 3220; // normalized soilMoistureValue reading from dry soil
const int WateredSoilValue = 2000; // normalized soilMoistureValue reading from watered soil

// variable declaration
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

// MQTT settings
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_user = MQTT_USER;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_client_id = "plant_1_moisture";
long lastReconnectAttempt = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);


boolean reconnect() {
  if (mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
    // Once connected, publish an announcement...
    // mqttClient.publish("outTopic","hello world");
    // ... and resubscribe
    // mqttClient.subscribe("inTopic");
  }
  return mqttClient.connected();
}

void setup_wifi()
{
    // We start by connecting to a WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.begin(wifi_ssid, wifi_password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
}

void setup()
{
    pinMode(SensorPin, INPUT); // sets the pin 36 as input
    Serial.begin(115200);
    setup_wifi();
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.connect(mqtt_client_id, mqtt_user, mqtt_password);
    mqttClient.publish("home/plants/plant1/config", "{\"name\":\"plant_1_moisture\",\"state_topic\":\"home/plants/plant1/moisture\"}");
    
    delay(10);
}

void loop()
{
  // Get sensor data
  soilMoistureValue = analogRead(SensorPin);
  // Map values for dry and watered soil
  soilmoisturepercent = map(soilMoistureValue, DrySoilValue, WateredSoilValue, 0, 100);
  
  Serial.print("Moisture: ");
  Serial.print(soilmoisturepercent);
  Serial.println("%");

  // Used for callibration
  Serial.print("Raw value: ");
  Serial.println(soilMoistureValue);

  if (WiFi.status() == !WL_CONNECTED) {
      WiFi.reconnect();
  }


  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      Serial.println("Attempt to reconnect");
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    mqttClient.loop();
  }

  // Publish to MQTT broker
  char buf[8];
  itoa(soilmoisturepercent, buf, 10);
  boolean rc = mqttClient.publish("home/plants/plant1/moisture", buf);

  delay(60000); // poll data every 60 secs
}

