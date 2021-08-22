/*
 * Zigbee.cpp
 *
 *  Created on: Sep 3, 2020
 *      Author: travu
 */
#include "ZigbeeUART.h"
#include "../utils.h"
#include "../frameMsg/FrameMsg.h"

//uint8_t* buffer;

#ifdef ZIGBEE_USE_SOFTWARE_SERIAL
ZigBee::ZigBee(SoftwareSerial &uart, uint32_t baud) :m_puart(&uart) {
	m_puart->begin(baud);
	rx_empty();
	_timeout=0;
	this->callback = NULL;
}
// HardwareSerial
#else
ZigBee::ZigBee(HardwareSerial &uart, uint32_t baud): m_puart(&uart)
{
    m_puart->begin(baud);
    rx_empty();
    _timeout=0;
}

// SoftwareSerial SIM(RX_PIN,TX_PIN);

#endif

void ZigBee::begin() {
//	m_puart->begin(57600);
//  buffer.reserve(ZIGBEE_BUFF_SIZE); //reserve memory to prevent intern fragmention
}

//
//PRIVATE METHODS
//
String ZigBee::_readSerial() {
	_timeout = 0;
	while (!m_puart->available() && _timeout < 3000) {
		delay(13);
		_timeout++;

	}
	if (m_puart->available()) {
		return m_puart->readString();
	}

	return "";

}

void ZigBee::rx_empty(void) {
//    m_puart->println("ZigBee::rx_empty");
	while (m_puart->available() > 0) {
		m_puart->read();
	}
}

void ZigBee::send(String value) {
	rx_empty();
	m_puart->println(value);
}

String ZigBee::recvString(String target, uint32_t timeout) {
	String data;
	char a;
	unsigned long start = millis();
	while (millis() - start < timeout) {
		while (m_puart->available() > 0) {
			a = m_puart->read();
			if (a == '\0')
				continue;
			data += a;
		}
		if (data.indexOf(target) != -1) {
			break;
		}
	}
	return data;
}

bool ZigBee::recvFind(String target, uint32_t timeout) {
	String data_tmp;
	data_tmp = recvString(target, timeout);
	if (data_tmp.indexOf(target) != -1) {
		return true;
	}
	return false;
}

uint32_t ZigBee::recv(uint8_t *buffer, uint32_t buffer_size, uint32_t timeout) {
	uint8_t a;
	uint32_t ret;
	unsigned long start;
	uint32_t i;

	bool has_data = false;
	bool end_data = false;
	uint8_t headers[2];
	uint8_t trailers[2];

	if (buffer == NULL) {
		return 0;
	}
//	send("ZigBee::recv");
	i = 0;
	ret = buffer_size;
	start = millis();

	if(!m_puart->available())
		return 0;
	memset(headers,0,sizeof(headers));
	//Start frame
	while (millis() - start < timeout) {

		if (m_puart->available() > 0) {
			a = m_puart->read();
			headers[i] = a;
			i++;
		}
		if(i==2){
			if((headers[0] == FRAME_HEADER_1)&&(headers[1] == FRAME_HEADER_2)){
				has_data = true;
				break;
			}
		}
	}

	if (has_data) {
		i = 0;
		memset(trailers,0,sizeof(trailers));
		memset(buffer, 0 , ZIGBEE_BUFF_SIZE);
//		SERIAL_RX_BUFFER_SIZE
		start = millis();

		while (millis() - start < timeout){
			while ((m_puart->available() > 0) && (i < ret)) {
				a = m_puart->read();
				if(a == FRAME_TRAILER_1){//check end frame
					trailers[0] = a;
//					continue;
				}else if (a == FRAME_TRAILER_2){
					if(trailers[0] != 0){
						trailers[1] = a;
						end_data = true;
//						break;
					}


				}else{
					if(trailers[0]!=0){ //byte # trailer2 ->
						memset(trailers,0,sizeof(trailers));
					}
				}
				if(end_data){
					buffer[i] = 0;
					i--;
					break;
				}
				buffer[i++] = a;
			}

			if (end_data) {
				rx_empty();
				return i;
			}

			if (i == ret) {
				rx_empty();
				return ret;
			}

		}
	}

	return 0;
}

bool ZigBee::send(const uint8_t *buffer, uint32_t len) {
	for (uint32_t i = 0; i < len; i++) {
		m_puart->write(buffer[i]);
	}
	return true;
//	return recvFind("OK", 10000);
}

void ZigBee::writeByte(uint8_t value) {
	rx_empty();
	m_puart->write(value);
}

void ZigBee::sendHeader(void) {
	int randomNo = random(25, 50);
	for (uint8_t i = 0; i < randomNo; i++) {
		uint8_t subRandomNo = random(255);
		while (subRandomNo == 0xFB)
			subRandomNo = random(255);
		m_puart->write(subRandomNo);
	}
}

void ZigBee::sendFooter(void) {
	int randomNo = random(25, 50);
	for (uint8_t i = 0; i < randomNo; i++) {
		uint8_t subRandomNo = random(255);
		while (subRandomNo == 0xFB)
			subRandomNo = random(255);
		m_puart->write(subRandomNo);
	}
}

void ZigBee::handle(uint8_t *buffer) {
//	PRINTS("ZigBee::handle");
//	const char* text= "Zigbee uart";
//	send((uint8_t*)text, 11);
	int len = recv(buffer, ZIGBEE_BUFF_SIZE, 50);
	if (len > 0) {
//		FrameMsg msg;
		bool result = false;
//		uint8_t cmd_type = buffer[0];
//		uint8_t data_len = buffer[1];
		uint8_t data_len = len - 2 ;// for testing use len buffer -2

		result = FrameMsg::bFrameValid(&buffer[DATA], data_len);
		result = true;//for testing , always allow checksum
		if(result){
			callback(len);

		}


	}

}


ZigBee&
ZigBee::setCallback(ZIGBEE_CALLBACK_SIGNATURE) {
	this->callback = callback;
	return *this;
}
