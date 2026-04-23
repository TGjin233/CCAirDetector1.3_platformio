#include <Arduino.h>

#include "Common.h"
#include "Task.h"
#include "net.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"

/**
CC温湿度仪  版本1.3

本次更新内容：

和风天气身份验证改版，导致新申请的和风账号无法顺利获取天气信息，这个版本加入了作者自己编写的库，
从IDE->项目->导入库->添加.zip库，将作者开源的JwtUtil库安装，然后再net.cpp文件中，替换5个和风相关的内容即可烧录。
具体更新操作，请参照Dudu时钟这一期的教程，“手把手复刻2.4寸Dudu天气时钟 已适配和风天气JWT认证”，
https://www.bilibili.com/video/BV1N3JEz4E57/?vd_source=c106ac8c60f1249e356ef02bdcc85de7

*/

unsigned long lastYuhangyuanAnimTime = 0; // 上次播放太空人动画的时间
const int YUHANGYUAN_ANIM_INTERVAL = 120; // 太空人动画间隔（毫秒），值越大动画越慢

// 联网获取信息失败时，跳至是否离线使用页面
void step2OffLine(){
  // 关闭加载动画
  loadingAnim = false;
  fadeOff();
  delay(200);
  // 绘制setting页面
  drawSettingOrOffline(true, "网络不给力，获取失败");
  // 使能按键
  buttonEnable = true;
  // 断开WIFI
  disconnectWiFi();
  // 创建屏幕渐显任务
  createFadeOnTask();
}

void setup(){
  Serial.begin(115200);
  // 获取初始化参数
  // setInfo4Test(); // 写入测试参数
  getInfo();
  // 初始化TFT_ESPI
  tftInit();
  // 初始化传感器
  sensorsInit();
  // 绘制开场动画
  if(!DEVELOP_MODE){
    drawStartLoadingAnim();
    if(ssid.length() == 0 || pass.length() == 0 || city.length() == 0){ // 未配过网
      currentPage = SETTING; // 将页面置为配置页面
      drawSettingOrOffline(true, "是否开始配置网络及地区？");
      createFadeOnTask();
    }else{ // 已配置过，尝试连接
      createDrawLoadingTask("尝试联网获取信息");
      delay(200);
      createFadeOnTask();
      // 连接WiFi,30秒超时就回到setting页面
      connectWiFi(30);
      if(connected){ // 成功联网
        // 初始化对时工具
        timeClient.begin();
        // 开始对时
        logInfoln("开始NTP对时");
        int NTPtimes = 1;
        while(!timeClient.isTimeSet()){
          logInfo("第");
          logInfo(String(NTPtimes));
          logInfoln("次对时");
          timeClient.update();
          delay(4000);
          if(NTPtimes >= DATA_FAILED_TIMES){
            logInfoln("网络异常，NTP对时失败");
            getDataFailed = true;
            break;
          }
          NTPtimes++;
        }
        if(getDataFailed){ // NTP对时全部失败，跳至是否离线使用页面
          step2OffLine();
        }else{ // NTP对时成功，才会进行下面的逻辑
          logInfo("对时成功，当前时间： ");
          logInfoln(timeClient.getFormattedTime());
          // Serial.println(timeClient.getEpochTime() - 8 * 3600);
          if(location.equals("")){ // 没有城市信息
            int cityCode;
            while(cityCode != HTTP_CODE_OK){
              cityCode = getCityID();
            }
          }
          // 有了城市信息，进行NTP对时，查询天气信息
          if(!location.equals("")){
            // 查询天气
            int weatherCode;
            int weatherTimes = 1;
            while(weatherCode != HTTP_CODE_OK){
              if(weatherTimes >= DATA_FAILED_TIMES){
                logInfoln("网络异常，获取天气信息失败");
                getDataFailed = true;
                break;
              }
              weatherCode = getWeather();
              weatherTimes++;
            }
            if(getDataFailed){ // 获取天气信息全部失败，跳至是否离线使用页面
              step2OffLine();
            }else{ // 获取天气信息成功，继续获取空气质量信息
              // 查询空气质量
              int airCode;
              int airTimes = 1;
              while(airCode != HTTP_CODE_OK){
                if(airTimes >= DATA_FAILED_TIMES){
                  logInfoln("网络异常，获取空气质量信息失败");
                  getDataFailed = true;
                  break;
                }
                airCode = getAir();
                airTimes++;              
              }
              if(getDataFailed){ // 获取天气信息全部失败，跳至是否离线使用页面
                step2OffLine();
              }else{ // 获取空气质量信息成功，继续下面的逻辑
                if(queryWeatherSuccess && queryAirSuccess){
                  mode = ONLINE_MODE;
                }
                currentPage = PAGE1;
                // 关闭加载动画
                loadingAnim = false;
                fadeOff();
                delay(200);
                // 绘制Page1页面
                drawPage1();
                // 使能按键
                buttonEnable = true;
                // 创建屏幕渐显任务
                createFadeOnTask();
              }
            }  
          }
        }
      }
    }
  }else{
    // 测试代码
    currentPage = PAGE1;
    delay(1000);
    mode = ONLINE_MODE;
    drawPage1();
  }
  // 创建核心0的传感器任务
  createAnotherCoreTask();
  // 创建核心0的ADC采样任务
  createADCTask();
}

void loop(){
  if(updateWeather){
    logInfoln("开始更新天气");
    createDrawLoadingTask("正在更新天气信息");
    unsigned long start = millis();
    getWeather();
    getAir();
    while((millis() - start) < 2000){
      delay(10);
    }
    // 更新天气标志重新置为false
    updateWeather = false;
    // 关闭加载动画
    loadingAnim = false;
    fadeOff();
    delay(200);
    // 重新绘制更新天气之前的页面
    drawCurrentPage();
    createFadeOnTask();
  }
  watchBtn();
  switch(currentPage){
    case SETTING:
      doClient();
      break;
    case PAGE1:
      if(!buttonEnable){
        return;
      }
      if((millis() - lastRefresh) >= 1000 || lastRefresh > millis()){
        drawPage1();
        lastRefresh = millis();
      }
      break;
    case PAGE2:
      // 刷新时间
      if(timeClient.isTimeSet() && timeClient.getMinutes() != displayMinute){
        String newM = timeClient.getFormattedTime().substring(3, 5);
        String newH = "";
        displayMinute = timeClient.getMinutes();
        if(timeClient.getHours() != displayHour){
          newH = timeClient.getFormattedTime().substring(0, 2);
          displayHour = timeClient.getHours();
        }
        // 绘制动画(10帧，每一帧10像素高度)
        clk.loadFont(page3Num_90);
        clk.setTextColor(backFillColor);
        clk.setTextDatum(CC_DATUM);
        for(int i = 1; i <= 10; i++){
          // 分钟
          clk.createSprite(110,i * 10);
          clk.fillSprite(penColor);
          clk.drawString(newM,55,55);
          clk.fillRect(0,48,110,5,penColor);
          clk.pushSprite(179,70);
          clk.deleteSprite();
          // 小时
          if(!newH.equals("")){
            clk.createSprite(110,i * 10);
            clk.fillSprite(penColor);
            clk.drawString(newH,55,55);
            clk.fillRect(0,48,110,5,penColor);
            clk.pushSprite(31,70);
            clk.deleteSprite();
          }
          delay(20);
        }
        clk.unloadFont();
      }
      // 刷新传感器区域
      if((millis() - lastRefresh) >= 1000 || lastRefresh > millis()){
        draw2Page2Sensors();
        lastRefresh = millis();
      }     
      break;
    case PAGE3:
      // 刷新顶部条
      if((millis() - lastRefresh) >= 1000 || lastRefresh > millis()){
        drawTop();
        lastRefresh = millis();
      }
      // 刷新时间
      if(timeClient.isTimeSet() && timeClient.getMinutes() != displayMinute){
        drawPage3(false);
        displayMinute = timeClient.getMinutes();
      }
      break;
    case CALENDAR:
      // 刷新顶部条
      if((millis() - lastRefresh) >= 1000 || lastRefresh > millis()){
        drawTop();
        lastRefresh = millis();
      }
      // 刷新日历
      if(timeClient.isTimeSet() && timeClient.getMinutes() != displayMinute && buttonEnable && monthOffset == 0){
        drawCalendar();
        displayMinute = timeClient.getMinutes();
      }
      break;
    case YUHANGYUAN:
      // 刷新顶部条
      if((millis() - lastRefresh) >= 1000 || lastRefresh > millis()){
        drawTop();
        lastRefresh = millis();
      }
      // 播放太空人动画（带帧率控制）
      if(yuhangyuanAnimRunning){
        if(millis() - lastYuhangyuanAnimTime >= YUHANGYUAN_ANIM_INTERVAL){
          drawYuhangyuanAnim();
          lastYuhangyuanAnimTime = millis();
        }
      }
      break;
    case CONFIG:
      if(!buttonEnable){
        return;
      }
      if((millis() - lastRefresh) >= 1000 || lastRefresh > millis()){
        drawTop();
        lastRefresh = millis();
      }
      break;  
    default:
      break;
  }
}
