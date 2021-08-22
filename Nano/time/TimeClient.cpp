#include "TimeClient.h"
//#include <Arduino.h>
//#include "../config.h"
#include "../utils.h"
/* for software wire use below
#include <SoftwareWire.h>  // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

SoftwareWire myWire(SDA, SCL);
RtcDS3231<SoftwareWire> Rtc(myWire);
 for software wire use above */

/* for normal hardware wire use below */
#include <Wire.h> // must be included here so that Arduino library object file references work
RtcDS3231<TwoWire> Rtc(Wire);
/* for normal hardware wire use above */

#define countof(a) (sizeof(a) / sizeof(a[0]))

const long utcOffsetInSeconds = 25200; //UTC +7
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char daysOfTheWeekShort[7][3] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//Month names
const String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char monthsShort[12][3]={"Jan", "Feb", "Mar", "Aprl", "May", "Jun", "Jul", "Aug", "Sepr", "Oct", "Nov", "Dec"};

static const char* TIME_TAG = "TimeClient";

uint32_t timeUpdate = 0;
uint32_t timeCount = MAX_TIME_UPDATE;
bool isCalled = false;


TimeClient::TimeClient() {
	this->callback = NULL;
	this->hour = 0;
	this->mins = 0;
	this->sec = 0;
	memset(fmTime,0,SIZE_OF_DATETIME);
	memset(fmTemp,0,SIZE_OF_TEMP);
}


TimeClient::~TimeClient() {

}


void TimeClient::setup() {
//	PRINTS("\n->TimeClient::setup()");
	init();
	timeUpdate = millis();
}

void TimeClient::init() {
//	PRINTS("\n->TimeClient::init()");
	initRTC();
}

void TimeClient::loop() {
	if((unsigned long)(millis() - timeUpdate)>1000){ //100ms
//		PRINTS("TimeClient::loop()");
		timeUpdate = millis();
		if (!Rtc.IsDateTimeValid()){
			if (Rtc.LastError() != 0){
				// we have a communications error
				// see https://www.arduino.cc/en/Reference/WireEndTransmission for
				// what the number means
//				PRINTS("RTC communications error = ");
				PRINT("\nRTC communications error =", Rtc.LastError());
			}
			else{
				// Common Causes:
				//    1) the battery on the device is low or even missing and the power line was disconnected
				PRINTS("\nRTC lost confidence in the DateTime!");
			}
		}
//
		RtcDateTime now = Rtc.GetDateTime();
		printDateTime(now);//

		RtcTemperature temp = Rtc.GetTemperature();
		String stringOne =  String(temp.AsFloatDegC(), 2);
		snprintf_P(fmTemp,
					countof(fmTemp),
					PSTR("%sC"),
					stringOne.c_str());
//		PRINT("\ntimeClient::temp ", getTemp());
		callback(); // only callback after 1 second
	}
}

void TimeClient::updateTime(frmDateTime* dateTime){
	PRINTS("\n-> TimeClient::updateTime()" );

	uint16_t year = dateTime->year + 2000;
	char datestring[20];
//	snprintf_P(datestring,
//			countof(datestring),
//			PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
//			dateTime->month,
//			dateTime->dayOfmonth,
//			dateTime->year,
//			dateTime->hour,
//			dateTime->min,
//			dateTime->sec );
//	PRINT("\ndateTime:",datestring);--

	RtcDateTime compiled = RtcDateTime(year,dateTime->month,dateTime->dayOfmonth,dateTime->hour,dateTime->min,dateTime->sec);

	snprintf_P(datestring,
					countof(datestring),
					PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
					compiled.Month(),
					compiled.Day(),
					compiled.Year(),
					compiled.Hour(),
					compiled.Minute(),
					compiled.Second() );

	PRINT("\ncompiled:",datestring);
	Rtc.SetDateTime(compiled);

}


uint8_t TimeClient::getHour() const {
	return hour;
}

void TimeClient::setHour(uint8_t hour) {
	this->hour = hour;
}


uint8_t TimeClient::getMins() const {
	return mins;
}

void TimeClient::setMins(uint8_t mins) {
	this->mins = mins;
}

uint8_t TimeClient::getSec() const {
	return sec;
}


void TimeClient::setSec(uint8_t sec) {
	this->sec = sec;
}


const char* TimeClient::getTemp() const {
	return fmTemp;
}

const char* TimeClient::getFmTime() const {
	return fmTime;
}

TimeClient& TimeClient::setCallback(TIMECLIENT_CALLBACK_SIGNATURE) {
//	PRINTS("TimeClient::setCallback()");
	this->callback = callback;
	return *this;
}

void TimeClient::initRTC() {
	PRINTS("\ncompiled: ");
	PRINT("\nDATE", __DATE__);
	PRINT("\nTIME",__TIME__);
	//--------RTC SETUP ------------
	// if you are using ESP-01 then uncomment the line below to reset the pins to
	// the available pins for SDA, SCL
	// Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

	Rtc.Begin();
	RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
//	RtcDateTime compiled = RtcDateTime(2020,6,11,21,21,21);
	printDateTime(compiled);


	if (!Rtc.IsDateTimeValid()){
		if (Rtc.LastError() != 0){
			// we have a communications error
			// see https://www.arduino.cc/en/Reference/WireEndTransmission for
			// what the number means
			PRINT("\nRTC communications error = ",Rtc.LastError());
//			Serial.println(Rtc.LastError());
		}
		else{
			// Common Causes:
			//    1) first time you ran and the device wasn't running yet
			//    2) the battery on the device is low or even missing

			PRINTS("\nRTC lost confidence in the DateTime!");

			// following line sets the RTC to the date & time this sketch was compiled
			// it will also reset the valid flag internally unless the Rtc device is
			// having an issue

			Rtc.SetDateTime(compiled);
		}
	}

	if (!Rtc.GetIsRunning()){
		PRINTS("\nRTC was not actively running, starting now");
		Rtc.SetIsRunning(true);
	}

	RtcDateTime now = Rtc.GetDateTime();
	if (now < compiled){
		PRINTS("\nRTC is older than compile time!  (Updating DateTime)");
		Rtc.SetDateTime(compiled);
	}
	else if (now > compiled){
		PRINTS("\nRTC is newer than compile time. (this is expected)");
	}
	else if (now == compiled){
		PRINTS("\nRTC is the same as compile time! (not expected but all is fine)");
	}

	// never assume the Rtc was last configured by you, so
	// just clear them to your needed state
	Rtc.Enable32kHzPin(false);
	Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

}



void TimeClient::printDateTime(const RtcDateTime& dt){
//	PRINTS("\nprintDateTime\n");
	setHour(dt.Hour());
	setMins(dt.Minute());
	setSec(dt.Second());
//    char datestring[20];
//    snprintf_P(datestring,
//            countof(datestring),
//            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
//            dt.Month(),
//            dt.Day(),
//            dt.Year(),
//            dt.Hour(),
//            dt.Minute(),
//            dt.Second() );
    String strDayofWeek = String(daysOfTheWeekShort[dt.DayOfWeek()]);
    snprintf_P(fmTime,
                countof(fmTime),
                PSTR("%s %02u/%02u/%04u "),
				daysOfTheWeekShort[dt.DayOfWeek()],
                dt.Month(),
                dt.Day(),
                dt.Year());

//    PRINT("\ntimeClient::fmTime ", getFmTime());
//    PRINT("datestring: %s \n", datestring);
}
