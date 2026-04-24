#include "SPIFFSUtil.h"
#include "LogUtil.h"

bool SPIFFSUtil::begin() {
    if(!SPIFFS.begin(true)){
        logInfoln("SPIFFS 初始化失败");
        return false;
    }
    logInfo("SPIFFS 初始化成功, 总空间: ");
    logInfoln(String(SPIFFS.totalBytes()));
    return true;
}

void SPIFFSUtil::listFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    logInfoln("=== SPIFFS 文件列表 ===");
    while(file){
        logInfo("文件: ");
        logInfo(file.name());
        logInfo(", 大小: ");
        logInfoln(String(file.size()));
        file = root.openNextFile();
    }
    logInfo("已用空间: ");
    logInfoln(String(SPIFFS.usedBytes()));
    logInfo("总空间: ");
    logInfoln(String(SPIFFS.totalBytes()));
    logInfoln("========================");
}

bool SPIFFSUtil::writeFile(const char* path, const char* content) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if(!file){
        logInfo("写入文件失败: ");
        logInfoln(path);
        return false;
    }
    size_t bytesWritten = file.print(content);
    file.close();
    logInfo("写入成功: ");
    logInfoln(path);
    return bytesWritten > 0;
}

bool SPIFFSUtil::writeFile(const String& path, const String& content) {
    return writeFile(path.c_str(), content.c_str());
}

String SPIFFSUtil::readFile(const char* path) {
    File file = SPIFFS.open(path, FILE_READ);
    if(!file){
        logInfo("读取文件失败: ");
        logInfoln(path);
        return "";
    }
    String content = "";
    while(file.available()){
        content += (char)file.read();
    }
    file.close();
    return content;
}

String SPIFFSUtil::readFile(const String& path) {
    return readFile(path.c_str());
}

bool SPIFFSUtil::deleteFile(const char* path) {
    if(SPIFFS.remove(path)){
        logInfo("删除成功: ");
        logInfoln(path);
        return true;
    }
    logInfo("删除失败: ");
    logInfoln(path);
    return false;
}

bool SPIFFSUtil::deleteFile(const String& path) {
    return deleteFile(path.c_str());
}

bool SPIFFSUtil::exists(const char* path) {
    return SPIFFS.exists(path);
}

bool SPIFFSUtil::exists(const String& path) {
    return exists(path.c_str());
}

size_t SPIFFSUtil::getFileSize(const char* path) {
    File file = SPIFFS.open(path, FILE_READ);
    if(!file){
        return 0;
    }
    size_t size = file.size();
    file.close();
    return size;
}

size_t SPIFFSUtil::getFileSize(const String& path) {
    return getFileSize(path.c_str());
}

bool SPIFFSUtil::appendFile(const char* path, const char* content) {
    File file = SPIFFS.open(path, FILE_APPEND);
    if(!file){
        logInfo("追加文件失败: ");
        logInfoln(path);
        return false;
    }
    size_t bytesWritten = file.print(content);
    file.close();
    return bytesWritten > 0;
}

bool SPIFFSUtil::appendFile(const String& path, const String& content) {
    return appendFile(path.c_str(), content.c_str());
}

void SPIFFSUtil::format() {
    logInfoln("开始格式化 SPIFFS...");
    SPIFFS.format();
    logInfoln("格式化完成");
}

size_t SPIFFSUtil::getTotalBytes() {
    return SPIFFS.totalBytes();
}

size_t SPIFFSUtil::getUsedBytes() {
    return SPIFFS.usedBytes();
}
