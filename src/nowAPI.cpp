#include "nowAPI.h"
#include "LogUtil.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <Preferences.h>

#define HTTP_TIMEOUT 10000
#define MAX_INDICES 10  // 最大支持的指数数量
#define SPIFFS_FILE_PATH "/index_data.txt"

// 关注的指数列表 - 使用新服务器的代码格式
const char* WATCHED_INDICES[] = {
  "sh000001",    // 上证指数
  "sz399001",    // 深证成指
  "sz399006",    // 创业板指
  "gb_ixic",     // 纳斯达克
  "gb_ndx",      // 纳斯达克100
};
const int WATCHED_INDICES_COUNT = sizeof(WATCHED_INDICES) / sizeof(WATCHED_INDICES[0]);

// 全局存储
IndexData globalIndexData[MAX_INDICES];

// 频率限制相关
static unsigned long indexLastFetchTime = 0;   // 上次API请求的millis时间戳
static unsigned long indexLastFetchEpoch = 0;  // 上次API请求的epoch时间(用于跨重启保留)
static int requestsThisHour = 0;               // 本小时已请求次数
static unsigned long hourWindowStart = 0;      // 本小时窗口起始时间(millis)
static int nextFetchIndex = 0;                 // 下次要获取的指数索引（轮询）
static Preferences indexPrefs;                 // NVS存储实例
static bool indexRateLimitInited = false;      // 频率限制初始化标志

// 初始化频率限制（从NVS读取上次请求时间）
static void initRateLimit() {
  if (indexRateLimitInited) return;
  indexPrefs.begin("index_api", true);  // 只读模式
  indexLastFetchEpoch = indexPrefs.getULong("last_fetch", 0);
  nextFetchIndex = indexPrefs.getInt("next_idx", 0);
  indexPrefs.end();
  indexRateLimitInited = true;
  
  logInfo("指数API上次请求时间: ");
  logInfoln(String(indexLastFetchEpoch));
  logInfo("下次获取索引: ");
  logInfoln(String(nextFetchIndex));
}

// 保存请求状态到NVS
static void saveFetchState() {
  indexPrefs.begin("index_api", false);  // 读写模式
  indexPrefs.putULong("last_fetch", millis());
  indexPrefs.putInt("next_idx", nextFetchIndex);
  indexPrefs.end();
  indexLastFetchTime = millis();
}

// 检查是否可以发起API请求（频率限制：10次/小时）
static bool canFetchIndex() {
  initRateLimit();
  
  unsigned long now = millis();
  
  // 重置小时窗口（超过1小时）
  if (hourWindowStart == 0 || (now - hourWindowStart) >= 3600000) {
    hourWindowStart = now;
    requestsThisHour = 0;
    logInfoln("指数API小时窗口已重置");
  }
  
  // 检查本小时请求次数
  if (requestsThisHour >= INDEX_API_MAX_PER_HOUR) {
    logInfo("指数API本小时已请求 ");
    logInfo(String(requestsThisHour));
    logInfoln(" 次，已达上限");
    return false;
  }
  
  // 检查最小间隔（防止短时间内多次请求）
  if (indexLastFetchTime > 0 && (now - indexLastFetchTime) < INDEX_API_MIN_INTERVAL) {
    unsigned long remaining = INDEX_API_MIN_INTERVAL - (now - indexLastFetchTime);
    logInfo("指数API请求间隔不足，剩余等待: ");
    logInfo(String(remaining / 1000));
    logInfoln("秒");
    return false;
  }
  
  return true;
}

// 检查所有指数缓存是否都有效
bool isAllIndexCacheFresh() {
  initRateLimit();
  
  // 检查所有指数是否都有有效数据
  for (int i = 0; i < WATCHED_INDICES_COUNT; i++) {
    if (globalIndexData[i].lastPrice <= 0) {
      return false;  // 有缺失数据
    }
  }
  return true;
}

// 获取上次请求时间
unsigned long getIndexLastFetchTime() {
  return indexLastFetchEpoch;
}

// 初始化SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    logError("SPIFFS初始化失败");
  } else {
    logInfoln("SPIFFS初始化成功");
    
    // 检查文件系统是否正常
    size_t totalBytes = SPIFFS.totalBytes();
    size_t usedBytes = SPIFFS.usedBytes();
    logInfo("SPIFFS总空间: ");
    logInfo(String(totalBytes));
    logInfo("  已使用: ");
    logInfoln(String(usedBytes));
  }
}

// 保存所有指数数据到SPIFFS（批量写入，只打开一次文件）
void saveAllIndicesToSPIFFS() {
  File file = SPIFFS.open(SPIFFS_FILE_PATH, FILE_WRITE);
  if (!file) {
    logError("无法打开SPIFFS文件进行写入");
    return;
  }

  for (int i = 0; i < WATCHED_INDICES_COUNT; i++) {
    IndexData& data = globalIndexData[i];
    if (data.lastPrice > 0) {
      file.print(i);
      file.print("|");
      file.print(data.inxid);
      file.print("|");
      file.print(data.inxnm);
      file.print("|");
      file.print(data.lastPrice);
      file.print("|");
      file.print(data.riseFall);
      file.print("|");
      file.print(data.riseFallPer);
      file.print("|");
      file.print(data.openPrice);
      file.print("|");
      file.print(data.highPrice);
      file.print("|");
      file.print(data.lowPrice);
      file.print("|");
      file.print(data.yesyPrice);
      file.print("|");
      file.print(data.amplitude);
      file.print("|");
      file.print(data.volume);
      file.print("|");
      file.print(data.turnover);
      file.print("|");
      file.print(data.uptime);
      file.print("\n");
    }
  }

  file.close();
  logInfoln("已保存所有指数数据到SPIFFS");
}

// 从SPIFFS加载所有指数数据
void loadAllIndicesFromSPIFFS() {
  File file = SPIFFS.open(SPIFFS_FILE_PATH, FILE_READ);
  if (!file) {
    logError("无法打开SPIFFS文件进行读取");
    return;
  }

  int loaded = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int firstPipe = line.indexOf('|');
    int index = line.substring(0, firstPipe).toInt();

    if (index >= 0 && index < MAX_INDICES) {
      IndexData& data = globalIndexData[index];

      int pos = firstPipe + 1;
      int pipePos[15];
      pipePos[0] = firstPipe;
      
      for (int i = 1; i < 14; i++) {
        pipePos[i] = line.indexOf('|', pipePos[i-1] + 1);
        if (pipePos[i] == -1) pipePos[i] = line.length();
      }

      data.inxid = line.substring(pipePos[0] + 1, pipePos[1]);
      data.inxnm = line.substring(pipePos[1] + 1, pipePos[2]);
      data.lastPrice = line.substring(pipePos[2] + 1, pipePos[3]).toFloat();
      data.riseFall = line.substring(pipePos[3] + 1, pipePos[4]).toFloat();
      data.riseFallPer = line.substring(pipePos[4] + 1, pipePos[5]);
      data.openPrice = line.substring(pipePos[5] + 1, pipePos[6]).toFloat();
      data.highPrice = line.substring(pipePos[6] + 1, pipePos[7]).toFloat();
      data.lowPrice = line.substring(pipePos[7] + 1, pipePos[8]).toFloat();
      data.yesyPrice = line.substring(pipePos[8] + 1, pipePos[9]).toFloat();
      data.amplitude = line.substring(pipePos[9] + 1, pipePos[10]);
      data.volume = line.substring(pipePos[10] + 1, pipePos[11]).toFloat();
      data.turnover = line.substring(pipePos[11] + 1, pipePos[12]).toFloat();
      data.uptime = line.substring(pipePos[12] + 1, pipePos[13]);

      loaded++;
    }
  }

  file.close();

  if (loaded > 0) {
    logInfo("从SPIFFS加载了 ");
    logInfo(String(loaded));
    logInfoln(" 个指数数据");
  }
}

// 只显示SPIFFS中存储的数据（不获取网络数据）
void displayStoredIndices() {
  // 初始化SPIFFS
  initSPIFFS();
  
  logInfoln("========== 读取存储的指数数据 ==========");
  
  // 尝试从SPIFFS加载数据
  loadAllIndicesFromSPIFFS();
  
  // 显示数据
  printAllIndices();
  
  // 记录已加载缓存数据，避免后续重复请求
  if (indexLastFetchTime == 0) {
    indexLastFetchTime = millis();
  }
}

// 获取所有指数数据（单次HTTP请求）
static bool fetchAllIndices() {
  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    logError("WiFi未连接，无法获取指数数据");
    return false;
  }
  
  HTTPClient http;

  // 构建URL - 一次请求获取所有主要指数
  String url = "http://" + String(STOCK_API_HOST) + ":" + String(STOCK_API_PORT) + String(STOCK_API_INDEX_URL);

  logInfoln("获取所有指数数据...");

  http.begin(url);
  http.setConnectTimeout(HTTP_TIMEOUT);
  http.setTimeout(HTTP_TIMEOUT);
  http.addHeader("Authorization", String("Bearer ") + STOCK_API_TOKEN);

  int httpCode = http.GET();
  bool success = false;

  if (httpCode == HTTP_RET_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      if (doc["success"] == true) {
        JsonArray dataArray = doc["data"];
        int matched = 0;

        for (JsonObject item : dataArray) {
          String code = item["code"].as<const char*>();
          
          // 查找匹配的关注指数
          for (int i = 0; i < WATCHED_INDICES_COUNT; i++) {
            if (code == WATCHED_INDICES[i]) {
              IndexData& data = globalIndexData[i];
              
              data.inxid = code;
              data.inxnm = item["name"].as<const char*>();
              data.lastPrice = item["current_price"].as<float>();
              data.riseFall = item["change_amount"].as<float>();
              data.riseFallPer = String(item["change_percent"].as<float>()) + "%";
              data.openPrice = item["open_price"].as<float>();
              data.highPrice = item["high_price"].as<float>();
              data.lowPrice = item["low_price"].as<float>();
              data.yesyPrice = item["previous_close"].as<float>();
              
              // 计算振幅
              if (data.yesyPrice > 0) {
                float amplitude = ((data.highPrice - data.lowPrice) / data.yesyPrice) * 100;
                data.amplitude = String(amplitude, 2) + "%";
              } else {
                data.amplitude = "0%";
              }
              
              data.volume = item["volume"].as<float>();
              data.turnover = 0;  // API未返回成交额
              data.uptime = item["timestamp"].as<const char*>();

              matched++;
              break;
            }
          }
        }
        
        if (matched > 0) {
          success = true;
          logInfo("成功获取 ");
          logInfo(String(matched));
          logInfoln(" 个指数数据");
        } else {
          logError("未匹配到关注的指数");
        }
      } else {
        logError("API返回失败");
      }
    } else {
      logError("JSON解析失败: ");
      logError(error.c_str());
    }
  } else {
    logError("HTTP请求失败: ");
    logError(String(httpCode));
    
    // 读取错误响应
    if (httpCode > 0) {
      String errorPayload = http.getString();
      logError("错误信息: ");
      logError(errorPayload);
    }
  }

  http.end();
  return success;
}

// 获取所有指数数据（一次请求获取全部）
void fetchGlobalIndex() {
  // 初始化SPIFFS
  initSPIFFS();

  logInfoln("========== 获取指数数据 ==========");

  // 先从SPIFFS加载缓存数据
  loadAllIndicesFromSPIFFS();

  // 检查频率限制
  if (!canFetchIndex()) {
    logInfoln("频率限制，使用缓存数据");
    printAllIndices();
    return;
  }

  // 发起请求获取所有指数
  bool ok = fetchAllIndices();

  if (ok) {
    requestsThisHour++;
    // 保存状态到NVS
    saveFetchState();
    // 批量保存到SPIFFS
    saveAllIndicesToSPIFFS();
    logInfoln("指数数据获取成功");
  } else {
    logError("指数获取失败，保留缓存数据");
  }

  logInfoln("======================================");
  printAllIndices();
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

void printAllIndices() {
  logInfoln("==========================================");
  logInfoln("            关注指数汇总");
  logInfoln("==========================================");
  for (int i = 0; i < WATCHED_INDICES_COUNT; i++) {
    if (globalIndexData[i].lastPrice > 0) {
      logInfoln("----------------------------------------");
      logInfoln("【" + globalIndexData[i].inxnm + "】");
      logInfoln("指数编号: " + globalIndexData[i].inxid);
      logInfoln("----------------------------------------");
      logInfo("当前价格: ");
      logInfo(String(globalIndexData[i].lastPrice, 2));
      logInfo("  |  涨跌额: ");
      logInfo(String(globalIndexData[i].riseFall, 2));
      logInfo("  |  涨跌幅: ");
      logInfoln(globalIndexData[i].riseFallPer);
      logInfo("今开: ");
      logInfo(String(globalIndexData[i].openPrice, 2));
      logInfo("  |  最高: ");
      logInfo(String(globalIndexData[i].highPrice, 2));
      logInfo("  |  最低: ");
      logInfoln(String(globalIndexData[i].lowPrice, 2));
      logInfo("昨收: ");
      logInfo(String(globalIndexData[i].yesyPrice, 2));
      logInfo("  |  振幅: ");
      logInfoln(globalIndexData[i].amplitude);
      logInfoln("----------------------------------------");
      logInfo("成交量: ");
      logInfo(String(globalIndexData[i].volume, 0));
      logInfo("  |  成交额: ");
      logInfoln(String(globalIndexData[i].turnover / 100000000, 2) + " 亿");
      logInfo("更新时间: ");
      logInfoln(globalIndexData[i].uptime);
      logInfoln("");
    }
  }
  logInfoln("==========================================");
}
