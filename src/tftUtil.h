#ifndef __TFTUTIL_H
#define __TFTUTIL_H

#include <TFT_eSPI.h>

#define TEM   0
#define HUM   1
#define TVOC  2
#define CH2O  3
#define CO2   4
#define AQI   5
#define OPTION_BRIGHT 0
#define OPTION_VOICE 1
#define OPTION_OFFSET 2
#define OPTION_THEME 3
#define OPTION_WLAN 4
#define OPTION_RESET 5
#define TVOC_LV1  0.6
#define TVOC_LV2  1
#define TVOC_LV3  1.5
#define CH2O_LV1  0.1
#define CH2O_LV2  0.3
#define CH2O_LV3  0.5
#define CO2_LV1   450
#define CO2_LV2   1000
#define CO2_LV3   2000
#define TEM_LV1   35
#define TEM_LV2   50
#define TEM_LV3   65
#define HUM_LV1   65
#define HUM_LV2   80
#define HUM_LV3   95
#define AQI_LV1   50
#define AQI_LV2   100
#define AQI_LV3   150
#define COLOR_LV1 0x0E27
#define COLOR_LV2 0xFFE0
#define COLOR_LV3 0xFBE3
#define COLOR_LV4 0xF800
#define CALENDAR_RIGHT_OFFSET 0
#define CALENDAR_LEFT_OFFSET  1

extern TFT_eSPI tft;
extern TFT_eSprite clk;
extern int backColor;
extern int bright;
extern uint16_t backFillColor;
extern uint16_t penColor;
extern int zoneIndex[];
extern int currentBigZone;
extern int configChoosedIndex;
extern bool modalLeftChoosed;
extern int displayMinute;
extern int displayHour;
extern bool getDataFailed;
extern int monthOffset;
extern int offsetDerection;
void tftInit();
void refreshTFT();
void fadeOff();
void fadeOn();
uint16_t getColor(float value, int type);
void drawStartLoadingAnim();
void drawSettingOrOffline(bool refresh, String text);
void draw2LineText(String text1, String text2);
void drawLoading(bool firstTime, String text, int *angle);
void drawPage1();
void drawTop();
void drawPage1Z(int type, int offsetX, int offsetY);
void drawPage1SZ(int x, int y, int type, int offsetX, int offsetY);
void drawPage1BZ(int type, int offsetX, int offsetY);
void drawPage2();
void draw2Page2Sensors();
void drawPage3(bool refresh);
void drawConfig();
void drawCalendar();
void drawCalendarDate();
void drawModal(String text, bool refresh);
void drawBrightModal(bool refresh);
void drawOffsetModal(bool refresh);
void drawConfigOption(int index);
void exchangeTheme();
void drawCurrentPage();
void drawBongoCat();
void drawYuhangyuan();
int getTotalDays(int year, int month);
String week(int tm_wday);
String monthDay(int tm_mon, int tm_mday);
String getWea(int icon);

#endif