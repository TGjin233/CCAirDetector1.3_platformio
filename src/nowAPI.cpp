#include "nowAPI.h"
#include "LogUtil.h"
#include <WiFi.h>
#include <SPIFFS.h>

#define NOWAPI_HTTP_TIMEOUT 10000
#define MAX_INDICES 10  // 最大支持的指数数量
#define SPIFFS_FILE_PATH "/index_data.txt"

// 关注的指数列表 - 在这里修改
const char* WATCHED_INDICES[] = {
  "1010",    // 上证指数
  "1011",    // 深证成指
  "1013",    // 创业板指
  "1114",    // 纳斯达克
  "1118",    // 纳斯达克100
};
const int WATCHED_INDICES_COUNT = sizeof(WATCHED_INDICES) / sizeof(WATCHED_INDICES[0]);

// 全局存储
IndexData globalIndexData[MAX_INDICES];

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

// 保存指数数据到SPIFFS
void saveIndexToSPIFFS(int index, const IndexData& data) {
  if (index >= MAX_INDICES) return;

  File file = SPIFFS.open(SPIFFS_FILE_PATH, FILE_WRITE);
  if (!file) {
    logError("无法打开SPIFFS文件进行写入");
    return;
  }

  // 使用简单的格式保存：每个指数用一行，字段用|分隔
  file.print(index);
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

  file.close();

  logInfo("已保存指数数据到SPIFFS: ");
  logInfoln(data.inxnm);
}

// 保存所有指数数据到SPIFFS
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

// 从SPIFFS加载单个指数数据
bool loadIndexFromSPIFFS(int index, IndexData& data) {
  if (index >= MAX_INDICES) return false;

  File file = SPIFFS.open(SPIFFS_FILE_PATH, FILE_READ);
  if (!file) {
    logError("无法打开SPIFFS文件进行读取");
    return false;
  }

  bool found = false;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int firstPipe = line.indexOf('|');
    int savedIndex = line.substring(0, firstPipe).toInt();

    if (savedIndex == index) {
      // 解析数据
      int pos = firstPipe + 1;
      int pipePos[15];
      pipePos[0] = firstPipe;
      
      for (int i = 1; i < 14; i++) {
        pipePos[i] = line.indexOf('|', pipePos[i-1] + 1);
        if (pipePos[i] == -1) pipePos[i] = line.length();
      }

      // 提取各字段
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

      found = true;
      break;
    }
  }

  file.close();

  if (found) {
    logInfo("从SPIFFS加载指数数据: ");
    logInfoln(data.inxnm);
  }

  return found;
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
}

void fetchSingleIndex(const char* inxid, IndexData& data, int index) {
  HTTPClient http;

  // 构建URL - 每次只获取一个指数
  String url = "https://" + String(NOWAPI_HOST) + "/?app=finance.globalindex&inxids=" + String(inxid) + "&appkey=" + String(NOWAPI_APPKEY) + "&sign=" + String(NOWAPI_SIGN) + "&format=json";

  logInfo("获取指数: ");
  logInfoln(inxid);

  http.begin(url);
  http.setConnectTimeout(NOWAPI_HTTP_TIMEOUT);
  http.setTimeout(NOWAPI_HTTP_TIMEOUT);

  int httpCode = http.GET();

  if (httpCode == HTTP_RET_OK) {
    String payload = http.getString();

    // 解析JSON数据
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // 检查成功标志
      if (doc["success"] == "1") {
        JsonObject result = doc["result"];
        JsonObject lists = result["lists"];

        // 获取第一个（也是唯一的）指数数据
        for (JsonPair indexPair : lists) {
          JsonObject item = indexPair.value().as<JsonObject>();

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

          // 保存到SPIFFS
          saveIndexToSPIFFS(index, data);

          break;  // 只获取第一个
        }
      } else {
        logError("API返回失败: ");
        logError(doc["msgid"].as<const char*>());
        logError(doc["msg"].as<const char*>());

        // 尝试从SPIFFS加载
        if (loadIndexFromSPIFFS(index, data)) {
          logInfoln("使用缓存数据");
        }
      }
    } else {
      logError("JSON解析失败: ");
      logError(error.c_str());

      // 尝试从SPIFFS加载
      if (loadIndexFromSPIFFS(index, data)) {
        logInfoln("使用缓存数据");
      }
    }
  } else {
    logError("HTTP请求失败: ");
    logError(String(httpCode));

    // 尝试从SPIFFS加载
    if (loadIndexFromSPIFFS(index, data)) {
      logInfoln("使用缓存数据");
    }
  }

  http.end();
}

void fetchGlobalIndex() {
  // 初始化SPIFFS
  initSPIFFS();

  logInfoln("========== 获取全球指数数据 ==========");

  for (int i = 0; i < WATCHED_INDICES_COUNT; i++) {
    logInfo("正在获取 ");
    logInfo(String(i + 1));
    logInfo("/");
    logInfoln(String(WATCHED_INDICES_COUNT));

    fetchSingleIndex(WATCHED_INDICES[i], globalIndexData[i], i);
    delay(100);  // 避免请求过快
  }

  // 保存所有数据到SPIFFS
  saveAllIndicesToSPIFFS();

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
