// Do not remove the include below
#include "config.h"
#include "main.h"
#include "utils.h"


#ifdef USING_ECLIPSE
#include "time/TimeClient.h"
#include "displayMatrix/DisplayMatrix.h"
#include "ZigbeeUART/ZigbeeUART.h"
#include "frameMsg/FrameMsg.h"
#else
#include "TimeClient.h"
#include "DisplayMatrix.h"
#include "ZigbeeUART.h"
#include "FrameMsg.h"
#endif


#ifdef ZIGBEE_USE_SOFTWARE_SERIAL
#include "SoftwareSerial.h"
SoftwareSerial zUart(RX_PIN,TX_PIN); //RX,TX
ZigBee myZigbee(zUart,57600);
#else
ZigBee myZigbee(Serial,57600);
#endif

TimeClient timeClient;
DisplayMatrix myDisplay;

uint8_t zigbee_buff[ZIGBEE_BUFF_SIZE];

void timerCallback(void){
#if DEBUG
//  PRINTS("-> timerCallback() ");
#endif
  myDisplay.timeTick(timeClient);

}

void displayCallback(void)
{
#if DEBUG
//	PRINTS("\n-> displayCallback() ");
#endif

}

void zigBeeCallback(unsigned int len)
{
	PRINT("\n-> zigBeeCallback() :", len);
	for(int i =0; i < len ; ++i ){
		PRINTX("\n%X:: ",zigbee_buff[i]);
	}

	uint8_t cmd_type = zigbee_buff[CMD_TYPE];
//	uint8_t data_len = zigbee_buff[DATA_LEN];
	uint8_t data_len = len - 2;//for testing always = len -6; 2 bytes len and 4 bytes CRC
	switch (cmd_type){
		case CMD_TYPE_BYTE:
			vProcessFrmMsg(&zigbee_buff[DATA], data_len);
			break;
		case CMD_TYPE_STRING:
			break;

	}
}

void vProcessFrmMsg(uint8_t* buff ,int len){
	PRINT("\n-> vProcessMsg() :", len);
	//by pass all ID:
	uint16_t cmd = ((uint16_t)buff[CMD]<<8) + (buff[CMD+1]);

	PRINT("\ncmd :", cmd);

	switch (cmd){
		case CMD_UPDATE_TIME: //dateTime Data will be put in reserver
			frmDateTime dateTime;
			dateTime.year = buff[RESERVED];
			dateTime.month = buff[RESERVED+1];
			dateTime.dayOfmonth = buff[RESERVED+2];

			dateTime.hour = buff[RESERVED+3];
			dateTime.min = buff[RESERVED+4];
			dateTime.sec = buff[RESERVED+5];

			timeClient.updateTime(&dateTime);
			break;

		case CMD_UPDATE_DISPLAY:
			char msg[len - RESERVED + 1];
			snprintf_P(msg,
						countof(msg),
						PSTR("%s"),
						&buff[RESERVED]);
//			String msg = String(buff[RESERVED], len - RESERVED);
			PRINT("\nmsg: ", msg);
			myDisplay.vSetUpdateMsg(msg);
			break;
	}




}
//The setup function is called once at startup of the sketch

void setup()
{
// Add your initialization code here
	// initialize serial communication at 9600 bits per second:
	  Serial.begin(57600);

	  while (!Serial) {
	    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
	  }

	  PRINTS("\nBegin set up");
	  PRINTS("\n==========================================");

	  myZigbee.begin();
	  myZigbee.setCallback(zigBeeCallback);

	  myDisplay.init();
	  myDisplay.printText("  ***  ");

	  timeClient.setup();
	  timeClient.setCallback(timerCallback);

	  PRINTS("\nEnd setup");
	  PRINTS("\n==========================================");

}

// The loop function is called in an endless loop
void loop()
{
//Add your repeated code here
	timeClient.loop();
	myDisplay.runMatrixAnimation();
	myZigbee.handle(zigbee_buff);
//	PRINTS("\nmyZigbee.begin()");
//	zUart.println(">>>>>===============================");
//	delay(1000);
}
