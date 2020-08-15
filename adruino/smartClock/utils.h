/*
 * config.h
 *
 *  Created on: June 9, 2020
 *      Author: travuanh
 */

#ifndef UTILS_H_
#define UTILS_H_
#include "Arduino.h"

#define MYESP       "MyClock"

//#define ON      1
//#define OFF     0


#define  PRINT_CALLBACK 0
//#define DEBUG 1
#define LED_HEARTBEAT 1
#define HB_LED  LED_BUILTIN//D2

#if DEBUG
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
#define PRINTS(s)   { Serial.print(F(s)); }
#else
#define PRINT(s, v)
#define PRINTS(s)
#endif


#if LED_HEARTBEAT
#define HB_LED_TIME 500 // in milliseconds
#endif

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define MAX_DEVICES 4

#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

#define MAX_DISPLAY_TIME  45 // secs
#define MAX_DISPLAY_DATE  5 //secs
#define MAX_DISPLAY_TEMP  5 //secs
#define MAX_DISPLAY_IDLE  5 //secs
#define MAX_DISPLAY_DELAY -3 //secs


#define MAX_TIME_TO_CONNECT_WIFI 15*60 //15 mins

#define MAX_TIME_TO_CONFIG_WIFI 10 //180 secs
#endif /* UTILS_H_ */
