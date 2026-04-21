#include <OneButton.h>
#include <DFRobot_DHT20.h>
#include <Ticker.h>
#include "esp_adc_cal.h"
#include "net.h"
#include "Task.h"
#include "tftUtil.h"
#include "PreferencesUtil.h"

enum CurrentPage currentPage = SETTING; // 记录当前页面
unsigned long lastRefresh;  // 上次刷新传感器数据的时间
bool updateWeather = false; // 是否需要更新天气
// 按钮
OneButton button1(BTN1, true);
OneButton button2(BTN2, true);
OneButton button3(BTN3, true);
OneButton bootButton(BOOT_BTN, true);
// 系统变量
int mode = OFFLINE_MODE; // 运行模式
bool buttonEnable = true; // 按键使能
bool settingChoosed = true; // 选择的是开始配置按键
bool voice = true; // 声音是否开启
bool loadingAnim = false; // 加载动画是否执行
bool modalShowed = false; // 模态框是否已显示
float tempOffset; // 温度偏移值
float tmpTempOffset; // 记录临时温度偏移值
int tmpBright; // 记录临时亮度
// JW01
uint8_t packet[9];
String tvoc = "0.00";
String ch2o = "0.00";
String co2 = "0";
int dirtyDataCount;
// DHT20
DFRobot_DHT20 dht20;
String temperature = "0.0";
String humidity = "0.0";
// ADC区域
static esp_adc_cal_characteristics_t *adcChar; // 斜率曲线
float batteryMin = 2.9; // 电池最小电压
float batteryMax = 4.2; // 电池最大电压
float batteryVoltage;
int batteryPercent;
float tmpVoltage = 0.00; // 记录临时基准电压
bool Charging = false; // 是否正在充电中
int creaseTimes; // 记录大于临时基准电压的次数
int decreaseTimes; // 记录小于临时基准电压的次数
// adc偏移值
float adcOffset = 0.02;

// 开启JW01和DHT20
void sensorsInit(){
  pinMode(JW01_SW, OUTPUT);
  digitalWrite(JW01_SW, HIGH);
  bool textShowed = false;
  while(dht20.begin()){
    logInfoln("DHT20初始化失败");
    // 屏幕上绘制文字
    if(!textShowed){
      draw2LineText("未识别到温湿度传感器","请检查设备");
      textShowed = true;
    }
    delay(1000);
  }
  logInfoln("DHT20初始化成功");
  Serial2.begin(9600);
  logInfoln("JW01初始化成功"); 
}
// 按键声
void Dida(){
  if(voice){
    tone(BUZZER, 800);
    delay(40);
    noTone(BUZZER);
  }
}
///////////////////////////////////Freertos区域///////////////////////////////////////
// 获取传感器数据
void getJW01Data(){ 
  int count = 0;
  // 检查是否有足够数据可读
  while(Serial2.available()){
    if(count < 9){
      packet[count] = Serial2.read();
    }else{
      Serial2.read();
    }
    count++;
  }
  // Serial.println(count);
  // for(int i = 0;i < 9; i++){
  //   Serial.println(packet[i]);
  // }
  if(count==9 && packet[0] == 44 && packet[1] == 228){ // 校验数据
    // 计算污染气体浓度值
    float f_tvoc = (packet[2] * 256 + packet[3]) * 0.01;
    float f_ch2o = (packet[4] * 256 + packet[5]) * 0.01;
    float f_co2 = packet[6] * 256 + packet[7];
    // 进行一系列判断，垃圾模块，便宜没好货，太难伺候了！！！
    if(f_tvoc >= 3.0f){ // 大于3可能是电压不足，也可能是湿度增加引起的脏数据
      dirtyDataCount++;
      if(dirtyDataCount >= 180){ // 连续采集180次（3分钟）都是脏数据，重启jw01
        digitalWrite(JW01_SW, LOW);
        vTaskDelay(1000);
        digitalWrite(JW01_SW, HIGH);
        dirtyDataCount = 0;
      }
    }else{
      dirtyDataCount = 0;
    }
    if(f_tvoc > 10.0f) f_tvoc-=10.0f;
    if(tvoc.equals("0.00")){
      tvoc = String(f_tvoc, 2);
      ch2o = String(f_ch2o, 2);
      co2 = String(f_co2, 0);
    }else{
      // 取平均值，增加数据平滑性
      tvoc = String((tvoc.toFloat() + f_tvoc) / 2, 2);
      ch2o = String((ch2o.toFloat() + f_ch2o) / 2, 2);
      co2 = String((co2.toFloat() + f_co2) / 2, 0);
    }
    logDebug("TVOC: ");logDebug(tvoc);logDebugln(" mg/m3");
    logDebug("CH2O: ");logDebug(ch2o);logDebugln(" mg/m3");
    logDebug("CO2: ");logDebug(co2);logDebugln(" PPM");
  }
}
void getDHTData(){
  float tmp = dht20.getTemperature();
  // Serial.println(tmp);
  tmp-=tempOffset; 
  // Serial.println(tmp);
  float hum = dht20.getHumidity()*100;
  // DHT20温度范围-40 - 80℃，超过80，就是脏数据
  if(tmp > 80.0f){
    return;
  }
  if(temperature.equals("0.0")){
    temperature = String(tmp, 1);
    humidity = String(hum, 1);
  }else{
    // 取平均值，增加数据平滑性
    temperature = String((temperature.toFloat() + tmp) / 2, 1);
    humidity = String((humidity.toFloat() + hum) / 2, 1);
  }
  logDebug("temperature: ");logDebug(temperature);logDebugln("℃");
  logDebug("humidity: ");logDebug(humidity);logDebugln(" %RH");
  logDebugln("------------------------------------------------------");
}
uint32_t adcRead(){
    long sum = 0;
    for (int i = 0; i < ADC_FREQUENCY; i++){
      sum += adc1_get_raw(ADC_CHANNEL);
      vTaskDelay(1);
    }
    return esp_adc_cal_raw_to_voltage((sum / (float)ADC_FREQUENCY), adcChar);;
}
// 任务句柄
TaskHandle_t anotherCoreTask;
TaskHandle_t drawLoadingTask;
TaskHandle_t fadeOnTask;
TaskHandle_t adcTask;
// 任务内容
void anotherCore_task(void *pvParameters){
  logInfoln(String("核心") + String(xPortGetCoreID()) + String("开始执行传感器任务"));
  // 初始化按键
  btnInit();
  logInfoln("按键初始化成功");
  // 开始定时更新天气的任务
  startTickerUpdateWeather();
  // 循环获取数据
  while(true){
    if(mode == ONLINE_MODE && wifiConnected()){
      timeClient.update();
    }    
    getJW01Data();
    getDHTData();
    vTaskDelay(1000);
  }
  vTaskDelete(anotherCoreTask);
}
void drawLoading_task(void *pvParameters){
  String text = (char *)pvParameters;
  int angle = 0;
  drawLoading(true, text, &angle);
  while(loadingAnim){
    drawLoading(false, text, &angle);
    vTaskDelay(10);
  }
  vTaskDelete(drawLoadingTask);
}
void fadeOn_task(void *pvParameters){
  while(true){
    fadeOn();
    break;
  }
  vTaskDelete(fadeOnTask);
}
void adc_task(void *pvParameters){
  logInfoln(String("核心") + String(xPortGetCoreID()) + String("开始执行ADC采样任务"));
  // 设置ADC相关
  adc1_config_width(ADC_WIDTH_BIT);
  adc1_config_channel_atten(ADC_CHANNEL, ADC_ATTEN_DB);
  adcChar = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB, ADC_WIDTH_BIT, ESP_ADC_CAL_VAL_DEFAULT_VREF, adcChar);
  // 循环获取数据
  while(true){
    // 获取adc电压(mv)
    float adcVoltage = (float)adcRead() / 1000.0f + adcOffset;
    // logDebug("ADC电压: ");
    // logDebug(String(adcVoltage));
    // logDebugln("V");
    // 分压电阻阻值3:1，所以实际电压为ADC电压的四倍
    batteryVoltage = 4 * adcVoltage;
    if(tmpVoltage == 0.00){
      tmpVoltage = batteryVoltage;
    }else{
      if(batteryVoltage >= 4.3f){// 大于4.3V，肯定是typeC供电
        Charging = true;
        tmpVoltage = batteryVoltage;
      }else{
        if((batteryVoltage - tmpVoltage) > 0.01){
          creaseTimes++;
          if(creaseTimes >= 10){
            Charging = true;
            tmpVoltage = batteryVoltage;
            creaseTimes = 0;
          }
        }else{
          creaseTimes = 0;
        }
        if((tmpVoltage - batteryVoltage) > 0.01){
          decreaseTimes++;
          if(decreaseTimes >= 5){
            Charging = false;
            tmpVoltage = batteryVoltage;
            decreaseTimes = 0;
          }
        }else{
          decreaseTimes = 0;
        }
      }
    }
    batteryPercent = ((batteryVoltage - batteryMin) / (batteryMax - batteryMin)) * 100;
    if(batteryPercent > 100){
      batteryPercent = 100;
    }
    if(batteryPercent < 0){
      batteryPercent = 0;
    }
    logDebug("电池电压: ");
    logDebug(String(batteryVoltage));
    logDebugln("V");        
    logDebug("电量: ");
    logDebug(String(batteryPercent));
    logDebugln("%");
    logDebugln("======================================================");
    vTaskDelay(500);
  }
  vTaskDelete(adcTask);
}
// 创建任务
void createAnotherCoreTask(){
  xTaskCreatePinnedToCore(anotherCore_task, "anotherCore_task", 2 * 1024, NULL, 1, &anotherCoreTask, 0);
}
void createDrawLoadingTask(char *text){
  loadingAnim = true;
  xTaskCreatePinnedToCore(drawLoading_task, "drawLoading_task", 2 * 1024, (void *)text, 1, &drawLoadingTask, 0);
}
void createFadeOnTask(){
  xTaskCreatePinnedToCore(fadeOn_task, "fadeOn_task", 1 * 1024, NULL, 1, &fadeOnTask, 0);
}
void createADCTask(){
  xTaskCreatePinnedToCore(adc_task, "adc_task", 2 * 1024, NULL, 1, &adcTask, 0);
}
//////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////定时器区域//////////////////////////////////////////
Ticker ticker_updateWeather;
void updateWeatherTask(){
  if(mode == OFFLINE_MODE || !wifiConnected()){
    return;
  }
  updateWeather = true;
}
void startTickerUpdateWeather(){
  // 每隔一段时间更新一次天气
  ticker_updateWeather.attach(UPDATE_WEATHER_INTERVAL, updateWeatherTask);
}
//////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////// 按键区////////////////////////////////////////
// 按键方法
void btn1click(){
  if(!buttonEnable){
    return;
  }
  Dida();
  switch(currentPage){
    int lastChoosedIndex;
    case SETTING:
      if(!settingChoosed){
        settingChoosed = true;
        drawSettingOrOffline(false, "");
      }
      break;
    case PAGE1:
      buttonEnable = false; // 先停止刷新，停止按键监控
      int tmp;
      int nextBigZone;
      switch(currentBigZone){
        case TEM:
          nextBigZone = CO2;
          break;
        case HUM:
          nextBigZone = TEM;
          break;
        case TVOC:
          nextBigZone = HUM;
          break;
        case CH2O:
          nextBigZone = TVOC;
          break;
        case CO2:
          nextBigZone = CH2O;
          break; 
        default:
          break;      
      }
      for(int i = 0; i <= 20; i++){
        drawPage1Z(currentBigZone, -i * 16, 0);
        drawPage1Z(nextBigZone, i * 8, 0);
      }
      tmp = zoneIndex[nextBigZone];
      zoneIndex[nextBigZone] = zoneIndex[currentBigZone];
      zoneIndex[currentBigZone] = tmp; 
      for(int i = 20; i >= 0; i--){
        drawPage1Z(nextBigZone, i * 16, 0);
        drawPage1Z(currentBigZone, -i * 8, 0);       
      }
      currentBigZone = nextBigZone;
      buttonEnable = true;
      break;
    case CONFIG:
      if(!modalShowed){
        buttonEnable = false;
        lastChoosedIndex = configChoosedIndex;
        configChoosedIndex = (configChoosedIndex - 1) >= 0 ? (configChoosedIndex - 1) : 5;
        drawConfigOption(lastChoosedIndex);
        drawConfigOption(configChoosedIndex);
        buttonEnable = true;
      }else{
        if(configChoosedIndex == OPTION_WLAN || configChoosedIndex == OPTION_RESET){
          modalLeftChoosed = true;
          drawModal("", false);
        }else if(configChoosedIndex == OPTION_BRIGHT){
          if(bright == MIN_BRIGHT){
            return;
          }
          bright--;
          analogWrite(BL, bright);
          drawBrightModal(false);
        }else if(configChoosedIndex == OPTION_OFFSET){
          if(tempOffset >= 0.1){
            tempOffset-=0.1;
            drawOffsetModal(false);
          }
        }
      }
      break;
    case CALENDAR:
      // 上一个月
      buttonEnable = false;
      monthOffset-=1;
      offsetDerection = CALENDAR_LEFT_OFFSET;
      drawCalendarDate();
      buttonEnable = true;
      break;
    case BONGOCAT:
      buttonEnable = false;
      if(backFillColor == BACK_BLACK){
        tft.pushImage(25,90,80,80,left_down_black);
      }else{
        tft.pushImage(25,90,80,80,left_down_white);
      }
      delay(75);
      if(backFillColor == BACK_BLACK){
        tft.pushImage(25,90,80,80,left_up_black);
      }else{
        tft.pushImage(25,90,80,80,left_up_white);
      }
      buttonEnable = true;
      break;
    default:
      break;
  }
}
void btn2click(){
  if(!buttonEnable){
    return;
  }
  Dida();
  switch(currentPage){
    case SETTING:
      if(settingChoosed){ // 开始启动服务器配网或重启重新获取数据
        if(getDataFailed){
          ESP.restart();
        }else{
          buttonEnable = false;
          createDrawLoadingTask("服务器启动中");
          // 开启AP配网
          wifiConfigBySoftAP();
          // 关闭加载动画
          loadingAnim = false;
          fadeOff();
          delay(200);
          // 绘制文字等待用户连接
          draw2LineText("连接CC Air Detector热点", "进入192.168.1.1配置");
          createFadeOnTask();
        }      
      }else{ // 离线模式，进入Page1
        getDataFailed = false;
        mode = OFFLINE_MODE;
        currentPage = PAGE1;
        drawPage1();
      }
      break;
    case CONFIG:
      switch(configChoosedIndex){
        case OPTION_BRIGHT:
          if(!modalShowed){
            modalShowed = true;
            tmpBright = bright;
            drawBrightModal(true);
          }else{
            if(bright != tmpBright){
              setBright();
            }
            modalShowed = false;
            drawConfigOption(2);
            drawConfigOption(3);
          }
          break;
        case OPTION_VOICE:
          voice = !voice;
          setVoice();
          drawConfigOption(configChoosedIndex);
          break;
        case OPTION_THEME:
          exchangeTheme();
          drawConfig();
          break;
        case OPTION_WLAN:
          if(!modalShowed){
            modalLeftChoosed = false;
            drawModal("开始配置网络", true);
            modalShowed = true;
          }else{
            if(modalLeftChoosed){
              disconnectWiFi();
              buttonEnable = false;
              currentPage = SETTING;
              createDrawLoadingTask("服务器启动中");
              // 开启AP配网
              wifiConfigBySoftAP();
              // 关闭加载动画
              loadingAnim = false;
              fadeOff();
              delay(200);
              // 绘制文字等待用户连接
              draw2LineText("连接CC Air Detector热点", "进入192.168.1.1配置");
              createFadeOnTask();
            }else{
              drawConfigOption(0);
              drawConfigOption(1);
              drawConfigOption(2);
              drawConfigOption(3);
              modalShowed = false;
            }
          }
          break;
        case OPTION_RESET:
          if(!modalShowed){
            modalLeftChoosed = false;
            drawModal("即将恢复出厂", true);
            modalShowed = true;
          }else{
            if(modalLeftChoosed){
              clearInfo();
              draw2LineText("已恢复出厂", "即将重启");
              delay(1500);
              ESP.restart(); 
            }else{
              drawConfigOption(0);
              drawConfigOption(1);
              drawConfigOption(2);
              drawConfigOption(3);
              modalShowed = false;
            }
          }
          break;
        case OPTION_OFFSET:
          if(!modalShowed){
            modalShowed = true;
            tmpTempOffset = tempOffset;
            drawOffsetModal(true);
          }else{
            if(tempOffset != tmpTempOffset){
              setTempOffset();
            }
            modalShowed = false;
            drawConfigOption(3);
            drawConfigOption(4);
          }
          break;
        default:
          break;          
      }
      break;
    case CALENDAR:
      // 当前月
      buttonEnable = false;
      if(monthOffset > 0){
        offsetDerection = CALENDAR_LEFT_OFFSET;
      }else{
        offsetDerection = CALENDAR_RIGHT_OFFSET;
      }
      monthOffset = 0;
      drawCalendarDate();
      buttonEnable = true;
      break;
    case BONGOCAT:
      buttonEnable = false;
      if(backFillColor == BACK_BLACK){
        tft.pushImage(25,90,80,80,left_down_black);
        tft.pushImage(140,115,80,80,right_down_black);
      }else{
        tft.pushImage(25,90,80,80,left_down_white);
        tft.pushImage(140,115,80,80,right_down_white);
      }
      delay(75);
      if(backFillColor == BACK_BLACK){
        tft.pushImage(25,90,80,80,left_up_black);
        tft.pushImage(140,115,80,80,right_up_black);
      }else{
        tft.pushImage(25,90,80,80,left_up_white);
        tft.pushImage(140,115,80,80,right_up_white);
      }
      buttonEnable = true;
      break;
    default:
      break;
  }
}
void btn3click(){
  if(!buttonEnable){
    return;
  }
  Dida();
  switch(currentPage){
    int lastChoosedIndex;
    case SETTING:
      if(settingChoosed){
        settingChoosed = false;
        drawSettingOrOffline(false, "");
      } 
      break;
    case PAGE1:
      buttonEnable = false; // 先停止刷新，停止按键监控
      int tmp;
      int nextBigZone;
      switch(currentBigZone){
        case TEM:
          nextBigZone = HUM;
          break;
        case HUM:
          nextBigZone = TVOC;
          break;
        case TVOC:
          nextBigZone = CH2O;
          break;
        case CH2O:
          nextBigZone = CO2;
          break;
        case CO2:
          nextBigZone = TEM;
          break; 
        default:
          break;      
      }
      for(int i = 0; i <= 20; i++){
        drawPage1Z(currentBigZone, i * 16, 0);
        drawPage1Z(nextBigZone, -i * 8, 0);
      }
      tmp = zoneIndex[nextBigZone];
      zoneIndex[nextBigZone] = zoneIndex[currentBigZone];
      zoneIndex[currentBigZone] = tmp; 
      for(int i = 20; i >= 0; i--){
        drawPage1Z(nextBigZone, -i * 16, 0);
        drawPage1Z(currentBigZone, i * 8, 0);       
      }
      currentBigZone = nextBigZone;
      buttonEnable = true;
      break;  
    case CONFIG:
      if(!modalShowed){
        buttonEnable = false;
        lastChoosedIndex = configChoosedIndex;
        configChoosedIndex = (configChoosedIndex + 1) <= 5 ? (configChoosedIndex + 1) : 0;
        drawConfigOption(lastChoosedIndex);
        drawConfigOption(configChoosedIndex);
        buttonEnable = true;
      }else{
        if(configChoosedIndex == OPTION_WLAN || configChoosedIndex == OPTION_RESET){
          modalLeftChoosed = false;
          drawModal("", false);
        }else if(configChoosedIndex == OPTION_BRIGHT){
          if(bright == MAX_BRIGHT){
            return;
          }
          bright++;
          analogWrite(BL, bright);
          drawBrightModal(false);
        }else if(configChoosedIndex == OPTION_OFFSET){
          if(tempOffset < 9.9){
            tempOffset+=0.1;
            drawOffsetModal(false);
          }
        }
      }
      break;
    case CALENDAR:
      // 下一个月
      buttonEnable = false;
      monthOffset+=1;
      offsetDerection = CALENDAR_RIGHT_OFFSET;
      drawCalendarDate();
      buttonEnable = true;
      break;  
    case BONGOCAT:
      buttonEnable = false;
      if(backFillColor == BACK_BLACK){
        tft.pushImage(140,115,80,80,right_down_black);
      }else{
        tft.pushImage(140,115,80,80,right_down_white);
      }
      delay(75);
      if(backFillColor == BACK_BLACK){
        tft.pushImage(140,115,80,80,right_up_black);
      }else{
        tft.pushImage(140,115,80,80,right_up_white);
      } 
      buttonEnable = true;
      break;
    default:
      break;
  }
}
void btn1LongClick(){
  if(!buttonEnable || modalShowed){
    return;
  }
  Dida();
  buttonEnable = false;
  switch(currentPage){   
    case PAGE1:
      fadeOff();
      currentPage = CONFIG;
      drawConfig();
      createFadeOnTask();
      break;
    case PAGE2:
      fadeOff();
      currentPage = PAGE1;
      drawPage1();
      createFadeOnTask();
      break;
    case PAGE3:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = PAGE2;
        drawPage2();
      }else{
        currentPage = PAGE1;
        drawPage1();
      }
      createFadeOnTask();
      break;
    case CALENDAR:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = PAGE3;
        drawPage3(true);
      }else{
        currentPage = PAGE1;
        drawPage1();
      }
      createFadeOnTask();
      break;
    case CONFIG:
      fadeOff();
      currentPage = BONGOCAT;
      drawBongoCat();
      createFadeOnTask();
      break;
    case BONGOCAT:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = CALENDAR;
        drawCalendar();
      }else{
        currentPage = PAGE1;
        drawPage1();
      }
      createFadeOnTask();
      break;  
    default:
      break;
  }
  buttonEnable = true;
}
void btn2LongClick(){
  Dida();
  refreshTFT();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
  logInfoln("进入休眠...");
  esp_deep_sleep_start();
}
void btn3LongClick(){
  if(!buttonEnable || modalShowed){
    return;
  }
  Dida();
  buttonEnable = false;
  switch(currentPage){
    case PAGE1:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = PAGE2;
        drawPage2();
      }else{
        currentPage = BONGOCAT;
        drawBongoCat();
      }
      createFadeOnTask();
      break;
    case PAGE2:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = PAGE3;
        drawPage3(true);
      }else{
        currentPage = CONFIG;
        drawConfig();
      }
      createFadeOnTask();
      break;
    case PAGE3:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = CALENDAR;
        drawCalendar();
      }else{
        currentPage = CONFIG;
        drawConfig();
      }
      createFadeOnTask();
      break;
    case CALENDAR:
      fadeOff();
      currentPage = BONGOCAT;
      drawBongoCat();
      createFadeOnTask();
      break;
    case BONGOCAT:
      fadeOff();
      currentPage = CONFIG;
      drawConfig();
      createFadeOnTask();
      break;  
    case CONFIG:
      fadeOff();
      currentPage = PAGE1;
      drawPage1();
      createFadeOnTask();
      break;  
    default:
      break;
  }
  buttonEnable = true;
}
void bootBtnClick(){
  if(!buttonEnable || modalShowed){
    return;
  }
  Dida();
  buttonEnable = false;
  switch(currentPage){
    case PAGE1:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = PAGE2;
        drawPage2();
      }else{
        currentPage = BONGOCAT;
        drawBongoCat();
      }
      createFadeOnTask();
      break;
    case PAGE2:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = PAGE3;
        drawPage3(true);
      }else{
        currentPage = CONFIG;
        drawConfig();
      }
      createFadeOnTask();
      break;
    case PAGE3:
      fadeOff();
      if(mode == ONLINE_MODE){
        currentPage = CALENDAR;
        drawCalendar();
      }else{
        currentPage = CONFIG;
        drawConfig();
      }
      createFadeOnTask();
      break;
    case CALENDAR:
      fadeOff();
      currentPage = BONGOCAT;
      drawBongoCat();
      createFadeOnTask();
      break;
    case BONGOCAT:
      fadeOff();
      currentPage = CONFIG;
      drawConfig();
      createFadeOnTask();
      break;  
    case CONFIG:
      fadeOff();
      currentPage = PAGE1;
      drawPage1();
      createFadeOnTask();
      break;  
    default:
      break;
  }
  buttonEnable = true;
}
void btn1DuringLongPress(){
  if(modalShowed){
    if(configChoosedIndex == OPTION_BRIGHT){
      if(bright == MIN_BRIGHT){
        return;
      }
      bright--;
      analogWrite(BL, bright);
      drawBrightModal(false);
    }else if(configChoosedIndex == OPTION_OFFSET){
      if(tempOffset >= 0.1){
        tempOffset-=0.1;
        drawOffsetModal(false);
      }
    }
    delay(10);
  }
}
void btn3DuringLongPress(){
  if(modalShowed){
    if(configChoosedIndex == OPTION_BRIGHT){
      if(bright == MAX_BRIGHT){
        return;
      }
      bright++;
      analogWrite(BL, bright);
      drawBrightModal(false);
    }else if(configChoosedIndex == OPTION_OFFSET){
      if(tempOffset < 9.9){
        tempOffset+=0.1;
        drawOffsetModal(false);
      }
    }
    delay(10);
  }
}
// 初始化各按键
void btnInit(){
  button1.attachClick(btn1click);
  button1.setDebounceMs(20); //设置消抖时长 
  button2.attachClick(btn2click);
  button2.setDebounceMs(20); //设置消抖时长 
  button3.attachClick(btn3click);
  button3.setDebounceMs(20); //设置消抖时长 
  bootButton.attachClick(bootBtnClick);
  bootButton.setDebounceMs(20); //设置消抖时长 
  button1.attachLongPressStart(btn1LongClick);
  button1.setPressMs(1200); //设置长按时间
  button2.attachLongPressStart(btn2LongClick);
  button2.setPressMs(1200); //设置长按时间
  button3.attachLongPressStart(btn3LongClick);
  button3.setPressMs(1200); //设置长按时间
  button1.attachDuringLongPress(btn1DuringLongPress);
  button3.attachDuringLongPress(btn3DuringLongPress);
}
// 监控按键
void watchBtn(){
  button1.tick();
  button2.tick();
  button3.tick();
  bootButton.tick();
}
//////////////////////////////////////////////////////////////////////////////////////

