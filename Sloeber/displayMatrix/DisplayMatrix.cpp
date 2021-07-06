/*
 * DisplayMatrix.cpp
 *
 *  Created on: Mar 5, 2021
 *      Author: travu
 */


#include "DisplayMatrix.h"
#include "../utils.h"
//#include <MD_MAX72xx.h>
//#include <SPI.h>

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

struct LineDefinition
{
  char    message[MESG_SIZE];      // message for this display
  boolean newMessageAvailable;    // true if new message arrived
};

struct LineDefinition  Line[MAX_LINE_MSG] ;

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW //MD_MAX72XX::PAROLA_HW  //edit this as per your LED matrix hardware type
MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);


char curMessage[MESG_SIZE];
//char newMessage[MESG_SIZE];
//char listMsg[MESG_SIZE][MAX_LINE_MSG];

enum {S_IDLE, S_TIME, S_DATE, S_TEMP, S_UPDATE, S_ALERT} eDisplayState = S_IDLE;

bool firstUpdateDiplay = false;
int timeDisplayCount = 0;

bool isRunning = true;
bool newMessageAvailable =  false;
uint32_t prevTime = 0;

bool bRestart = true;

//bool changeState = false;

String updateMsg = "";

//char WebResponse[] = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
//
//char WebPage[] =
//"<!DOCTYPE html>" \
//"<html>" \
//  "<head>" \
//    "<title>SMART CLOCK</title>" \
//    "<style>" \
//    "html, body" \
//    "{" \
//    "width: 600px;" \
//    "height: 400px;" \
//    "margin: 0px;" \
//    "border: 0px;" \
//    "padding: 10px;" \
//    "background-color: white;" \
//    "}" \
//    "#container " \
//    "{" \
//    "width: 100%;" \
//    "height: 100%;" \
//    "margin-left: 200px;" \
//    "border: solid 2px;" \
//    "padding: 10px;" \
//    "background-color: #b3cbf2;" \
//    "}" \
//    "</style>"\
//    "<script>" \
//    "strLine = \"\";" \
//    "function SendText()" \
//    "{" \
//    "  nocache = \"/&nocache=\" + Math.random() * 1000000;" \
//    "  var request = new XMLHttpRequest();" \
//    "  strLine = \"&MSG=\" + document.getElementById(\"txt_form\").Message.value;" \
//    "  request.open(\"GET\", strLine + nocache, false);" \
//    "  request.send(null);" \
//    "}" \
//    "</script>" \
//  "</head>" \
//  "<body>" \
//    "<div id=\"container\">"\
//      "<H1><b>WiFi MAX7219 LED Matrix Display</b></H1>" \
//      "<form id=\"txt_form\" name=\"frmText\">" \
//      "<label>Msg:<input type=\"text\" name=\"Message\" maxlength=\"255\"></label><br><br>" \
//      "</form>" \
//      "<br>" \
//      "<input type=\"submit\" value=\"Send Text\" onclick=\"SendText()\">" \
//    "</div>" \
//  "</body>" \
//
//"</html>";



DisplayMatrix::DisplayMatrix ()
{
//  this->newMessageAvailable = false;
  this->blink = false;
}

DisplayMatrix::~DisplayMatrix ()
{
}

void DisplayMatrix::init ()
{
  // Display initialization
  mx.begin();
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


char* DisplayMatrix::err2Str (wl_status_t code)
{
  switch (code)
    {
    case WL_IDLE_STATUS:    return("IDLE");           break; // WiFi is in process of changing between statuses
    case WL_NO_SSID_AVAIL:  return("NO_SSID_AVAIL");  break; // case configured SSID cannot be reached
    case WL_CONNECTED:      return("CONNECTED");      break; // successful connection is established
    case WL_CONNECT_FAILED: return("CONNECT_FAILED"); break; // password is incorrect
    case WL_DISCONNECTED:   return("CONNECT_FAILED"); break; // module is not configured in station mode
    default: return("??");
    }
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

//void DisplayMatrix::handleWiFi (void)
//{
//  static enum { S_IDLE, S_WAIT_CONN, S_READ, S_EXTRACT, S_RESPONSE, S_DISCONN } state = S_IDLE;
//  static char szBuf[1024];
//  static uint16_t idxBuf = 0;
//  static WiFiClient client;
//  static uint32_t timeStart;
//
//  switch (state)
//  {
//  case S_IDLE:   // initialise
//    PRINTS("\nS_IDLE");
//    idxBuf = 0;
//    state = S_WAIT_CONN;
//    break;
//
//  case S_WAIT_CONN:   // waiting for connection
//    {
//      client = server.available();
//      if (!client) break;
//      if (!client.connected()) break;
//
//#if DEBUG
//      char szTxt[20];
//      sprintf(szTxt, "%03d:%03d:%03d:%03d", client.remoteIP()[0], client.remoteIP()[1], client.remoteIP()[2], client.remoteIP()[3]);
//      PRINT("\nNew client @ ", szTxt);
//#endif
//
//      timeStart = millis();
//      state = S_READ;
//    }
//    break;
//
//  case S_READ: // get the first line of data
//    PRINTS("\nS_READ");
//    while (client.available())
//    {
//      char c = client.read();
//      if ((c == '\r') || (c == '\n'))
//      {
//        szBuf[idxBuf] = '\0';
//        client.flush();
//        PRINT("\nRecv: ", szBuf);
//        state = S_EXTRACT;
//      }
//      else
//        szBuf[idxBuf++] = (char)c;
//    }
//    if (millis() - timeStart > 1000)
//    {
//      PRINTS("\nWait timeout");
//      state = S_DISCONN;
//    }
//    break;
//
//
//  case S_EXTRACT: // extract data
//    PRINTS("\nS_EXTRACT");
//    // Extract the string from the message if there is one
//    newMessageAvailable = getText(szBuf, newMessage, MESG_SIZE);
//    PRINT("\nNew Msg: ", newMessage);
//    state = S_RESPONSE;
//    eDisplayState = S_ALERT; // set display state to alert mode ;
//    break;
//
//  case S_RESPONSE: // send the response to the client
//    PRINTS("\nS_RESPONSE");
//    // Return the response to the client (web page)
//    client.print(WebResponse);
//    client.print(WebPage);
//    state = S_DISCONN;
//    break;
//
//  case S_DISCONN: // disconnect client
//    PRINTS("\nS_DISCONN");
//    client.flush();
//    client.stop();
//    state = S_IDLE;
//    break;
//
//  default:  state = S_IDLE;
//  }
//}







void DisplayMatrix::graphicText (uint8_t modStart, uint8_t modEnd,const char *pMsg)
// Print the text string to the LED matrix modules specified.
// Message area is padded with blank columns after printing.
{
  uint8_t   state = 0;
  uint8_t   curLen;
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
    PRINTS("\n scrollDataSource ->S_IDLE");
    p = curMessage;      // reset the pointer to start of message
    nextState();
    state = S_NEXT_CHAR;
    break;

  case S_NEXT_CHAR: // Load the next character from the font table
    PRINTS("\nS_NEXT_CHAR");
    if (*p == '\0')
      state = S_IDLE;
    else
    {
      PRINTC("\nLoading ", *p);
      showLen = mx.getChar(*p++, sizeof(cBuf) / sizeof(cBuf[0]), cBuf);
      curLen = 0;
      state = S_SHOW_CHAR;
    }
    break;

  case S_SHOW_CHAR: // display the next part of the character
    PRINTS("\nS_SHOW_CHAR");
    colData = cBuf[curLen++];
    if (curLen < showLen)
      break;

    // set up the inter character spacing
    showLen = (*p != '\0' ? CHAR_SPACING : (MAX_DEVICES*COL_SIZE)/2);
    curLen = 0;
    state = S_SHOW_SPACE;
    // fall through

  case S_SHOW_SPACE:  // display inter-character spacing (blank column)
    PRINT("\nS_ICSPACE: ", curLen);
    PRINT("/", showLen);
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
  if(!firstUpdateDiplay){
    firstUpdateDiplay = true;
//    sprintf(Line[S_IDLE].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
//    Line[S_IDLE].newMessageAvailable = true;

    sprintf(Line[S_TIME].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
    Line[S_TIME].newMessageAvailable = true;

    sprintf(Line[S_DATE].message, " %s", timeClient.getFmTime().c_str());
    Line[S_DATE].newMessageAvailable = true;

    sprintf(Line[S_TEMP].message," %s", timeClient.getTemp().c_str());
    Line[S_TEMP].newMessageAvailable = true;

    if(!updateMsg.isEmpty())
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

  PRINT("\ntimeDisplayCount: ", timeDisplayCount);
  timeDisplayCount++;
  if(timeDisplayCount == 1)
    {
     blink = true;
    }
  switch (eDisplayState){
   case S_IDLE:
       //prepair data
     PRINT("\nS_IDLE: ", eDisplayState);
//     sprintf(Line[S_IDLE].message," %02d:%02d", timeClient.getHour(), timeClient.getMins());
//     Line[S_IDLE].newMessageAvailable = true;
     isRunning = true;
     break;

   case S_ALERT:
       //print online msg
     break;

   case S_TIME:
       //print time
     PRINT("\nS_TIME: ", eDisplayState);
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
     PRINT("\nS_DATE: ", eDisplayState);
     sprintf(Line[S_DATE].message, " %s", timeClient.getFmTime().c_str());
     Line[S_DATE].newMessageAvailable = true;
     break;

   case S_TEMP:
     PRINT("\nS_TEMP: ", eDisplayState);
     sprintf(Line[S_TEMP].message," %s", timeClient.getTemp().c_str());
     Line[S_TEMP].newMessageAvailable = true;

     break;

   case S_UPDATE:
     PRINT("\nS_UPDATE: ", eDisplayState);
     //print info
     if(!updateMsg.isEmpty())
       {
         sprintf(Line[S_UPDATE].message," %s", updateMsg.c_str());
         Line[S_UPDATE].newMessageAvailable = true;
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

void DisplayMatrix::nextState ()
//call when scroll off display
{
  PRINTS("\n---->nextState(): ");
  PRINT("\n-----eDisplayState before: %d",eDisplayState);
  switch (eDisplayState)
  {
    case S_IDLE:
      eDisplayState = S_TIME;

      break;

    case S_ALERT:
      eDisplayState = S_IDLE;
      break;

    case S_TIME:
      if(isRunning)
      {
        isRunning = false;
        timeDisplayCount = 0;
        bRestart = true;
        return;
      }
      break;
    case S_DATE:
      eDisplayState = S_TEMP;
      break;
    case S_TEMP:
      eDisplayState = S_UPDATE;
      break;

    case S_UPDATE:
      eDisplayState = S_IDLE;
      break;

    default:
      eDisplayState = S_IDLE;

  }
  PRINT("\n-----eDisplayState after: %d",eDisplayState);
  bRestart = true;
  timeDisplayCount = 0;
  if (Line[eDisplayState].newMessageAvailable || (eDisplayState == S_IDLE))  // there is a new message waiting
  {
    strcpy(curMessage, Line[eDisplayState].message); // copy it in
    Line[eDisplayState].newMessageAvailable = false;
    PRINT("\n-----curMessage: %s",curMessage);
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
  if (bRestart)
  {
    PRINTS("\n--- Initializing ScrollText");
//    resetMatrix();
//    strcpy(curMessage, pmsg);
    bRestart = false;
  }

  // Is it time to scroll the text?
  if (millis() - prevTime >= SCROLL_DELAY)
  {
    mx.transform(MD_MAX72XX::TSL);  // scroll along - the callback will load all the data
    prevTime = millis();      // starting point for next time
  }
  return (bRestart);
}

void DisplayMatrix::resetMatrix(void)
{
  PRINTS("\n ->resetMatrix() ");
  mx.control(MD_MAX72XX::INTENSITY, MAX_INTENSITY/2);
  mx.control(MD_MAX72XX::UPDATE, MD_MAX72XX::ON);
  mx.clear();

}

bool DisplayMatrix::printText (const char *pmsg)
{
  // are we initializing?
    if (bRestart)
    {
      PRINTS("\n--- printText init");
      resetMatrix();
      bRestart = false;
    }
    graphicText(0, MAX_DEVICES -1 , pmsg);
    return (bRestart);
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
            PRINT("\n-----curMessage: %s",curMessage);
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


