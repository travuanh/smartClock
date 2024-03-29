/*
 * DisplayMatrix.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: travu
 */


#include "DisplayMatrix.h"
#include "Arduino.h"
#include "../utils.h"
#include <SPI.h>


// IP address for the ESP8266 is displayed on the scrolling display
// after startup initialisation and connected to the WiFi network.
//
// Connections for ESP8266 hardware SPI are:
// Vcc       3v3     LED matrices seem to work at 3.3V
// GND       GND     GND
// DIN        D7     HSPID or HMOSI
// CS or LD   D8     HSPICS or HCS
// CLK        D5     CLK or HCLK



// SPI hardware interface
//MD_MAX72XX mx = MD_MAX72XX(CS_PIN, MAX_DEVICES);

// Arbitrary pins
//MD_MAX72XX mx = MD_MAX72XX(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#define CLK_PIN   D5 // or SCK
#define DATA_PIN  D7 // or MOSI
#define CS_PIN    D8 // or SS

static const char* DISPLAY_TAG = "Display";

struct LineDefinition
{
  char    message[MESG_SIZE];      // message for this display
  boolean newMessageAvailable;    // true if new message arrived
};

struct LineDefinition  Line[MAX_LINE_MSG] ;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW //MD_MAX72XX::PAROLA_HW  //edit this as per your LED matrix hardware type
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

uint32_t prevTimeAnim = 0;    // Used for remembering the millis() value in animations

char curMessage[MESG_SIZE];
//char newMessage[MESG_SIZE];
//char listMsg[MESG_SIZE][MAX_LINE_MSG];

enum {S_IDLE, S_TIME, S_DATE, S_TEMP, S_UPDATE, S_ALERT} eDisplayState = S_IDLE;

bool firstUpdateDiplay = false;
int timeDisplayCount = 0;

bool isRunning = true;
bool newMessageAvailable =  false;
uint32_t prevTime = 0;

static bool bRestart = true;

//bool changeState = false;

String updateMsg = "";




DisplayMatrix::DisplayMatrix ()
{
//  this->newMessageAvailable = false;
  this->blink = false;
  this->callback = NULL;
}

DisplayMatrix::~DisplayMatrix ()
{

}

void DisplayMatrix::init ()
{
  // Display initialization
//  PRINTS("\n->DisplayMatrix::init()");
  mx.begin();
//  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  mx.setShiftDataInCallback(scrollDataSource);
  mx.setShiftDataOutCallback(scrollDataSink);

  curMessage[0] =  '\0';
  for(int i = 0; i< MAX_LINE_MSG;++i)
    {
//      listMsg[i][0]= '\0';
      Line[i].message[0] = '\0';
      Line[i].newMessageAvailable = false;
    }


  printText("  ***  ");
}


uint8_t DisplayMatrix::htoi (char c)
{
  c = toupper(c);
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A' + 0xa);
  return(0);
}



boolean DisplayMatrix::getText (char *szMesg, char *psz, uint8_t len)
{
  boolean isValid = false;  // text received flag
  char *pStart, *pEnd;      // pointer to start and end of text

  // get pointer to the beginning of the text
  pStart = strstr(szMesg, "/&MSG=");

  if (pStart != NULL)
  {
    pStart += 6;  // skip to start of data
    pEnd = strstr(pStart, "/&");

    if (pEnd != NULL)
    {
      while (pStart != pEnd)
      {
        if ((*pStart == '%') && isdigit(*(pStart+1)))
        {
          // replace %xx hex code with the ASCII character
          char c = 0;
          pStart++;
          c += (htoi(*pStart++) << 4);
          c += htoi(*pStart++);
          *psz++ = c;
        }
        else
          *psz++ = *pStart++;
      }

      *psz = '\0'; // terminate the string
      isValid = true;
    }
  }

  return(isValid);
}


void DisplayMatrix::graphicText (uint8_t modStart, uint8_t modEnd,const char *pMsg)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
  uint8_t   state = 0;
  uint8_t   curLen =0;
  uint16_t  showLen;
  uint8_t   cBuf[8];
  int16_t   col = ((modEnd + 1) * COL_SIZE) - 1;

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  do     // finite state machine to print the characters in the space available
  {
    switch(state)
    {
      case 0: // Load the next character from the font table
        // if we reached end of message, reset the message pointer
        if (*pMsg == '\0')
        {
          showLen = col - (modEnd * COL_SIZE);  // padding characters
          state = 2;
          break;
        }

        // retrieve the next character form the font file
        showLen = mx.getChar(*pMsg++, sizeof(cBuf)/sizeof(cBuf[0]), cBuf);
        curLen = 0;
        state++;
        // !! deliberately fall through to next state to start displaying

      case 1: // display the next part of the character
        mx.setColumn(col--, cBuf[curLen++]);

        // done with font character, now display the space between chars
        if (curLen == showLen)
        {
          showLen = CHAR_SPACING;
          state = 2;
        }
        break;

      case 2: // initialize state for displaying empty columns
        curLen = 0;
        state++;
        // fall through

      case 3: // display inter-character spacing or end of message padding (blank columns)
        mx.setColumn(col--, 0);
        curLen++;
        if (curLen == showLen)
          state = 0;
        break;

      default:
        col = -1;   // this definitely ends the do loop
    }
  } while (col >= (modStart * COL_SIZE));

  mx.control(modStart, modEnd, MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
}

DisplayMatrix& DisplayMatrix::setCallback (DISPLAY_CALLBACK_SIGNATURE)
{
  this->callback = callback;
  return *this;
}
void DisplayMatrix::scrollDataSink (uint8_t dev, MD_MAX72XX::transformType_t t,
                               uint8_t col)
// Callback function for data that is being scrolled off the display
{
  #if PRINT_CALLBACK
  Serial.print("\n cb ");
  Serial.print(dev);
  Serial.print(' ');
  Serial.print(t);
  Serial.print(' ');
  Serial.println(col);
#endif

}

uint8_t DisplayMatrix::scrollDataSource (uint8_t dev, MD_MAX72XX::transformType_t t)
// Callback function for data that is required for scrolling into the display
{
  static enum { S_IDLE, S_NEXT_CHAR, S_SHOW_CHAR, S_SHOW_SPACE } state = S_IDLE;
  static char   *p;
  static uint16_t curLen, showLen;
  static uint8_t  cBuf[8];
  uint8_t colData = 0;

  // finite state machine to control what we do on the callback
  switch (state)
  {
  case S_IDLE: // reset the message pointer and check for new message to load
//    PRINTS("\n scrollDataSource ->S_IDLE");
    p = curMessage;      // reset the pointer to start of message
    nextState();
    state = S_NEXT_CHAR;
    break;

  case S_NEXT_CHAR: // Load the next character from the font table
//    PRINTS("\nS_NEXT_CHAR");
    if (*p == '\0')
      state = S_IDLE;
    else
    {
//      PRINTC("\nLoading ", *p);
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state = S_SHOW_CHAR;
    }
    break;

  case S_SHOW_CHAR: // display the next part of the character
//    PRINTS("\nS_SHOW_CHAR");
    colData = cBuf[curLen++];
    if (curLen < showLen)
      break;

    // set up the inter character spacing
    showLen = (*p != '\0' ? CHAR_SPACING : (MAX_DEVICES*COL_SIZE)/2);
    curLen = 0;
    state = S_SHOW_SPACE;
    // fall through

  case S_SHOW_SPACE:  // display inter-character spacing (blank column)
//    PRINT("\nS_ICSPACE: ", curLen);
//    PRINT("/", showLen);
    curLen++;
    if (curLen == showLen)
      {
        state = S_NEXT_CHAR;
      }
    break;

  default:
    state = S_IDLE;
  }

//  PRINT("\ncolData: ", colData);
  return(colData);
}

//Call back from timeClient due to second tick
void DisplayMatrix::timeTick (TimeClient &timeClient)
{
//	PRINT("\ntimeTick: ", timeClient.getTemp());
//	PRINT("\ntimeTick: ", timeClient.getFmTime());
	if(!firstUpdateDiplay){
		firstUpdateDiplay = true;
	//    sprintf(Line[S_IDLE].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
	//    Line[S_IDLE].newMessageAvailable = true;

		sprintf(Line[S_TIME].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
		Line[S_TIME].newMessageAvailable = true;

		sprintf(Line[S_DATE].message, " %s", timeClient.getFmTime());
		Line[S_DATE].newMessageAvailable = true;

		sprintf(Line[S_TEMP].message," %s", timeClient.getTemp());
		Line[S_TEMP].newMessageAvailable = true;

		if(!updateMsg.length())
		{
		  sprintf(Line[S_UPDATE].message," %s", updateMsg.c_str());
		  Line[S_UPDATE].newMessageAvailable = true;
		}
		else
		{
		   Line[S_UPDATE].message[0]= '\0';
		   Line[S_UPDATE].newMessageAvailable = false;
		}
		return;

  }

//  PRINT("\ntimeDisplayCount: ", timeDisplayCount);
  timeDisplayCount++;
  if(timeDisplayCount == 1){
	 blink = false;
  }
  if (eDisplayState != S_TIME){
	  sprintf(Line[S_TIME].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
  }

  switch (eDisplayState){
   case S_IDLE:
       //prepair data
//     PRINT("\nS_IDLE: ", eDisplayState);
     blink = false;
     isRunning = true;
     break;

   case S_ALERT:
       //print online msg
     break;

   case S_TIME:
       //print time
//     PRINT("\nS_TIME: ", eDisplayState);
     blink = !blink;
     if(!blink){
      sprintf(Line[S_TIME].message," %02d %02d", timeClient.getHour(), timeClient.getMins());
     }else{
      sprintf(Line[S_TIME].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
     }
     Line[S_TIME].newMessageAvailable = true;

     break;

   case S_DATE:
     //print date
//     PRINT("\nS_DATE: ", eDisplayState);
     sprintf(Line[S_DATE].message, " %s", timeClient.getFmTime());
     Line[S_DATE].newMessageAvailable = true;
//     PRINT("\nLine[S_DATE].message: ", Line[S_DATE].message);
     break;

   case S_TEMP:
//     PRINT("\nS_TEMP: ", eDisplayState);
     sprintf(Line[S_TEMP].message," %s", timeClient.getTemp());
     Line[S_TEMP].newMessageAvailable = true;
//     PRINT("\nLine[S_TEMP].message: ", Line[S_TEMP].message);
     break;

   case S_UPDATE:
//     PRINT("\nS_UPDATE: ", eDisplayState);
     //print info
     if(updateMsg.length() > 0)
       {
         sprintf(Line[S_UPDATE].message," %s", updateMsg.c_str());
         Line[S_UPDATE].newMessageAvailable = true;
//         PRINT("\nS_UPDATE: ", Line[S_UPDATE].message);
       }
     else
     {
         Line[S_UPDATE].message[0]= '\0';
         Line[S_UPDATE].newMessageAvailable = false;
     }
     break;

   default:
     eDisplayState = S_IDLE;
  }
}

void DisplayMatrix::vSetUpdateMsg(char* newMsg){
	PRINTS("\nDisplayMatrix::vSetUpdateMsg: ");

//	sprintf(updateMsg.begin()," %s", newMsg);
	updateMsg = newMsg;

	PRINT("\nupdateMsg: ", updateMsg);

}

void DisplayMatrix::nextState ()
//call when scroll off display
{
//  PRINTS("\n---->nextState(): ");
//  PRINT("\n-----eDisplayState before: ",eDisplayState);
  switch (eDisplayState)
  {
    case S_IDLE:
      eDisplayState = S_TIME;

//      char tmp[20];
//      sprintf(tmp," %s    ", Line[S_TIME].message);
//      strcpy(Line[eDisplayState].message, tmp); // copy it in

//      sprintf(Line[S_TIME].message," %02d:%02d   ", timeClient.getHour(), timeClient.getMins());

      break;

    case S_ALERT:
      eDisplayState = S_IDLE;
      break;

    case S_TIME:
      if(isRunning)
      {
//        isRunning = false;
        timeDisplayCount = 0;
//        bRestart = true;
        return;
      }
      break;
    case S_DATE:
      eDisplayState = S_TEMP;
      break;
    case S_TEMP:
    	if(updateMsg.length() > 0){
    		eDisplayState = S_UPDATE;
//    		PRINT("\n-----eDisplayState after: ",eDisplayState);
    	}

    	else
    		eDisplayState = S_IDLE;

      break;

    case S_UPDATE:
      eDisplayState = S_IDLE;
      break;

    default:
      eDisplayState = S_IDLE;

  }
//  PRINT("\n-----eDisplayState after: ",eDisplayState);
  bRestart = true;
  timeDisplayCount = 0;
  if (Line[eDisplayState].newMessageAvailable || (eDisplayState == S_IDLE))  // there is a new message waiting
  {
    strcpy(curMessage, Line[eDisplayState].message); // copy it in
    Line[eDisplayState].newMessageAvailable = false;
//    PRINT("\n-----curMessage: %s",curMessage);
  }

}

void DisplayMatrix::display ()
{
  if(!firstUpdateDiplay)
    return;

  if(isRunning){
      scrollText();
  }else{
    if (Line[S_TIME].newMessageAvailable){  // there is a new message waiting
      strcpy(curMessage, Line[S_TIME].message); // copy it in
      Line[S_TIME].newMessageAvailable = false;
      printText(curMessage);
    }
  }
}

bool DisplayMatrix::scrollText ()
{
  // are we initializing?
	static uint32_t idx = 0;
	const uint8_t offset = 2; // there is space before text, hence + 3 offset
	if (bRestart)
	{
	//    PRINTS("\n--- Initializing ScrollText");
	//    resetMatrix();
	//    strcpy(curMessage, pmsg);
	bRestart = false;
	}

	// Is it time to scroll the text?
	if (millis() - prevTime >= SCROLL_DELAY)
	{
	  mx.transform(MD_MAX72XX::TSL);
	  if(eDisplayState == S_TIME){
		  idx++;
		  if(idx == (mx.getColumnCount() + offset) ){
			  idx = 0;
//			  PRINTS("\n--- idx == mx.getColumnCount()");
			  isRunning = false;
		  }
	  }else{
		  idx = 0;
	  }
	//	  if(eDisplayState == S_IDLE)
	//		  mx.transform(MD_MAX72XX::TSU);
	//	  else
	//		  mx.transform(MD_MAX72XX::TSL);  // scroll along - the callback will load all the data

	prevTime = millis();      // starting point for next time
	}
	return (bRestart);
}

void DisplayMatrix::resetMatrix(void)
{
//  PRINTS("\n ->resetMatrix() ");
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY/2);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mx.clear();
  prevTime = 0;

}

bool DisplayMatrix::graphicBounceBall(bool bInit)
{
  static uint8_t  idx = 0;      // position
  static int8_t   idOffs = 1;   // increment direction

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- BounceBall init");
    resetMatrix();
    bInit = false;
  }

  // Is it time to animate?
  if (millis()-prevTimeAnim < SCANNER_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  PRINT("\nBB R:", idx);
  PRINT(" D:", idOffs);

  // now run the animation
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  // turn off the old ball
  mx.setColumn(idx, 0);
  mx.setColumn(idx+1, 0);

  idx += idOffs;
  if ((idx == 0) || (idx == mx.getColumnCount()-2))
    idOffs = -idOffs;

  // turn on the new lines
  mx.setColumn(idx, 0x18);
  mx.setColumn(idx+1, 0x18);

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  return(bInit);
}

bool DisplayMatrix::graphicPacman(bool bInit)
{
  #define MAX_FRAMES  4   // number of animation frames
  #define PM_DATA_WIDTH  18
  const uint8_t pacman[MAX_FRAMES][PM_DATA_WIDTH] =  // ghost pursued by a pacman
  {
    { 0x3c, 0x7e, 0x7e, 0xff, 0xe7, 0xc3, 0x81, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
    { 0x3c, 0x7e, 0xff, 0xff, 0xe7, 0xe7, 0x42, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
    { 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xe7, 0x66, 0x24, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
    { 0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe },
  };

  static int16_t idx;        // display index (column)
  static uint8_t frame;      // current animation frame
  static uint8_t deltaFrame; // the animation frame offset for the next frame

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- Pacman init");
    resetMatrix();
    bInit = false;
    idx = -1; //DATA_WIDTH;
    frame = 0;
    deltaFrame = 1;
  }

  // Is it time to animate?
  if (millis() - prevTimeAnim < PACMAN_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  PRINT("\nPAC I:", idx);

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx.clear();

  // clear old graphic
  for (uint8_t i = 0; i<PM_DATA_WIDTH; i++)
    mx.setColumn(idx - PM_DATA_WIDTH + i, 0);
  // move reference column and draw new graphic
  idx++;
  for (uint8_t i = 0; i<PM_DATA_WIDTH; i++)
    mx.setColumn(idx - PM_DATA_WIDTH + i, pacman[frame][i]);

  // advance the animation frame
  frame += deltaFrame;
  if (frame == 0 || frame == MAX_FRAMES - 1)
    deltaFrame = -deltaFrame;

  // check if we are completed and set initialize for next time around
  if (idx == mx.getColumnCount() + PM_DATA_WIDTH) bInit = true;

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  return(bInit);
}

bool DisplayMatrix::graphicArrowRotate(bool bInit)
{
  static uint16_t idx;        // transformation index

  uint8_t arrow[COL_SIZE] =
  {
    0b00000000,
    0b00011000,
    0b00111100,
    0b01111110,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00000000
  };

  MD_MAX72XX::transformType_t  t[] =
  {
    MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR, MD_MAX72XX::TSR,
    MD_MAX72XX::TRC, MD_MAX72XX::TRC,
    MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL, MD_MAX72XX::TSL,
    MD_MAX72XX::TRC,
  };

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- ArrowRotate init");
    resetMatrix();
    bInit = false;
    idx = 0;

    // use the arrow bitmap
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
    for (uint8_t j = 0; j<mx.getDeviceCount(); j++)
      mx.setBuffer(((j + 1)*COL_SIZE) - 1, COL_SIZE, arrow);
    mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  }

  // Is it time to animate?
  if (millis() - prevTimeAnim < ARROWR_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  mx.transform(t[idx++]);
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);

  // check if we are completed and set initialize for next time around
  if (idx == (sizeof(t) / sizeof(t[0]))) bInit = true;

  return(bInit);
}

bool DisplayMatrix::graphicInvader(bool bInit)
{
  const uint8_t invader1[] = { 0x0e, 0x98, 0x7d, 0x36, 0x3c };
  const uint8_t invader2[] = { 0x70, 0x18, 0x7d, 0xb6, 0x3c };
  const uint8_t dataSize = (sizeof(invader1)/sizeof(invader1[0]));

  static int8_t idx;
  static bool   iType;

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- Invader init");
    resetMatrix();
    bInit = false;
    idx = -dataSize;
    iType = false;
  }

  // Is it time to animate?
  if (millis()-prevTimeAnim < INVADER_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  // now run the animation
  PRINT("\nINV I:", idx);

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  mx.clear();
  for (uint8_t i=0; i<dataSize; i++)
  {
    mx.setColumn(idx-dataSize+i, iType ? invader1[i] : invader2[i]);
    mx.setColumn(idx+dataSize-i-1, iType ? invader1[i] : invader2[i]);
  }
  idx++;
  if (idx == mx.getColumnCount()+(dataSize*2)) bInit = true;
  iType = !iType;
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  return(bInit);
}

bool DisplayMatrix::graphicSinewave(bool bInit)
{
  static uint8_t curWave = 0;
  static uint8_t idx;

  #define SW_DATA_WIDTH  11    // valid data count followed by up to 10 data points
  const uint8_t waves[][SW_DATA_WIDTH] =
  {
    {  9,   8,  6,   1,   6,  24,  96, 128,  96,  16,   0 },
    {  6,  12,  2,  12,  48,  64,  48,   0,   0,   0,   0 },
    { 10,  12,   2,   1,   2,  12,  48,  64, 128,  64, 48 },

  };
  const uint8_t WAVE_COUNT = sizeof(waves) / (SW_DATA_WIDTH * sizeof(uint8_t));

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- Sinewave init");
    resetMatrix();
    bInit = false;
    idx = 1;
  }

  // Is it time to animate?
  if (millis() - prevTimeAnim < SINE_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::ON);
  mx.transform(MD_MAX72XX::TSL);
  mx.setColumn(0, waves[curWave][idx++]);
  if (idx > waves[curWave][0])
  {
    curWave = random(WAVE_COUNT);
    idx = 1;
  }
  mx.control(MD_MAX72XX::WRAPAROUND, MD_MAX72XX::OFF);

  return(bInit);
}

bool DisplayMatrix::graphicSpectrum2(bool bInit)
{
  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- Spectrum2init");
    resetMatrix();
    bInit = false;
  }

  // Is it time to animate?
  if (millis() - prevTimeAnim < SPECTRUM_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  // now run the animation
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);
  for (uint8_t i = 0; i<mx.getColumnCount(); i++)
  {
    uint8_t r = random(ROW_SIZE);
    uint8_t cd = 0;

    for (uint8_t j = 0; j<r; j++)
      cd |= 1 << j;

    mx.setColumn(i, ~cd);
  }
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  return(bInit);
}

bool DisplayMatrix::graphicScroller(bool bInit)
{
  const uint8_t   width = 3;     // width of the scroll bar
  const uint8_t   offset = mx.getColumnCount()/3;
  static uint8_t  idx = 0;      // counter

  // are we initializing?
  if (bInit)
  {
    PRINTS("\n--- Scroller init");
    resetMatrix();
    idx = 0;
    bInit = false;
  }

  // Is it time to animate?
  if (millis()-prevTimeAnim < SCANNER_DELAY)
    return(bInit);
  prevTimeAnim = millis();    // starting point for next time

  PRINT("\nS I:", idx);

  // now run the animation
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::OFF);

  mx.transform(MD_MAX72XX::TSL);

  mx.setColumn(0, idx>=0 && idx<width ? 0xff : 0);
  if (++idx == offset) idx = 0;

  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);

  return(bInit);
}

void DisplayMatrix::runWelcomeDisplay(void){
//	bRestart = graphicArrowRotate(bRestart);
	bRestart = graphicPacman(bRestart);
//	bRestart= graphicBounceBall(bRestart);
//	bRestart= graphicInvader(bRestart);
//	bRestart= graphicSinewave(bRestart);
//	bRestart= graphicSpectrum2(bRestart);
//	bRestart = graphicScroller(bRestart);
}
bool DisplayMatrix::printText (const char *pmsg)
{
  // are we initializing?
    if (bRestart)
    {
//      PRINTS("\n--- printText init");
      resetMatrix();
      bRestart = false;
    }
    graphicText(0, MAX_DEVICES -1 , pmsg);
    return (bRestart);
}

void DisplayMatrix::bRestartDisplay(void){
	bRestart = true;
	eDisplayState = S_IDLE;
	printText("  ***  ");
	isRunning = false;

}

void DisplayMatrix::runMatrixAnimation (void)
{

  switch (eDisplayState)
  {
    case S_IDLE:
//      eDisplayState = S_TIME;
      isRunning = true;
      break;

    case S_ALERT:
      break;
    case S_TIME:
      if(timeDisplayCount >= MAX_DISPLAY_TIME)
        {
          isRunning = true;
          eDisplayState = S_DATE;
          if (Line[eDisplayState].newMessageAvailable)  // there is a new message waiting
          {
            strcpy(curMessage, Line[eDisplayState].message); // copy it in
            Line[eDisplayState].newMessageAvailable = false;
//            PRINT("\n-----curMessage: ",curMessage);
          }

        }

      break;
    case S_DATE:
//          eDisplayState = S_TEMP;
      break;
    case S_TEMP:
//          eDisplayState = S_UPDATE;
      break;
    case S_UPDATE:
//          eDisplayState = S_IDLE;
      break;

    default:
      eDisplayState = S_IDLE;

  }
  display();

}


