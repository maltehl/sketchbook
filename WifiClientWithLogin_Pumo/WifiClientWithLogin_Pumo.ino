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

#include "debugutils.h"
#include "wifitools.h"

int tPumpPin1 = D5; //1 == TX PIn 3 = RX PIn
int tPumpPin2 = D6; 
int tPumpPin3 = D7;
int tPumpPin4 = D8;

// Alarmtime in Minutes
#define UPDATEINTRERVAL  24*60
#define WAKEUPINTERVALSHORT  3 
#define WAKEUPINTERVALLONG  30 
#define WAKEUPINTERVALONERROR  3 

unsigned int uiWakenterval = WAKEUPINTERVALLONG;

extern "C" {
#include "user_interface.h"
int readvdd33(void);
}

//MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);
RtcDS3231 Rtc;
String response;
int resultCode;

int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

void setup() {
  INIT_DEBUG_PRINT();
  pinMode(tPumpPin1,OUTPUT);
  pinMode(tPumpPin2,OUTPUT);
  pinMode(tPumpPin3,OUTPUT);
  pinMode(tPumpPin4,OUTPUT);
  digitalWrite(tPumpPin1,LOW);
  digitalWrite(tPumpPin2,LOW);
  digitalWrite(tPumpPin3,LOW);
  digitalWrite(tPumpPin4,LOW);
  Wire.begin(D1,D2) ;
  Rtc.Begin(D1,D2);
  
  DEBUG_PRINT("Start Wifi");
  
   if(0==setupWifi())
  {
    GoToSleep(WAKEUPINTERVALONERROR);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT("If we get here something went wring!");
    GoToSleep(WAKEUPINTERVALONERROR);
  }

  
}

int value = 0;

void loop() {
  //delay(50000);
  char status;
  ++value;
  DEBUG_PRINT("connecting to ");
  DEBUG_PRINT(speak_server);
    // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  
/*    //Start Client - Connection
  if (!client.connect(thingspeak_server, httpPort)) {
    DEBUG_PRINT("connection failed");
    GoToSleep(WAKEUPINTERVALONERROR);
    return;
  }
  DEBUG_PRINT("connected");
  //Read Temperature
   
*/
   pinMode(A0, INPUT);

  float volt;

  
   float vdd = 0.0;
  {
    int vdd33[5] ;
    
    int i = 0;
    
    for(i=0; i<5;i++)
    {
        vdd33[i] = analogRead(A0);
  	vdd += vdd33[i] /5;
    }
    volt=vdd/1023.0;
    volt=volt*4200;
  }
  RtcDateTime now = Rtc.GetDateTime();
  if (!client.connect(speak_server, httpPort)) {
    DEBUG_PRINT("connection failed");
    GoToSleep(WAKEUPINTERVALONERROR);
    return;
  }
  
  String url2 = "/sensor/actuator.php";
  url2 += "?mac=";
  url2 += WiFi.macAddress();
  String url = "&field1=";
  url += 0;//pinSet;
  url += "&field2=";
  url += 0;
  url += "&field3=";
  url += volt;
  
  //Serial.print("Requesting URL: ");
  //Serial.println(url);
  DEBUG_PRINT("send");
  // This will send the request to the server
  client.print(String("GET ") + url2 + url + " HTTP/1.1\r\n" +
               "Host: " + speak_server + "\r\n" + 
               "Connection: close\r\n\r\n");
 // delay(10);
  //DEBUG_PRINT(url2);
  unsigned long lasttime = millis();
  while (!client.available() && millis() - lasttime < 1000) {delay(1);} 
  
  // Read all the lines of the reply from server and print them to Serial
  String sReturn;
  while(client.available()){
    char line = client.read();
    sReturn += line;
   // Serial.write(line);
  }
  //DEBUG_PRINT(sReturn);
  int bodyStart = sReturn.indexOf("<return>");
  int bodyEnd = sReturn.indexOf("<\return>");
  sReturn = sReturn.substring(bodyStart+8,bodyEnd-bodyStart);
  
  String xval = getValue(sReturn, ':', 0);
  String xval2 = getValue(sReturn, ':', 1);
  String xval3 = getValue(sReturn, ':', 2);
  String xval4 = getValue(sReturn, ':', 3);
/*  DEBUG_PRINT("1:");
  DEBUG_PRINT(xval);
  DEBUG_PRINT("2:");
  DEBUG_PRINT(xval2);
  DEBUG_PRINT("3:");
  DEBUG_PRINT(xval3);
  DEBUG_PRINT("4:");
  DEBUG_PRINT(xval4);*/
  
  
    //ON/OFF
  int pinSet = 0;
  int pump1on=0;
  int pump2on=0;
  int pump3on=0;
  int pump4on=0;
  if(xval.toInt() != 0)
  {
    pinSet = 1;
    pump1on=1;
    digitalWrite(tPumpPin1,HIGH);
    DEBUG_PRINT("1 AN");
    delay(3000);
    digitalWrite(tPumpPin1,LOW);
  }
    if(xval2.toInt() != 0)
  {
    pinSet = 1;
    pump2on=1;
    digitalWrite(tPumpPin2,HIGH);
    DEBUG_PRINT("2 AN");
    delay(3000);
    digitalWrite(tPumpPin2,LOW);
  }
    if(xval3.toInt() != 0)
  {
    pinSet = 1;
    pump3on=1;
    DEBUG_PRINT("3 AN");
    digitalWrite(tPumpPin3,HIGH);
    delay(3000);
    digitalWrite(tPumpPin3,LOW);
  }
  if(xval4.toInt() != 0)
  {
    pinSet = 1;
    pump4on=1;
    DEBUG_PRINT("4 AN");
    digitalWrite(tPumpPin4,HIGH);
    delay(3000);
    digitalWrite(tPumpPin4,LOW);
  }
  
  if(pinSet == 1)
  {
    digitalWrite(tPumpPin1,LOW);
    digitalWrite(tPumpPin2,LOW);
    digitalWrite(tPumpPin3,LOW);
    digitalWrite(tPumpPin4,LOW);
    uiWakenterval = WAKEUPINTERVALSHORT;
  }
  else
  {
    digitalWrite(tPumpPin1,LOW);
    digitalWrite(tPumpPin2,LOW);
    digitalWrite(tPumpPin3,LOW);
    digitalWrite(tPumpPin4,LOW);
    uiWakenterval = WAKEUPINTERVALLONG;
  }
  
  if (!client.connect(speak_server, httpPort)) {
    DEBUG_PRINT("connection failed");
    GoToSleep(WAKEUPINTERVALONERROR);
    return;
  }
  {
   int i;
   int vdd33[5] ;
   vdd=0;
    for(i=0; i<5;i++)
    {
        vdd33[i] = analogRead(A0);
  	vdd += vdd33[i] /5;
    }
    volt=vdd/1023.0;
    volt=volt*4200;
  }
  
  url2 = "/sensor/sensor2.php";
  url2 += "?mac=";
  url2 += WiFi.macAddress();
  
  String sensor = "&field1=";
  sensor += 0;
  sensor += "&field2=";
  sensor += 0;
  sensor += "&field3=";
  sensor += volt;
  sensor += "&field4=";
  sensor += pump1on;
  sensor += "&field5=";
  sensor += pump2on;
  sensor += "&field6=";
  sensor += pump3on;
  sensor += "&field7=";
  sensor += pump4on;
  //Serial.print("Requesting URL: ");
  //Serial.println(url);
  DEBUG_PRINT("send");
  // This will send the request to the server
  client.print(String("GET ") + url2 + sensor + " HTTP/1.1\r\n" +
               "Host: " + speak_server + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  
  
  

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
   
   if(bUpdateneeded ==1)
   {
      t_httpUpdate_return ret = 
        ESPhttpUpdate.update("http://deltafx.bplaced.net/sensor/arduino.php",String(compiled));
        //t_httpUpdate_return  ret = ESPhttpUpdate.update("https://server/file.bin");

        switch(ret) {
            case HTTP_UPDATE_FAILED:
                DEBUG_PRINTF("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                DEBUG_PRINT("HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                DEBUG_PRINT("HTTP_UPDATE_OK");
                break;
        }
   }
   
    GoToSleep(uiWakenterval);
}

/////////////////////////////////////////////////////////////////////
////////// Sleep_Function ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void GoToSleep(int minutes)
{
    RtcDateTime now = Rtc.GetDateTime();
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
DEBUG_PRINT("Sleep");
    // setup external interupt 
    Rtc.LatchAlarmsTriggeredFlagsAndReset();
  

  system_deep_sleep(1*60*1000*1000); 
  delay(10000);


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

