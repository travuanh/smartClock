// Do not remove the include below
#include "smartClock_v2.h"



#include <ESP8266WiFi.h>
#include "config.h"
#include "utils.h"
#include <WiFiManager.h>

//for LED status
#include <Ticker.h>
Ticker ticker;

#ifdef USING_ECLIPSE
#include "time/TimeClient.h"
#include "displayMatrix/DisplayMatrix.h"
#else
#include "TimeClient.h"
#include "DisplayMatrix.h"
#endif



// WiFi login parameters - network name and password
const char* ssid = "";                   // edit your wifi SSID here
const char* password = "";            // edit your wifi password here

String devID;

// WiFi Server object and parameters
//WiFiServer server(80);
//WiFiClient espClient;

// Define NTP Client to get time
WiFiUDP ntpUDP;
TimeClient timeClient;
DisplayMatrix myDisplay;

bool updateTimeClient = false;
bool secondTick = false;


uint32_t timeConnectWifi = 0;

void tick()
{
  //toggle state
  digitalWrite(HB_LED, !digitalRead(HB_LED));     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void setup_wifi() {
  delay(10);
  Serial.println();
/***************************************************************************************************/

  // put your setup code here, to run once:
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(MAX_TIME_TO_CONFIG_WIFI);//180

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
//  wifiManager.autoConnect(MYESP);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect(MYESP)) {
    Serial.println("failed to connect and hit timeout");
//    delay(3000);
    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
    delay(5000);
  }
  ticker.detach();
//    //keep LED on
//  digitalWrite(LED, LOW);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  devID = "myClock_" + WiFi.macAddress();
  Serial.println(devID);
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
}

void timerCallback(){
#if DEBUG
//  Serial.println("-> timerCallback() ");
#endif
  secondTick = true;
  timeConnectWifi++;
  if(timeConnectWifi > MAX_TIME_TO_CONNECT_WIFI){
    timeConnectWifi = 0;
    if (WiFi.isConnected()){
      WiFi.disconnect();
    }
  }

  myDisplay.timeTick(timeClient);


}
void displayCallback()
{
#if DEBUG
//  Serial.println("-> displayCallback() ");
#endif


}
void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

#if DEBUG
  Serial.begin(115200);
  PRINTS("\n[MD_MAX72XX WiFi Message Display]\nType a message for the scrolling display from your internet browser");
#endif

#if LED_HEARTBEAT
  pinMode(HB_LED, OUTPUT);
  digitalWrite(HB_LED, LOW);
#endif

  delay(10);

#if DEBUG
  Serial.println("-> DEBUG MODE!!!");
#endif

  ticker.attach(0.6, tick);

  myDisplay.init();
  myDisplay.setCallback(displayCallback);

  myDisplay.printText("  ***  ");
  setup_wifi();

  // Connect to and initialize WiFi network
  PRINT("\nConnecting to ", ssid);

  timeClient.setup(ntpUDP);
  timeClient.setCallback(timerCallback);


  myDisplay.printText("");
  // Start the server
//  server.begin();


}
void loop()
{
  if(WiFi.isConnected()){
  #if LED_HEARTBEAT
//    static uint32_t timeLast = 0;
//
//    if (millis() - timeLast >= HB_LED_TIME) {
//      digitalWrite(HB_LED, digitalRead(HB_LED) == LOW ? HIGH : LOW);
//      timeLast = millis();
//    }
    digitalWrite(HB_LED, LOW);
  #endif
//    handleWiFi();
  }
  else{
    digitalWrite(HB_LED, HIGH);
  }

  timeClient.loop();
  myDisplay.runMatrixAnimation();
//  myDisplay.printText(0, MAX_DEVICES -1 , "  ***  ");

}

