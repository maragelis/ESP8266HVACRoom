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

const char* ssid = "SSID";
const char* password = "*********";
 char* mqtt_server = "192.168.2.230";
char*  mqtt_port = "1883";
 char* root_topicOut = "HvacAthanasia/Out";
const char* root_topicIn = "HvacAthanasia/In";


WiFiClient espClient;
PubSubClient client(espClient);
bool shouldSaveConfig = false;



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


HvacPayloadSTR LastHvacPayloadSTR;


void sendHvacMitsubishi(
    HvacMode                HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
    int                     HVAC_Temp,           // Example 21  (°c)
    HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
    HvacVanneMode           HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
    int                     OFF                  // Example false
  );


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
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
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
 HvacPayload datax = Decodejson((char*)Payload.c_str());
 //HvacMode,HvacFanMode,HvacVanneMode
  sendHvacMitsubishi(datax.HVAC_Mode, datax.HVAC_Temp, datax.HVAC_FanMode, datax.HVAC_VanneMode, datax.OFF);
   /*;
    *   HvacMode  HVAC_Mode;
  int HVAC_Temp;
  HvacFanMode  HVAC_FanMode;
  HvacVanneMode  HVAC_VanneMode;
  int OFF;
    */
    
  
}


 HvacFanMode get_HvacFanMode(String HVAC_Modestr)
{

    if (HVAC_Modestr == "FAN_SPEED_1")
           { return FAN_SPEED_1;}
            
         else if (HVAC_Modestr == "FAN_SPEED_2")
            { return FAN_SPEED_2;}
            
         else if (HVAC_Modestr == "FAN_SPEED_3")
            { return FAN_SPEED_3;}
            
       else if (HVAC_Modestr == "FAN_SPEED_4")
           {  return FAN_SPEED_4;           }
 else if (HVAC_Modestr == "FAN_SPEED_5")
           {  return FAN_SPEED_5;           }
 else if (HVAC_Modestr == "FAN_SPEED_AUTO")
           {  return FAN_SPEED_AUTO;           }
            else if (HVAC_Modestr == "FAN_SPEED_SILENT")
           {  return FAN_SPEED_SILENT;           }

            
           return FAN_SPEED_AUTO;  
    
}


HvacVanneMode get_HvacVanneMode(String HVAC_Modestr)
{

  /*
   * HvacVanneMode {
  VANNE_AUTO,
  VANNE_H1,
  VANNE_H2,
  VANNE_H3,
  VANNE_H4,
  VANNE_H5,
  VANNE_AUTO_MOVE
   */
    if (HVAC_Modestr == "VANNE_H1")
           { return VANNE_H1;}
            
         else if (HVAC_Modestr == "VANNE_H2")
            { return VANNE_H2;}
            
         else if (HVAC_Modestr == "VANNE_H3")
            { return VANNE_H3;}
            
       else if (HVAC_Modestr == "VANNE_H4")
           {  return VANNE_H4;           }
 else if (HVAC_Modestr == "VANNE_H5")
           {  return VANNE_H5;           }
 else if (HVAC_Modestr == "VANNE_AUTO_MOVE")
           {  return VANNE_AUTO_MOVE;           }

           return VANNE_AUTO_MOVE;  
    
}

/*
 * 
 * HvacMode {
  HVAC_HOT,
  HVAC_COLD,
  HVAC_DRY,
  HVAC_FAN, // used for Panasonic only
  HVAC_AUTO
 */
HvacMode get_HvacMode(String HVAC_Modestr)
{
    if (HVAC_Modestr == "HVAC_HOT")
           { return HVAC_HOT;}

         else  if (HVAC_Modestr == "HVAC_COLD")
           { return HVAC_COLD;}

        
            
         else if (HVAC_Modestr == "HVAC_DRY")
            { return HVAC_DRY;}
            
         else if (HVAC_Modestr == "HVAC_FAN")
            { return HVAC_FAN;}
            
       else if (HVAC_Modestr == "HVAC_AUTO")
           {  return HVAC_AUTO;           }

           return HVAC_AUTO;  
    
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
    SendMessage(LastHvacPayloadSTR);
  }else
{
  Serial.println("NO Status Message received");
}
  
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
    
  
  return data;
}



void SendMessage(HvacPayloadSTR Message)
{
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["HVAC_Mode"] = Message.HVAC_Mode;
  root["HVAC_Temp"] = Message.HVAC_Temp;
  root["HVAC_FanMode"] = Message.HVAC_FanMode;
  root["HVAC_VanneMode"] = Message.HVAC_VanneMode;
  root["OFF"] = Message.OFF;
  char buffer[256];
        root.printTo(buffer, sizeof(buffer));
  client.publish(root_topicOut,buffer );
}


void SendMessage(float Message)
{
   char buf[10];
dtostrf(Message, 0,3, buf);
  client.publish(root_topicOut, buf);
}



void SendMessage(char* Message)
{
  
  client.publish(root_topicOut, Message);
}

void mountfs()
{
   if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}


void setup_wifi(){


  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(180);
  
  wifiManager.addParameter(&custom_mqtt_server);
  
  wifiManager.autoConnect("NextionAP");


strcpy(mqtt_server, custom_mqtt_server.getValue());
  


  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
 
  
  Serial.println(custom_mqtt_server.getValue());
  
  client.setServer( mqtt_server, 1883);
  
  client.setCallback(callback);
  reconnect();

  
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  

}

  void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(root_topicOut, "Connected!");
      // ... and resubscribe
      client.subscribe(root_topicIn);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//HvacMode,HvacFanMode,HvacVanneMode

            /****************************************************************************
            /* Send IR command to Mitsubishi HVAC - sendHvacMitsubishi
            /***************************************************************************/
  void sendHvacMitsubishi(
    HvacMode                HVAC_Mode,           // Example HVAC_HOT  HvacMitsubishiMode
    int                     HVAC_Temp,           // Example 21  (°c)
    HvacFanMode             HVAC_FanMode,        // Example FAN_SPEED_AUTO  HvacMitsubishiFanMode
    HvacVanneMode           HVAC_VanneMode,      // Example VANNE_AUTO_MOVE  HvacMitsubishiVanneMode
    int                     OFF                  // Example false
  )
  {

    //#define  HVAC_MITSUBISHI_DEBUG;  // Un comment to access DEBUG information through Serial Interface

    byte mask = 1; //our bitmask
    byte data[18] = { 0x23, 0xCB, 0x26, 0x01, 0x00, 0x20, 0x08, 0x06, 0x30, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F };
    // data array is a valid trame, only byte to be chnaged will be updated.

    byte i;

#ifdef HVAC_MITSUBISHI_DEBUG
    Serial.println("Packet to send: ");
    for (i = 0; i < 18; i++) {
      Serial.print("_");
      Serial.print(data[i], HEX);
    }
    Serial.println(".");
#endif

    // Byte 6 - On / Off
    if (OFF) {
      data[5] = (byte)0x0; // Turn OFF HVAC
    }
    else {
      data[5] = (byte)0x20; // Tuen ON HVAC
    }

    // Byte 7 - Mode
    switch (HVAC_Mode)
    {
    case HVAC_HOT:   data[6] = (byte)0x08; break;
    case HVAC_COLD:  data[6] = (byte)0x18; break;
    case HVAC_DRY:   data[6] = (byte)0x10; break;
    case HVAC_AUTO:  data[6] = (byte)0x20; break;
    default: break;
    }

    // Byte 8 - Temperature
    // Check Min Max For Hot Mode
    byte Temp;
    if (HVAC_Temp > 31) { Temp = 31; }
    else if (HVAC_Temp < 16) { Temp = 16; }
    else { Temp = HVAC_Temp; };
    data[7] = (byte)Temp - 16;

    // Byte 10 - FAN / VANNE
    switch (HVAC_FanMode)
    {
    case FAN_SPEED_1:       data[9] = (byte)B00000001; break;
    case FAN_SPEED_2:       data[9] = (byte)B00000010; break;
    case FAN_SPEED_3:       data[9] = (byte)B00000011; break;
    case FAN_SPEED_4:       data[9] = (byte)B00000100; break;
    case FAN_SPEED_5:       data[9] = (byte)B00000100; break; //No FAN speed 5 for MITSUBISHI so it is consider as Speed 4
    case FAN_SPEED_AUTO:    data[9] = (byte)B10000000; break;
    case FAN_SPEED_SILENT:  data[9] = (byte)B00000101; break;
    default: break;
    }

    switch (HVAC_VanneMode)
    {
    case VANNE_AUTO:        data[9] = (byte)data[9] | B01000000; break;
    case VANNE_H1:          data[9] = (byte)data[9] | B01001000; break;
    case VANNE_H2:          data[9] = (byte)data[9] | B01010000; break;
    case VANNE_H3:          data[9] = (byte)data[9] | B01011000; break;
    case VANNE_H4:          data[9] = (byte)data[9] | B01100000; break;
    case VANNE_H5:          data[9] = (byte)data[9] | B01101000; break;
    case VANNE_AUTO_MOVE:   data[9] = (byte)data[9] | B01111000; break;
    default: break;
    }

    // Byte 18 - CRC
    data[17] = 0;
    for (i = 0; i < 17; i++) {
      data[17] = (byte)data[i] + data[17];  // CRC is a simple bits addition
    }

#ifdef HVAC_MITSUBISHI_DEBUG
    Serial.println("Packet to send: ");
    for (i = 0; i < 18; i++) {
      Serial.print("_"); Serial.print(data[i], HEX);
    }
    Serial.println(".");
    for (i = 0; i < 18; i++) {
      Serial.print(data[i], BIN); Serial.print(" ");
    }
    Serial.println(".");
#endif

    enableIROut(38);  // 38khz
    space(0);
    for (int j = 0; j < 2; j++) {  // For Mitsubishi IR protocol we have to send two time the packet data
                   // Header for the Packet
      mark(HVAC_MITSUBISHI_HDR_MARK);
      space(HVAC_MITSUBISHI_HDR_SPACE);
      for (i = 0; i < 18; i++) {
        // Send all Bits from Byte Data in Reverse Order
        for (mask = 00000001; mask > 0; mask <<= 1) { //iterate through bit mask
          if (data[i] & mask) { // Bit ONE
            mark(HVAC_MITSUBISHI_BIT_MARK);
            space(HVAC_MITSUBISHI_ONE_SPACE);
          }
          else { // Bit ZERO
            mark(HVAC_MITSUBISHI_BIT_MARK);
            space(HVAC_MISTUBISHI_ZERO_SPACE);
          }
          //Next bits
        }
      }
      // End of Packet and retransmission of the Packet
      mark(HVAC_MITSUBISHI_RPT_MARK);
      space(HVAC_MITSUBISHI_RPT_SPACE);
      space(0); // Just to be sure
    }
  }

  /****************************************************************************
  /* enableIROut : Set global Variable for Frequency IR Emission
  /***************************************************************************/
  void enableIROut(int khz) {
    // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
    halfPeriodicTime = 500 / khz; // T = 1/f but we need T/2 in microsecond and f is in kHz
  }

  /****************************************************************************
  /* mark ( int time)
  /***************************************************************************/
  void mark(int time) {
    // Sends an IR mark for the specified number of microseconds.
    // The mark output is modulated at the PWM frequency.
    long beginning = micros();
    while (micros() - beginning < time) {
      digitalWrite(IRpin, HIGH);
      delayMicroseconds(halfPeriodicTime);
      digitalWrite(IRpin, LOW);
      delayMicroseconds(halfPeriodicTime); //38 kHz -> T = 26.31 microsec (periodic time), half of it is 13
    }
  }

  /****************************************************************************
  /* space ( int time)
  /***************************************************************************/
  /* Leave pin off for time (given in microseconds) */
  void space(int time) {
    // Sends an IR space for the specified number of microseconds.
    // A space is no output, so the PWM output is disabled.
    digitalWrite(IRpin, LOW);
    if (time > 0) delayMicroseconds(time);
  }

  /****************************************************************************
  /* sendRaw (unsigned int buf[], int len, int hz)
  /***************************************************************************/
  void sendRaw(unsigned int buf[], int len, int hz)
  {
    enableIROut(hz);
    for (int i = 0; i < len; i++) {
      if (i & 1) {
        space(buf[i]);
      }
      else {
        mark(buf[i]);
      }
    }
    space(0); // Just to be sure
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
 Serial.print(temp); // Why "byIndex"?  
   // You can have more than one DS18B20 on the same bus.  
   // 0 refers to the first IC on the wire 
   //delay(1000); 

   SendMessage(temp);
}
   
  }
