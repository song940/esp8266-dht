#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <DHT.h>

// WiFiManager settings
char mqttServer[40] = "192.168.8.160"; // Default MQTT server
char mqttPort[6] = "1883";             // Default MQTT port
char mqttUser[40] = "";                // Default MQTT user (can be an empty string)
char mqttPassword[40] = "";            // Default MQTT password (can be an empty string)

const char *apName = "ESP8266-DHT";
const char *clientID = "ESP8266-DHT";
const char *ledTopic = "led";
const char *dhtTopic = "dht";
const int ledPin = LED_BUILTIN;
unsigned long lastTransmissionTime = 0;

#define DHT_PIN 2
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wlan;
PubSubClient mqtt(wlan);

// WiFiManager callback function
void saveConfigCallback()
{
  Serial.println("Configuration saved");
  strcpy(mqttServer, WiFiManagerParameter("mqtt_server").getValue());
  strcpy(mqttPort, WiFiManagerParameter("mqtt_port").getValue());
  strcpy(mqttUser, WiFiManagerParameter("mqtt_user").getValue());
  strcpy(mqttPassword, WiFiManagerParameter("mqtt_password").getValue());
}

void reconnect()
{
  while (!mqtt.connected())
  {
    Serial.println("Attempting to connect to the MQTT server...");
    if (mqtt.connect(clientID, mqttUser, mqttPassword))
    {
      Serial.println("MQTT reconnected");
      mqtt.subscribe(ledTopic);
    }
    else
    {
      Serial.print("MQTT connect failed, retrying...");
      delay(2000);
    }
  }
}

void readDHT()
{
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (!isnan(humidity) && !isnan(temperature))
  {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, Temp: ");
    Serial.print(temperature);
    Serial.println(" Celsius");

    // Format the data as a JSON object
    String json = "{\"humidity\":" + String(humidity, 2) + ", \"temperature\":" + String(temperature, 2) + "}";

    // Publish the JSON data to the MQTT topic
    mqtt.publish(dhtTopic, json.c_str());

    // Update the last transmission time
    lastTransmissionTime = millis();
  }
  else
  {
    Serial.println("Failed to read from DHT sensor!");
  }
}

void setup()
{
  dht.begin();
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  WiFiManager wifiManager;
  // Add custom parameters for configuring MQTT server information
  WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT Server", mqttServer, 40);
  WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", mqttPort, 6);
  WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT User", mqttUser, 40);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT Password", mqttPassword, 40);

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);

  // Initialize WiFi connection and connect to a saved WiFi network or start AP mode for configuration
  wifiManager.autoConnect(apName);

  // Set the MQTT server and port
  mqtt.setServer(mqttServer, atoi(mqttPort));
  mqtt.setClient(wlan);
}

void loop()
{
  if (!mqtt.connected())
  {
    reconnect();
  }
  mqtt.loop();

  if (millis() - lastTransmissionTime >= 1000)
  {
    readDHT();
  }
}