#include <PubSubClient.h>

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <Wire.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <RtcDS3231.h>

//Sensoren
#include "TSL2561.h"
//#include "SFE_BMP180.h"
#include "SI7021.h"
#include "Adafruit_MPR121.h"
#include "Adafruit_BMP085.h"
#include "Adafruit_CCS811.h"

//Intern
#include "debugutils.h"
#include "wifitools.h"


// Alarmtime in Minutes
#define UPDATEINTRERVAL  24*60
#define WAKEUPINTERVALSHORT  1 
#define WAKEUPINTERVALLONG  3 
#define WAKEUPINTERVALONERROR  3 

unsigned int uiWakenterval = WAKEUPINTERVALSHORT;

extern "C" {
#include "user_interface.h"
int readvdd33(void);
}


RtcDS3231 Rtc;
String response;
int resultCode;

void callback(char* topic, byte* payload, unsigned int length);

WiFiClient client;
PubSubClient mqttClient(client);

int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

void macToStr(const uint8_t* ar, char* macAdr)
{
  sprintf(macAdr, "%2X:%2X:%2X:%2X:%2X:%2X", ar[0], ar[1], ar[2], ar[3], ar[4], ar[5]);

}

void callback(char* topic, byte* payload, unsigned int length) {
  int value = 0;
  Serial.println("");
  Serial.print("callback1\t");
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }

  payload[length] = 0;
  value = atoi((char*)payload);
  uiWakenterval = value;
  Serial.print("Wert: \t");
  Serial.print(value);  
  Serial.println();
}

void GoToSleep(int minutes);


ADC_MODE(ADC_VCC);

int ccs_init = false;
int cap_init = false;
int pressure_init = false;
int luftfeuchte_init = false;
int light_init = false;

TSL2561 tsl(TSL2561_ADDR_FLOAT);        //0x39
Adafruit_BMP085 pressure;               //0x77
SI7021 sensorLuftfeuchte;               //0x40
Adafruit_MPR121 cap = Adafruit_MPR121();//0x5A
Adafruit_CCS811 ccs;                    //0x5B -> PIN ADDR set to HIGH!!


void setup() {
  INIT_DEBUG_PRINT();

  
  
  DEBUG_PRINT("Start Wifi");

  
  
  if(0==setupWifi())
  {
    GoToSleep(WAKEUPINTERVALONERROR);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("If we get here something went wiring!");
    GoToSleep(WAKEUPINTERVALONERROR);
  }

  
}

int value = 0;

void loop() {
  //delay(50000);
  char status;
  char TotalTopic[100];
  char Message[100];
  double T = 0.0;
  double P = 0.0;
  
  float C[13];

  ++value;
  DEBUG_PRINT("connecting to ");
  DEBUG_PRINT(speak_server);
    // Use WiFiClient class to create TCP connections

  const int httpPort = 80;

  
  StaticJsonBuffer<200> jsonBuffer;
  StaticJsonBuffer<900> jsonBuffer2;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& sensors = jsonBuffer2.createObject();
  mqttClient.setServer(speak_server, 1883);
  mqttClient.setCallback(callback);
  
  
  while (!mqttClient.connected()) {
	if (mqttClient.connect(mqtt_topic)) 
	{
      Serial.println("connected");
      // Once connected, publish an announcement...
      sprintf(TotalTopic,"%s/%s",mqtt_topic,"state");
      mqttClient.publish(TotalTopic,"hello world");
      // ... and resubscribe
      sprintf(TotalTopic,"%s/%s","Delay","#");
      Serial.println(TotalTopic);
      mqttClient.subscribe(TotalTopic);
      mqttClient.loop();
      mqttClient.loop();
    } 
	else 
	{
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      GoToSleep(uiWakenterval);
    }
  }
  
  
  // pinMode(A0, INPUT);

 float vdd = 0.0;
  {
    float vdd33 ;
    
    int i = 0;
    
    for(i=0; i<5;i++)
    {
      
     vdd33 = ((float)ESP.getVcc())/1024;
     vdd += vdd33;
    }
  }
  
  sprintf(TotalTopic,"%s/%s",mqtt_topic,"voltage");
  dtostrf(vdd/5, 4, 3, Message);
  root["voltage"] = vdd/5;
  mqttClient.publish(TotalTopic,Message);

//WiFi.macAddress()
  sprintf(TotalTopic,"%s/%s",mqtt_topic,"mac");
  byte mac[6];   
  WiFi.macAddress(mac);
  char macAdr[20]; 
  macToStr(mac,macAdr);
  //sprintf(Message,"%s",macAdr);
  root["mac"] = macAdr;
  mqttClient.publish(TotalTopic,macAdr);
  


  sprintf(TotalTopic,"%s/%s",mqtt_topic,"rssi");
  sprintf(Message,"%ld",WiFi.RSSI());
  root["rssi"] = WiFi.RSSI();
  mqttClient.publish(TotalTopic,Message);

  //delay(100);
  sensors["state"] = "OK";
  JsonObject& PIRSensor = sensors.createNestedObject("PIR");

    PIRSensor["Motion"] = "ON";


	  while (!mqttClient.connected()) {
      
  
      if (WiFi.status() != WL_CONNECTED) {
        if(0==setupWifi())
        {
          GoToSleep(WAKEUPINTERVALONERROR);
        }
      }
	  if (mqttClient.connect("sensor_1")) {
		  Serial.println("connected");
		  // Once connected, publish an announcement...
		  sprintf(TotalTopic,"%s/%s",mqtt_topic,"state");
		  mqttClient.publish(TotalTopic,"hello world");
		  // ... and resubscribe
		  //mqttClient.subscribe("inTopic");
		} else {
		  Serial.print("failed, rc=");
		  Serial.print(mqttClient.state());
		  Serial.println(" try again in 5 seconds");
		  delay(5000);
		  //GoToSleep(uiWakenterval);
		}
	  }
  
  char JsonString[500];
  sensors.printTo(JsonString,sizeof(JsonString));
  Serial.print(JsonString);

  sprintf(TotalTopic,"%s/%s",mqtt_topic,"sensor");
  mqttClient.publish(TotalTopic,JsonString);

  mqttClient.loop();
  mqttClient.loop();
  delay(100);
  
  //RtcDateTime now = Rtc.GetDateTime();  
  
/*
  DEBUG_PRINT("set alarms");
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  char bTimeValid = 1;
  
  now = Rtc.GetDateTime();
  
  DEBUG_PRINT(now);
  if (now < compiled) 
  {
     DEBUG_PRINT("RTC is older than compile time!  (Updating DateTime)");
     Rtc.SetDateTime(compiled);
  }
    
    
  if (!Rtc.IsDateTimeValid()) 
  {
     DEBUG_PRINT("RTC lost confidence in the DateTime!");
     Rtc.SetDateTime(compiled);
     bTimeValid = 0;
  }

  if (!Rtc.GetIsRunning())
  {
     DEBUG_PRINT("RTC was not actively running, starting now");
     Rtc.SetIsRunning(true);
  }
  
  char bUpdateneeded = 0;
    
  DS3231AlarmFlag flag = Rtc.LatchAlarmsTriggeredFlags();

  if (flag & DS3231AlarmFlag_Alarm1 || bTimeValid == 0)
  {
    DEBUG_PRINT("alarm one triggered - update");

    bUpdateneeded =1;
    // Alarm 1 set to trigger every day when 
    // the hours, minutes, and seconds match
    RtcDateTime alarmTime = now + UPDATEINTRERVAL * 60; // into the future - every 30min (1800) nach Update gucken
    DS3231AlarmOne alarm1(
            alarmTime.Day(),
            alarmTime.Hour(),
            alarmTime.Minute(), 
            alarmTime.Second(),
    DS3231AlarmOneControl_HoursMinutesSecondsMatch);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmBoth); 
    Rtc.SetAlarmOne(alarm1);
    DEBUG_PRINT("SetAlarm 1");
  }
   
   */
    GoToSleep(uiWakenterval);
}

/////////////////////////////////////////////////////////////////////
////////// Sleep_Function ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void GoToSleep(int minutes)
{
   /* RtcDateTime now = Rtc.GetDateTime();
    //Rtc.Begin();
        // Alarm 2 set to trigger at the top of the minute
    RtcDateTime alarmTime = now + (minutes *60); // into the future - every 30min (1800) nach Update gucken    
    DS3231AlarmTwo alarm2(
            alarmTime.Day(),
            alarmTime.Hour(),
            alarmTime.Minute(), 
            DS3231AlarmTwoControl_HoursMinutesMatch);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmBoth); 
    Rtc.SetAlarmTwo(alarm2);

    // throw away any old alarm state before we ran
    //Rtc.LatchAlarmsTriggeredFlags();
    
    // setup external interupt 
    Rtc.LatchAlarmsTriggeredFlagsAndReset();
  
  */
  DEBUG_PRINT("Sleep");
  //system_deep_sleep(1*60*1000*1000); 
  delay(60000);


}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
