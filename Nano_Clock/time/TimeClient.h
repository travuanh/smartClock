/*
 *  Created on: Jan 30, 2020
 *      Author: travuanh
 */
/*
 * You need to adjust the UTC offset for your timezone in milliseconds.
 * Refer the list of UTC time offsets. Here are some examples for different timezones:
 * For UTC -5.00 : -5 * 60 * 60 : -18000
 * For UTC +1.00 : 1 * 60 * 60 : 3600
 * For UTC +0.00 : 0 * 60 * 60 : 0
 * For UTC +7.00 : 7 *60 *60 : 25200
 */
/*
 * Area	HostName
	Worldwide	pool.ntp.org
	Asia	asia.pool.ntp.org
	Europe	europe.pool.ntp.org
	North America	north-america.pool.ntp.org
	Oceania	oceania.pool.ntp.org
	South America	south-america.pool.ntp.org
 */
#ifndef _TIMECLIENT_H_
#define _TIMECLIENT_H_
#include "RtcDS3231.h"


// CONNECTIONS:
// DS3231 SDA --> SDA (D2)
// DS3231 SCL --> SCL (D1)
// DS3231 VCC --> 3.3v or 5v
// DS3231 GND --> GND

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define TIMECLIENT_CALLBACK_SIGNATURE std::function<void()> callback
#else
//typedef  void (*callback)(void);
#define TIMECLIENT_CALLBACK_SIGNATURE void (*callback)(void)
#endif

#define MAX_TIME_UPDATE 6*60*60 // 6*60 mis update realtime
#define ADD_USE_TIMER 0x20  //  1 is use timer;
#define ADD_HOUR_SET 0x20 + 1 // Address of eeprom
#define ADD_MINS_SET 0x20 + 2
//#define HOST_NTP "asia.pool.ntp.org"

#define SIZE_OF_DATETIME 15
#define SIZE_OF_TEMP 7

//currentYear,currentMonth,monthDay,hour,mins,sec
typedef struct fDateTime {
	uint8_t  year; //realYear = year + 2000;
	uint8_t month;
	uint8_t dayOfmonth;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} frmDateTime;

class TimeClient{
private:
	uint8_t hour;
	uint8_t mins;
	uint8_t sec;

	char fmTime[SIZE_OF_DATETIME]; //MON 01/20/2021
	char fmTemp[SIZE_OF_TEMP]; //XX.XXC

	TIMECLIENT_CALLBACK_SIGNATURE;
public:
	TimeClient();
//	TimeClient();
	~TimeClient();
	void setup();
	void init();
	void loop();
	void setTimer(uint8_t hh, uint8_t mins);
	void updateTime(frmDateTime* dateTime);

	void initRTC();
	void printDateTime(const RtcDateTime& dt);

	uint8_t getHour() const;
	void setHour(uint8_t hour);

	uint8_t getMins() const;
	void setMins(uint8_t mins);

	uint8_t getSec() const;
	void setSec(uint8_t sec);

	TimeClient& setCallback(TIMECLIENT_CALLBACK_SIGNATURE);

	const char* getTemp() const;
//	void setTemp(const String &temp);
	const char* getFmTime() const;
//	void setFmTime(const String &fmTime);
};

#endif /* _TIMECLIENT_H_ */
