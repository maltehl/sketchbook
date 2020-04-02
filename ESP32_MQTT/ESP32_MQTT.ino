/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

 mosquitto_sub -v -t zuHause/#



*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Update these with values suitable for your network.

const char* ssid = "D-Link";
const char* password = "PC0913idK@tb9";
const char* mqtt_server = "192.168.178.60";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
    Wire.begin(5, 4);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
 StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

    if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  double CO = root["CCS811"]["CO2"];
  double CV = root["CCS811"]["TVOC"];
  double Temp = root["SI7021"]["Temperature"];
  double Hum = root["SI7021"]["Humidity"];

  Serial.print("CO2: ");
  Serial.println(CO, 6);
  Serial.print("TVO: ");
  Serial.println(CV, 6);
  showValues(CO,CV,Temp,Hum);

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("sensor_1/sensor");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  

}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {

    delay(5000);
  }
}


void showValues(double value,double value2,double value3,double value4)
{
 char sstring[10];
  // Clear the buffer
  display.clearDisplay();


  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.print(F("CO2 : "));
  display.setTextSize(2);      // Normal 1:1 pixel scale
  sprintf(sstring,"%5.0f",value);
  display.print(sstring);
  display.setTextSize(1); 
  display.println(F(" ppm"));
  display.println(F(""));
  display.print(F("TVOC: "));
  display.setTextSize(2);      // Normal 1:1 pixel scale
  sprintf(sstring,"%5.0f",value2);
  display.print(sstring);
  display.setTextSize(1);
  display.println(F(" ppm"));
  display.println(F(""));
   display.print(F("Temp: "));
    display.setTextSize(2);      // Normal 1:1 pixel scale
  display.print(value3);
  display.setTextSize(1);
  display.println(F(" degC"));
  display.println(F(""));
   display.print(F("Humi: "));
    display.setTextSize(2);      // Normal 1:1 pixel scale
  display.print(value4);
  display.setTextSize(1);
  display.println(F(" %"));
  display.display();
}
