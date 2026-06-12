#ifndef __NOWAPI_H
#define __NOWAPI_H

#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Common.h"
// http://121.40.168.132/
// 自建服务器配置
#define STOCK_API_HOST "121.40.168.132"
#define STOCK_API_PORT 8000
#define STOCK_API_TOKEN "stock-api-token-2024"
#define STOCK_API_INDEX_URL "/api/index"

// API频率限制配置
#define INDEX_API_MAX_PER_HOUR  10    // 每小时最大请求次数
#define INDEX_API_MIN_INTERVAL  60000  // 最小请求间隔(ms) = 1分钟

// 指数数据相关
struct IndexData {
  String inxid;           // 指数编号 (如 sh000001)
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

// 关注的指数数量
extern const int WATCHED_INDICES_COUNT;

// 全局存储
extern IndexData globalIndexData[];

// 函数声明
void fetchGlobalIndex();       // 获取所有指数数据（单次请求）
void displayStoredIndices();   // 只显示缓存数据（不发起网络请求）
void loadAllIndicesFromSPIFFS();
void printIndexData(const IndexData& data);
void printAllIndices();
void initSPIFFS();
unsigned long getIndexLastFetchTime();

#endif
