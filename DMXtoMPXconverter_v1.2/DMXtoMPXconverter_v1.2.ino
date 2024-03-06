/*
DMX_to_MPX.ino - reads an arbitrary number of channels (up to 128) from DMX and outputs them on MPX
Copyright (c) 2014 by Dennis Engdahl <engdahl@snowcrest.net>. All rights reserved.
Modified Feb 20, 2024 by Scott Oberlin.

 DMX_Slave.ino - Example code for using the Conceptinetics DMX library
 Copyright (c) 2013 W.A. van der Meeren <danny@illogic.nl>.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 3 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Rdm_Defines.h>
#include <Rdm_Uid.h>
#include "Touchscreen_Processor.h"
#include <Conceptinetics.h>
#include <EEPROM.h>
#include "Fonts/Monospaced_plain_72.h"
#include "Fonts/lock_icon.h"
#include "Fonts/FreeSans18pt7bModified.h"

#define RXEN_PIN 2
#define SYNCBIT  3
#define PWMOUT   9
#define TRIGGER 12

// Configure a DMX slave controller
// Create a pointer to allow instantiating the object dynamically. This allows updating the number of channels by reading from EEPROM on startup.
DMX_Slave* dmx_slave;

static uint8_t DMX_SLAVE_CHANNELS = 64; // Max channels for MicroPlex is 128
const uint8_t ledPin = 4;
static unsigned char timer1cnt = 0;
static unsigned char outcnt = 0;
static unsigned char sync5ms = 1;
static unsigned long Frames = 0;
const unsigned long dmxTimeoutMillis = 10000UL;
const int DMX_DATA_SIGNATURE = 0xBAAFDEED;
unsigned long time_now = 0;
unsigned long time_now2 = 0;
int screenPage = 0;
bool screen1Rendered = false;
bool screen2Rendered = false;
bool touchReleased = true;
touchState touchstate = 0;
settingsToUpdate whichSettings = 0;
TS_Point p1;

#define MENU1_BTN_CNT 4
Adafruit_GFX_Button Menu1Buttons[MENU1_BTN_CNT];
char Menu1Labels[MENU1_BTN_CNT][5] = {"Up", "Sel", "Down", "Ret"};
uint16_t Menu1Colors[MENU1_BTN_CNT] = {ILI9341_DARKGREY, ILI9341_DARKGREY, 
                               ILI9341_DARKGREY, ILI9341_DARKGREY};

#define MENU2_BTN_CNT 4
Adafruit_GFX_Button Menu2Buttons[MENU2_BTN_CNT];
char Menu2Labels[MENU2_BTN_CNT][5] = {"Up", "Sel", "Down", "Ret"};
uint16_t Menu2Colors[MENU2_BTN_CNT] = {ILI9341_BLUE, ILI9341_BLUE, 
                               ILI9341_BLUE, ILI9341_BLUE};

struct converterSettings{
  // start with a 1-1 mapping of channels
  int16_t DMX_start = 1;
  int16_t DMX_end = 64;
  int16_t MPX_start = 1;
  int16_t MPX_end = 64;
};

converterSettings dmxSettings;

void(* resetFunc) (void) = 0;

void readDMXSettings(converterSettings& settings) {
  int signature;
  uint16_t storedAddress = 128;
  EEPROM.get(storedAddress, signature);
  // write signature if it doesn't exist or is invalid
  if (signature != DMX_DATA_SIGNATURE) {
    EEPROM.put(storedAddress, DMX_DATA_SIGNATURE);
    EEPROM.put(storedAddress + sizeof(signature), settings);
  }
  EEPROM.get(storedAddress + sizeof(signature), settings);
}

bool writeDMXSettings(converterSettings& settings) {
  // none of the previous rigamarole required, as PUT doesn't update EEPROM bytes unless needed.
  int signature;
  uint16_t storedAddress = 128;
  EEPROM.put(storedAddress + sizeof(signature), settings);
}

// timer1 OVERFLOW -- Every 16 microseconds
ISR(TIMER1_OVF_vect) {
  if ((timer1cnt & 0x0f) != 0) {
    timer1cnt++;
    return;
  }
  if (timer1cnt == 0) {
    OCR1A = 0;    // Send sync pulse
    pinMode (SYNCBIT, OUTPUT);
    digitalWrite (SYNCBIT, HIGH);    // turn on sync bit
    if (sync5ms == 1) {
      //digitalWrite (TRIGGER, HIGH);
    }
  }
  else if (timer1cnt == 160 && sync5ms >= 1) {
    if (sync5ms == 1) {
      sync5ms = 2;
      timer1cnt = 0;
    }
    else {
      sync5ms = 0;  // End of 5ms sync
      timer1cnt = 16;
      outcnt = 0;
      //digitalWrite (TRIGGER, LOW);
    }
  }
  if (timer1cnt == 16 && sync5ms == 0) {
    pinMode (SYNCBIT, INPUT);  // turn off sync
    if (outcnt > DMX_SLAVE_CHANNELS) {
      outcnt = 0;  // end of channels, reset
      sync5ms = 1;
      timer1cnt = 0;
    }
    else {
      OCR1A = dmx_slave->getChannelValue (++outcnt);
      timer1cnt++;
    }
  }
  else if (timer1cnt == 32 && sync5ms == 0) {
    timer1cnt = 0;
  }
  else timer1cnt++;
}

// the setup routine runs once when you press reset:
void setup() {
  
  /*
    Todo:
    - add EEPROM reading code for start address and TFT touchscreen calibration data.
    DONE - add TFT Display setup code. Maybe call separate .ino file?
  */
  initTouchscreen(); //done in calibratedisplay.cpp

  readDMXSettings(dmxSettings);

  DMX_SLAVE_CHANNELS = dmxSettings.DMX_end - dmxSettings.DMX_start + 1;

  // Dynamically instantiate dmx slave object so we can set the number of channels on startup instead of at compile time
  dmx_slave = new DMX_Slave(DMX_SLAVE_CHANNELS , RXEN_PIN );

  // Enable DMX slave interface and start recording
  // DMX data
  dmx_slave->enable ();

  // Set start address to 1, this is also the default setting
  // You can change this address at any time during the program
  dmx_slave->setStartAddress (1);

  // Register on frame complete event to determine signal timeout
  
  dmx_slave->onReceiveComplete ( OnFrameReceiveComplete );

  // Setup led pin as output pin
  pinMode ( ledPin, OUTPUT );
  pinMode ( PWMOUT, OUTPUT );
  //pinMode ( TRIGGER, OUTPUT);

  // new code and comments:
  // set up timer 1 for pwm
  // clear OCR1A on compare match
  // Update OCR1A at BOTTOM, non-inverting mode 
  // Fast PWM, 8bit
  TCCR1A = (1 << COM1A1) | (1 << WGM10);
  // Prescaler: clk/1 = 16MHz
  // PWM frequency = 16MHz / (255 + 1) = 62.5kHz
  TCCR1B =  (1 << CS10) | (1 << WGM12);
  // set initial duty cycle to zero */
  OCR1A = 0;
  TIMSK1 = (1 << TOIE1);	// interrupt on overflow

  sei(); //Enable interrupts

}

void loop()
{
  // If we didn't receive a DMX frame within the timeout period
  // clear all dmx channels
  if ( ++Frames > 250000 ) {
    dmx_slave->getBuffer().clear();
    Frames = 0;
    digitalWrite ( ledPin, LOW );
    //PORTD &= !B00010000; // turn off LED via port manipulation
  }
  //
  // If the first channel comes above 1% the led will switch on
  // and below 1% the led will be turned off
  //
  /*
  if ( dmx_slave->getChannelValue (1) > 1 )
    digitalWrite ( ledPin, HIGH );
  else
    digitalWrite ( ledPin, LOW ); 
*/
  //check touchscreen every 33 ms or 30fps
  processTouchscreen();
  // if (millis() - time_now > 33) {
  //   time_now = millis();

  // }
}

void OnFrameReceiveComplete (unsigned short nrchannels)
{
  Frames = 0;
  digitalWrite ( ledPin, HIGH );
  //PORTD |= B00010000; //turn on LED via port manipulation
}

void processTouchscreen() {
  if(!isCalibrated) {
    if (!isCalibrating) {
    //  beginCalibration();
    }
    
    //processCalibrationInput();
  }
  else
  {    
    // check which screen we are on, and redraw screens if needed.
    switch(screenPage){
      case 0:
        if (!screen1Rendered) {
          screen2Rendered = false;
          redrawHomeScreen();
          screen1Rendered = true;
        }
        //do any other logic on the screen here. How do we figure out how to touch a button?
        if (processTouchInput())
        {
          // get point, step through all touchable areas and update screen areas accordingly, change screenpage as needed 
          if(checkBounds(0, 53, 135, 108)){
            whichSettings = DMX_START_ADDRESS;
            screenPage = 1;
          }
          else if(checkBounds(180, 53, 310, 108)){
            whichSettings = DMX_END_ADDRESS;
            screenPage = 1;
          }
          else if(checkBounds(0, 175, 135, 235)){
            whichSettings = MPX_START_ADDRESS;
            screenPage = 1;
          }
          else if(checkBounds(185, 175, 310, 235)){
            whichSettings = MPX_END_ADDRESS;
            screenPage = 1;
          }
        }
      break;
      case 1:
        if (!screen2Rendered) {
          screen1Rendered = false;
          redrawAdjustScreen();
          screen2Rendered = true;
        }
        if (processTouchInput())
        {
          if(checkBounds(270, 18, 310, 116)){
            screenPage = 0;
          }
          if(checkBounds(270, 126, 310, 224)){
            screenPage = 0;
          }
        }
      break;
      case 2:
        drawRestartingScreen();
        delay(2000);
//        resetFunc();
      break;
    }
  }
}

bool checkBounds(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
  if(p1.x >= xStart && p1.x <= xEnd && p1.y >= yStart && p1.y <= yEnd) {
    return true;
  }
  else {
    return false;
  }
}

bool processTouchInput() {
  TS_Point p2;
  boolean isTouched = ts->touched();
  
  if (isTouched) {
        p2 = ts->getPoint(); // update touch point coords
        ts_display->mapTStoDisplay(p2.x, p2.y, &p1.x, &p1.y); //map to display
  }
  switch (touchstate) {
    case STATE_WAIT:
      if (isTouched) {
        //p1 = ts->getPoint(); 
        touchstate = STATE_WAIT_TOUCHED;
      }
    break;
    case STATE_WAIT_TOUCHED:
      if (!isTouched) {
        touchstate = STATE_RELEASED;
        tft->setCursor(0,23);
        tft->setFont(&FreeSans9pt7b);
        tft->setTextColor(ILI9341_WHITE);
        tft->fillRect(0, 0, 100, 28, ILI9341_BLACK);
        tft->print(p1.x, 10);
        tft->print(",");
        tft->print(p1.y, 10); 
        return true;
      }
    break;
    case STATE_RELEASED:
      if (!isTouched) {
        touchstate = STATE_WAIT; // once the touchscreen has been marked as released, we can revert to the "waiting for a touch" stage.
      }
    break;
  }
  return false;
}

void drawLockIcon(bool locked) 
{
  tft->setTextColor(ILI9341_LIGHTGREY);
  tft->setCursor(274,175);
  tft->setFont(&lock_icon);
  if(locked)
  {
    tft->print("L");
  }
  else {
    tft->print("M");
  }
}

void drawSettingsHeader(uint16_t x, uint16_t y, uint16_t color, char *text)
{
  tft->setFont(&FreeSans18pt7bModified);
  tft->setTextSize(1);
  tft->setCursor(x, y);
  tft->setTextColor(color);
  tft->print(text);
}

void drawSettingsValue(uint16_t x, uint16_t y, uint16_t color, uint16_t setting1, uint16_t setting2) {

  char buf[3];

  sprintf(buf, "%03d", setting1);
  tft->setFont(&Monospaced_plain_72);
  tft_printf(x, y, color, buf);
  tft->print("-");
  sprintf(buf, "%03d", setting2);
  tft->print(buf);
}

void redrawHomeScreen() {
  

  tft->fillScreen(COLOR_BKGD);
  drawSettingsHeader(6, 40, ILI9341_ORANGE, "DMX in:");

  drawSettingsValue(6,105, ILI9341_WHITE, dmxSettings.DMX_start, dmxSettings.DMX_end);
  drawSettingsHeader(6, 160, ILI9341_CYAN, "MPX out:");

  drawLockIcon(false);

  drawSettingsValue(6,225, ILI9341_WHITE, dmxSettings.MPX_start, dmxSettings.MPX_end);
}

void redrawAdjustScreen() {

  // clear screen
  tft->fillScreen(COLOR_BKGD);

  tft->setTextColor(ILI9341_WHITE);
  tft->setFont(&Monospaced_plain_72);
  tft->setTextSize(2);
  tft->setCursor(0, 74);
  tft->print("...");
  
  char buf2[3];
  sprintf(buf2, "%03d", getDmxSettingToUpdate());
  tft->setCursor(1, 172);
  //tft_printf(1, 172, ILI9341_WHITE, buf2);
  tft->print(buf2);
  
  tft->setCursor(0, 271);
  tft->print("///");

  //print ok button
  renderOkBtn(270, 18, 40, 98);
  renderCancelBtn(270, 126, 40, 98);
}

uint16_t getDmxSettingToUpdate() {
  switch (whichSettings) {
    case DMX_START_ADDRESS:
    return dmxSettings.DMX_start;
    case DMX_END_ADDRESS:
    return dmxSettings.DMX_end;
    case MPX_START_ADDRESS:
    return dmxSettings.MPX_start;
    case MPX_END_ADDRESS:
    return dmxSettings.MPX_end;
  }
}

void drawRestartingScreen() {
  tft->fillScreen(COLOR_BKGD);
  tft->setFont(&FreeSans9pt7b);
  tft->setCursor(80, 130);
  tft->setTextColor(ILI9341_GREEN);
  tft->setTextSize(2);
  tft->print("Restarting...");
}

void renderOkBtn(int16_t x, int16_t y, int16_t width, int16_t height) {

  tft->fillRect(x, y, width, height, ILI9341_GREEN);

  int startX = x+((width - 24)/2);
  int startY = y+(((height - 38)/2) + 34);
  int bottomX = startX + 8;
  int bottomY = startY + 8;
  int topX = bottomX + 16;
  int topY = startY - 38;

  for (int i = -1; i <= 2; i++) {
    tft->drawLine(startX + i, startY, bottomX + i, bottomY, ILI9341_BLACK);
  }
  for(int i = 0; i < 3; i++){
    tft->drawLine(bottomX + i, bottomY, topX + i, topY, ILI9341_BLACK);
  }
}

void renderCancelBtn(int16_t x, int16_t y, int16_t width, int16_t height){
  tft->fillRect(x, y, width, height, ILI9341_RED);

  int startX = x+((width - 24)/2);
  int startY = y+(((height - 38)/2) + 34);
  int topX = startX + 24;
  int topY = startY - 32;

  // draw more than one line in each direction to thicken the X and make it readable
  for (int i = -2; i <= 1; i++) {
    tft->drawLine(startX + i, startY, topX + i, topY, ILI9341_BLACK);
  }

  for(int i = -2; i <= 1; i++) {
    tft->drawLine(startX+i, topY, topX+i, startY, ILI9341_BLACK);
  }
  
}