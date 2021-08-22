/*
 * MyConfig.h
 *
 *  Created on: Aug 15, 2021
 *      Author: travu
 */

#ifndef CONFIGURATION_MYCONFIG_H_
#define CONFIGURATION_MYCONFIG_H_
#include <Arduino.h>

#define CLOCK_MODE_24H 			0
#define CLOCK_MODE_12H			1

#define	DATE_FORMAT_MONTH_FIRST			0 //mm/dd/yyyyy
#define	DATE_FORMAT_DAY_FIRST			1 //dd/mm/yyyyy

typedef struct Configuration {
	uint8_t dateformat; //0: mm/dd/yyyy; 1: dd/mm/yyyy
	uint8_t mode24h; //0: 24h, 1: am/pm
} fConfig;

class MyConfig{
public:
	MyConfig();
	~MyConfig();
	bool bLoadConfig();
	bool bSaveConfig();
	fConfig* getConfig();
	void init(void);
private:
	fConfig* pConfig;
};


#endif /* CONFIGURATION_MYCONFIG_H_ */
