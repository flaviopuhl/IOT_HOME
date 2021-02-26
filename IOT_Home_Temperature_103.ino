
/*
* PROGRAM :        IOT_Home_Temperature             
* 
* DESCRIPTION :
*       Temperature monitor 
*
* NOTES :
*       DS18B20 ..................[100]
*       LED RGB diagnostics.......[100]
*       Thinger.io................[100]
*       OTA.......................[100]
*       UDP time..................[101]
*       VCC monitoring ...........[104]
*        
* HARDWARE:       
*       DS18B20 ......... D1/20/GPIO5/SCL
*       LED Green ....... D5/5/GPIO14/HSCLK
*       LED Red ......... D6/6/GPIO12/HMISO
*       LED Blue ........ D7/7/GPIO13/CTS0/HMOSI
*       
* AUTHOR :    Flávio Puhl        START DATE :    Dez20
*
* Description :
* Temperature measured using DS18B20;
* LED RGB gives diagnostic
* Data is saved in Thinger.io (url)
* UDP time from web to show last update
* 
* Changes
* [102] - Bug fix and cleanup
* 
* Links
* OneWire Library: https://github.com/PaulStoffregen/OneWire
* DallasTemperature Library: https://github.com/milesburton/Arduino-Temperature-Control-Library
*/

/************************************************************************
* Libraries
************************************************************************/ 

#include <OneWire.h>            // DS18B20
#include <DallasTemperature.h>  // Including the library of DS18B20
//  WiFi
#include <ESP8266WiFi.h>        //Wifi
//  THINGER.IO 
#include <ThingerESP8266.h>     //Thinger
//  OTA
#include <ESP8266mDNS.h>        //OTA
#include <WiFiUdp.h>            //OTA
#include <ArduinoOTA.h>         //OTA
#include <NTPClient.h>          //NTP

/************************************************************************
* Define pinout and constants
************************************************************************/ 

OneWire oneWire(5);     // DS18B20 on nodemcu D1 
DallasTemperature DS18B20(&oneWire);

int LedPinG = 14;        //D5 Green
int LedPinR = 12;        //D6 Red
int LedPinB = 13;        //D7 Blue

int redeRSSI = 99;

const long utcOffsetInSeconds = -10800; //Timezone
String formattedDate = "default";

ADC_MODE(ADC_VCC);      //Battery monitoring
int VCCSupply;

/************************************************************************
* Thinger.io connection parameters 
************************************************************************/ 
#define user "fpuhl"
#define device_Id "Node"
#define device_credentials "Hu$tZIOuj56r"
ThingerESP8266 thing(user, device_Id, device_credentials);

/************************************************************************
* WiFi connection parameters and constants
************************************************************************/ 
const char WiFi_ssid[]="CasaDoTheodoro";      //WiFi SSID
const char WiFi_password[]="09012011";        //WiFi password

/************************************************************************
* Define NTP client to get time
************************************************************************/
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
NTPClient timeClient(ntpUDP, "br.pool.ntp.org", utcOffsetInSeconds);

void setup() {

/************************************************************************
* Initialize functions
************************************************************************/     
    
    Serial.begin(9600);
      Serial.println("Serial communication @ 9600bpps");
      Serial.println("Software version: 102");
    DS18B20.begin();
      Serial.println("DS18B20 started");
    timeClient.begin();
      Serial.println("timeClient UDP started");
      
    pinMode(LedPinR, OUTPUT);
    pinMode(LedPinG, OUTPUT);
    pinMode(LedPinB, OUTPUT);

    digitalWrite(LedPinR, HIGH);
    digitalWrite(LedPinG, HIGH);
    digitalWrite(LedPinB, HIGH);
    
/************************************************************************
* Setup WiFi
************************************************************************/ 
    Serial.println("WiFi connecting...");
    WiFi.mode(WIFI_STA);
    thing.add_wifi(WiFi_ssid, WiFi_password);
      while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      digitalWrite(LedPinR, LOW);
      Serial.println("Connection Failed! Rebooting...");
      delay(3000);
      ESP.restart();}

/************************************************************************
* OTA Functions - do not remove
************************************************************************/ 
    ArduinoOTA.onStart([]() {
    digitalWrite(LedPinG, LOW);
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    digitalWrite(LedPinR, LOW);
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  digitalWrite(LedPinG, HIGH);
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());


/************************************************************************
* Data Acquisition
************************************************************************/ 

  
/************************************************************************
* Thinger definitions
************************************************************************/   
 
  thing["data"] >> [](pson& out){
    // Add the values and the corresponding code
    out["t_DS18B20_Celsius"] = DS18B20.getTempCByIndex(0);
    out["r_RSSI_dB"] = WiFi.RSSI();
    out["lastUpdate"] = timeClient.getFormattedDate();
    out["VCC_Supply"] = ESP.getVcc();
  };
  
} //End setup

void loop() {
/************************************************************************
* Get Date&Time
************************************************************************/
timeClient.update();

String formattedDate = timeClient.getFormattedDate(); //2020-03-27T15:09:40Z

/************************************************************************
* Data Acquisition
************************************************************************/ 
  DS18B20.requestTemperatures();
  redeRSSI = WiFi.RSSI();
  VCCSupply = ESP.getVcc();   // output the battery value

/************************************************************************
* Serial Debugg
************************************************************************/ 
   
    Serial.print("DS18B20 temperatura = [");
      Serial.print(DS18B20.getTempCByIndex(0)); 
        Serial.print("C]");
    Serial.println("");
    Serial.print("WiFi RSSI = [");
      Serial.print(redeRSSI); 
        Serial.print("dB]");
    Serial.println("");
        Serial.print("Last Update = [");
      Serial.print(formattedDate); 
        Serial.print("]");
    Serial.println("");
        Serial.print("VCCSupply = [");
      Serial.print(VCCSupply); 
        Serial.print("]");
    Serial.println("");
    Serial.println("");
    
/************************************************************************
* OTA & Thinger handles - do not remove
************************************************************************/ 
 ArduinoOTA.handle();
 thing.handle();


/************************************************************************
* Loop time
************************************************************************/ 
  digitalWrite(LedPinR, HIGH);
  digitalWrite(LedPinB, LOW);
   delay(100);
  digitalWrite(LedPinB, HIGH);
   delay(15000);
   
}
