#ifndef __TASK_H
#define __TASK_H

#include "Common.h"

#define ADC_CHANNEL     ADC1_CHANNEL_6
#define ADC_ATTEN_DB    ADC_ATTEN_DB_2_5
#define ADC_WIDTH_BIT   ADC_WIDTH_BIT_12
#define ADC_FREQUENCY   256

extern enum CurrentPage currentPage;
extern unsigned long lastRefresh;
extern bool updateWeather;
// 数据相关
extern String tvoc;
extern String ch2o;
extern String co2;
extern String temperature;
extern String humidity;
// 系统变量
extern bool settingChoosed;
extern int mode;
extern bool buttonEnable;
extern bool voice;
extern bool loadingAnim;
extern bool modalShowed;
extern float tempOffset;
// ADC相关
extern float batteryVoltage;
extern int batteryPercent;
extern bool Charging;
// 初始化函数
void sensorsInit();
void btnInit();
void watchBtn();
// FreeRTOS
void createFadeOnTask();
void createAnotherCoreTask();
void createDrawLoadingTask(char *text);
void createADCTask();
// Ticker
void startTickerUpdateWeather();

#endif

