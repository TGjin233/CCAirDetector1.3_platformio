#include "stockAPI.h"
#include "LogUtil.h"
#include <WiFi.h>

#define STOCK_HTTP_TIMEOUT 10000

// 用户关注的基金列表 - 在这里添加你想关注的基金代码
const char* WATCHED_STOCKS[] = {
  "159001",   // 中欧远见
  "026622",   // 中欧远见
};
const int WATCHED_STOCKS_COUNT = sizeof(WATCHED_STOCKS) / sizeof(WATCHED_STOCKS[0]);

// 存储所有关注股票的数据
StockRealData watchedStocks[WATCHED_STOCKS_COUNT];

void fetchStockRealData(const String& stockCode, StockRealData& data) {
  HTTPClient http;
  String code = stockCode;
  code.replace(".SH", "");
  code.replace(".SZ", "");
  
  String url = "https://" + String(STOCK_API_HOST) + "/fund/real/ssjy/" + code + "?token=" + String(STOCK_TOKEN);
  
  logInfo("获取基金数据: ");
  logInfoln(stockCode);
  
  http.begin(url);
  http.setConnectTimeout(STOCK_HTTP_TIMEOUT);
  http.setTimeout(STOCK_HTTP_TIMEOUT);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_RET_OK) {
    String payload = http.getString();
    logInfoln("原始数据:");
    logInfoln(payload);
    
    // 解析JSON数据（只需要4KB就足够了）
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // 清空数据
      memset(&data, 0, sizeof(StockRealData));
      
      // 检查数据是否有效
      if (doc.containsKey("p") || doc.containsKey("price")) {
        data.code = stockCode;
        
        // 提取各字段（基金API可能有不同的字段名）
        if (doc.containsKey("p")) data.currentPrice = doc["p"].as<float>();
        if (doc.containsKey("price")) data.currentPrice = doc["price"].as<float>();
        if (doc.containsKey("ud")) data.changeAmount = doc["ud"].as<float>();
        if (doc.containsKey("change")) data.changeAmount = doc["change"].as<float>();
        if (doc.containsKey("pc")) data.changePercent = doc["pc"].as<float>();
        if (doc.containsKey("changePercent")) data.changePercent = doc["changePercent"].as<float>();
        if (doc.containsKey("o")) data.openPrice = doc["o"].as<float>();
        if (doc.containsKey("open")) data.openPrice = doc["open"].as<float>();
        if (doc.containsKey("h")) data.highPrice = doc["h"].as<float>();
        if (doc.containsKey("high")) data.highPrice = doc["high"].as<float>();
        if (doc.containsKey("l")) data.lowPrice = doc["l"].as<float>();
        if (doc.containsKey("low")) data.lowPrice = doc["low"].as<float>();
        if (doc.containsKey("yc")) data.closePrice = doc["yc"].as<float>();
        if (doc.containsKey("close")) data.closePrice = doc["close"].as<float>();
        if (doc.containsKey("v")) data.volume = doc["v"].as<float>();
        if (doc.containsKey("volume")) data.volume = doc["volume"].as<float>();
        if (doc.containsKey("cje")) data.amount = doc["cje"].as<float>();
        if (doc.containsKey("amount")) data.amount = doc["amount"].as<float>();
        if (doc.containsKey("zf")) data.amplitude = doc["zf"].as<float>();
        if (doc.containsKey("amplitude")) data.amplitude = doc["amplitude"].as<float>();
        if (doc.containsKey("tr")) data.turnoverRate = doc["tr"].as<float>();
        if (doc.containsKey("turnoverRate")) data.turnoverRate = doc["turnoverRate"].as<float>();
        if (doc.containsKey("pe")) data.pe = doc["pe"].as<float>();
        if (doc.containsKey("pb_ratio")) data.pb = doc["pb_ratio"].as<float>();
        if (doc.containsKey("pb")) data.pb = doc["pb"].as<float>();
        if (doc.containsKey("mc")) data.name = doc["mc"].as<String>();
        if (doc.containsKey("name")) data.name = doc["name"].as<String>();
      } else {
        logError("股票数据格式错误");
      }
    } else {
      logError("JSON解析失败: " + String(error.c_str()));
    }
  } else {
    logError("HTTP请求失败: " + String(httpCode));
  }
  
  http.end();
}

void fetchWatchedStocks() {
  logInfoln("========== 获取关注基金数据 ==========");
  
  for (int i = 0; i < WATCHED_STOCKS_COUNT; i++) {
    logInfo("正在获取 ");
    logInfo(String(i + 1));
    logInfo("/");
    logInfoln(String(WATCHED_STOCKS_COUNT));
    
    fetchStockRealData(WATCHED_STOCKS[i], watchedStocks[i]);
    delay(100);  // 避免请求过快
  }
  
  logInfoln("====================================");
  printAllWatchedStocks();
}

void printStockRealData(const StockRealData& data) {
  logInfoln("基金代码: " + data.code);
  logInfoln("基金名称: " + data.name);
  logInfoln("当前价格: " + String(data.currentPrice));
  logInfoln("涨跌额: " + String(data.changeAmount) + " (" + String(data.changePercent) + "%)");
  logInfoln("开盘价: " + String(data.openPrice));
  logInfoln("最高价: " + String(data.highPrice));
  logInfoln("最低价: " + String(data.lowPrice));
  logInfoln("昨收价: " + String(data.closePrice));
  logInfoln("成交量: " + String(data.volume) + " 手");
  logInfoln("成交额: " + String(data.amount / 100000000, 2) + " 亿元");
  logInfoln("振幅: " + String(data.amplitude) + "%");
  logInfoln("换手率: " + String(data.turnoverRate) + "%");
  if (data.pe > 0) logInfoln("市盈率(PE): " + String(data.pe));
  if (data.pb > 0) logInfoln("市净率(PB): " + String(data.pb));
  logInfoln("----------------------------------");
}

void printAllWatchedStocks() {
  logInfoln("========== 关注基金汇总 ==========");
  for (int i = 0; i < WATCHED_STOCKS_COUNT; i++) {
    if (watchedStocks[i].currentPrice > 0) {
      logInfo(String(i + 1));
      logInfo(". ");
      logInfo(watchedStocks[i].code);
      logInfo(" | 名称: ");
      logInfo(watchedStocks[i].name);
      logInfo(" | 价格: ");
      logInfo(String(watchedStocks[i].currentPrice));
      logInfo(" | 涨跌: ");
      logInfoln(String(watchedStocks[i].changePercent) + "%");
    }
  }
  logInfoln("==================================");
}
