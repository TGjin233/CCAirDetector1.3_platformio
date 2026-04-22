#include <TFT_eSPI.h>
#include "common.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"
#include "Task.h"
#include "net.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clk = TFT_eSprite(&tft);
int backColor;
int bright;
uint16_t backFillColor;
uint16_t penColor;
// setting页面用
bool getDataFailed = false;
// page1页面用
int zoneIndex[] = {0,1,2,3,4};
int currentBigZone = TEM;
// config页面用
int configChoosedIndex; // 记录当前选中的是第几个选项
bool modalLeftChoosed; // 模态框左选项被选中
int displayMinute; // 正在显示的分钟数
int displayHour; // 正在显示的小时数
// calendar页面用
int year, month, wday, mday, lines, totalDays, lineHeight;
int monthOffset; // 记录月份选择偏移量
int offsetDerection; // 记录当前便宜方向
int firstWday,lastWday; // 记录当前显示的日历页的头一天和末一天是周几

// 初始化tft
void tftInit(){
  tft.init();
  tft.initDMA();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  clk.setSwapBytes(true);

  // ========== 添加LEDC初始化 ==========
  // 初始化GPIO17为LEDC PWM输出
  // 通道0, 频率1kHz, 8位分辨率
  ledcSetup(0, 1000, 8);      // 通道0, 1kHz, 8位
  ledcAttachPin(BL, 0);        // GPIO17连接到通道0
  // ===================================

  analogWrite(BL, bright); //调节屏幕亮度
  if(backColor == BACK_BLACK){
    backFillColor = 0x0000;
    penColor = 0xFFFF;
  }else{
    backFillColor = 0xFFFF;
    penColor = 0x0000;
  }
  tft.fillScreen(backFillColor);
}
// 按背景颜色刷新整个屏幕
void refreshTFT(){
  tft.fillScreen(backFillColor);
}
// 绘制开场动画
void drawStartLoadingAnim(){
  tft.fillScreen(backFillColor);
  delay(1000);
  // 绘制项目名字
  clk.createSprite(240, 50);
  clk.loadFont(projectName_26);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString("Air Detector",120,25);
  for(int i = 0; i < 60; i++){
    clk.pushSprite(40,i);
    delay(3);
  }
  clk.deleteSprite();
  clk.unloadFont();
  // 绘制我的名字
  clk.createSprite(240, 50);
  clk.loadFont(name_24);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString("大聪明的二手脑袋",120,25);
  for(int i = 0; i <= 160; i++){
    clk.pushSprite(i - 120, 100);
    delay(1);
  } 
  clk.deleteSprite();
  clk.unloadFont();
  delay(800);
  // 渐隐
  fadeOff();
}
// 绘制配网、离线使用选择页面
void drawSettingOrOffline(bool refresh, String text){
  String leftOption = "";
  if(getDataFailed){ // 是同步数据失败进入的此页面
    leftOption = "重新获取";
  }else{
    leftOption = "开始配置";
  }
  clk.loadFont(settingPage_22);
  if(refresh){
    refreshTFT();
    // 绘制文字
    clk.createSprite(320, 30);
    clk.fillSprite(backFillColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(penColor);
    clk.drawString(text,160,15);
    clk.pushSprite(0,60);
    clk.deleteSprite();
  }
  // 绘制左按钮
  clk.createSprite(120, 50);
  clk.fillSprite(backFillColor);
  if(settingChoosed){
    clk.fillRoundRect(0,0,120,50,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(5,5,110,40,7,tft.color565(23,114,180));
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE);
  clk.drawString(leftOption,60,25);
  clk.pushSprite(30,140);
  clk.deleteSprite();
  // 绘制右按钮
  clk.createSprite(120, 50);
  clk.fillSprite(backFillColor);
  if(!settingChoosed){
    clk.fillRoundRect(0,0,120,50,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(5,5,110,40,7,tft.color565(237,51,51));
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_WHITE);
  clk.drawString("离线使用",60,25);
  clk.pushSprite(170,140);
  clk.deleteSprite();
  clk.unloadFont();
}
// 绘制PAGE1
void drawPage1(){
  // 绘制顶部状态栏
  drawTop();
  // 绘制各传感器数据
  drawPage1Z(TEM, 0, 0);
  drawPage1Z(HUM, 0, 0);
  drawPage1Z(TVOC, 0, 0);
  drawPage1Z(CH2O, 0, 0);
  drawPage1Z(CO2, 0, 0);
}
// 绘制PAGE2
void drawPage2(){
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  // 顶部
  clk.createSprite(320,60);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  // 天气图标
  String s = getWea(weather.icon);
  if(backColor == BACK_BLACK){
    if(s.equals("雪")){
      clk.pushImage(20,5,50,50,xue_black);
    }else if(s.equals("雷")){
      clk.pushImage(20,5,50,50,lei_black);
    }else if(s.equals("沙尘")){
      clk.pushImage(20,5,50,50,shachen_black);
    }else if(s.equals("雾")){
      clk.pushImage(20,5,50,50,wu_black);
    }else if(s.equals("冰雹")){
      clk.pushImage(20,5,50,50,bingbao_black);
    }else if(s.equals("多云")){
      clk.pushImage(20,5,50,50,yun_black);
    }else if(s.equals("雨")){
      clk.pushImage(20,5,50,50,yu_black);
    }else if(s.equals("阴")){
      clk.pushImage(20,5,50,50,yin_black);
    }else if(s.equals("晴")){
      clk.pushImage(20,5,50,50,qing_black);
    }
  }else{
    if(s.equals("雪")){
      clk.pushImage(20,5,50,50,xue);
    }else if(s.equals("雷")){
      clk.pushImage(20,5,50,50,lei);
    }else if(s.equals("沙尘")){
      clk.pushImage(20,5,50,50,shachen);
    }else if(s.equals("雾")){
      clk.pushImage(20,5,50,50,wu);
    }else if(s.equals("冰雹")){
      clk.pushImage(20,5,50,50,bingbao);
    }else if(s.equals("多云")){
      clk.pushImage(20,5,50,50,yun);
    }else if(s.equals("雨")){
      clk.pushImage(20,5,50,50,yu);
    }else if(s.equals("阴")){
      clk.pushImage(20,5,50,50,yin);
    }else if(s.equals("晴")){
      clk.pushImage(20,5,50,50,qing);
    }
  }
  // 城市、AQI指数
  clk.loadFont(page3_18);
  clk.drawString(city,160,20);
  clk.drawString(String(weather.temp) + "℃",160,45);
  clk.drawString("AQI",280,20);
  clk.drawString(String(weather.air),280,40);
  clk.fillRoundRect(260, 50, 40, 5, 2, getColor(weather.air, AQI));
  clk.pushSprite(0,0);
  clk.unloadFont();
  clk.deleteSprite();
  // 时间
  clk.createSprite(320,120);
  clk.loadFont(page3Num_90);
  clk.setTextColor(backFillColor);
  clk.fillSprite(backFillColor);
  if(backColor == BACK_BLACK){
    clk.pushImage(20,5,132,110,page2_white);
    clk.pushImage(168,5,132,110,page2_white);
  }else{
    clk.pushImage(20,5,132,110,page2_black);
    clk.pushImage(168,5,132,110,page2_black);
  }
  clk.drawString(timeClient.getFormattedTime().substring(0, 2),86,65);
  clk.drawString(timeClient.getFormattedTime().substring(3, 5),234,65);
  displayMinute = timeClient.getMinutes();
  displayHour = timeClient.getHours();
  clk.fillRect(29,58,114,5,penColor);
  clk.fillRect(177,58,114,5,penColor);
  clk.pushSprite(0,60);
  clk.unloadFont();
  clk.deleteSprite();
  // 传感器
  draw2Page2Sensors();
}
// 绘制PAGE3
void drawPage3(bool refresh){
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *tm_ptr = gmtime((time_t *) &epochTime);
  String s = String(tm_ptr->tm_year + 1900) + "年" + monthDay(tm_ptr->tm_mon, tm_ptr->tm_mday) + "   " + week(tm_ptr->tm_wday);
  // 绘制星期
  clk.createSprite(320,30);
  clk.loadFont(page3_18);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  clk.drawString(s, 160, 18);
  clk.pushSprite(0,20);
  clk.unloadFont();
  clk.deleteSprite();
  // 绘制时间
  clk.createSprite(320,90);
  clk.loadFont(page3Num_90);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor); 
  // clk.drawString("10:45",160,50);
  clk.drawString(timeClient.getFormattedTime().substring(0, 5),160,50);
  clk.pushSprite(0,50);
  clk.unloadFont();
  clk.deleteSprite();
  if(refresh){
    drawTop();
    // 第一排小内容
    clk.createSprite(320,50);  
    clk.fillSprite(backFillColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(penColor);
    // 天气图标
    String s = getWea(weather.icon);
    if(backColor == BACK_BLACK){
      if(s.equals("雪")){
        clk.pushImage(35,0,50,50,xue_black);
      }else if(s.equals("雷")){
        clk.pushImage(35,0,50,50,lei_black);
      }else if(s.equals("沙尘")){
        clk.pushImage(35,0,50,50,shachen_black);
      }else if(s.equals("雾")){
        clk.pushImage(35,0,50,50,wu_black);
      }else if(s.equals("冰雹")){
        clk.pushImage(35,0,50,50,bingbao_black);
      }else if(s.equals("多云")){
        clk.pushImage(35,0,50,50,yun_black);
      }else if(s.equals("雨")){
        clk.pushImage(35,0,50,50,yu_black);
      }else if(s.equals("阴")){
        clk.pushImage(35,0,50,50,yin_black);
      }else if(s.equals("晴")){
        clk.pushImage(35,0,50,50,qing_black);
      }
    }else{
      if(s.equals("雪")){
        clk.pushImage(35,0,50,50,xue);
      }else if(s.equals("雷")){
        clk.pushImage(35,0,50,50,lei);
      }else if(s.equals("沙尘")){
        clk.pushImage(35,0,50,50,shachen);
      }else if(s.equals("雾")){
        clk.pushImage(35,0,50,50,wu);
      }else if(s.equals("冰雹")){
        clk.pushImage(35,0,50,50,bingbao);
      }else if(s.equals("多云")){
        clk.pushImage(35,0,50,50,yun);
      }else if(s.equals("雨")){
        clk.pushImage(35,0,50,50,yu);
      }else if(s.equals("阴")){
        clk.pushImage(35,0,50,50,yin);
      }else if(s.equals("晴")){
        clk.pushImage(35,0,50,50,qing);
      }
    }
    // 城市和空气质量颜色条
    clk.loadFont(page3_18);
    clk.drawString(city, 160, 14);
    clk.fillRoundRect(140, 35, 40, 6, 2, getColor(weather.air, AQI));
    // 空气质量
    clk.drawString("AQI指数", 265, 14);
    clk.unloadFont();
    clk.loadFont(configTitle_26);
    clk.drawString(String(weather.air), 265, 37);
    clk.unloadFont();
    clk.pushSprite(0,140);  
    clk.deleteSprite();
    // 第一排小内容
    clk.createSprite(320,50);
    clk.fillSprite(backFillColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(penColor);
    // 三个小标题
    clk.loadFont(page3_18);
    clk.drawString("温度", 55, 14);
    clk.drawString("湿度", 160, 14);
    clk.drawString("PM2.5", 265, 14);
    clk.unloadFont();
    // 三个数字
    clk.loadFont(configTitle_26);
    clk.drawString(String(weather.temp) + "℃", 55, 37);
    clk.drawString(String(weather.humidity) + "%", 160, 37);
    clk.drawString(weather.pm2p5, 265, 37);
    clk.unloadFont();
    clk.pushSprite(0,190);
    clk.deleteSprite();
  }
}
// 绘制CALENDAR页面
void drawCalendar(){
  drawTop();
  // 计算年月日星期
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *tm_ptr = gmtime((time_t *) &epochTime);
  year = tm_ptr->tm_year + 1900;
  month = tm_ptr->tm_mon + 1;
  mday = tm_ptr->tm_mday;
  wday = tm_ptr->tm_wday;
  // 重置月份偏移量
  monthOffset = 0;
  // 绘制XX年XX月
  clk.createSprite(320,35);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  clk.loadFont(calendar_22);
  clk.drawString(String(year) + " 年 " + String(month) + " 月", 160, 14);
  clk.drawFastHLine(20,30,280,penColor);
  clk.unloadFont();
  clk.loadFont(iconFont_16);
  clk.setCursor(40, 6);
  clk.drawGlyph(0xe61f);
  clk.setCursor(266, 6);
  clk.drawGlyph(0xe672);
  clk.unloadFont();
  clk.pushSprite(0,20);
  clk.deleteSprite();
  // 绘制星期字符串
  clk.createSprite(320,25);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  clk.loadFont(page2sensor_16);
  clk.drawString("日", 40, 12);
  clk.drawString("一", 80, 12);
  clk.drawString("二", 120, 12);
  clk.drawString("三", 160, 12);
  clk.drawString("四", 200, 12);
  clk.drawString("五", 240, 12);
  clk.drawString("六", 280, 12);
  clk.pushSprite(0,55);
  clk.unloadFont();
  clk.deleteSprite();
  // 计算日期数组
  totalDays = getTotalDays(year, month);
  lines = 0;
  int daysArray[totalDays + 1];
  for(int i = mday; i >= 1; i--){
    daysArray[i] = wday - (mday - i);
    while(daysArray[i] < 0){
      daysArray[i]+=7;
    }
    if(daysArray[i] == 6){
      lines++;
    }
  }
  for(int i = mday + 1; i <= totalDays; i++){
    daysArray[i] = wday +  (i - mday);
    while(daysArray[i] >= 7){
      daysArray[i]-=7;
    }
    if(daysArray[i] == 6){
      lines++;
    }
  }
  if(daysArray[totalDays] != 6){
    lines++;
  }
  firstWday = daysArray[1];
  lastWday = daysArray[totalDays];
  // 根据lines计算每行的高度
  if(lines == 6){
    lineHeight = 27;
  }else{
    lines = 5; // 4行和5行都统一为5行显示
    lineHeight = 32;
  }
  // 绘制日期 
  clk.setTextDatum(CC_DATUM); 
  clk.setTextColor(penColor);
  clk.loadFont(calendar_18);
  int index = 1;
  bool startDrawing = false;
  for(int i = 0; i < lines; i++){
    clk.createSprite(320, lineHeight);
    clk.fillSprite(backFillColor);
    for(int j = 0; j < 7; j++){
      if(daysArray[index] == j){
        startDrawing = true;
      }
      if(startDrawing){
        if(index > totalDays){
          break;
        }
        if(index == mday && monthOffset == 0){ // 当天的日期
          clk.fillCircle(40 + j * 40, lineHeight/2, lineHeight / 2, TFT_BLUE);
          clk.setTextColor(TFT_WHITE);
          clk.drawString(String(index), 40 + j * 40, lineHeight/2 + 4);
          clk.setTextColor(penColor);
        }else{
          clk.drawString(String(index), 40 + j * 40, lineHeight/2 + 4);
        }
        index++;
      }
    }
    clk.pushSprite(0,80 + lineHeight * i);
    clk.deleteSprite();
  }
  clk.unloadFont(); 
}
// 绘制CONFIG页面
void drawConfig(){
  // 绘制顶部状态栏
  drawTop();
  // 绘制标题
  clk.setTextColor(penColor);
  clk.createSprite(320, 30);
  clk.fillSprite(backFillColor);
  clk.loadFont(configTitle_26);
  clk.setTextDatum(CC_DATUM);
  clk.drawString("设 置",160,15);
  clk.pushSprite(0,20);
  clk.deleteSprite();
  clk.unloadFont();
  // 绘制选项卡
  drawConfigOption(0);
  drawConfigOption(1);
  drawConfigOption(2);
  drawConfigOption(3);
  drawConfigOption(4);
  drawConfigOption(5);
}
// 绘制等待用户连接的文字
void draw2LineText(String text1, String text2){
  refreshTFT();
  clk.loadFont(settingPage_22);
  clk.createSprite(320, 100);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.drawString(text1, 160, 15);
  clk.drawString(text2, 160, 60);
  clk.pushSprite(0,60);
  clk.deleteSprite();
  clk.unloadFont();
}
// 绘制加载转圈页面
void drawLoading(bool firstTime, String text, int *angle){
  // 第一次进来，先清屏，再绘制文字
  if(firstTime){
    refreshTFT();
    clk.loadFont(settingPage_22);
    clk.createSprite(320, 30);
    clk.fillSprite(backFillColor);
    clk.setTextDatum(CC_DATUM);
    clk.setTextColor(penColor);
    clk.drawString(text, 160, 15);
    clk.pushSprite(0,60);
    clk.deleteSprite();
    clk.unloadFont();
  }
  clk.createSprite(60, 60);
  clk.fillSprite(backFillColor);
  clk.fillCircle(30 + 20 * cos(*angle * 2 * M_PI / 360), 30 + 20 * sin(*angle * 2 * M_PI / 360), 7, penColor);
  clk.fillCircle(30 + 20 * cos(((*angle - 60) >= 360?(360 - (*angle - 60)) : (*angle - 60)) * 2 * M_PI / 360), 30 + 20 * sin(((*angle - 60) >= 360?(360 - (*angle - 60)) : (*angle - 60)) * 2 * M_PI / 360), 6, penColor);
  clk.fillCircle(30 + 20 * cos(((*angle - 120) >= 360?(360 - (*angle - 120)) : (*angle - 120)) * 2 * M_PI / 360), 30 + 20 * sin(((*angle - 120) >= 360?(360 - (*angle - 120)) : (*angle - 120)) * 2 * M_PI / 360), 4, penColor);
  clk.pushSprite(130,110);
  clk.deleteSprite();
  (*angle)+=3;
  if(*angle >= 360){
    *angle = 0;
  }
}
// 屏幕渐隐
void fadeOff(){
  int delays = 300000 / bright;
  for(int i = bright; i >= 0; i--){
    analogWrite(BL, i);
    delayMicroseconds(delays);
  }
}
// 屏幕渐显
void fadeOn(){
  int delays = 500000 / bright;
  for(int i = 0; i <= bright; i++){
    analogWrite(BL, i);
    delayMicroseconds(delays);
  }
}
// 根据传来的数值，得到矩形条颜色
uint16_t getColor(float value, int type){
  switch(type){
    case TEM:
      if(value <= TEM_LV1){
        return COLOR_LV1;
      }else if(value <= TEM_LV2){
        return COLOR_LV2;
      }else if(value <= TEM_LV2){
        return COLOR_LV3;
      }else{
        return COLOR_LV4;
      }
    case HUM:
      if(value <= HUM_LV1){
        return COLOR_LV1;
      }else if(value <= HUM_LV2){
        return COLOR_LV2;
      }else if(value <= HUM_LV2){
        return COLOR_LV3;
      }else{
        return COLOR_LV4;
      }
    case TVOC:
      if(value <= TVOC_LV1){
        return COLOR_LV1;
      }else if(value <= TVOC_LV2){
        return COLOR_LV2;
      }else if(value <= TVOC_LV2){
        return COLOR_LV3;
      }else{
        return COLOR_LV4;
      }
    case CH2O:
      if(value <= CH2O_LV1){
        return COLOR_LV1;
      }else if(value <= CH2O_LV2){
        return COLOR_LV2;
      }else if(value <= CH2O_LV2){
        return COLOR_LV3;
      }else{
        return COLOR_LV4;
      }
    case CO2:
      if(value <= CO2_LV1){
        return COLOR_LV1;
      }else if(value <= CO2_LV2){
        return COLOR_LV2;
      }else if(value <= CO2_LV2){
        return COLOR_LV3;
      }else{
        return COLOR_LV4;
      }
    case AQI:
      if(value <= AQI_LV1){
        return COLOR_LV1;
      }else if(value <= AQI_LV2){
        return COLOR_LV2;
      }else if(value <= AQI_LV2){
        return COLOR_LV3;
      }else{
        return COLOR_LV4;
      }  
    default:
      return COLOR_LV1;
  }
}
// 绘制顶部状态栏
void drawTop(){
  clk.createSprite(320, 20);
  clk.setTextDatum(CL_DATUM);
  clk.setTextColor(penColor);
  clk.fillSprite(backFillColor);
  // 绘制wifi和时间
  clk.setCursor(5, 1);
  clk.loadFont(iconFont_16);
  if(mode == ONLINE_MODE){  
    clk.drawGlyph(0xe6b8);
    clk.unloadFont();
    clk.loadFont(unit_time_16);
    // clk.drawString("10:45",30,10);
    if(timeClient.isTimeSet() && currentPage != PAGE3){
      clk.drawString(timeClient.getFormattedTime().substring(0, 5),30,10);
    }
  }else{  
    clk.drawGlyph(0xe61e);   
  }
  clk.unloadFont();
  // 绘制电池
  clk.loadFont(iconFont_20);
  clk.setCursor(290, -1);
  if(Charging){
    clk.drawGlyph(0xe7a3);
  }else{
    if(batteryPercent < 20){
      clk.setTextColor(TFT_RED);
    }
    clk.drawGlyph(0xe7a8);
    clk.setTextColor(penColor);
    clk.fillRect(293,6,12 * batteryPercent / 100,6,penColor);
  }
  clk.unloadFont();
  // 绘制电压
  clk.setTextDatum(CR_DATUM);
  clk.loadFont(batteryNum_14);
  clk.drawString(String(batteryVoltage, 1) + "V",280,10);
  clk.unloadFont();
  clk.pushSprite(0,0);
  clk.deleteSprite();
}
// 绘制PAGE1小区域
void drawPage1SZ(int x, int y, int type, int offsetX, int offsetY){
  // 获取值
  String s;
  float v;
  String t;
  String u;
  switch(type){
    case TEM:
      s = temperature;
      v = temperature.toFloat();
      t = "温度";
      u = "℃";
      break;
    case HUM:
      s = humidity;
      v = humidity.toFloat();
      t = "相对湿度";
      u = "%RH";
      break;
    case TVOC:
      s = tvoc;
      v = tvoc.toFloat();
      t = "tVOC";
      u = "mg/m³";
      break;
    case CH2O:
      s = ch2o;
      v = ch2o.toFloat();
      t = "CH2O";
      u = "mg/m³";
      break;
    case CO2:
      s = co2;
      v = co2.toFloat();
      t = "CO2";
      u = "PPM";
      break;  
    default:
      break;  
  }
  clk.setTextColor(penColor);
  clk.createSprite(160, 65);
  clk.fillSprite(backFillColor);
  clk.loadFont(pollutantName_18);
  clk.setTextDatum(CL_DATUM);
  clk.drawString(t, 5, 10);
  clk.unloadFont();
  clk.loadFont(page1Num_35);
  clk.drawString(s, 5, 37);
  clk.unloadFont();
  clk.loadFont(unit_time_16);
  clk.drawString(u, clk.getCursorX() + 5, clk.getCursorY() + 17);
  clk.unloadFont();
  clk.fillRoundRect(5, 52, 150, 5, 2, getColor(v, type));
  if(offsetX != 0 || offsetY != 0){
    clk.setScrollRect(0, 0, 160, 65, backFillColor);
    clk.scroll(offsetX, offsetY);
  }
  clk.pushSprite(x, y);
  clk.deleteSprite();
}
// 绘制PAGE1大区域
void drawPage1BZ(int type, int offsetX, int offsetY){
  // 获取值
  String s;
  float v;
  String t;
  String u;
  switch(type){
    case TEM:
      s = temperature;
      v = temperature.toFloat();
      t = "温度";
      u = "℃";
      break;
    case HUM:
      s = humidity;
      v = humidity.toFloat();
      t = "相对湿度";
      u = "%RH";
      break;
    case TVOC:
      s = tvoc;
      v = tvoc.toFloat();
      t = "tVOC";
      u = "mg/m³";
      break;
    case CH2O:
      s = ch2o;
      v = ch2o.toFloat();
      t = "CH2O";
      u = "mg/m³";
      break;
    case CO2:
      s = co2;
      v = co2.toFloat();
      t = "CO2";
      u = "PPM";
      break;  
    default:
      break;  
  }
  clk.setTextColor(penColor);
  clk.createSprite(320, 90);
  clk.fillSprite(backFillColor);
  clk.loadFont(pollutantName_18);
  clk.setTextDatum(CL_DATUM);
  clk.drawString(t,5,15);
  clk.unloadFont();
  clk.loadFont(page1Num_64);
  clk.drawString(s, 5, 55);
  clk.unloadFont();
  clk.loadFont(unit_time_16);
  clk.drawString(u, clk.getCursorX() + 5, clk.getCursorY() + 37);
  clk.unloadFont();
  clk.fillRoundRect(5, 77, 310, 6, 2, getColor(v, type)); 
  if(offsetX != 0 || offsetY != 0){
    clk.setScrollRect(0, 0, 320, 90, backFillColor);
    clk.scroll(offsetX, offsetY);
  }
  clk.pushSprite(0,20);
  clk.deleteSprite();
}
// 绘制PAGE1区域
void drawPage1Z(int type, int offsetX, int offsetY){
  if(zoneIndex[type] == 0){
    drawPage1BZ(type, offsetX, offsetY);
  }else{
    int x,y;
    if(zoneIndex[type] == 1){
      x = 0;
      y = 110;
    }else if(zoneIndex[type] == 2){
      x = 160;
      y = 110;
    }else if(zoneIndex[type] == 3){
      x = 0;
      y = 175;
    }else if(zoneIndex[type] == 4){
      x = 160;
      y = 175;
    }
    drawPage1SZ(x, y, type, offsetX, offsetY);
  }
}
// 刷新page2的传感器区域
void draw2Page2Sensors(){
  clk.createSprite(320,60);
  clk.fillSprite(backFillColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(penColor);
  clk.loadFont(page2sensor_16);
  clk.drawString("温度", 32, 15);
  clk.drawString("湿度", 96, 15);
  clk.drawString("tVOC", 160, 15);
  clk.drawString("CH2O", 224, 15);
  clk.drawString("CO2", 288, 15);
  clk.unloadFont();
  clk.loadFont(page2SensorNum_22);
  clk.drawString(temperature, 32, 38);
  clk.drawString(humidity, 96, 38);
  clk.drawString(tvoc, 160, 38);
  clk.drawString(ch2o, 224, 38);
  clk.drawString(co2, 288, 38);
  clk.unloadFont();
  clk.fillRoundRect(5, 53, 54, 5, 2, getColor(temperature.toFloat(), TEM));
  clk.fillRoundRect(69, 53, 54, 5, 2, getColor(humidity.toFloat(), HUM));
  clk.fillRoundRect(133, 53, 54, 5, 2, getColor(tvoc.toFloat(), TVOC));
  clk.fillRoundRect(197, 53, 54, 5, 2, getColor(ch2o.toFloat(), CH2O));
  clk.fillRoundRect(261, 53, 54, 5, 2, getColor(co2.toFloat(), CO2));
  clk.pushSprite(0,180);
  clk.deleteSprite();
}
// 绘制config选项卡
void drawConfigOption(int index){
  uint16_t glyph;
  String s;
  switch(index){
    case OPTION_BRIGHT:
      s = "亮度调节";
      glyph = 0xe72a;
      break;
    case OPTION_VOICE:
      s = "按键声音";
      glyph = 0xe8b8;
      break;
    case OPTION_OFFSET:
      s = "温度补偿";
      glyph = 0xe66b;
      break;  
    case OPTION_THEME:
      s = "主题切换";
      glyph = 0xe605;
      break;
    case OPTION_WLAN:
      s = "配置网络";
      glyph = 0xe8c4;
      break;
    case OPTION_RESET:
      s = "恢复出厂";
      glyph = 0xe644;
      break;
    default:
      break;          
  }
  clk.createSprite(320, 31);
  clk.setTextDatum(CC_DATUM);
  if(index == configChoosedIndex){
    clk.fillSprite(penColor);
    clk.setTextColor(backFillColor);
  }else{
    clk.fillSprite(backFillColor);
    clk.setTextColor(penColor);
  }
  clk.loadFont(iconFont_22);
  if(index == OPTION_BRIGHT || index == OPTION_WLAN || index == OPTION_RESET || index == OPTION_OFFSET){
    clk.setCursor(290, 4);
    clk.drawGlyph(0xe61b);
  }
  clk.setCursor(5, 4);
  clk.drawGlyph(glyph);
  clk.unloadFont();
  clk.loadFont(configOption_18);
  clk.drawString(s, 70, 17);
  if(index == OPTION_VOICE){
    if(voice){
      clk.drawString("开", 300, 17);
    }else{
      clk.drawString("关", 300, 17);
    }
  }
  if(index == OPTION_THEME){
    if(backColor == BACK_BLACK){
      clk.drawString("黑", 300, 17);
    }else{
      clk.drawString("白", 300, 17);
    }
  }
  clk.unloadFont();
  clk.pushSprite(0, 50 + (index) * 31);
  clk.deleteSprite();
}
// 绘制calendar日期部分
void drawCalendarDate(){
  int tmpYear,tmpMonth;
  tmpYear = year + monthOffset / 12;
  tmpMonth = month + monthOffset % 12;
  if(tmpMonth > 12){
    tmpYear++;
    tmpMonth-=12;
  }else if(tmpMonth <= 0){
    tmpYear--;
    tmpMonth+=12;
  }
  // 绘制XX年XX月
  clk.createSprite(320,35);
  clk.setTextDatum(CC_DATUM);
  clk.fillSprite(backFillColor);
  clk.setTextColor(penColor);
  clk.loadFont(calendar_22);
  clk.drawString(String(tmpYear) + " 年 " + String(tmpMonth) + " 月", 160, 14);
  clk.drawFastHLine(20,30,280,penColor);
  clk.unloadFont();
  clk.loadFont(iconFont_16);
  clk.setCursor(40, 6);
  clk.drawGlyph(0xe61f);
  clk.setCursor(266, 6);
  clk.drawGlyph(0xe672);
  clk.unloadFont();
  clk.pushSprite(0,20);
  clk.deleteSprite();
  // 计算该月的天数
  totalDays = getTotalDays(tmpYear, tmpMonth);
  lines = 0;
  int daysArray[totalDays + 1];
  // 计算日期数组和日期行数
  if(monthOffset == 0){ // 当前页
    for(int i = mday; i >= 1; i--){
      daysArray[i] = wday - (mday - i);
      while(daysArray[i] < 0){
        daysArray[i]+=7;
      }
      if(daysArray[i] == 6){
        lines++;
      }
    }
    for(int i = mday + 1; i <= totalDays; i++){
      daysArray[i] = wday +  (i - mday);
      while(daysArray[i] >= 7){
        daysArray[i]-=7;
      }
      if(daysArray[i] == 6){
        lines++;
      }
    }
  }else{ // 非当前页，根据上下文处理
    if(offsetDerection == CALENDAR_LEFT_OFFSET){ // 前一个月的最后一天就是之前第一天的周几-1
      for(int i = totalDays; i >= 1; i--){
        daysArray[i] = firstWday - 1 - (totalDays - i);
        while(daysArray[i] < 0){
          daysArray[i]+=7;
        }
        if(daysArray[i] == 6){
          lines++;
        }
      }
    }else{ // 后一个月的第一天就是之前最后一天的周几+1
      for(int i = 1; i <= totalDays; i++){
        daysArray[i] = lastWday + 1 + (i - 1);
        while(daysArray[i] >= 7){
          daysArray[i]-=7;
        }
        if(daysArray[i] == 6){
          lines++;
        }
      }
    }
  }
  if(daysArray[totalDays] != 6){
    lines++;
  }
  // 重置首日末日数据
  firstWday = daysArray[1];
  lastWday = daysArray[totalDays];
  // 根据lines计算每行的高度
  if(lines == 6){
    lineHeight = 27;
  }else{
    lines = 5; // 4行和5行都统一为5行显示
    lineHeight = 32;
  }
  // 绘制日期 
  clk.setTextDatum(CC_DATUM); 
  clk.setTextColor(penColor);
  clk.loadFont(calendar_18);
  int index = 1;
  bool startDrawing = false;
  for(int i = 0; i < lines; i++){
    clk.createSprite(320, lineHeight);
    clk.fillSprite(backFillColor);
    for(int j = 0; j < 7; j++){
      if(daysArray[index] == j){
        startDrawing = true;
      }
      if(startDrawing){
        if(index > totalDays){
          break;
        }
        if(index == mday && monthOffset == 0){ // 当天的日期
          clk.fillCircle(40 + j * 40, lineHeight/2, lineHeight / 2, TFT_BLUE);
          clk.setTextColor(TFT_WHITE);
          clk.drawString(String(index), 40 + j * 40, lineHeight/2 + 4);
          clk.setTextColor(penColor);
        }else{
          clk.drawString(String(index), 40 + j * 40, lineHeight/2 + 4);
        }
        index++;
      }
    }
    for(int m = 1; m <= 80; m++){
      if(offsetDerection == CALENDAR_LEFT_OFFSET){
        clk.pushSprite(-320 + m * 4,80 + lineHeight * i);
      }else{
        clk.pushSprite(320 - m * 4,80 + lineHeight * i);
      }   
      delay(3);
    }
    clk.deleteSprite();
  }
  clk.unloadFont(); 
}
// 绘制手鼓猫页面
void drawBongoCat(){
  if(backFillColor == BACK_BLACK){
    tft.pushImage(0,0,320,240,bongoCat_black);
  }else{
    tft.pushImage(0,0,320,240,bongoCat_white);
  }
}
// 绘制配置网络和恢复出厂的模态框
void drawModal(String text, bool refresh){
  clk.loadFont(configOption_18);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(backFillColor);
  if(refresh){
    tft.fillRoundRect(30, 60, 260, 100, 10, penColor);
    // 绘制标题
    clk.createSprite(200, 30);
    clk.fillSprite(penColor);
    clk.drawString(text,130,15);
    clk.pushSprite(30,70);
    clk.deleteSprite();
  }
  // 绘制左按钮
  clk.createSprite(64, 34);
  clk.fillSprite(penColor);
  if(modalLeftChoosed){
    clk.fillRoundRect(0,0,64,34,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(2,2,60,30,7,tft.color565(23,114,180));
  clk.setTextColor(TFT_WHITE);
  clk.drawString("确认",32,18);
  clk.pushSprite(70,115);
  clk.deleteSprite();
  // 绘制右按钮
  clk.createSprite(64, 34);
  clk.fillSprite(penColor);
  if(!modalLeftChoosed){
    clk.fillRoundRect(0,0,64,34,7,tft.color565(196,203,207));
  }
  clk.fillRoundRect(2,2,60,30,7,tft.color565(237,51,51));
  clk.setTextColor(TFT_WHITE);
  clk.drawString("取消",32,18);
  clk.pushSprite(186,115);
  clk.deleteSprite();
  clk.unloadFont();
}
// 绘制亮度调节的模态框
void drawBrightModal(bool refresh){
  if(refresh){
    tft.fillRoundRect(30, 120, 260, 35, 7, penColor);
    tft.setTextColor(backFillColor);
    tft.loadFont(iconFont_22);
    tft.setCursor(35,125);
    tft.drawGlyph(0xe695);
    tft.setCursor(262,125);
    tft.drawGlyph(0xe632);
    tft.unloadFont();
  }
  // 绘制亮度条
  clk.createSprite(200, 14);
  clk.fillSprite(penColor);
  clk.drawRoundRect(0,0,200,14,7,backFillColor);
  clk.fillRoundRect(2,2,bright,10,4,backFillColor);
  clk.pushSprite(59,130);
  clk.deleteSprite();
}
// 绘制温度补偿调节的模态框
void drawOffsetModal(bool refresh){
  if(refresh){
    tft.fillRoundRect(110, 150, 100, 33, 7, penColor);
    tft.setTextColor(backFillColor);
    tft.loadFont(settingPage_22);
    tft.drawString("-", 120, 155);
    tft.drawString("+", 188, 155);
    tft.unloadFont();
  }
  clk.createSprite(50, 30);
  clk.fillSprite(penColor);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(backFillColor);
  clk.loadFont(calendar_22);
  clk.drawString(String(tempOffset, 1), 25, 16);
  clk.pushSprite(135,152);
  clk.unloadFont();
  clk.deleteSprite();
}
// 切换主题
void exchangeTheme(){
  if(backColor == BACK_BLACK){
    backColor = BACK_WHITE;
    backFillColor = 0xFFFF;
    penColor = 0x0000;
  }else{
    backColor = BACK_BLACK;
    backFillColor = 0x0000;
    penColor = 0xFFFF;
  }
  setTheme();
}
// 绘制当前页面
void drawCurrentPage(){
  switch(currentPage){
    case PAGE1:
      drawPage1();
      break;
    case PAGE2:
      drawPage2();
      break;
    case PAGE3:
      drawPage3(true);
      break;
    case CALENDAR:
      drawCalendar();
      break; 
    case BONGOCAT:
      drawBongoCat();
      break;   
    case CONFIG:
      drawConfig();
      break;  
    default:
      break;
  }
}
//处理星期
String week(int tm_wday){
  String wk[7] = {"日","一","二","三","四","五","六"};
  String s = "星期" + wk[tm_wday];
  return s;
}
//处理月日
String monthDay(int tm_mon, int tm_mday){
  String s = "";
  s = s + (tm_mon + 1);
  s = s + "月" + tm_mday + "日";
  return s;
}
//根据年份和月份获取这个月的天数
int getTotalDays(int year, int month){
  if(month == 1 || month == 3|| month == 5|| month == 7|| month == 8|| month == 10|| month == 12){
    return 31;
  }else if(month == 4 || month == 6 || month == 9 || month == 11){
    return 30;
  }else{
    if(year % 4 == 0){
      return 29;
    }else{
      return 28;
    }
  }
}
// 根据icon获得天气状况
String getWea(int icon){
  String s = "";
  switch (icon){
    case 100:
    case 150:
      s = "晴";
      break;
    case 104:
      s = "阴";
      break;
    case 300:
    case 301:
    case 305:
    case 306:
    case 307:
    case 308:
    case 309:
    case 310:
    case 311:
    case 312:
    case 313:
    case 314:
    case 315:
    case 316:
    case 317:
    case 318:
    case 350:
    case 351:
    case 399:
      s = "雨";
      break;
    case 101:
    case 102:
    case 103:
    case 151:
    case 152:
    case 153:
      s = "多云";
      break;
    case 304:
      s = "冰雹";
      break;
    case 500:
    case 501:
    case 502:
    case 509:
    case 510:
    case 511:
    case 512:
    case 513:
    case 514:
    case 515:
      s = "雾";
      break;
    case 503:
    case 504:
    case 507:
    case 508:
      s = "沙尘";
      break;
    case 302:
    case 303:
      s = "雷";
      break;
    case 400:
    case 401:
    case 402:
    case 403:
    case 404:
    case 405:
    case 406:
    case 407:
    case 408:
    case 409:
    case 410:
    case 456:
    case 457:
    case 499:
      s = "雪";
      break;
    default:
      s = "晴";
      break;  
  }
  return s;
}

