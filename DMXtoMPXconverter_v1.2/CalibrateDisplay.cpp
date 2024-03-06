#include "Touchscreen_Processor.h"

Adafruit_ILI9341* tft;
XPT2046_Touchscreen* ts;
TS_Display* ts_display;
nonvolatileSettings NVsettings;
int16_t x_ULcorner, y_ULcorner, x_LRcorner, y_LRcorner;
int16_t TSx_ULcorner, TSy_ULcorner, TSx_LRcorner, TSy_LRcorner;
int16_t x_UL, y_UL, x_LR, y_LR;
int16_t TSx_UL, TSy_UL, TSx_LR, TSy_LR;
eState state;
char TFTprintfBuf[TFT_PRINTF_BUF_SIZE];
const int WRITTEN_SIGNATURE = 0xBEEFDEED;
bool isCalibrated = true;
bool isCalibrating = false;

void tft_printf(int16_t x, int16_t y, int16_t color, const char* format, ...) {
  tft->setCursor(x, y);
  tft->setTextColor(color);
  va_list args;
  va_start(args, format);
  vsnprintf(TFTprintfBuf, TFT_PRINTF_BUF_SIZE, format, args);
  va_end(args);
  tft->print(TFTprintfBuf);
}

void drawPlus(int16_t x, int16_t y, int16_t color, uint8_t len) {
  tft->drawFastVLine(x, y - len, 2 * len + 1, color);
  tft->drawFastHLine(x - len, y, 2 * len + 1, color);
}

void showTS_XY(int16_t x, int16_t y, int16_t TSx, int16_t TSy) {
  tft_printf(x, y, COLOR_TEXT, "TX = %d,  TY = %d", TSx, TSy);
}

bool readNonvolatileSettings(nonvolatileSettings& settings, const nonvolatileSettings& defaults) {
  int signature;
  uint16_t storedAddress = 0;
  EEPROM.get(storedAddress, signature);

  if (signature != WRITTEN_SIGNATURE) {
    //monitor.printf("EEPROM is uninitialized, writing defaults");
    EEPROM.put(storedAddress, WRITTEN_SIGNATURE);
    EEPROM.put(storedAddress + sizeof(signature), defaults);
    EEPROM.get(storedAddress + sizeof(signature), settings);
    return false;  // if eeprom was uninitialized, use this to tell our program the screen was uncalibrated.
  }
  EEPROM.get(storedAddress + sizeof(signature), settings);
  return true;
}

bool writeNonvolatileSettingsIfChanged(nonvolatileSettings& settings) {
  int signature;
  uint16_t storedAddress = 0;
  EEPROM.put(storedAddress + sizeof(signature), settings);

  return (true);
}

void showNVsettings(char* title, const nonvolatileSettings& settings) {
  /*monitor.printf("%s", title);
  monitor.printf(" TS_LR_X: %d  TS_LR_Y: %d  TS_UL_X: %d  TS_UL_Y: %d",
                 settings.TS_LR_X, settings.TS_LR_Y, settings.TS_UL_X, settings.TS_UL_Y);*/
}

void initTouchscreen() {

  //monitor.begin(&Serial, 115200);
  //monitor.printf("Initializing");

  tft = new Adafruit_ILI9341(TFT_CS_PIN, TFT_DC_PIN, TFT_RST);
  pinMode(TFT_LED_PIN, OUTPUT);
  digitalWrite(TFT_LED_PIN, LOW);
  tft->begin();
  tft->setRotation(ROTATION);
  tft->setTextSize(1);
  tft->setTextWrap(false);
  tft->setFont(&FreeSans9pt7b);
  tft->setTextColor(COLOR_TEXT);

  ts = new XPT2046_Touchscreen(TOUCH_CS_PIN);

  ts->begin();
  ts->setRotation(tft->getRotation());

  ts_display = new TS_Display();
  ts_display->begin(ts, tft);

  nonvolatileSettings defaults;
  ts_display->getTS_calibration(&defaults.TS_LR_X, &defaults.TS_LR_Y, &defaults.TS_UL_X,
                                &defaults.TS_UL_Y);

  if (!readNonvolatileSettings(NVsettings, defaults)){
    isCalibrated = false;
  }
  ts_display->setTS_calibration(NVsettings.TS_LR_X, NVsettings.TS_LR_Y, NVsettings.TS_UL_X,
                                NVsettings.TS_UL_Y);

  x_ULcorner = 0;
  y_ULcorner = 0;
  x_LRcorner = tft->width() - 1;
  y_LRcorner = tft->height() - 1;

  ts_display->GetCalibration_UL_LR(PLUS_ARM_LEN + 2, &x_UL, &y_UL, &x_LR, &y_LR);

  
  if (!isCalibrated) {
    tft->fillScreen(COLOR_BKGD);
    drawPlus(x_UL, y_UL, COLOR_PLUS_CALIB);
    tft_printf(80, 20, COLOR_TEXT, TEXT_TAP_PLUS);
    state = STATE_WAIT_UL;
  }
  //monitor.printf("Waiting for tap at UL corner");
}

void beginCalibration() {
  isCalibrated = false;
  isCalibrating = true;
  tft->fillScreen(COLOR_BKGD);
  drawPlus(x_UL, y_UL, COLOR_PLUS_CALIB);
  tft_printf(80, 20, COLOR_TEXT, TEXT_TAP_PLUS);
  state = STATE_WAIT_UL;
}

void processCalibrationInput() {
  boolean isTouched = ts->touched();
  TS_Point p;
  if (isTouched)
    p = ts->getPoint();

  switch (state) {
    case STATE_WAIT_UL:
      if (isTouched) {
        TSx_UL = p.x;
        TSy_UL = p.y;
        state = STATE_WAIT_UL_RELEASE;

        showTS_XY(10, 50, TSx_UL, TSy_UL);
        //monitor.printf("UL corner tapped at position (%d, %d)\n", TSx_UL, TSy_UL);
      }
      break;

    case STATE_WAIT_UL_RELEASE:
      if (!isTouched) {
        state = STATE_WAIT_LR;

        drawPlus(x_UL, y_UL, COLOR_BKGD);
        tft_printf(80, 20, COLOR_BKGD, TEXT_TAP_PLUS);

        drawPlus(x_LR, y_LR, COLOR_PLUS_CALIB);
        tft_printf(80, tft->height() - 20, COLOR_TEXT, TEXT_TAP_PLUS);
        //monitor.printf("Waiting for tap at LR corner");
      }
      break;

    case STATE_WAIT_LR:
      if (isTouched) {
        TSx_LR = p.x;
        TSy_LR = p.y;
        state = STATE_WAIT_LR_RELEASE;

        showTS_XY(10, 70, TSx_LR, TSy_LR);
        //monitor.printf("LR corner tapped at position (%d, %d)\n", TSx_LR, TSy_LR);
      }
      break;

    case STATE_WAIT_LR_RELEASE:
      if (!isTouched) {
        state = STATE_WAIT_SHOW_CALIB;

        drawPlus(x_LR, y_LR, COLOR_BKGD);
        tft_printf(80, tft->height() - 20, COLOR_BKGD, TEXT_TAP_PLUS);
        tft->setCursor(10, 90);
        tft_printf(10, 90, COLOR_TEXT, TEXT_TAP_RESULT);
      }
      break;

    case STATE_WAIT_SHOW_CALIB:
      if (isTouched) {
        state = STATE_WAIT_RELEASE;

        ts_display->findTS_calibration(x_UL, y_UL, x_LR, y_LR, TSx_UL, TSy_UL, TSx_LR,
                                       TSy_LR, &NVsettings.TS_LR_X, &NVsettings.TS_LR_Y, &NVsettings.TS_UL_X,
                                       &NVsettings.TS_UL_Y);
        ts_display->setTS_calibration(NVsettings.TS_LR_X, NVsettings.TS_LR_Y,
                                      NVsettings.TS_UL_X, NVsettings.TS_UL_Y);

        writeNonvolatileSettingsIfChanged(NVsettings);
        showNVsettings("Calibration results:", NVsettings);

        ts_display->mapDisplayToTS(x_ULcorner, y_ULcorner, &TSx_ULcorner, &TSy_ULcorner);
        ts_display->mapDisplayToTS(x_LRcorner, y_LRcorner, &TSx_LRcorner, &TSy_LRcorner);
        tft_printf(10, 110, COLOR_TEXT, "UL (%d, %d) maps to:", x_ULcorner, y_ULcorner);
        showTS_XY(10, 130, TSx_ULcorner, TSy_ULcorner);
        tft_printf(10, 150, COLOR_TEXT, "LR (%d, %d) maps to:", x_LRcorner, y_LRcorner);
        showTS_XY(10, 170, TSx_LRcorner, TSy_LRcorner);
        /*monitor.printf("  UL corner (%d, %d) maps to touchscreen (%d, %d)\n",
                       x_ULcorner, y_ULcorner, TSx_ULcorner, TSy_ULcorner);
        monitor.printf("  LR corner (%d, %d) maps to touchscreen (%d, %d)\n",
                       x_LRcorner, y_LRcorner, TSx_LRcorner, TSy_LRcorner);*/
      }
      break;

    case STATE_WAIT_RELEASE:
      if (!isTouched) {
        state = STATE_WAIT_POINT_SHOW_IT;
        tft_printf(10, 200, COLOR_TEXT, TEXT_TAP_TEST);
        //monitor.printf("Waiting for random tap");
        isCalibrated = true;
      }
      break;

    case STATE_WAIT_POINT_SHOW_IT:
      if (isTouched) {
        state = STATE_WAIT_RELEASE;

        int16_t x, y;
        ts_display->mapTStoDisplay(p.x, p.y, &x, &y);
        tft->fillRect(x_ULcorner, y_ULcorner, tft->width(), tft->height(), COLOR_BKGD);
        drawPlus(x, y, COLOR_PLUS_TEST);
        //monitor.printf("Random touch at x = %d, y = %d maps to (%d, %d)\n", p.x, p.y, x, y);
      }
      break;
  }
  // delay(10); Use millis() calcs in main loop instead
}
