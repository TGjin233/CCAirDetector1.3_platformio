#ifndef SPIFFSUtil_h
#define SPIFFSUtil_h

#include <Arduino.h>
#include <SPIFFS.h>

class SPIFFSUtil {
public:
    static bool begin();
    static void listFiles();
    static bool writeFile(const char* path, const char* content);
    static bool writeFile(const String& path, const String& content);
    static String readFile(const char* path);
    static String readFile(const String& path);
    static bool deleteFile(const char* path);
    static bool deleteFile(const String& path);
    static bool exists(const char* path);
    static bool exists(const String& path);
    static size_t getFileSize(const char* path);
    static size_t getFileSize(const String& path);
    static bool appendFile(const char* path, const char* content);
    static bool appendFile(const String& path, const String& content);
    static void format();
    static size_t getTotalBytes();
    static size_t getUsedBytes();
};

#endif
