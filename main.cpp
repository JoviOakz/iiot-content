/*--- Includes ---*/
#include <WiFi.h>
#include <Arduino.h>
#include <string.h>
#include <iostream>
#include <vector>
#include "PubSubClient.h"
#include "Adafruit_Sensor.h"
#include "FirebaseESP32.h"
#include "DHT.h"

const uint8_t PIN_DHT = 27;
const uint8_t PIN_LED = 26;

/*--- Constants ---*/
const char *WIFI_SSID = "Vivo-Internet-BF17";
const char *WIFI_PW = "78814222";

const char *FB_HOST = "https://iiot-dta-default-rtdb.firebaseio.com";
const char *FB_KEY = "Ag5gJMhAnTWQgDVhegkDRF1uTjJfpMUDkXB8WBEa";

#define DHTTYPE DHT11

const char *MQTT_BROKER = "test.mosquitto.org";
const char *MQTT_USERNAME = "";
const char *MQTT_PASSWORD = "";
const int MQTT_PORT = 1883;

/*--- Variables ---*/
FirebaseData fbdo;
FirebaseAuth fbauth;
FirebaseConfig fbconfig;

DHT dht(PIN_DHT, DHTTYPE);
float humidity = 0;
float temperature = 0;

WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);

/*--- Functions ---*/
int binaryToDecimal(byte payload[])
{
  int decimalNumber = 0;
  int power = 1;

  for (int i = 8 - 1; i >= 0; --i)
  {
    decimalNumber += payload[i] * power;
    power *= 2;
  }

  return decimalNumber;
}

void sendData()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature))
  {
    Serial.println("Failed to read from DHT sensor \n");
    return;
  }

  bool send_temperature, send_humidity;

  send_temperature = Firebase.setFloat(fbdo, "/challenge02/subsys_16/temperature", temperature);
  send_humidity = Firebase.setFloat(fbdo, "/challenge02/subsys_16/humidity", humidity);

  if (!send_temperature || !send_humidity)
  {
    Serial.println(fbdo.errorReason().c_str());
  }
}

void callback(char topic[], byte payload[], uint16_t length)
{
  Serial.printf("Message arrived in topic: %s\n", topic);

  for (uint16_t i = 0; i < length; i++)
  {
    Serial.print(static_cast<char>(payload[i]));
  }

  int decimal_payload = binaryToDecimal(payload);

  if (strcmp(topic, "iiot-dta/check") == 0)
  {
    if (decimal_payload == 100)
    {
      mqtt_client.publish("check", "1");
    }
    else if (decimal_payload == 200)
    {
      digitalWrite(PIN_LED, HIGH);
      delay(5000);
      digitalWrite(PIN_LED, LOW);
    }
  }

  else if (strcmp(topic, "iiot-dta/request") == 0)
  {
    if (decimal_payload == 100)
    {
      sendData();
    }
  }
}

void connectWifi(const char ssid[], const char pass[])
{
  WiFi.begin(ssid, pass);

  if (Serial)
  {
    Serial.print("Connecting");
  }
  while (WiFiClass::status() != WL_CONNECTED)
  {
    if (Serial)
    {
      Serial.write('.');
      delay(500);
    }
  }

  if (Serial)
  {
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
  }
}

void connectFire(const char fbhost[], const char fbkey[])
{
  fbconfig.database_url = FB_HOST;
  fbconfig.signer.tokens.legacy_token = FB_KEY;
  fbdo.setBSSLBufferSize(4096, 1024);
  Firebase.reconnectWiFi(true);
  Firebase.begin(&fbconfig, &fbauth);
}

void connectMQTT(const char mqttbrok[], const int mqttpo)
{
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(callback);
  String mqtt_client_id = "esp32-client-" + String(WiFi.macAddress());

  mqtt_client.subscribe("iiot-dta/request");
  mqtt_client.subscribe("iiot-dta/check");

  while (!mqtt_client.connected())
  {
    Serial.printf("The client %s connects to the MQTT broker\n",
                  mqtt_client_id.c_str());

    if (mqtt_client.connect(mqtt_client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("MQTT broker connected");
    }
    else
    {
      Serial.print("Failed with state ");
      Serial.print(mqtt_client.state());
      delay(2000);
    }
  }
}

void setup()
{
  // Serial communication
  Serial.begin(115200);

  // WiFi
  connectWifi(WIFI_SSID, WIFI_PW);

  // Firebase
  connectFire(FB_HOST, FB_KEY);

  // MQTT
  connectMQTT(MQTT_BROKER, MQTT_PORT);

  // Hardware mapping
  pinMode(PIN_LED, OUTPUT);

  // Initialization of dht object
  dht.begin();
}

void loop()
{
  // mqtt connect
  mqtt_client.loop();

  // send data
  sendData();

  if (temperature > 30)
  {
    mqtt_client.publish("request", "10");
  }

  delay(30000);
}
