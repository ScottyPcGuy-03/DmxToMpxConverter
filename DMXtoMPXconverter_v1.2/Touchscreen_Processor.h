#ifndef TOUCHSCREEN_PROCESSOR_H
#define TOUCHSCREEN_PROCESSOR_H

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <Adafruit_ILI9341.h>
#include <TS_Display.h>
#include "Fonts/FreeSans9pt7b.h"
#include <monitor_printf.h>
#include <EEPROM.h>

#define EEPROM_EMULATION_SIZE     (4 * 64)
#define FLASH_DEBUG       0

#define TFT_CS_PIN    10
#define TFT_DC_PIN    8
#define TFT_LED_PIN   A2
#define TFT_RST 7
#define TOUCH_CS_PIN  6
#define TOUCH_IRQ_PIN A7

#define ROTATION 1

#define PLUS_ARM_LEN 10

#define COLOR_BKGD ILI9341_BLACK
#define COLOR_TEXT ILI9341_YELLOW
#define COLOR_PLUS_CALIB ILI9341_BLUE
#define COLOR_PLUS_TEST ILI9341_GREEN

#define TEXT_TAP_PLUS "Tap the +"
#define TEXT_TAP_RESULT "Tap to complete calibration"
#define TEXT_TAP_TEST "Tap to test calibration"

#define TFT_PRINTF_BUF_SIZE 80

typedef enum _eState {
  STATE_WAIT_UL,
  STATE_WAIT_UL_RELEASE,
  STATE_WAIT_LR,
  STATE_WAIT_LR_RELEASE,
  STATE_WAIT_SHOW_CALIB,
  STATE_WAIT_RELEASE,
  STATE_WAIT_POINT_SHOW_IT
} eState;

typedef enum _touchState {
  STATE_WAIT,
  STATE_WAIT_TOUCHED,
  STATE_RELEASED
} touchState;

typedef enum _settingsToUpdate {
  DMX_START_ADDRESS,
  DMX_END_ADDRESS,
  MPX_START_ADDRESS,
  MPX_END_ADDRESS
} settingsToUpdate;

struct nonvolatileSettings {
  int16_t TS_LR_X;
  int16_t TS_LR_Y;
  int16_t TS_UL_X;
  int16_t TS_UL_Y;
};

extern Adafruit_ILI9341* tft;
extern XPT2046_Touchscreen* ts;
extern TS_Display* ts_display;
extern nonvolatileSettings NVsettings;
extern int16_t x_ULcorner, y_ULcorner, x_LRcorner, y_LRcorner;
extern int16_t TSx_ULcorner, TSy_ULcorner, TSx_LRcorner, TSy_LRcorner;
extern eState state;
extern char TFTprintfBuf[TFT_PRINTF_BUF_SIZE];
extern bool isCalibrated;
extern bool isCalibrating;

void tft_printf(int16_t x, int16_t y, int16_t color, const char* format, ...);
void drawPlus(int16_t x, int16_t y, int16_t color, uint8_t len = PLUS_ARM_LEN);
void showTS_XY(int16_t x, int16_t y, int16_t TSx, int16_t TSy);
bool readNonvolatileSettings(nonvolatileSettings& settings, const nonvolatileSettings& defaults);
bool writeNonvolatileSettingsIfChanged(nonvolatileSettings& settings);
void showNVsettings(char* title, const nonvolatileSettings& settings);
void initTouchscreen();
void processCalibrationInput();
void beginCalibration();

#endif
