#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoMqttClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>

// WiFi
#define SSID "my_wifi"
#define PASS "password"
int status = WL_IDLE_STATUS;
WiFiClient wifi;

// MQTT
MqttClient mqttClient(wifi);
char broker[] = "10.1.4.254";
int port = 1883;
char topic[] = "lights";
char clientID[] = "lvingroomClient";
#define MQTT_USER "mqttuser"
#define MQTT_PASS "mqttpass"

// LED
const int ledDataPin = 0;
const int numPixels = 36;
const int numStrips = 2;
int red = 0;
int grn = 0;
int blu = 0;
int brt = 100;
const char* pwr = "OFF";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(numPixels, ledDataPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);

  while (status != WL_CONNECTED){
    Serial.print("Connecting to: ");
    Serial.println(SSID);
    status = WiFi.begin(SSID, PASS);
    delay(1000); 
  }

  Serial.println("Connected :)");
  Serial.print("My IP Address is: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  mqttClient.setId(clientID);
  mqttClient.setUsernamePassword(MQTT_USER, MQTT_PASS);

  Serial.print("Attempting to connect to broker at: ");
  Serial.print(broker);
  Serial.print(":");
  Serial.println(port);
  while (!connectToBroker()) {
    delay(1000);
  }
  Serial.println("Connected to MQTT broker");

  pixels.begin();
  pixels.clear();
  pixels.show();
}

void loop() {
  // Reconnect to the broker if we lose our connection
  if (!mqttClient.connected()){
    Serial.println("Lost connection to broker, reconnecting");
    connectToBroker();
  }

  if (mqttClient.parseMessage() > 0) {
    Serial.print("Got a message on topic: ");
    Serial.println(mqttClient.messageTopic());
    StaticJsonDocument<256> doc;
    deserializeJson(doc, mqttClient);
    char buffer[100];
    serializeJsonPretty(doc, buffer);
    Serial.println(buffer);

    if(!doc["color"].isNull()){
      red = doc["color"]["r"];
      grn = doc["color"]["g"];
      blu = doc["color"]["b"];
    }

    if (!doc["brightness"].isNull()){
      float brtRaw = doc["brightness"];
      brt = ((brtRaw/255) * 100);
    }

    if (!doc["state"].isNull()){
      pwr = doc["state"];
    }

    Serial.print("Red: ");
    Serial.println(red);
    Serial.print("Green: ");
    Serial.println(grn);
    Serial.print("Blue: ");
    Serial.println(blu);
    Serial.print("Brightness: ");
    Serial.println(brt);
    Serial.print("State: ");
    Serial.println(pwr);


    // Build a color from the MQTT message
    uint32_t newColor = pixels.Color(red,grn,blu);
    // Set the color on all LEDs
    for (int i = 0; i <= numPixels; i++){
      pixels.setPixelColor(i, newColor);
    }
    if(strcmp(pwr,"ON") == 0){
      pixels.setBrightness(brt);
    }
    else{
      pixels.setBrightness(0);
    }
    
    pixels.show();
  }
}

boolean connectToBroker() {
  // If the MQTT client is not connected, print the error.
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MOTT connection failed. Error no: ");
    Serial.println(mqttClient.connectError());
    return false;
  }
  mqttClient.subscribe("switch");
  return true;
}
