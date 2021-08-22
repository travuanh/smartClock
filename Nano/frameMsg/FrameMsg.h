/*
 * FrameMsg.h
 *
 *  Created on: Jan 12, 2018
 *      Author: travu
 */

#ifndef LIBRARIES_FRAMEMSG_FRAMEMSG_H_
#define LIBRARIES_FRAMEMSG_FRAMEMSG_H_

//#include "Arduino.h"
#define FRAME_HEADER_1  	0x01 //SOH
#define FRAME_HEADER_2  	0xAA
#define FRAME_TRAILER_1  	0x04 //EOT
#define FRAME_TRAILER_2  	0xFE

#define CMD_TYPE 		0
#define DATA_LEN 		1
#define DATA 			2

//================Control Message======================
#define CHECKSUM 			0
#define GATEWAY 			4
#define SRC_ID 				8
#define DEST_ID 			12
#define DEST_TYPE 			16
#define CMD 				18
#define STS 				20
#define VALUE 				21
#define RESERVED 			22

#define FRAME_SIZE 			32

#define CMD_TYPE_BYTE 				0
#define CMD_TYPE_STRING 			1
#define CMD_TYPE_UPDATE_DISPLAY 	2
#define DATA_IDX 					6

//=====================================================

//=============COMAND=================================
#define CMD_UPDATE_TIME 			0xF1
#define CMD_UPDATE_DISPLAY 			0xF2

//====================================================
//#define DEF_DELIMITER_FRAME  0x09
//	0			1				 2		3			4		5...5+n			5+n+1			5+n+2	  		5+n+3
// HEADER_1 - FRAME_HEADER_2   - CMD_TYPE - DATA_LEN - DATA n - DATA n+1 - FRAME_TRAILER_1 - FRAME_TRAILER_2  CHECKSUM

/*
 * CRC32 - 	GATEWAY_ID - SRC_ID - 	DEST_ID - 	DEST_TYPE - CMD - STATUS - VALUE - RESERVED - CHECKSUM - END_FRAME
	4	|		4		|	4	|		4		|	2	|	2	|	1	|	1	|		6	|	4
	0-3			4-7			8-11		12-15		16-17	18-19	20		21			22
	total : 32 BYTES
*/

typedef struct CtrlMessage {
	unsigned long gateway_id;
	unsigned long src_id;
	unsigned long dest_id;
	uint8_t dest_type;
	uint8_t cmd;
	uint8_t sts;
	uint8_t value;
} fMessage;

class FrameMsg{

private:
	uint8_t header1;
	uint8_t header2;
	uint8_t cmd;
	uint8_t data_len;
	uint8_t* data;
	uint8_t trailer1;
	uint8_t trailer2;
	unsigned long crc;

public:
	uint8_t* buffer;
	FrameMsg();
	FrameMsg(uint8_t* _msg);

	//parser
	bool bParserMsg(fMessage* fMsg, uint8_t* buff, uint32_t size);

	//checksum
	static bool bFrameValid(uint8_t* buff, uint32_t size);
	static uint32_t u32BuildFrame(uint8_t* out, fMessage* fMsg);
	static unsigned long crc32Caculate(uint8_t* buff, uint32_t size);

	static uint8_t encryptByte(uint8_t _encode, uint8_t val);
	static void encryptBytes(uint8_t _encode, uint8_t* val, uint32_t len);
	static  uint8_t complementBit(uint8_t _bit, uint8_t val);
	void vPrintFMsg(fMessage* fMsg);
};

//extern FrameMsg fMsg;

#endif /* LIBRARIES_FRAMEMSG_FRAMEMSG_H_ */
