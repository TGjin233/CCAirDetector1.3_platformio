#include "nowAPI.h"
#include "LogUtil.h"
#include <WiFi.h>

#define NOWAPI_HTTP_TIMEOUT 10000

void fetchGlobalIndex() {
  HTTPClient http;

  // 构建请求参数
  String inxids = "1010";  // 上证指数、深证成指

  // 构建URL
  String url = "https://" + String(NOWAPI_HOST) + "/?app=finance.globalindex&inxids=" + inxids + "&appkey=" + String(NOWAPI_APPKEY) + "&sign=" + String(NOWAPI_SIGN) + "&format=json";

  logInfo("URL: ");
  logInfoln(url);

  http.begin(url);
  http.setConnectTimeout(NOWAPI_HTTP_TIMEOUT);
  http.setTimeout(NOWAPI_HTTP_TIMEOUT);

  int httpCode = http.GET();

  if (httpCode == HTTP_RET_OK) {
    String payload = http.getString();
    logInfoln("========== 获取全球指数数据 ==========");
    logInfo("数据长度: ");
    logInfoln(String(payload.length()));
    logInfoln("原始数据:");
    logInfoln(payload);
    logInfoln("======================================");

    // 解析JSON数据
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // 检查成功标志
      if (doc["success"] == "1") {
        JsonObject result = doc["result"];

        // 获取指数列表
        JsonObject lists = result["lists"];

        // 遍历每个指数
        for (JsonPair index : lists) {
          JsonObject item = index.value().as<JsonObject>();

          IndexData data;
          data.inxid = item["inxid"].as<const char*>();
          data.inxnm = item["inxnm"].as<const char*>();
          data.lastPrice = item["last_price"].as<float>();
          data.riseFall = item["rise_fall"].as<float>();
          data.riseFallPer = item["rise_fall_per"].as<const char*>();
          data.openPrice = item["open_price"].as<float>();
          data.highPrice = item["high_price"].as<float>();
          data.lowPrice = item["low_price"].as<float>();
          data.yesyPrice = item["yesy_price"].as<float>();
          data.amplitude = item["amplitude_price_per"].as<const char*>();
          data.volume = item["volume"].as<float>();
          data.turnover = item["turnover"].as<float>();
          data.uptime = item["uptime"].as<const char*>();

          printIndexData(data);
          logInfoln("");
        }
      } else {
        logError("API返回失败: ");
        logError(doc["msgid"].as<const char*>());
        logError(doc["msg"].as<const char*>());
      }
    } else {
      logError("JSON解析失败: ");
      logError(error.c_str());
    }
  } else {
    logError("HTTP请求失败: ");
    logError(String(httpCode));
  }

  http.end();
}

void printIndexData(const IndexData& data) {
  logInfoln("----------------------------------------");
  logInfoln("指数名称: " + data.inxnm);
  logInfoln("指数编号: " + data.inxid);
  logInfoln("当前价格: " + String(data.lastPrice));
  logInfoln("涨跌额: " + String(data.riseFall));
  logInfoln("涨跌幅: " + data.riseFallPer);
  logInfoln("开盘价: " + String(data.openPrice));
  logInfoln("最高价: " + String(data.highPrice));
  logInfoln("最低价: " + String(data.lowPrice));
  logInfoln("昨收价: " + String(data.yesyPrice));
  logInfoln("振幅: " + data.amplitude);
  logInfoln("成交量: " + String(data.volume));
  logInfoln("成交额: " + String(data.turnover));
  logInfoln("更新时间: " + data.uptime);
  logInfoln("----------------------------------------");
}
