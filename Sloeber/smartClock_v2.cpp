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
#include "configuration/MyConfig.h"
#else
#include "TimeClient.h"
#include "DisplayMatrix.h"
#include "MyConfig.h"
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

MyConfig myConfig;

bool updateTimeClient = false;
bool secondTick = false;


uint32_t timeConnectWifi = 0;

WiFiManager *wm ;


//flag for saving data
bool shouldSaveConfig = false;
uint8_t tickTime = 0;

void tick()
{
  //toggle state
  myDisplay.runWelcomeDisplay();
  tickTime++;
  if(tickTime>2){
	  digitalWrite(HB_LED, !digitalRead(HB_LED));     // set pin to the opposite state
	  tickTime = 0;
  }
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
	PRINTS("\n-> Entered config mode");
	Serial.println(WiFi.softAPIP());
	//if you used auto generated SSID, print it
	PRINT("\n ConfigPortalSSID:",myWiFiManager->getConfigPortalSSID());
	//entered config mode, make led toggle faster
	ticker.attach(0.1, tick);
}

String getParam(String name){
  //read parameter from server, for customhmtl input
  String value;

  if(wm->server->hasArg(name)) {
    value = wm->server->arg(name);
  }
  return value;
}
//
void saveParamCallback(){
  PRINTS("\n [CALLBACK] saveParamCallback fired");
  shouldSaveConfig = true;

  fConfig* fPtr = myConfig.getConfig();
  fPtr->dateformat = getParam("datefrm").toInt();
  fPtr->mode24h = getParam("clockmode").toInt();
  PRINT("\n PARAM clockmode = " , fPtr->mode24h);
  PRINT("\n PARAM datefrm = " , fPtr->dateformat);
  PRINTS("\n");
}

void setup_wifi() {
	delay(10);
	PRINTS("\n->setup_wifi()");
/***************************************************************************************************/

	// put your setup code here, to run once:
	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
//	WiFiManager wifiManager;



	//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
//	wifiManager.setAPCallback(configModeCallback);
	wm = new WiFiManager;
	//reset saved settings
//	wifiManager.resetSettings();
//	wm->resetSettings();
	wm->setAPCallback(configModeCallback);
	//set custom ip for portal
	//wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

	//sets timeout until configuration portal gets turned off
	//useful to make it all retry or go to sleep
	//in seconds
//	wifiManager.setTimeout(MAX_TIME_TO_CONFIG_WIFI);//180
//	wifiManager.setConfigPortalTimeout(MAX_TIME_TO_CONFIG_WIFI); // auto close configportal after n seconds
	wm->setConfigPortalTimeout(MAX_TIME_TO_CONFIG_WIFI);
//	wm->setTimeout(30);

	// add a custom input field
//	int customFieldLength = 40;


	// new (&custom_field) WiFiManagerParameter("customfieldid", "Custom Field Label", "Custom Field Value", customFieldLength,"placeholder=\"Custom Field Placeholder\"");

	// test custom html input type(checkbox)
	WiFiManagerParameter clockMode;
	const char* custom_clockmode_str = "<br/><label for='clockmode'>Clock Mode : </label> <br>"
											"<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' name='clockmode' value='0' checked> 24H <br>"
											"<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' name='clockmode' value='1'> 12H <br>";
	new (&clockMode) WiFiManagerParameter(custom_clockmode_str); // custom html input
//	new (&clockMode) WiFiManagerParameter("clock_mode", "Clock Mode 24H:", "Clock Mode Value", customFieldLength,"placeholder=\"Custom Field Placeholder\" type=\"checkbox\""); // custom html type


	// test custom html(radio)
	WiFiManagerParameter dateFmt;
	const char* custom_datefrm_str = "<br/><label for='datefrm'>Date Format: </label> <br>"
										"<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' name='datefrm' value='0' checked> MM/DD/YYYY <br>"
										"<input style='width: auto; margin: 0 10px 10px 10px;' type='radio' name='datefrm' value='1'> DD/MM/YYYY <br>";

	new (&dateFmt) WiFiManagerParameter(custom_datefrm_str); // custom html input

//	wifiManager.addParameter(&clockMode);
//	wifiManager.addParameter(&dateFmt);
//	wifiManager.setSaveParamsCallback(saveParamCallback);
	wm->addParameter(&clockMode);
	wm->addParameter(&dateFmt);
	wm->setSaveParamsCallback(saveParamCallback);

	// custom menu via array or vector
	//
	// menu tokens, "wifi","wifinoscan","info","param","close","sep","erase","restart","exit" (sep is seperator) (if param is in menu, params will not show up in wifi page!)
	// const char* menu[] = {"wifi","info","param","sep","restart","exit"};
	// wm.setMenu(menu,6);
	std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
//	wifiManager.setMenu(menu);
	wm->setMenu(menu);

	// set dark theme
//	wifiManager.setClass("invert");
	wm->setClass("invert");
//	bool isSuccessed = wifiManager.autoConnect(MYESP);
	bool isSuccessed = wm->autoConnect(MYESP);



  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
//  wifiManager.autoConnect(MYESP);
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!isSuccessed) {
    Serial.println("failed to connect and hit timeout");
//    delay(3000);
    //reset and try again, or maybe put it to deep sleep
//    ESP.reset();
    delay(5000);
  }
  ticker.detach();

//    //keep LED on
//  digitalWrite(LED, LOW);

  //save the custom parameters to FS
  if (shouldSaveConfig) {
	  PRINTS("\n SaveConfig.");
//	  shouldSaveConfig = false;
	  myConfig.bSaveConfig();

  }

  PRINTS("\n	WiFi connected");
  PRINT("\n		IP address: ", WiFi.localIP());
  devID = "myClock_" + WiFi.macAddress();
  PRINT("\n		devID:", devID);
  PRINTS("\n");


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
  Serial.begin(115200);

#if LED_HEARTBEAT
  pinMode(HB_LED, OUTPUT);
  digitalWrite(HB_LED, LOW);
#endif

  delay(10);

#if DEBUG
  PRINTS("\n-> DEBUG MODE!!!");
#endif

//  ticker.attach(0.6, tick);
  myDisplay.init();
  myDisplay.setCallback(displayCallback);

  myDisplay.printText("  ***  ");
  setup_wifi();

  myConfig.init();

  // Connect to and initialize WiFi network
  PRINT("\nConnecting to ", ssid);
  myConfig.bLoadConfig();

  timeClient.setfConfig(myConfig.getConfig());
  timeClient.setup(ntpUDP);
  timeClient.setCallback(timerCallback);



  myDisplay.printText("");
  // Start the server
//  server.begin();
  free(wm);

}
void loop()
{
  if(WiFi.isConnected()){
	  #if LED_HEARTBEAT
		LED_ON;
	  #endif
	//    handleWiFi();
  }
  else{
    LED_OFF;
  }
  timeClient.loop();
  myDisplay.runMatrixAnimation();

}

