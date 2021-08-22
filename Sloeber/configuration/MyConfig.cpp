/*
 * MyConfig.cpp
 *
 *  Created on: Aug 15, 2021
 *      Author: travu
 */

#include "MyConfig.h"

#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
#include "../utils.h"

MyConfig::MyConfig() {
	pConfig = new fConfig;
	/*
	 * default :
	 * 			mode 24h
	 * 			dateFmt: mm/dd/yyyy
	*/
	pConfig->mode24h = CLOCK_MODE_24H;
	pConfig->dateformat = DATE_FORMAT_MONTH_FIRST;

}

MyConfig::~MyConfig() {
	free(pConfig);
}

bool MyConfig::bLoadConfig() {
	PRINTS("\n ->MyConfig::bLoadConfig()");
	File configFile = LittleFS.open("/config.json", "r");
	if (!configFile) {
		PRINTS("\n Failed to open config file");
		return false;
	}

	size_t size = configFile.size();
	if (size > 1024) {
		PRINTS("\n Config file size is too large");
		return false;
	}

	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);

	// We don't use String here because ArduinoJson library requires the input
	// buffer to be mutable. If you don't use ArduinoJson, you may as well
	// use configFile.readString instead.
	configFile.readBytes(buf.get(), size);

	StaticJsonDocument<200> doc;
	auto error = deserializeJson(doc, buf.get());
	if (error) {
		PRINTS("\n Failed to parse config file");
		return false;
	}

	const uint8_t mode24h = doc["clockMode"];
	const uint8_t dateformat = doc["dateFormat"];

	pConfig->mode24h = mode24h;
	pConfig->dateformat = dateformat;

	PRINT("\n mode24h: ", pConfig->mode24h);
	PRINT("\n dateformat: ", pConfig->dateformat);


	// Real world application would store these values in some variables for
	// later use.


	return true;
}

bool MyConfig::bSaveConfig() {
	PRINTS("\n ->MyConfig::bSaveConfig()");
	StaticJsonDocument<200> doc;
	doc["clockMode"] = pConfig->mode24h;
	doc["dateFormat"] = pConfig->dateformat;

	File configFile = LittleFS.open("/config.json", "w");
	if (!configFile) {
		PRINTS("\n Failed to open config file for writing");
		return false;
	}

	serializeJson(doc, configFile);
	return true;
}

fConfig* MyConfig::getConfig() {
	return pConfig;

}

void MyConfig::init(void) {
	PRINTS("\n ->MyConfig::init()");
	if (!LittleFS.begin()) {
		PRINTS("\n Failed to mount file system");
		return;
	}
}
