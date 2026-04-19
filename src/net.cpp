#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include "ArduinoZlib.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"
#include "Task.h"
#include "net.h"
#include "JwtUtil.h"

// 和风天气身份认证，需要替换成你们自己的
char PrivateKey[] = "MC4CAQAwBQYDK2VwBCIEIJ2chxNmNiqzo4mVHEKXEopevYWvBaUoWU7gurXBiCHt";   // 私钥
char PublicKey[] = "MCowBQYDK2VwAyEAa6+RJUEWILK43n7xapze5FW0YtfR//2IpRMFx5JWnUU=";        // 公钥
String KeyID = "TNPKENUB54";                                                              // 凭据ID
String ProjectID = "4N2B2MJ7BH";                                                          // 项目ID
String ApiHost = "mp5ctr76fm.re.qweatherapi.com";                                         // API Host   

// Wifi相关
String ssid;  //WIFI名称
String pass;  //WIFI密码
String city;  // 城市
String adm; // 上级城市区划
String location; // 城市ID
String lat; // 经度
String lon; // 纬度
String WifiNames; // 根据搜索到的wifi生成的option字符串
bool connected = true; // 是否成功连接网络
// SoftAP相关
const char *APssid = "CC Air Detector";
IPAddress staticIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);
WebServer server(80);
// 查询天气超时时间(ms)
int queryTimeout = 5000;
// 天气接口相关
static HTTPClient httpClient;
String data = "";
uint8_t *outbuf;
Weather weather; // 记录查询到的天气数据
bool queryWeatherSuccess = false;
bool queryAirSuccess = false;
// 对时相关
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP, 8 * 3600, TIME_CHECK_INTERVAL * 1000);

// 开启SoftAP进行配网
void wifiConfigBySoftAP(){
  // 开启AP模式
  startAP();
  // 扫描WiFi,并将扫描到的WiFi组成option选项字符串
  scanWiFi();
  // 启动服务器
  startServer();
}
// 开启AP模式
void startAP(){
  logInfoln("开启AP模式...");
  WiFi.enableAP(true); // 使能AP模式
  //传入参数静态IP地址,网关,掩码
  WiFi.softAPConfig(staticIP, gateway, subnet);
  if (!WiFi.softAP(APssid)) {
    logInfoln("AP模式启动失败");
  }  
  logInfoln("AP模式启动成功");
}
// 扫描WiFi,并将扫描到的Wifi组成option选项字符串
void scanWiFi(){
  logInfoln("开始扫描WiFi");
  int n = WiFi.scanNetworks();
  if (n){
    logInfo("扫描到");
    logInfo(String(n));
    logInfoln("个WIFI");
    WifiNames = "";
    for (size_t i = 0; i < n; i++){
      int32_t rssi = WiFi.RSSI(i);
      String signalStrength;
      if(rssi >= -35){
        signalStrength = " (信号极强)";
      }else if(rssi >= -50){
        signalStrength = " (信号强)";
      }else if(rssi >= -70){
        signalStrength = " (信号中)";
      }else{
        signalStrength = " (信号弱)";
      }
      WifiNames += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + signalStrength + "</option>";
    }
  }else{
    logInfoln("没扫描到WIFI");
  }
}
// 处理404情况的函数'handleNotFound'
void handleNotFound(){
  handleRoot();
}
// 处理网站根目录的访问请求
void handleRoot(){
  server.send(200,"text/html", ROOT_HTML_PAGE1 + WifiNames + ROOT_HTML_PAGE2);
}
// 提交数据后的提示页面
void handleConfigWifi(){
  //判断是否有WiFi名称
  if (server.hasArg("ssid")){
    logInfo("获得WiFi名称:");
    ssid = server.arg("ssid");
    logInfoln(ssid);
  }else{
    logInfoln("错误, 没有发现WiFi名称");
    server.send(200, "text/html", "<meta charset='UTF-8'>错误, 没有发现WiFi名称");
    return;
  }
  //判断是否有WiFi密码
  if (server.hasArg("pass")){
    logInfo("获得WiFi密码:");
    pass = server.arg("pass");
    logInfoln(pass);
  }else{
    logInfoln("错误, 没有发现WiFi密码");
    server.send(200, "text/html", "<meta charset='UTF-8'>错误, 没有发现WiFi密码");
    return;
  }
  //判断是否有city名称
  if (server.hasArg("city")){
    if(!server.arg("city").equals(city) ){
      location = "";
    }
    logInfo("获得城市:");
    city = server.arg("city");
    logInfoln(city);
  }else{
    logInfoln("错误, 没有发现城市名称");
    server.send(200, "text/html", "<meta charset='UTF-8'>错误, 没有发现城市名称");
    return;
  }
  logInfo("获得上级区划:");
  adm = server.arg("adm");
  logInfoln(adm);
  // 将信息存入nvs中
  setInfo();
  // 获得了所需要的一切信息，给客户端回复
  server.send(200, "text/html", "<meta charset='UTF-8'><style type='text/css'>body {font-size: 2rem;}</style><br/><br/>WiFi: " + ssid + "<br/>密码: " + pass + "<br/>城市: " + city + "<br/>上级区划: " + adm + "<br/>已取得相关信息,即将尝试连接,请手动关闭此页面。");
  // 绘制重启提示文字
  draw2LineText("已取得配置信息", "即将重启");
  delay(1500);
  ESP.restart();
}
// 启动服务器
void startServer(){
  // 当浏览器请求服务器根目录(网站首页)时调用自定义函数handleRoot处理，设置主页回调函数，必须添加第二个参数HTTP_GET，否则无法强制门户
  server.on("/", HTTP_GET, handleRoot);
  // 当浏览器请求服务器/configwifi(表单字段)目录时调用自定义函数handleConfigWifi处理
  server.on("/configwifi", HTTP_POST, handleConfigWifi);
  // 当浏览器请求的网络资源无法在服务器找到时调用自定义函数handleNotFound处理   
  server.onNotFound(handleNotFound);
  server.begin();
  logInfoln("服务器启动成功！");
}
// 处理服务器请求
void doClient(){
  server.handleClient();
}
// 连接WiFi
void connectWiFi(int timeOut_s){
  int connectTime = 0; //用于连接计时，如果长时间连接不成功，复位设备
  logInfoln("正在连接网络 ");
  logInfo("ssid: ");logInfoln(ssid);
  logInfo("pass: ");logInfoln(pass);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    logInfo(".");
    delay(500);
    connectTime++;
    if (connectTime > 2 * timeOut_s){
      logInfoln("网络连接失败");
      connected = false;
      break;
    }
  }
  if(connected){
    logInfoln("网络连接成功");
  }else{
    // 关闭加载动画
    loadingAnim = false;
    fadeOff();
    delay(200);
    // 绘制setting页面
    drawSettingOrOffline(true, "连接失败，重新配置？");
    // 使能按键
    buttonEnable = true;  
    // 断开WIFI
    WiFi.disconnect();
    // 创建屏幕渐显任务
    createFadeOnTask();
  } 
}
// 检查WiFi状态
bool wifiConnected(){
  if(WiFi.status() == WL_CONNECTED){
    return true;
  }else{
    return false;
  }
}
// url中文编码
String urlEncode(const String& text){
  String encodedText = "";
  for (size_t i = 0; i < text.length(); i++) {
    char c = text[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedText += c;
    } else if (c == ' ') {
      encodedText += '+';
    } else {
      encodedText += '%';
      char hex[4];
      sprintf(hex, "%02X", (uint8_t)c);
      encodedText += hex;
    }
  }
  return encodedText;
}
// 查询城市id
int getCityID(){
  logInfoln("开始获取城市id");
  // 计算jwt
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  // Serial.println(jwt);
  bool flag = false; // 是否成功获取到城市id的标志
  String url = "https://" + ApiHost + cityURL + "?location=" + urlEncode(city) + "&adm=" + urlEncode(adm);
  // logInfoln(url);
  httpClient.setConnectTimeout(queryTimeout * 2);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  // 处理服务器答复
  if (httpCode == HTTP_CODE_OK) {
    // 解压Gzip数据流
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while (httpClient.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();  // 还剩下多少数据没有读完？
      if (size) {
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        if (len > 0) len -= readBytesSize;
        outbuf = (uint8_t *)malloc(sizeof(uint8_t) * 5120);
        uint32_t outprintsize = 0;
        int result = ArduinoZlib::libmpq__decompress_zlib(buff, readBytesSize, outbuf, 5120, outprintsize);
        for (int i = 0; i < outprintsize; i++) {
          data += (char)outbuf[i];
        }
        free(outbuf);
        logInfoln(data);
      }
      delay(1);
    }
    // 解压完，转换json数据
    StaticJsonDocument<2048> doc; //声明一个静态JsonDocument对象
    DeserializationError error = deserializeJson(doc, data); //反序列化JSON数据
    if(!error){ //检查反序列化是否成功
      //读取json节点
      String code = doc["code"].as<const char*>();
      if(code.equals("200")){
        flag = true;
        // 多结果的情况下，取第一个
        city = doc["location"][0]["name"].as<const char*>();
        location = doc["location"][0]["id"].as<const char*>();
        lat = doc["location"][0]["lat"].as<const char*>();
        lon = doc["location"][0]["lon"].as<const char*>();
        logInfoln("城市id :" + location);
        // 将信息存入nvs中
        setInfo();
      }
    }  
  }
  //关闭与服务器连接
  httpClient.end();
  if(!flag){
    logInfo("获取城市id错误：");
    logInfoln(String(httpCode));
    if(httpCode == HTTP_CODE_OK){
      // 关闭加载动画
      loadingAnim = false;
      fadeOff();
      delay(200);
      // 绘制setting页面
      drawSettingOrOffline(true, "城市ID获取失败，重新配置？");
      // 使能按键
      buttonEnable = true;  
      // 断开WIFI
      WiFi.disconnect();
      // 创建屏幕渐显任务
      createFadeOnTask();
    }
  }else{
    logInfoln("获取成功");
  }
  return httpCode;
}
// 查询实时天气
int getWeather(){
  Serial.println("正在获取天气数据");
  // 计算jwt
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  data = "";
  queryWeatherSuccess = false; // 先置为false
  String url = "https://" + ApiHost + nowURL + "?location=" + location;
  httpClient.setConnectTimeout(queryTimeout);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  // Serial.println(ESP.getFreeHeap());
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {
    // 解压Gzip数据流
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while (httpClient.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();  // 还剩下多少数据没有读完？
      // Serial.println(size);
      if (size) {
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        // Serial.println(realsize);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        // Serial.write(buff,readBytesSize);
        if (len > 0) len -= readBytesSize;
        outbuf = (uint8_t *)malloc(sizeof(uint8_t) * 5120);
        uint32_t outprintsize = 0;
        int result = ArduinoZlib::libmpq__decompress_zlib(buff, readBytesSize, outbuf, 5120, outprintsize);
        // Serial.write(outbuf, outprintsize);
        for (int i = 0; i < outprintsize; i++) {
          data += (char)outbuf[i];
        }
        free(outbuf);
        Serial.println(data);
      }
      delay(1);
    }
    // 解压完，转换json数据
    StaticJsonDocument<2048> doc; //声明一个静态JsonDocument对象
    DeserializationError error = deserializeJson(doc, data); //反序列化JSON数据
    if(!error){ //检查反序列化是否成功
      //读取json节点
      String code = doc["code"].as<const char*>();
      if(code.equals("200")){
        queryWeatherSuccess = true;       
        //读取json节点
        weather.text = doc["now"]["text"].as<const char*>();
        weather.icon = doc["now"]["icon"].as<int>();
        weather.temp = doc["now"]["temp"].as<int>();
        String feelsLike = doc["now"]["feelsLike"]; // 体感温度
        weather.feelsLike = "体感温度" + feelsLike + "℃";
        String windDir = doc["now"]["windDir"];
        String windScale = doc["now"]["windScale"];
        weather.win = windDir + windScale + "级";
        weather.humidity = doc["now"]["humidity"].as<int>();
        String vis = doc["now"]["vis"];
        weather.vis = "能见度" + vis + " KM";
        Serial.println("获取成功");
      }
    }  
  }
  //关闭与服务器连接
  httpClient.end();
  if(!queryWeatherSuccess){
    logInfo("获取天气数据错误：");
    logInfoln(String(httpCode));
  }
  return httpCode;
}
// 查询空气质量
int getAir(){
  Serial.println("正在获取空气质量数据");
  // 计算jwt
  String jwt = generateJWT(PrivateKey, PublicKey, KeyID, ProjectID, timeClient.getEpochTime() - 8 * 3600);
  data = "";
  queryAirSuccess = false; // 先置为false
  String url = "https://" + ApiHost + airURL + lat + "/" + lon;
  httpClient.setConnectTimeout(queryTimeout);
  httpClient.begin(url);
  httpClient.addHeader("Authorization", "Bearer " + jwt);
  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();  
  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK) {
    // 解压Gzip数据流
    int len = httpClient.getSize();
    uint8_t buff[2048] = { 0 };
    WiFiClient *stream = httpClient.getStreamPtr();
    while (httpClient.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();  // 还剩下多少数据没有读完？
      // Serial.println(size);
      if (size) {
        size_t realsize = ((size > sizeof(buff)) ? sizeof(buff) : size);
        // Serial.println(realsize);
        size_t readBytesSize = stream->readBytes(buff, realsize);
        // Serial.write(buff,readBytesSize);
        if (len > 0) len -= readBytesSize;
        outbuf = (uint8_t *)malloc(sizeof(uint8_t) * 20480);
        uint32_t outprintsize = 0;
        int result = ArduinoZlib::libmpq__decompress_zlib(buff, readBytesSize, outbuf, 20480, outprintsize);
        // Serial.write(outbuf, outprintsize);
        for (int i = 0; i < outprintsize; i++) {
          data += (char)outbuf[i];
        }
        free(outbuf);
        Serial.println(data);
      }
      delay(1);
    }
    // 解压完，转换json数据
    StaticJsonDocument<2048> doc; //声明一个静态JsonDocument对象
    DeserializationError error = deserializeJson(doc, data); //反序列化JSON数据
    if(!error){ //检查反序列化是否成功
      //读取json节点
      queryAirSuccess = true;
      weather.air = doc["indexes"][0]["aqi"].as<int>();
      // Serial.println(nowWeather.air);
      // 获取 pollutants 数组
      JsonArray pollutants = doc["pollutants"];
      // 遍历数组中的每个对象
      for (JsonObject pollutant : pollutants) {
        String code = pollutant["code"].as<const char*>(); // 获取 code，比如pm2p5,pm10,no2,so2,co,o3
        unsigned int value = pollutant["concentration"]["value"].as<int>(); // 获取值
        // Serial.print(code);
        // Serial.print(": ");
        // Serial.println(value);
        if(code.equals("pm2p5")){
          weather.pm2p5 = String(value);
        }else if(code.equals("pm10")){
          weather.pm10 = String(value);
        }else if(code.equals("no2")){
          weather.no2 = String(value);
        }else if(code.equals("so2")){
          weather.so2 = String(value);
        }else if(code.equals("co")){
          weather.co = String(value);
        }else if(code.equals("o3")){
          weather.o3 = String(value);
        }
      }
      Serial.println("获取成功");
    }  
  } 
  //关闭与服务器连接
  httpClient.end();
  if(!queryAirSuccess){
    logInfo("获取空气质量数据错误：");
    logInfoln(String(httpCode));
  }
  return httpCode;
}
// 断开Wifi
void disconnectWiFi(){
  WiFi.disconnect();
}


