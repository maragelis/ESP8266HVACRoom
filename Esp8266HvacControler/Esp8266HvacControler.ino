#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <FS.h>   
#include <WiFiManager.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>  



int halfPeriodicTime;
int IRpin;
int khz;
const long interval = 60000; 
unsigned long previousMillis = 0;




typedef enum HvacMode {
  HVAC_HOT,
  HVAC_COLD,
  HVAC_DRY,
  HVAC_FAN, // used for Panasonic only
  HVAC_AUTO
} HvacMode_t; // HVAC  MODE

typedef enum HvacFanMode {
  FAN_SPEED_1,
  FAN_SPEED_2,
  FAN_SPEED_3,
  FAN_SPEED_4,
  FAN_SPEED_5,
  FAN_SPEED_AUTO,
  FAN_SPEED_SILENT
} HvacFanMode_;  // HVAC  FAN MODE

typedef enum HvacVanneMode {
  VANNE_AUTO,
  VANNE_H1,
  VANNE_H2,
  VANNE_H3,
  VANNE_H4,
  VANNE_H5,
  VANNE_AUTO_MOVE
} HvacVanneMode_;  // HVAC  VANNE MODE

typedef enum HvacWideVanneMode {
  WIDE_LEFT_END,
  WIDE_LEFT,
  WIDE_MIDDLE,
  WIDE_RIGHT,
  WIDE_RIGHT_END
} HvacWideVanneMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacAreaMode {
  AREA_SWING,
  AREA_LEFT,
  AREA_AUTO,
  AREA_RIGHT
} HvacAreaMode_t;  // HVAC  WIDE VANNE MODE

typedef enum HvacProfileMode {
  NORMAL,
  QUIET,
  BOOST
} HvacProfileMode_t;  // HVAC PANASONIC OPTION MODE


            // HVAC MITSUBISHI_
#define HVAC_MITSUBISHI_HDR_MARK    3400
#define HVAC_MITSUBISHI_HDR_SPACE   1750
#define HVAC_MITSUBISHI_BIT_MARK    450
#define HVAC_MITSUBISHI_ONE_SPACE   1300
#define HVAC_MISTUBISHI_ZERO_SPACE  420
#define HVAC_MITSUBISHI_RPT_MARK    440
#define HVAC_MITSUBISHI_RPT_SPACE   17100 // Above original iremote limit
 
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);


//  Hardware Connection
//  IR LED SIGNAL => ESP/GPIO_4
//  DHT SIGNAL => ESP/GPIO_5
// #include <ESP8266WiFi.h>

const bool useHomekit = true;
const char* ssid = "SSID";
const char* password = "*********";
 char* mqtt_server = "192.168.2.230";
char*  mqtt_port = "1883";
 char* root_topicOut = "HvacAthanasia/Out";
const char* root_topicIn = "HvacAthanasia/In";
char* Homebridge_set= "homebridge/to/set";
const char* Homebridge_get = "homebridge/from/get";
char* Homebridge_add = "homebridge/to/add";


WiFiClient espClient;
PubSubClient client(espClient);
bool shouldSaveConfig = false;
int chipid = ESP.getChipId();
const String HomeKitName = "MITSUBISHI_" + chipid ;


String DeviceCreate = "{\"name\":\"" + HomeKitName + "\",\"service_name\":\"" + HomeKitName+ "\",\"service\":\"Thermostat\" }";



struct JsonPayload{
  String Topic;
  String Payload;
} ;

struct HvacPayload
{
 
  HvacMode  HVAC_Mode;
  int HVAC_Temp;
  HvacFanMode  HVAC_FanMode;
  HvacVanneMode  HVAC_VanneMode;
  int OFF;
}  ;


struct HvacPayloadSTR
{
 
  String  HVAC_Mode;
  int HVAC_Temp;
  String  HVAC_FanMode;
  String  HVAC_VanneMode;
  int OFF;
}  ;


void sendHvacMitsubishi(
    HvacMode                HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
    int                     HVAC_Temp,           // Example 21  (°c)
    HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
    HvacVanneMode           HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
    int                     OFF                  // Example false
  );

struct HvacPayloadSTR LastHvacPayloadSTR;
struct HvacPayload Hvacdata;



void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
Serial.println("Program Start");
setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  reconnect();

  IRpin=4;
  khz=38;
  halfPeriodicTime = 500/khz;
  pinMode(IRpin, OUTPUT);
   Decodejson("1");
sensors.begin(); 
SendMessage((char*)DeviceCreate.c_str(), Homebridge_add);
delay(5000);
InitHavcData();
sendHvacMitsubishiData();
}


void loop() {
  
  unsigned long currentMillis = millis();
  //Serial.println(timestamp);

    if (!client.connected()) {
      reconnect();
    }
    client.loop();

if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
// call sensors.requestTemperatures() to issue a global temperature 
 // request to all devices on the bus 
/********************************************************************/
 Serial.print(" Requesting temperatures..."); 
 sensors.requestTemperatures(); // Send the command to get temperature readings 
 Serial.println("DONE"); 
/********************************************************************/
 Serial.print("Temperature is: "); 
 float temp = sensors.getTempCByIndex(0);
 Serial.println(temp); 

   SendMessage(temp,"temperature");
}
}

void InitHavcData()
{
  Hvacdata={HVAC_AUTO, 28, FAN_SPEED_AUTO, VANNE_AUTO, 1 };
}

void sendHvacMitsubishiData()
{
 sendHvacMitsubishi(Hvacdata.HVAC_Mode, Hvacdata.HVAC_Temp, Hvacdata.HVAC_FanMode, Hvacdata.HVAC_VanneMode, Hvacdata.OFF);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String Payload;
  for (int i = 0; i < length; i++) {
    Payload += (char)payload[i];
    
  }
  Serial.print("Payload=");
  Serial.println(Payload);

  if (useHomekit)
  {
    DecodejsonHomekit((char*)Payload.c_str());
    sendHvacMitsubishiData();
    
  }else
  {
    HvacPayload datax = Decodejson((char*)Payload.c_str());
    sendHvacMitsubishi(datax.HVAC_Mode, datax.HVAC_Temp, datax.HVAC_FanMode, datax.HVAC_VanneMode, datax.OFF);
  }

 
  
}

struct HvacPayload DecodejsonHomekit(char* Payload) 
{
DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(Payload);
   Serial.print("parseObject=");
   Serial.println(root.success());
  if (!root.success()) {
  }else
  {
    if (root["Characteristic"] == "TargetHeatingCoolingState")
    {
      if (root["value"] =="0")
      {
        Hvacdata.OFF = 1;
        SendHomeKit();
      }else
      {
        Hvacdata.OFF = 0;
        Hvacdata.HVAC_Mode = getHK_HvacMode(root["value"]);
        SendHomeKit();
      }
    }
    if (root["Characteristic"] == "TargetTemperature")
    {
      Hvacdata.HVAC_Temp == root["value"];
      

    }
  }
}


 
struct HvacPayload Decodejson(char* Payload) 
{
 
 HvacPayload data;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(Payload);
   Serial.print("parseObject=");
   Serial.println(root.success());
  if (!root.success()) {
    sendHvacMitsubishi(HVAC_AUTO, 26, FAN_SPEED_AUTO, VANNE_AUTO, true);
     LastHvacPayloadSTR = {"HVAC_AUTO",26, "FAN_SPEED_AUTO", "VANNE_AUTO", true};
    Serial.println("JSON parsing failed!");
    return data;
  } else
 {

Serial.print("Status=");

  if (root["Status"]==true)
  {
    Serial.println("Sending Status Message");
    SendMessage(LastHvacPayloadSTR,"Status");
  }else
{
  Serial.println("NO Status Message received");

  
    //HvacMode,HvacFanMode,HvacVanneMode
  String HVAC_Mode =root["Payload"]["HVAC_Mode"];
   Serial.print("HVAC_Mode=");
   Serial.println(HVAC_Mode);

   int HVAC_Temp = root["Payload"]["HVAC_Temp"];
   Serial.print("HVAC_Temp=");
   Serial.println(HVAC_Temp);

   String HVAC_FanMode = root["Payload"]["HVAC_FanMode"];
   Serial.print("HVAC_FanMode=");
   Serial.println(HVAC_FanMode);

   String HVAC_VanneMode = root["Payload"]["HVAC_VanneMode"];
   Serial.print("HVAC_VanneMode=");
   Serial.println(HVAC_VanneMode);

   int OFF = root["Payload"]["OFF"];
   Serial.print("OFF=");
   Serial.println(OFF);
  ///HvacMode,HvacFanMode,HvacVanneMode

HvacPayload Hvacdata;
   //get_HvacMode(HVAC_Mode);
    Hvacdata = {get_HvacMode(HVAC_Mode),HVAC_Temp,get_HvacFanMode(HVAC_FanMode),get_HvacVanneMode(HVAC_VanneMode),OFF};
    LastHvacPayloadSTR={HVAC_Mode,HVAC_Temp,HVAC_FanMode,HVAC_VanneMode,OFF};

    return Hvacdata;
    }
   
    
 }
    
  
  return data;
}

void SendHomeKit()
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  root["name"] = HomeKitName;
  root["service_name"] = HomeKitName;
  root["characteristic"] = "CurrentHeatingCoolingState";
  root["value"] = getHK_HvacModevalue(Hvacdata.HVAC_Mode, Hvacdata.OFF );
  
  char buffer[256];
        root.printTo(buffer, sizeof(buffer));
  client.publish(Homebridge_set,buffer );
}

void SendHomeKitTemp()
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  root["name"] = HomeKitName;
  root["service_name"] = HomeKitName;
  root["characteristic"] = "CurrentTemperature";
  root["value"] = Hvacdata.HVAC_Temp;
  
  char buffer[256];
        root.printTo(buffer, sizeof(buffer));
  client.publish(Homebridge_set,buffer );
}

void SendMessage(HvacPayloadSTR Message, char* Topic)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["HVAC_Mode"] = Message.HVAC_Mode;
  root["HVAC_Temp"] = Message.HVAC_Temp;
  root["HVAC_FanMode"] = Message.HVAC_FanMode;
  root["HVAC_VanneMode"] = Message.HVAC_VanneMode;
  root["OFF"] = Message.OFF;
   root["TOPIC"] = Topic;
  char buffer[256];
        root.printTo(buffer, sizeof(buffer));
  client.publish(root_topicOut,buffer );
}

String ChangetoChar(float value)
{
  char buf[10];
dtostrf(value, 0,3, buf);
  return buf;
}

String MakeJsonReturn(String Message, char* Topic)
{
   StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["TOPIC"] = Topic;
  root["Message"] = Message;

  char buffer[256];
        root.printTo(buffer, sizeof(buffer));
        return buffer;
}


void SendMessage(float Message, char* topic )
{
  String retval = MakeJsonReturn (ChangetoChar(Message).c_str(), topic);
  client.publish(root_topicOut, retval.c_str());
}




void SendMessage(char* Message, char* Topic )
{
   String retval =  MakeJsonReturn(Message, Topic) ;
  client.publish(root_topicOut,retval.c_str());
}

void SendMessage(char* Message)
{
  
  client.publish(root_topicOut, Message);
}





  
