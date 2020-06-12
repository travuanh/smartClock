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

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <functional>

#include <RtcDS3231.h>

#define MAX_TIME_UPDATE 6*60*60 // 6*60 mis update realtime //secs
#define ADD_USE_TIMER 0x20  //  1 is use timer;
#define ADD_HOUR_SET 0x20 + 1 // Address of eeprom
#define ADD_MINS_SET 0x20 + 2
#define HOST_NTP "asia.pool.ntp.org"


class TimeClient{
private:
	uint8_t hour;
	uint8_t mins;
	uint8_t sec;
	String dayOfWeek;
	String fmTime;
	String temp;


	uint8_t hourSet;
	uint8_t minsSet;

	WiFiUDP* ntpUDP;
	NTPClient* timeClient;
	bool enableTimer ;
	std::function<void()> callback;

public:
	TimeClient();
	TimeClient(WiFiUDP& ntpUDP);
	~TimeClient();
	void setup(WiFiUDP& ntpUDP);
	void init();
	void loop();
	void setTimer(uint8_t hh, uint8_t mins);
	void updateTime();
	bool isEnableTimer() const;
	bool readEnableTimer() ;
	void setEnableTImer(bool enable);
	void initRTC();
	void printDateTime(const RtcDateTime& dt);


	uint8_t getHour() const;
	void setHour(uint8_t hour);
	uint8_t getHourSet()const;
	uint8_t readHourSet();
	void setHourSet(uint8_t hourSet);
	uint8_t getMins() const;
	void setMins(uint8_t mins);
	uint8_t getMinsSet() const;
	uint8_t readMinsSet();
	void setMinsSet(uint8_t minsSet);
	uint8_t getSec() const;
	void setSec(uint8_t sec);
	const String& getDayOfWeek() const;
	void setDayOfWeek(const String &dayOfWeek);
	TimeClient& setCallback(std::function<void()> callback);
	bool bCheckTimer();

	const String& getTemp() const;
	void setTemp(const String &temp);
	const String& getFmTime() const;
	void setFmTime(const String &fmTime);
};

#endif /* _TIMECLIENT_H_ */
