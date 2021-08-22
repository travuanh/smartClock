/*
 * FrameMsg.cpp
 *
 *  Created on: Jan 13, 2018
 *      Author: travu
 */
#include <Arduino.h>
#include "FrameMsg.h"
#include "../utils.h"
#include <CRC32.h>

const char FRAME_TAG[] = "FrameMsg";

FrameMsg::FrameMsg(){
	this->buffer = NULL;
	cmd = 0;
	header1 = 0;
	header2 = 0;
	trailer1 = 0;
	trailer2 = 0;
	data_len = 0;
	data = NULL;
	crc = 0;
}

FrameMsg::FrameMsg(uint8_t* _msg){
	cmd = 0;
	header1 = 0;
	header2 = 0;
	trailer1 = 0;
	trailer2 = 0;
	data_len = 0;
	data = NULL;
	crc = 0;
	this->buffer = _msg;
//	parser(_msg);
}



bool FrameMsg::bFrameValid(uint8_t *buff, uint32_t size) {
	PRINTS("\n-> bFrameValid()");
	bool result = false;

	unsigned long checksum = ((unsigned long)buff[CHECKSUM]<<24) + ((unsigned long)buff[CHECKSUM+1] <<16) + ((unsigned long)buff[CHECKSUM + 2]<<8) + (buff[CHECKSUM+3]);
	PRINTX("\nchecksum: %X::", checksum);

	uint32_t crc = crc32Caculate(&buff[GATEWAY],size-4);
	PRINTX("\ncrc32Caculate: %X::", crc);

	if(checksum == crc)
		result = true;

	return result;

}

uint32_t FrameMsg::u32BuildFrame(uint8_t *out, fMessage *fMsg) {
	PRINTS("-> u32BuildFrame()");
	uint32_t size = 0;
	if (fMsg == NULL ){
		PRINTS("fMsg == NULL");
		return 0;
	}
	out[GATEWAY] 	= (fMsg->gateway_id >>24) & 0xFF;
	out[GATEWAY+1] 	= (fMsg->gateway_id >>16) & 0xFF;
	out[GATEWAY+2] 	= (fMsg->gateway_id >>8) & 0xFF;
	out[GATEWAY+3] 	= (fMsg->gateway_id) & 0xFF;

	out[SRC_ID] 	= (fMsg->src_id >>24) & 0xFF;
	out[SRC_ID+1] 	= (fMsg->src_id >>16) & 0xFF;
	out[SRC_ID+2] 	= (fMsg->src_id >>8) & 0xFF;
	out[SRC_ID+3] 	= (fMsg->src_id)  & 0xFF;

	out[DEST_ID] 	= (fMsg->dest_id >>24) & 0xFF;
	out[DEST_ID+1] 	= (fMsg->dest_id >>16) & 0xFF;
	out[DEST_ID+2] 	= (fMsg->dest_id >>8) & 0xFF;
	out[DEST_ID+3] 	= (fMsg->dest_id) & 0xFF;

	out[DEST_TYPE] 		= (fMsg->dest_type >>8) & 0xFF ;
	out[DEST_TYPE+1] 	= fMsg->dest_type & 0xFF ;

	out[CMD] 			= (fMsg->cmd >>8) & 0xFF;
	out[CMD+1] 			= fMsg->cmd & 0xFF;

	out[STS] 			= fMsg->sts & 0xFF;
	out[VALUE] 			= fMsg->value & 0xFF;

	size = VALUE + 1;

	uint32_t crc = crc32Caculate(&out[GATEWAY], size);
	PRINTX("crc32Caculate: %X ::", crc);

	out[CHECKSUM] 		= (crc >>24) & 0xFF;
	out[CHECKSUM+1] 	= (crc >>16) & 0xFF;
	out[CHECKSUM+2] 	= (crc >>8) & 0xFF;
	out[CHECKSUM+3] 	= (crc) & 0xFF;

	return size;

}

unsigned long FrameMsg::crc32Caculate(uint8_t *buff, uint32_t size) {
	unsigned long crc32 = CRC32::calculate(buff, size);
	return crc32;

}

bool FrameMsg::bParserMsg(fMessage *fMsg, uint8_t *buff, uint32_t size) {
	PRINTS("-> bParserMsg()");
	if (fMsg == NULL ){
		PRINTS("fMsg == NULL");
		return false;
	}

	if(!bFrameValid(buff, size)){
		PRINTS("checksum error !!!");
		return false;
	}

	fMsg->gateway_id = ((unsigned long)buff[GATEWAY]<<24) +((unsigned long)buff[GATEWAY+1] <<16) + ((unsigned long)buff[GATEWAY + 2]<<8) + (buff[GATEWAY+3]);
	fMsg->src_id = ((unsigned long)buff[SRC_ID]<<24) +((unsigned long)buff[SRC_ID+1] <<16) + ((unsigned long)buff[SRC_ID + 2]<<8) + (buff[SRC_ID+3]);
	fMsg->dest_id = ((unsigned long)buff[DEST_ID]<<24) +((unsigned long)buff[DEST_ID+1] <<16) + ((unsigned long)buff[DEST_ID + 2]<<8) + (buff[DEST_ID+3]);

	fMsg->dest_type = (buff[DEST_TYPE]<<8) +(buff[DEST_TYPE+1]);
	fMsg->cmd = (buff[CMD]<<8) +(buff[CMD+1]);

	fMsg->sts = buff[STS];

	fMsg->value = buff[VALUE];
	vPrintFMsg(fMsg);
	return true;

}

uint8_t FrameMsg::complementBit(uint8_t _bit, uint8_t val){
	uint8_t ret = 0;

	switch (_bit){
		case 1:
			if((val&0x01) == 0x01){
				ret = val & 0xFE;
			}else{
				ret = val | 0x01;
			}
			break;
		case 2:
				if((val & 0x02) == 0x02){
					ret = val & 0xFD;
				}else{
					ret = val | 0x02;
				}
				break;
		case 3:
				if((val & 0x04) == 0x04){
					ret = val & 0xFB;
				}else{
					ret = val | 0x04;
				}
				break;
		case 4:
				if((val & 0x08) == 0x08){
					ret = val & 0xF7;
				}else{
					ret = val | 0x08;
				}
				break;
		case 5:
				if((val & 0x10) == 0x10){
					ret = val & 0xEF;
				}else{
					ret = val | 0x10;
				}
				break;
		case 6:
				if((val & 0x20) == 0x20){
					ret = val & 0xDF;
				}else{
					ret = val | 0x20;
				}
				break;
		case 7:
				if((val & 0x40) == 0x40){
					ret = val & 0xBF;
				}else{
					ret = val | 0x40;
				}
				break;


		}
		return ret;
}

uint8_t FrameMsg::encryptByte(uint8_t _encode, uint8_t val){
	uint8_t ret = 0;
	ret = val;
	uint8_t offset = _encode % 7;
	if(offset == 0)
		offset = 7;
	for(int i =1; i<= offset; i++){
		ret = complementBit(i, ret);
	}

	return ret;


}

void FrameMsg::encryptBytes(uint8_t _encode, uint8_t* val, uint32_t len){
//	uint32_t len = sizeof(val);
	uint32_t i = 0;
	//i = 0 is encode Byte
	for (i = 1; i< len; i++){
		if(val[i] > 250)
			continue;

		val[i] = encryptByte(_encode, val[i]);
		if(val[i] > 250){
			val[i] = encryptByte(_encode, val[i]);
		}

	}

}

void FrameMsg::vPrintFMsg(fMessage* fMsg){
	PRINTS("fMessage : ");
	PRINTS("====================================");
	PRINTX("gateway_id: %X::", fMsg->gateway_id);
	PRINTX("src_id: %X::", fMsg->src_id);
	PRINTX("dest_id: %X::" , fMsg->dest_id);
	PRINT("dest_type: %d" , fMsg->dest_type);
	PRINT("cmd: %d" , fMsg->cmd);
	PRINT("sts: %d" , fMsg->sts);
	PRINT("value: %d" , fMsg->value);
	PRINTS("====================================");


}
