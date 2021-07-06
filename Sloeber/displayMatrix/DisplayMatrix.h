#ifndef _DISPLAYMATRIX_H_
#define _DISPLAYMATRIX_H_

//
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include "../config.h"

#ifdef USING_ECLIPSE
#include "../time/TimeClient.h"
#else
#include "TimeClient.h"
#endif

#define MAX_DEVICES 4

#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

#define MAX_DISPLAY_TIME  45//45 // secs

#define MAX_DISPLAY_DATE  5 //secs
#define MAX_DISPLAY_TEMP  5 //secs
#define MAX_DISPLAY_UPDATE  5 //secs
#define MAX_DISPLAY_ALARM  5 //secs
#define MAX_DISPLAY_IDLE  0 //secs

#define MAX_DISPLAY_DELAY 0 //secs

#define MAX_TIME_TO_CONNECT_WIFI 15*60 //15 mins

#define MAX_TIME_TO_CONFIG_WIFI 180 //180 secs


// Global message buffers shared by Wifi and Scrolling functions
#define MESG_SIZE  75
#define CHAR_SPACING  1
#define SCROLL_DELAY  75
#define TIME_BLINK  500
#define MAX_LINE_MSG 5

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define DISPLAY_CALLBACK_SIGNATURE std::function<void()> callback
#else
#define DISPLAY_CALLBACK_SIGNATURE void (*callback)()
#endif


class DisplayMatrix{
private:
  bool blink;
  DISPLAY_CALLBACK_SIGNATURE;

  char *err2Str(wl_status_t code);
  uint8_t htoi(char c);
  bool scrollText();
  void graphicText(uint8_t modStart, uint8_t modEnd, const char *pMsg);
  boolean getText(char *szMesg, char *psz, uint8_t len);
  void resetMatrix(void);
//  void handleWiFi(void);
  static void nextState();

public:
  DisplayMatrix();
  ~DisplayMatrix();
  static void scrollDataSink(uint8_t dev, MD_MAX72XX::transformType_t t, uint8_t col);
  static uint8_t scrollDataSource(uint8_t dev, MD_MAX72XX::transformType_t t);

  void init();
  void display();

  bool printText(const char *pmsg);
  DisplayMatrix& setCallback(DISPLAY_CALLBACK_SIGNATURE);
  void runMatrixAnimation(void);
  void timeTick(TimeClient& timeClient);

};

#endif /* _DISPLAYMATRIX_H_ */
