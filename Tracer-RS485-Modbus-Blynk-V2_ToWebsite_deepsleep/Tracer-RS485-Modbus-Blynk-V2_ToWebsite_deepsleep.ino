// CONNECT THE RS485 MODULE.
// MAX485 module <-> ESP8266
//  - DI -> D10 / GPIO1 / TX
//  - RO -> D9 / GPIO3 / RX
//  - DE and RE are interconnected with a jumper and then connected do eighter pin D1 or D2
//  - VCC to +5V / VIN on ESP8266
//  - GNDs wired together
// -------------------------------------
// You do not need to disconnect the RS485 while uploading code.
// After first upload you should be able to upload over WiFi
// Tested on NodeMCU + MAX485 module
// RJ 45 cable: Green -> A, Blue -> B, Brown -> GND module + GND ESP8266
// MAX485: DE + RE interconnected with a jumper and connected to D1 or D2
//
// Developed by @jaminNZx
// With modifications by @tekk
#include <FS.h>  
#include <ESP8266WiFi.h>
#include <DNSServer.h>
//#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiManager.h>
#include <ArduinoJson.h> 
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ModbusMaster.h>
#include "settings.h"
#include <SimpleTimer.h> 
#include "debugutils.h"
#include "wifitools.h" 



#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

const int defaultBaudRate = 115200;
int timerTask1, timerTask2, timerTask3;
float battChargeCurrent, battDischargeCurrent, battOverallCurrent, battChargePower;
float bvoltage, ctemp, btemp, bremaining, lpower, lcurrent, pvvoltage, pvcurrent, pvpower;
float stats_today_pv_volt_min, stats_today_pv_volt_max;
float generateEnergieToday,  generateEnergieMonth, generateEnergieJear, generateEnergieTotal;
uint8_t result;
bool rs485DataReceived = true;
bool loadPoweredOn = true;

void AddressRegistry_311A();
void AddressRegistry_3100();
void AddressRegistry_3106();
void AddressRegistry_310D();
void AddressRegistry_311A();
void AddressRegistry_330C();
void AddressRegistry_331B();
 void executeCurrentRegistryFunction();
 void uploadToBlynk();

#define MAX485_DE D1
#define MAX485_RE_NEG D2

int sendout = 0;

boolean _3100_success = false;
boolean _3106_success = false;
boolean _310D_success = false;
boolean _311A_success = false;
boolean _330C_success = false;
boolean _331B_success = false;

ModbusMaster node;
SimpleTimer timer;
  WiFiClient client;
  const int httpPort = 80;

void preTransmission() {
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission() {
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

// A list of the regisities to query in order
typedef void (*RegistryList[])();

RegistryList Registries = {
  AddressRegistry_3100,
  AddressRegistry_3106,
  AddressRegistry_310D,
  AddressRegistry_311A,
  AddressRegistry_330C,
  AddressRegistry_331B,
};

// keep log of where we are
uint8_t currentRegistryNumber = 0;

// function to switch to next registry
void nextRegistryNumber() {
  // better not use modulo, because after overlow it will start reading in incorrect order
  currentRegistryNumber++;
  if (currentRegistryNumber >= ARRAY_SIZE(Registries)) {
    currentRegistryNumber = 0;
  }
}

// ****************************************************************************

void setup()
{
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  Serial.begin(defaultBaudRate);

  // Modbus slave ID 1
  node.begin(1, Serial);

  // callbacks to toggle DE + RE on MAX485
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  

  
 /*  
#if defined(USE_LOCAL_SERVER)
  Blynk.begin(AUTH, WIFI_SSID, WIFI_PASS, SERVER);
#else
  Blynk.begin(AUTH, WIFI_SSID, WIFI_PASS);
#endif

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  Serial.println("Connected.");
  Serial.print("Connecting to Blynk...");

  while (!Blynk.connect()) {
    Serial.print(".");
    delay(100);
  }

  Serial.println();
  Serial.println("Connected to Blynk.");
  Serial.println("Starting ArduinoOTA...");

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword((const char *)OTA_PASS);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd of update");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  
  ArduinoOTA.begin();

  Serial.print("ArduinoOTA running. ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());*/  
  Serial.println("Starting timed actions...");

  timerTask1 = timer.setInterval(1000L, executeCurrentRegistryFunction);
  timerTask2 = timer.setInterval(1000L, nextRegistryNumber);
  timerTask3 = timer.setInterval(60000L, uploadToBlynk);

  Serial.println("Setup OK!");
  Serial.println("----------------------------");
  Serial.println();
}

// --------------------------------------------------------------------------------
  
  // upload values
  void uploadToBlynk() {
sendout = 1;
  Serial.println("Connecting to Wifi...");
  
 setupWifi();
 
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("If we get here something went wring!");
  }


      //Start Client - Connection
    if (!client.connect(thingspeak_server, httpPort)) {
      DEBUG_PRINT("connection failed");
      return;
    }
    Serial.print("pvpower  : ");
    Serial.println(pvpower);
    Serial.print("pvcurrent: ");
    Serial.println(pvcurrent);
    Serial.print("pvvoltage: ");
    Serial.println(pvvoltage);
    Serial.print("bvoltage : ");
    Serial.println(bvoltage);
    Serial.print("bCCurrent: ");
    Serial.println(battChargeCurrent);
    Serial.print("bCPower  : ");
    Serial.println(battChargePower);
    Serial.print("gToday   : ");
    Serial.println(generateEnergieToday);
    Serial.print("gEMonth  : ");
    Serial.println(generateEnergieMonth);
    Serial.print("geEJear  : ");
    Serial.println(generateEnergieJear);
    Serial.print("geETotal : ");
    Serial.println(generateEnergieTotal);
    Serial.print("btemp    : ");
    Serial.println(btemp);


    String url = "/sensor/sensor2.php";
  url += "?mac=";
  url += WiFi.macAddress();
  url += "&field1=";
  url += pvpower;//tsl.calculateLux(full, ir);
  url += "&field2=";
  url += pvcurrent;
  url += "&field3=";
  url += pvvoltage;
  url += "&field4=";
  url += bvoltage;
  url += "&field5=";
  url += battChargeCurrent;
  url += "&field6=";
  url += battChargePower;
  url += "&field7=";
  url += generateEnergieToday;
  url += "&field8=";
  url += generateEnergieMonth;
  url += "&field9=";
  url += generateEnergieTotal;
  url += "&field10=";
  url += btemp;
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + thingspeak_server + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(10);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
    }
  //client.disconnect();
  }
/*    Blynk.virtualWrite(vPIN_PV_POWER,                   pvpower);
    Blynk.virtualWrite(vPIN_PV_CURRENT,                 pvcurrent);
    Blynk.virtualWrite(vPIN_PV_VOLTAGE,                 pvvoltage);
    Blynk.virtualWrite(vPIN_LOAD_CURRENT,               lcurrent);
    Blynk.virtualWrite(vPIN_LOAD_POWER,                 lpower);
    Blynk.virtualWrite(vPIN_BATT_TEMP,                  btemp);
    Blynk.virtualWrite(vPIN_BATT_VOLTAGE,               bvoltage);
    Blynk.virtualWrite(vPIN_BATT_REMAIN,                bremaining);
    Blynk.virtualWrite(vPIN_CONTROLLER_TEMP,            ctemp);
    Blynk.virtualWrite(vPIN_BATTERY_CHARGE_CURRENT,     battChargeCurrent);
    Blynk.virtualWrite(vPIN_BATTERY_CHARGE_POWER,       battChargePower);
    Blynk.virtualWrite(vPIN_BATTERY_OVERALL_CURRENT,    battOverallCurrent);
    Blynk.virtualWrite(vPIN_LOAD_ENABLED,               loadPoweredOn);
  }*/
  
  // exec a function of registry read (cycles between different addresses)
  void executeCurrentRegistryFunction() {
    Registries[currentRegistryNumber]();
  }
  
  uint8_t setOutputLoadPower(uint8_t state) {
    Serial.print("Writing coil 0x0006 value to: ");
    Serial.println(state);

    delay(10);
    // Set coil at address 0x0006 (Force the load on/off)
    result = node.writeSingleCoil(0x0006, state);

    if (result == node.ku8MBSuccess) {
      node.getResponseBuffer(0x00);
      Serial.println("Success.");
    }

    return result;
  }
  
  /*// callback to on/off button state changes from the Blynk app
  BLYNK_WRITE(vPIN_LOAD_ENABLED) {
    uint8_t newState = (uint8_t)param.asInt();
    
    Serial.print("Setting load state output coil to value: ");
    Serial.println(newState);

    result = setOutputLoadPower(newState);
    //readOutputLoadState();
    result &= checkLoadCoilState();
    
    if (result == node.ku8MBSuccess) {
      Serial.println("Write & Read suceeded.");
    } else {
      Serial.println("Write & Read failed.");
    }
    
    Serial.print("Output Load state value: ");
    Serial.println(loadPoweredOn);
    Serial.println();
    Serial.println("Uploading results to Blynk.");
        
    uploadToBlynk();
  }
  */
  uint8_t readOutputLoadState() {
    delay(10);
    result = node.readHoldingRegisters(0x903D, 1);
    
    if (result == node.ku8MBSuccess) {
      loadPoweredOn = (node.getResponseBuffer(0x00) & 0x02) > 0;
  
      Serial.print("Set success. Load: ");
      Serial.println(loadPoweredOn);
    } else {
      // update of status failed
      Serial.println("readHoldingRegisters(0x903D, 1) failed!");
    }
    return result;
  }
  
  // reads Load Enable Override coil
  uint8_t checkLoadCoilState() {
    Serial.print("Reading coil 0x0006... ");

    delay(10);
    result = node.readCoils(0x0006, 1);
    
    Serial.print("Result: ");
    Serial.println(result);

    if (result == node.ku8MBSuccess) {
      loadPoweredOn = (node.getResponseBuffer(0x00) > 0);

      Serial.print(" Value: ");
      Serial.println(loadPoweredOn);
    } else {
      Serial.println("Failed to read coil 0x0006!");
    }

    return result;
 }

// -----------------------------------------------------------------

  void AddressRegistry_3100() {
    result = node.readInputRegisters(0x3100, 6);
  
    if (result == node.ku8MBSuccess) {
      
      pvvoltage = node.getResponseBuffer(0x00) / 100.0f;
    //  Serial.print("PV Voltage: ");
   //   Serial.println(pvvoltage);
  
      pvcurrent = node.getResponseBuffer(0x01) / 100.0f;
   //   Serial.print("PV Current: ");
  //    Serial.println(pvcurrent);
  
      pvpower = (node.getResponseBuffer(0x02) | node.getResponseBuffer(0x03) << 16) / 100.0f;
  //    Serial.print("PV Power: ");
  //    Serial.println(pvpower);
      
      bvoltage = node.getResponseBuffer(0x04) / 100.0f;
  //    Serial.print("Battery Voltage: ");
  //    Serial.println(bvoltage);
      
      battChargeCurrent = node.getResponseBuffer(0x05) / 100.0f;
  //    Serial.print("Battery Charge Current: ");
  //    Serial.println(battChargeCurrent);
      _3100_success = true;
    }
     else {
      rs485DataReceived = false;
      Serial.println("Read register 0x3100 failed!");
      _3100_success = false;
    } 
  }

  void AddressRegistry_3106()
  {
    result = node.readInputRegisters(0x3106, 2);

    if (result == node.ku8MBSuccess) {
      battChargePower = (node.getResponseBuffer(0x00) | node.getResponseBuffer(0x01) << 16)  / 100.0f;
     // Serial.print("Battery Charge Power: ");
     // Serial.println(battChargePower);
     _3106_success = true;
    }
    else {
      rs485DataReceived = false;
      Serial.println("Read register 0x3106 failed!");
      _3106_success = false;
    }    
  }

  void AddressRegistry_310D() 
  {
    result = node.readInputRegisters(0x310D, 3);

    if (result == node.ku8MBSuccess) {
      lcurrent = node.getResponseBuffer(0x00) / 100.0f;
  //    Serial.print("Load Current: ");
  //    Serial.println(lcurrent);
  
      lpower = (node.getResponseBuffer(0x01) | node.getResponseBuffer(0x02) << 16) / 100.0f;
  //    Serial.print("Load Power: ");
  //    Serial.println(lpower);
        _310D_success = true;
    } else {
      rs485DataReceived = false;
      Serial.println("Read register 0x310D failed!");
      _310D_success = false;
    }    
  } 

  void AddressRegistry_311A() {
    result = node.readInputRegisters(0x310F, 2);
   
    if (result == node.ku8MBSuccess) {    
      bremaining = node.getResponseBuffer(0x00) / 1.0f;
//      Serial.print("Battery Remaining %: ");
//      Serial.println(bremaining);
      
      btemp = node.getResponseBuffer(0x01) / 100.0f;
 //     Serial.print("Battery Temperature: ");
   //   Serial.println(btemp);
      _311A_success = true;
    } else {
      rs485DataReceived = false;
      Serial.println("Read register 0x311A failed!");
      _311A_success = false;
    }
  }

  void AddressRegistry_330C() {
    result = node.readInputRegisters(0x330C, 8);
  
    if (result == node.ku8MBSuccess) {
      
      generateEnergieToday = (node.getResponseBuffer(0x00)| node.getResponseBuffer(0x01) << 16) / 100.0f;
      generateEnergieMonth = (node.getResponseBuffer(0x02)| node.getResponseBuffer(0x03) << 16) / 100.0f;
      generateEnergieJear = (node.getResponseBuffer(0x04)| node.getResponseBuffer(0x05) << 16) / 100.0f;
      generateEnergieTotal = (node.getResponseBuffer(0x06)| node.getResponseBuffer(0x07) << 16) / 100.0f;
      _330C_success = true;
    } else {
      rs485DataReceived = false;
      Serial.println("Read register 0x330C failed!");
      _330C_success = false;
    }
  }


  void AddressRegistry_331B() {
    result = node.readInputRegisters(0x331B, 2);
    
    if (result == node.ku8MBSuccess) {
      battOverallCurrent = (node.getResponseBuffer(0x00) | node.getResponseBuffer(0x01) << 16) / 100.0f;
    //  Serial.print("Battery Discharge Current: ");
    //  Serial.println(battOverallCurrent);
      _331B_success = true;
    } else {
      rs485DataReceived = false;
      Serial.println("Read register 0x331B failed!");
      _331B_success = false;
    }
  }

void GoToSleep(int minutes)
{
    // throw away any old alarm state before we ran
    //Rtc.LatchAlarmsTriggeredFlags();
    DEBUG_PRINT("Sleep");
    // setup external interupt 

  

  system_deep_sleep(minutes*60*1000*1000); 
  delay(1000);


}

void loop()
{
  //Blynk.run();
  //ArduinoOTA.handle();
 // timer.run();
  for(int i = 0; i<2; i++)
  {
    if(_3100_success == false)
    {
      delay(100);
      AddressRegistry_3100();
    }
    if(_3106_success == false)
    {
      delay(100);
      AddressRegistry_3106();
    }
    if(_310D_success == false)
    {
        delay(100);
        AddressRegistry_310D();
    }
    if(_311A_success == false)
    { 
      delay(100);
      AddressRegistry_311A();
    }
    if(_330C_success == false)
    {
      delay(100);
      AddressRegistry_330C();
    }
    if(_331B_success == false)
    {
      delay(100);
      AddressRegistry_331B();
    }

  }
  uploadToBlynk();
  if (sendout==1)
  {
      
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.persistent(true);
    Serial.println("Wifi disconnected");
    
    GoToSleep(10);
  }
}
