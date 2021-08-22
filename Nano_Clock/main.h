// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef __MAIN_H_
#define __MAIN_H_
#include "Arduino.h"
//add your includes for the project NanoTest here


//end of add your includes here


//add your function definitions for the project NanoTest here

void vProcessFrmMsg(uint8_t* buff ,int len);
void zigBeeCallback(unsigned int len);
void displayCallback(void);
void timerCallback(void);

//Do not add code below this line
#endif /* __MAIN_H_ */
