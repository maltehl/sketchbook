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
#include <SFE_BMP180.h>
#include <Adafruit_MPR121.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <SI7021.h>
#include "TSL2561.h"
#include <RtcDS3231.h>
#include <StreamString.h>
#include "debugutils.h"
#include "wifitools.h"

// Alarmtime in Minutes
#define UPDATEINTRERVAL  24*60
#define WAKEUPINTERVAL  10
#define WAKEUPINTERVALONERROR  10


extern "C" {
#include "user_interface.h"
int readvdd33(void);
}

SI7021 sensorLuftfeuchte;
Adafruit_MPR121 cap = Adafruit_MPR121();
TSL2561 tsl(TSL2561_ADDR_FLOAT); 
RtcDS3231 Rtc;
SFE_BMP180 pressure;


int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

void setup() {
  INIT_DEBUG_PRINT();
 
  Wire.begin(0,2) ;
  Rtc.Begin(0,2);
  DEBUG_PRINT("Start Wifi");
  DEBUG_PRINT("Voltage :");
  { 
    float vdd = 0.0;  
    int vdd33[5] ;
    
    int i = 0;
    
    for(i=0; i<5;i++)
    {
        vdd33[i] = system_adc_read();//readvdd33();
  	vdd += vdd33[i];
    }
    //vdd= (vdd /193.0) *1000.0;
    DEBUG_PRINT(vdd);
  }
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
  double T = 0.0;
  double P = 0.0;
  float C[5];
  char status;
  ++value;
  DEBUG_PRINT("connecting to ");
  DEBUG_PRINT(thingspeak_server);
    // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  
    //Start Client - Connection
  if (!client.connect(thingspeak_server, httpPort)) {
    DEBUG_PRINT("connection failed");
    GoToSleep(WAKEUPINTERVALONERROR);
    return;
  }
  DEBUG_PRINT("connected");
 /* 
  while(1)
  {
  //Bodenfeuchte messen
  {
    DEBUG_PRINT("cap");
        if (!cap.begin(0x5A)) {
		DEBUG_PRINT("MPR121 not found, check wiring?");
	        C[0] = 0.0;
                C[1] = 0.0;
                C[2] = 0.0;
        }
        else
        {
	        C[0] = 0.0;
                C[1] = 0.0;
                C[2] = 0.0;
              int i;
	      int count = 0;
	      for(i=0;i<10;i++)
	      {
  	          delay(10);
                  C[0] += cap.getCapacity(0);
                  C[1] += cap.getCapacity(1);
                  C[2] += cap.getCapacity(2);
	          count ++;
	      }
	      C[0] = C[0] / count;
              C[1] = C[1] / count;
              C[2] = C[2] / count;
      }
  }
  DEBUG_PRINT("C1: ");
  DEBUG_PRINT(C[0]);
  DEBUG_PRINT("C2: ");
  DEBUG_PRINT(C[1]);
  DEBUG_PRINT("C3: ");
  DEBUG_PRINT(C[2]);
  

  }*/
  //Start Pressure Sensor and get Temp Sensor Value
  {
    DEBUG_PRINT("press");
	pressure.begin();
	status= pressure.startTemperature();
	if (status != 0)
	{
        // Wait for the measurement to complete:
        delay(status);
	}
 
	DEBUG_PRINT("temp ");
	pressure.getTemperature(T);
  }

  //Get Pressure Sensor Value
  {  
	DEBUG_PRINT("luftdruck");
	status=pressure.startPressure(3);
	if (status != 0)
	{
        // Wait for the measurement to complete:
        delay(status);
	}
	pressure.getPressure(P,T);
  }
  
  float vdd = 0.0;
  {
    int vdd33[5] ;
    
    int i = 0;
    
    for(i=0; i<5;i++)
    {
        vdd33[i] = system_adc_read();//readvdd33();
  	vdd += vdd33[i];
    }
  }
  
  si7021_env dataLuft;
  //Get Luftfeuchtewert
  
  {
	DEBUG_PRINT("feuchte");
	sensorLuftfeuchte.begin(0,2);
	dataLuft = sensorLuftfeuchte.getHumidityAndTemperature();
  }
  uint16_t ir  = 0;
  uint16_t full = 0;
  //Get Light Intensity
  
  {
	tsl.begin();
	tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
	tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS); 
	uint32_t lum = tsl.getFullLuminosity();
	
	ir = lum >> 16;
	full = lum & 0xFFFF;
  }

 
  // We now create a URI for the request  https://api.thingspeak.com/update?key=EN2MTX538Q74SVKN&field1=0&field2=3
  String url = "/sensor.php";
  url += "?mac=";
  url += WiFi.macAddress();
  url += "&field1=";
  url += "0";//tsl.calculateLux(full, ir);
  url += "&field2=";
  url += full - ir;
  url += "&field3=";
  url += vdd;
  url += "&field4=";
  url += T;
  url += "&field5=";
  url += P;
  url += "&field6=";
  url += 0;//C[0];
  url += "&field7=";
  url += ((float)dataLuft.celsiusHundredths)/100;
  url += "&field8=";
  url += ((float)dataLuft.humidityBasisPoints)/100;
  url += "&field9=";
  url += 0;//C[1];
  url += "&field10=";
  url += 0;//C[2];
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  DEBUG_PRINT("send");
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + thingspeak_server + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    //Serial.print(line);
  }
DEBUG_PRINT("set alarms");
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    char bTimeValid = 1;
   


    RtcDateTime now = Rtc.GetDateTime();
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
   
    GoToSleep(WAKEUPINTERVAL);
}

/////////////////////////////////////////////////////////////////////
////////// Sleep_Function ///////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void GoToSleep(int minutes)
{
    RtcDateTime now = Rtc.GetDateTime();
    //Rtc.Begin();
        // Alarm 2 set to trigger at the top of the minute
    RtcDateTime alarmTime = now + (minutes *60) ; // into the future - every 30min (1800) nach Update gucken    
    DS3231AlarmTwo alarm2(
            alarmTime.Day(),
            alarmTime.Hour(),
            alarmTime.Minute(), 
            DS3231AlarmTwoControl_MinutesMatch);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeAlarmBoth); 
    Rtc.SetAlarmTwo(alarm2);

    // throw away any old alarm state before we ran
    //Rtc.LatchAlarmsTriggeredFlags();
    DEBUG_PRINT("Sleep");
    // setup external interupt 
    Rtc.LatchAlarmsTriggeredFlagsAndReset();
  

  system_deep_sleep(1*60*1000*1000); 
  delay(1000);


}

