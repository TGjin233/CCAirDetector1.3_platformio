#ifndef __STOCK_API_H
#define __STOCK_API_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Common.h"

// 智兔数服API配置
#define STOCK_API_HOST "api.zhituapi.com"
#define STOCK_TOKEN "00541FD3-C136-4778-B275-82D28C362FF5"

// 股票数据相关
struct StockRealData {
  String code;       // 股票代码
  String name;       // 股票名称（需要单独获取）
  float currentPrice;   // 当前价格
  float changePercent;  // 涨跌幅(%)
  float changeAmount;   // 涨跌额
  float openPrice;      // 开盘价
  float highPrice;      // 最高价
  float lowPrice;       // 最低价
  float closePrice;     // 昨收价
  float volume;         // 成交量(手)
  float amount;         // 成交额(元)
  float amplitude;       // 振幅(%)
  float turnoverRate;    // 换手率(%)
  float pe;             // 市盈率
  float pb;             // 市净率
};

// 用户关注的股票列表
extern const char* WATCHED_STOCKS[];
extern const int WATCHED_STOCKS_COUNT;

// 函数声明
void fetchStockRealData(const String& stockCode, StockRealData& data);
void fetchWatchedStocks();
void printStockRealData(const StockRealData& data);
void printAllWatchedStocks();

#endif
