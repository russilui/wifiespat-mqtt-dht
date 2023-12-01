/*
  ArduinoMqttClient - WiFi Simple Sender

  Author: Luigi Russi
  Date: 2023-12-01

  This example connects to a MQTT broker and publishes a message to
  a topic once a second. It makes use of UnoWiFiDevEdSerial1 and 
  WiFiEspAT with persistent WiFi settings to send DHT sensor 
  readings to the MQTT broker.
  
  The circuit:
  - Uno WiFi Dev Ed board
  - DHT22 (AM2302)

  This example code is in the public domain.
*/
#include "arduino_secrets.h"

#include <ArduinoMqttClient.h>

#include <UnoWiFiDevEdSerial1.h>
#include <WiFiEspAT.h>

#include "DHT.h"
#define DHTPIN 13     // Digital pin connected to the DHT sensor
#define VCCPIN 12     // Digital pin used as +5V (make sure I<20mA)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);

// Emulate Serial1 on pins 6/7 if not present
#if defined(ARDUINO_ARCH_AVR) && !defined(HAVE_HWSERIAL1)
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#define AT_BAUD_RATE 9600
#else
#define AT_BAUD_RATE 115200
#endif

// Initialise WiFi client
int status = WL_IDLE_STATUS;     // the Wifi radio's status
WiFiClient client;

// Initialise mqtt client
MqttClient mqttClient(client);

const char broker[] = BROKER_ADDRESS;
int        port     = 1883;
const char topic[]  = "arduino/simpleDHT";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial1.begin(AT_BAUD_RATE);
  WiFi.init(Serial1);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  // you're connected now, so print out the data:
  Serial.println();
  Serial.println("Waiting for connection to WiFi");
  for (int i = 0; i < 10; i++) {
    if (WiFi.status() == WL_CONNECTED)
      break;
    delay(1000);
    Serial.print('.');
  }
  Serial.println();

  printWifiStatus();
  Serial.println();
  
  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  mqttClient.setUsernamePassword(BROKER_USER, BROKER_PASS);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  pinMode(VCCPIN, OUTPUT);
  digitalWrite(VCCPIN, HIGH);
  dht.begin();
  Serial.println(F("DHT started"));
}

void loop() {
  // Read DHT sensor
  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(F("Heat index: "));
  Serial.print(hic);
  Serial.println(F("°C "));
  
  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();

  // to avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    // save the last time a message was sent
    previousMillis = currentMillis;

    Serial.print("Sending message to topic: ");
    Serial.print(topic);
    Serial.print(" msgID ");
    Serial.println(count);

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print("msgID:");
    mqttClient.print(count);
    mqttClient.print(" T:");
    mqttClient.print(t);
    mqttClient.print("°C");
    mqttClient.print(" RH:");
    mqttClient.print(h);
    mqttClient.print("%");
    mqttClient.print(" HI:");
    mqttClient.print(hic);
    mqttClient.print("°C");
    mqttClient.endMessage();

    Serial.println();

    count++;
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
