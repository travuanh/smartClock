#include "TimeClient.h"
#include "EEPROM.h"

const long utcOffsetInSeconds = 25200; //UTC +7
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

uint32_t timeUpdate = 0;
int timeCount = MAX_TIME_UPDATE;
bool isCalled = false;

#define ON 			1
#define OFF			0

TimeClient::TimeClient() {
	this->enableTimer = false;
	this->hourSet = 0;
	this->hour = 0;
	this->minsSet = 0;
	this->mins = 0;
	this->sec = 0;
	this->ntpUDP = NULL;
	this->timeClient = NULL;
}

TimeClient::TimeClient(WiFiUDP &ntpUDP) {
	this->enableTimer = false;
	this->hourSet = 0;
	this->hour = 0;
	this->minsSet = 0;
	this->mins = 0;
	this->sec = 0;
	this->ntpUDP = &ntpUDP;
	this->timeClient = NULL;
}

TimeClient::~TimeClient() {
}


void TimeClient::setup(WiFiUDP &ntpUDP) {
	EEPROM.begin(512);
	this->ntpUDP = &ntpUDP;
	this->timeClient = new NTPClient(ntpUDP,HOST_NTP, utcOffsetInSeconds);
	init();
	timeUpdate = millis();
}

void TimeClient::init() {
	readEnableTimer();
	readHourSet();
	readMinsSet();
}

bool TimeClient::bCheckTimer() {
	bool result = false;
	if(enableTimer){
		if((hourSet == hour) && (minsSet == mins)){
			result = true;
		}
	}
	return result;
}

void TimeClient::loop() {
	if((unsigned long)(millis() - timeUpdate)>1000){
		timeUpdate = millis();
		sec++;
		if(sec>=60){
			sec = 0;
			mins++;
			if(mins>=60){
				mins = 0;
				hour++;
				if(hour>=24){
					hour = 0;
				}
			}
		}
		timeCount++;
		if(timeCount > MAX_TIME_UPDATE){
			timeCount = 0;
			updateTime();
		}
//		Serial.print(hour);
//		Serial.print(":");
//		Serial.print(mins);
//		Serial.print(":");
//		Serial.println(sec);

		if(bCheckTimer()){
			if(!isCalled){
				isCalled = true;
				callback();
			}
		}else{
			isCalled = false;
		}
	}

}



void TimeClient::setTimer(uint8_t hh, uint8_t mins) {

}

void TimeClient::updateTime() {
	timeClient->update();

	setDayOfWeek(daysOfTheWeek[timeClient->getDay()]);
	setHour(timeClient->getHours());
	setMins(timeClient->getMinutes());
	setSec(timeClient->getSeconds());
	setFmTime(timeClient->getFormattedTime());

	Serial.print(dayOfWeek);
	Serial.print(", ");
	Serial.print(hour);
	Serial.print(":");
	Serial.print(mins);
	Serial.print(":");
	Serial.println(sec);

	Serial.println(fmTime);
}

bool TimeClient::isEnableTimer() const {
	return enableTimer;
}

bool TimeClient::readEnableTimer() {
	enableTimer = EEPROM.read(ADD_USE_TIMER);
	return enableTimer;
}

void TimeClient::setEnableTImer(bool enable) {
	this->enableTimer = enable;
	EEPROM.write(ADD_USE_TIMER, enable);
	EEPROM.commit();
}


uint8_t TimeClient::getHour() const {
	return hour;
}

void TimeClient::setHour(uint8_t hour) {
	this->hour = hour;
}

uint8_t TimeClient::getHourSet() const {
	return hourSet;
}

uint8_t TimeClient::readHourSet() {
	hourSet = EEPROM.read(ADD_HOUR_SET);
	return hourSet;
}

uint8_t TimeClient::readMinsSet() {
	minsSet = EEPROM.read(ADD_MINS_SET);
	return minsSet;
}
void TimeClient::setHourSet(uint8_t hourSet) {
	this->hourSet = hourSet;
	EEPROM.write(ADD_HOUR_SET, hourSet);
	EEPROM.commit();
}

void TimeClient::setMinsSet(uint8_t minsSet) {
	this->minsSet = minsSet;
	EEPROM.write(ADD_MINS_SET, minsSet);
	EEPROM.commit();
}

uint8_t TimeClient::getMins() const {
	return mins;
}

void TimeClient::setMins(uint8_t mins) {
	this->mins = mins;
}

uint8_t TimeClient::getMinsSet() const {
	return minsSet;
}



uint8_t TimeClient::getSec() const {
	return sec;
}


void TimeClient::setSec(uint8_t sec) {
	this->sec = sec;
}

const String& TimeClient::getDayOfWeek() const {
	return dayOfWeek;
}

void TimeClient::setDayOfWeek(const String &dayOfWeek) {
	this->dayOfWeek = dayOfWeek;
}

TimeClient& TimeClient::setCallback(std::function<void()> callback) {
	this->callback = callback;
	return *this;
}


