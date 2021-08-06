#include <string.h>
/////////////////////////////////////////////////////////////////////
////////// Wifi Connect + Parameter /////////////////////////////////
/////////////////////////////////////////////////////////////////////
#define MAXIMUMRETRY 200

const char* ssidb     = "D-Link";
const char* password = "PC0913idK@tb9";
//for the request  https://api.thingspeak.com/update?key=EN2MTX538Q74SVKN&field1=0&field2=3
//const char* host = "184.106.153.149";
//const char* host =  "api.thingspeak.com";
//const char* privateKey = "JVW0C1GD5F4NZY49"; //Wohnzimmer
//const char* privateKey = "NLL2PMCFEFT1M2J3"; //Musikzimmer
//const char* privateKey = "MDH9TVLFIGPLWSLH"; // Feigenbaum
//CFGM2QIT5Y0D2387
char thingspeak_server[40] ="184.106.153.149";
char thingspeak_port[6] = "8080";
char thingspeak_token[34] = "MDH9TVLFIGPLWSLH";
String ip = "0";
String mask = "0";
String gw = "0";


//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  DEBUG_PRINT("Should save config");
  shouldSaveConfig = true;
}

int setupWifi() {
  // put your setup code here, to run once:
 // {
  // SPIFFS.format();
   WiFiManager wifiManager;
  // wifiManager.resetSettings();
 // }
  delay(100);
  DEBUG_PRINT("Cleaning up");
  

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    //clean FS, for testing
    //SPIFFS.format();
    DEBUG_PRINT("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      
      //file exists, reading and loading
      DEBUG_PRINT("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        DEBUG_PRINT("opened config file");
        size_t size = configFile.size();
        if(size <10)
        {
          DEBUG_PRINT("Config File empty");
          return 0;
        }
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          DEBUG_PRINT("\nparsed json");
       //   DEBUG_PRINT(json);
          strcpy(thingspeak_server, json["thingspeak_server"]);
          strcpy(thingspeak_port, json["thingspeak_port"]);
          strcpy(thingspeak_token, json["thingspeak_token"]);
          ip = json["IP"].asString();
          mask = json["NetMask"].asString();
          gw = json["Gateway"].asString();
	//	  strcpy(ip, json["IP"].asString());
	//	  strcpy(mask, json["NetMask"]);
	//	  strcpy(gw, json["Gateway"]);
	  DEBUG_PRINT(ip);	
          DEBUG_PRINT(mask);
          DEBUG_PRINT(gw);		

        } else {
          DEBUG_PRINT("failed to load json config");
          return 0;
        }
      }
    }
  } else {
    DEBUG_PRINT("failed to mount FS");
    return 0;
  }
  //end read

  DEBUG_PRINT("ExtraParameter");

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_thingspeak_server("server", "thingspeak server", thingspeak_server, 40);
  WiFiManagerParameter custom_thingspeak_port("port", "thingspeak port", thingspeak_port, 5);
  WiFiManagerParameter custom_thingspeak_token("token", "thingspeak token", thingspeak_token, 32);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
 
  //wifiManager.resetSettings();
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  
  if(ip != "0" && gw != "0" && mask != "0")
  {
        DEBUG_PRINT("Parse IP");
        IPAddress sip;
	IPAddress sgw;
	IPAddress smask;

	if (sip.fromString(ip) != true) {
		DEBUG_PRINT("invalid IP: ");
		DEBUG_PRINT(ip);
	}
        if (sgw.fromString(gw) != true) {
		DEBUG_PRINT("invalid IP: ");
		DEBUG_PRINT(ip);
	}
        if (smask.fromString(mask) != true) {
		DEBUG_PRINT("invalid IP: ");
		DEBUG_PRINT(ip);
	}
    wifiManager.setSTAStaticIPConfig(sip, sgw, smask);
	DEBUG_PRINT("Connect with StaticIP");
	DEBUG_PRINT(sip);
	DEBUG_PRINT(sgw);
	DEBUG_PRINT(smask);
  }
  else
  {
	shouldSaveConfig = 1;
	DEBUG_PRINT("Connect with Dynacmic IP");
  }
  
  
  //add all your parameters here
  wifiManager.addParameter(&custom_thingspeak_server);
  wifiManager.addParameter(&custom_thingspeak_port);
  wifiManager.addParameter(&custom_thingspeak_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  //WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA );
  if (!wifiManager.autoConnect()) {
    DEBUG_PRINT("failed to connect and hit timeout");
    return 0;
  }

  //if you get here you have connected to the WiFi
  DEBUG_PRINT("connected...yeey :)");

  //read updated parameters
  strcpy(thingspeak_server, custom_thingspeak_server.getValue());
  strcpy(thingspeak_port, custom_thingspeak_port.getValue());
  strcpy(thingspeak_token, custom_thingspeak_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    IPAddress currentIP,netMask,gateway;

    currentIP = WiFi.localIP();
    netMask = WiFi.subnetMask();
    gateway = WiFi.gatewayIP();
  
    DEBUG_PRINT("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["thingspeak_server"] = thingspeak_server;
    json["thingspeak_port"] = thingspeak_port;
    json["thingspeak_token"] = thingspeak_token;
    json["IP"] = currentIP.toString();
    json["NetMask"] = netMask.toString();
    json["Gateway"] = gateway.toString();

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      DEBUG_PRINT("failed to open config file for writing");
      return 0;
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  DEBUG_PRINT("local ip");
  DEBUG_PRINT(WiFi.localIP());
  return 1;

}
