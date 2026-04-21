#ifndef __COMMON_H
#define __COMMON_H

#include "LogUtil.h"
#include "img/wea/xue.h"
#include "img/wea/lei.h"
#include "img/wea/shachen.h"
#include "img/wea/wu.h"
#include "img/wea/bingbao.h"
#include "img/wea/yun.h"
#include "img/wea/yu.h"
#include "img/wea/yin.h"
#include "img/wea/qing.h"
#include "img/wea/xue_black.h"
#include "img/wea/lei_black.h"
#include "img/wea/shachen_black.h"
#include "img/wea/wu_black.h"
#include "img/wea/bingbao_black.h"
#include "img/wea/yun_black.h"
#include "img/wea/yu_black.h"
#include "img/wea/yin_black.h"
#include "img/wea/qing_black.h"
#include "img/page2_white.h"
#include "img/page2_black.h"
#include "img/bongoCat/bongoCat_black.h"
#include "img/bongoCat/bongoCat_white.h"
#include "img/bongoCat/left_down_black.h"
#include "img/bongoCat/left_down_white.h"
#include "img/bongoCat/left_up_black.h"
#include "img/bongoCat/left_up_white.h"
#include "img/bongoCat/right_down_black.h"
#include "img/bongoCat/right_down_white.h"
#include "img/bongoCat/right_up_black.h"
#include "img/bongoCat/right_up_white.h"
#include "fonts/name_24.h"
#include "fonts/projectName_26.h"
#include "fonts/settingPage_22.h"
#include "fonts/iconFont_16.h"
#include "fonts/iconFont_20.h"
#include "fonts/iconFont_22.h"
#include "fonts/unit_time_16.h"
#include "fonts/pollutantName_18.h"
#include "fonts/page1Num_64.h"
#include "fonts/page1Num_35.h"
#include "fonts/page2SensorNum_22.h"
#include "fonts/page2sensor_16.h"
#include "fonts/page3Num_90.h"
#include "fonts/page3_18.h"
#include "fonts/configTitle_26.h"
#include "fonts/configOption_18.h"
#include "fonts/batteryNum_14.h"
#include "fonts/calendar_22.h"
#include "fonts/calendar_18.h"

#define DEVELOP_MODE  false
#define NTP   "ntp5.aliyun.com"
#define HTTP_CODE_OK  200
#define ONLINE_MODE   0
#define OFFLINE_MODE  1
#define BACK_BLACK    0
#define BACK_WHITE    1
#define BRIGHT        50  //屏幕默认亮度
#define MAX_BRIGHT    196  //屏幕最大亮度
#define MIN_BRIGHT    1    //屏幕最小亮度
#define BL         17  //屏幕背光
#define BUZZER     13  //蜂鸣器
#define BTN1       25 // 按钮1 左移
#define BTN2       26 // 按钮2 确定
#define BTN3       27 // 按钮3 右移
#define BOOT_BTN   0  // BOOT按键（GPIO0）
#define JW01_SW    12 // JW01开关
#define TIME_CHECK_INTERVAL   10800 // NTP对时间隔（s） 3小时进行一次对时
#define UPDATE_WEATHER_INTERVAL   7200 // 更新天气间隔（s） 2小时更新一次天气
#define DATA_FAILED_TIMES  10  // NTP和获取天气失败次数，达到指定次数，进入离线模式

// 和风天气接口
const String cityURL = "/geo/v2/city/lookup";  // 查询城市代码的接口
const String nowURL = "/v7/weather/now";  // 实时天气接口
const String airURL = "/airquality/v1/current/";  // 空气质量接口
// 页面枚举
enum CurrentPage{
  SETTING, PAGE1, PAGE2, PAGE3, CALENDAR, BONGOCAT, CONFIG
};
// 定义结构体
typedef struct {
  String text;
  int icon;
  int temp;
  String feelsLike;
  String win;
  String vis;
  int humidity;
  int air;
  String pm10;
  String pm2p5;
  String no2;
  String so2;
  String co;
  String o3;
} Weather;
// 配置WiFi的网页代码
const String ROOT_HTML_PAGE1 PROGMEM = R"rawliteral(
  <!DOCTYPE html><html lang='zh'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <link href='favicon.ico' rel='shortcut icon'>
    <title>CC空气监测仪配置页面</title>
    <style type='text/css'>
        #titleDiv{
            margin-top: 20px;
            height: 10%;
            width: 100%;
            text-align: center;
            font-size: 2rem;
            font-weight: bold;
        }
        .titleOption{
            text-align: center;
            margin-top: 30px;
            height: 40px;
            background-color: dodgerblue; 
            position: relative;
            color: #ffffff;
            border-radius: 5px;
            line-height: 40px;
        }
        #selectDiv {
            margin-top: 20px;
            height: 40px;
            border-radius: 5px;
            box-shadow: 0 0 5px #ccc;
            position: relative;   
        }
        select {
            border: none;
            outline: none;
            width: 100%;
            height: 40px;
            line-height: 40px;
            appearance: none;
            -webkit-appearance: none;
            -moz-appearance: none;
            text-align: center;
            font-size: 1rem;
        }
        .passAndCity{
            border: none;
            margin-top: 20px;
            height: 40px;
            border-radius: 5px;
            box-shadow: 0 0 5px #ccc;
            font-size: 1rem;
            position: relative;
            text-align: center;
        }
        #sub{
            text-align: center;
            margin-top: 50px;
            height: 40px;
            background-color: dodgerblue; 
            position: relative;
            color: #ffffff;
            border-radius: 5px;
            line-height: 40px;
            cursor: pointer;
        }
        #tail{
            font-size: 0.9rem;
            margin-top: 5px;
            width: 100%;
            text-align: center;
            color: #757575;
        }
    </style>
</head>
<body>
    <div id='titleDiv'>CC Air Detector</div>
    <div id='tail'>呈杰希工作室&nbsp&nbsp&nbsp&nbsp出品</div>
    <form action='configwifi' method='post' id='form' accept-charset="UTF-8">
        <div class='titleOption commonWidth'>WiFi名称</div>
        <div id='selectDiv' class='commonWidth'>
            <select name='ssid' id='ssid'>
                <option value=''></option>
)rawliteral";
const String ROOT_HTML_PAGE2 PROGMEM = R"rawliteral(
  </select>
        </div>
        <div class='titleOption commonWidth'>WiFi密码</div>
        <input type='text' placeholder='请输入WiFi密码' name='pass' id='pass' class='passAndCity commonWidth'>
        <div class='titleOption commonWidth'>城市名称（ 无需"市区县", 例 : 江阴 ）</div>
        <input type='text' placeholder='请输入城市名称' name='city' id='city' class='passAndCity commonWidth'>
        <div class='titleOption commonWidth'>上级行政区划</div>
        <input type='text' placeholder='用于区分重名城市、区县，可不填' name='adm' id='adm' class='passAndCity commonWidth'>
        <div id='sub' onclick='doSubmit()'>提交</div>
    </form>
    <script type='text/javascript'>
        function doSubmit(){
            var select = document.getElementById('ssid');
            var selectValue = select.options[select.selectedIndex].value;
            if(selectValue == ''){
                alert('请选择要连接的WiFi');
                return;
            }
            if(document.getElementById('pass').value == ''){
                alert('请输入该WiFi的密码');
                return;
            }
            if(document.getElementById('city').value == ''){
                alert('请输入城市名称');
                return;
            }
            document.getElementById('form').submit();
        }
        var nodes = document.getElementsByClassName('commonWidth');
        var node = document.getElementById('sub');
        var screenWidth = window.screen.width;
        function setWidth(width){
            nodes[0].setAttribute('style',width);
            nodes[1].setAttribute('style',width);
            nodes[2].setAttribute('style',width);
            nodes[3].setAttribute('style',width);
            nodes[4].setAttribute('style',width);
            nodes[5].setAttribute('style',width);
            nodes[6].setAttribute('style',width);
            nodes[7].setAttribute('style',width);
        }
        if(screenWidth > 1000){
            setWidth('width: 40%;left: 30%;');
            node.setAttribute('style','width: 14%;left: 43%;');
        }else if(screenWidth > 800 && screenWidth <= 1000){
            setWidth('width: 50%;left: 25%;');
            node.setAttribute('style','width: 16%;left: 42%;');
        }else if(screenWidth > 600 && screenWidth <= 800){
            setWidth('width: 60%;left: 20%;');
            node.setAttribute('style','width: 20%;left: 40%;');
        }else if(screenWidth > 400 && screenWidth <= 600){
            setWidth('width: 74%;left: 13%;');
            node.setAttribute('style','width: 26%;left: 37%;');
        }else{
            setWidth('width: 90%;left: 5%;');
            node.setAttribute('style','width: 40%;left: 30%;');
        }
    </script>
</body>
</html>
)rawliteral";  
#endif
