#include "TimeClient.h"
#include "EEPROM.h"

// CONNECTIONS:
// DS3231 SDA --> SDA
// DS3231 SCL --> SCL
// DS3231 VCC --> 3.3v or 5v
// DS3231 GND --> GND

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
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char daysOfTheWeekShort[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

uint32_t timeUpdate = 0;
uint32_t timeCount = MAX_TIME_UPDATE;
bool isCalled = false;


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
  initRTC();
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
  if((unsigned long)(millis() - timeUpdate)>1000){ //100ms
    timeUpdate = millis();
    timeCount++;
    if(timeCount > MAX_TIME_UPDATE){ // only update when boot complete
      timeCount = 0;
      updateTime();
    }else if (timeCount == MAX_TIME_UPDATE){
      timeCount = 0;
    }

    if (!Rtc.IsDateTimeValid()){
      if (Rtc.LastError() != 0){
        // we have a communications error
        // see https://www.arduino.cc/en/Reference/WireEndTransmission for
        // what the number means
        Serial.print("RTC communications error = ");
        Serial.println(Rtc.LastError());
      }
      else{
        // Common Causes:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
      }
    }
    RtcDateTime now = Rtc.GetDateTime();
    printDateTime(now);
//    Serial.println();

    RtcTemperature temp = Rtc.GetTemperature();
    char strTmp[6];
    snprintf_P(strTmp,
          countof(strTmp),
          PSTR("%02f"),
          temp.AsFloatDegC());
    this->temp = String(strTmp) + "C";
    callback();
  }

//  temp.Print(Serial);
  // you may also get the temperature as a float and print it
  // Serial.print(temp.AsFloatDegC());
//  Serial.print(strTmp);
//  Serial.println("C");

//  delay(10000); // ten seconds


}



void TimeClient::setTimer(uint8_t hh, uint8_t mins) {

}

void TimeClient::updateTime() {
  bool isUpdated = timeClient->update();

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

  unsigned long epochTime = timeClient->getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);

  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime);

  int monthDay = ptm->tm_mday;
  Serial.print("Month day: ");
  Serial.println(monthDay);

  int currentMonth = ptm->tm_mon+1;
  Serial.print("Month: ");
  Serial.println(currentMonth);

  String currentMonthName = months[currentMonth-1];
  Serial.print("Month name: ");
  Serial.println(currentMonthName);

  int currentYear = ptm->tm_year+1900;
  Serial.print("Year: ");
  Serial.println(currentYear);

  //Print complete date:
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  Serial.print("Current date: ");
  Serial.println(currentDate);

  Serial.println("");
  RtcDateTime dt = Rtc.GetDateTime();
  if((hour!=dt.Hour())&&(mins!=dt.Minute())){
    RtcDateTime compiled = RtcDateTime(currentYear,currentMonth,monthDay,hour,mins,sec);
    Serial.println("Prepare Update RTC !");
    if (isUpdated){
      Serial.println("Update RTC !");
      Rtc.SetDateTime(compiled);
    }
  }


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

const String& TimeClient::getTemp() const {
  return temp;
}

void TimeClient::setTemp(const String &temp) {
  this->temp = temp;
}

const String& TimeClient::getFmTime() const {
  return fmTime;
}

void TimeClient::setFmTime(const String &fmTime) {
  this->fmTime = fmTime;
}

TimeClient& TimeClient::setCallback(std::function<void()> callback) {
  this->callback = callback;
  return *this;
}

void TimeClient::initRTC() {
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);

  //--------RTC SETUP ------------
  // if you are using ESP-01 then uncomment the line below to reset the pins to
  // the available pins for SDA, SCL
  // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
//  RtcDateTime compiled = RtcDateTime(2020,6,11,21,21,21);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid()){
    if (Rtc.LastError() != 0){
      // we have a communications error
      // see https://www.arduino.cc/en/Reference/WireEndTransmission for
      // what the number means
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else{
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");

      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning()){
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled){
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled){
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled){
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

}



void TimeClient::printDateTime(const RtcDateTime& dt){
  setHour(dt.Hour());
  setMins(dt.Minute());
  setSec(dt.Second());
//  hour = dt.Hour();
//  mins = dt.Minute();
//  sec = dt.Second();
    char datestring[20];
//    snprintf_P(datestring,
//            countof(datestring),
//            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
//            dt.Month(),
//            dt.Day(),
//            dt.Year(),
//            dt.Hour(),
//            dt.Minute(),
//            dt.Second() );
    snprintf_P(datestring,
                countof(datestring),
                PSTR("%02u/%02u/%04u "),
                dt.Month(),
                dt.Day(),
                dt.Year());
//    Serial.print(daysOfTheWeek[dt.DayOfWeek()]);
    fmTime = String(daysOfTheWeekShort[dt.DayOfWeek()]) + " " + String(datestring);
//    Serial.print(datestring);
}
