/*
 * config.h
 *
 *  Created on: June 9, 2020
 *      Author: travuanh
 */

#ifndef UTILS_H_
#define UTILS_H_
#include "Arduino.h"

#define MYESP 			"MyClock"

//#define ON 			1
//#define OFF			0


#define  PRINT_CALLBACK 0
#define DEBUG 1

#define LED_HEARTBEAT 1
#define HB_LED  LED_BUILTIN//D2

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define LED_ON			digitalWrite(HB_LED, LOW);
#define LED_OFF			digitalWrite(HB_LED, HIGH);

#if DEBUG
#define PRINT(s, v)   { Serial.print(F(s)); Serial.print(v); }        // Print a string followed by a value (decimal)
#define PRINTX(s, v)  { Serial.print(F(s)); Serial.print(v, HEX); }   // Print a string followed by a value (hex)
#define PRINTB(s, v)  { Serial.print(F(s)); Serial.print(v, BIN); }   // Print a string followed by a value (binary)
#define PRINTC(s, v)  { Serial.print(F(s)); Serial.print((char)v); }  // Print a string followed by a value (char)
#define PRINTS(s)     { Serial.print(F(s)); }                         // Print a string
#else
#define PRINT(s, v)   // Print a string followed by a value (decimal)
#define PRINTX(s, v)  // Print a string followed by a value (hex)
#define PRINTB(s, v)  // Print a string followed by a value (binary)
#define PRINTC(s, v)  // Print a string followed by a value (char)
#define PRINTS(s)     // Print a string
#endif


#if LED_HEARTBEAT
#define HB_LED_TIME 500 // in milliseconds
#endif





#endif /* UTILS_H_ */
