/*
 * ZigbeeUART.h
 *
 *  Created on: Sep 3, 2020
 *      Author: travu
 */
#ifndef ZIGBEEUART_H
#define ZIGBEEUART_H

#include "Arduino.h"


//#define ZIGBEE_USE_SOFTWARE_SERIAL 1

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#define ZIGBEE_CALLBACK_SIGNATURE std::function<void(unsigned int)> callback
#else
#define ZIGBEE_CALLBACK_SIGNATURE void (*callback)(unsigned int)
#endif

#define D7 7
#define D8 8

#ifdef ZIGBEE_USE_SOFTWARE_SERIAL
#include "SoftwareSerial.h"
#define RX_PIN D8
#define TX_PIN D7
#endif

#define ZIGBEE_BUFF_SIZE 255
#define DEF_START_FRAME '$'
#define DEF_END_FRAME	'#'



class ZigBee {
private:
	int _timeout;
	ZIGBEE_CALLBACK_SIGNATURE;
//	Vector<callback> callbacks;
	String
	_readSerial();
	/*
	 * Recvive data from uart. Return all received data if target found or timeout.
	 */
	String
	recvString(String target, uint32_t timeout = 1000);
	/*
	 * Recvive data from uart and search first target. Return true if target found, false for timeout.
	 */
	bool
	recvFind(String target, uint32_t timeout = 1000);

public:

#ifdef ZIGBEE_USE_SOFTWARE_SERIAL
	ZigBee(SoftwareSerial &uart, uint32_t baud = 57600);
#else /* HardwareSerial */
	ZigBee(HardwareSerial &uart, uint32_t baud = 57600);
#endif

	void
	begin();
	void
	rx_empty(void);
	void
	send(String value);
	void
	writeByte(uint8_t value);
	bool
	send(const uint8_t *buffer, uint32_t len);

	uint32_t
	recv(uint8_t *buffer, uint32_t buffer_size, uint32_t timeout = 1000);
	void
	sendHeader(void);
	void
	sendFooter(void);
	void
	handle(uint8_t *buffer);

	ZigBee&
	setCallback(ZIGBEE_CALLBACK_SIGNATURE);

#ifdef ZIGBEE_USE_SOFTWARE_SERIAL
	SoftwareSerial *m_puart; /* The UART to communicate with SIM800L */
#else
    HardwareSerial *m_puart; /* The UART to communicate with SIM800L */
#endif

};

#endif

