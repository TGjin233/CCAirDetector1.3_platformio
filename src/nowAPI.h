#ifndef __NOWAPI_H
#define __NOWAPI_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Common.h"

// NowAPI配置
#define NOWAPI_HOST "sapi.k780.com"
#define NOWAPI_APPKEY "79063"
#define NOWAPI_SIGN "7328f4ca82939be6741f091daff4ef4f"

// 指数数据相关
struct IndexData {
  String inxid;           // 指数编号
  String inxnm;           // 指数名称
  float lastPrice;        // 当前价
  float riseFall;         // 涨跌额
  String riseFallPer;     // 涨跌幅(%)
  float openPrice;        // 开盘价
  float highPrice;        // 最高价
  float lowPrice;         // 最低价
  float yesyPrice;        // 昨日收盘价
  String amplitude;       // 振幅
  float volume;           // 成交量
  float turnover;         // 成交额
  String uptime;          // 数据更新时间
};

// 函数声明
void fetchGlobalIndex();
void parseIndexData(const String& jsonData);
void printIndexData(const IndexData& data);

#endif
