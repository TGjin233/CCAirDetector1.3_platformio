#ifndef __NET_H
#define __NET_H

#include <NTPClient.h>
#include "common.h"

void wifiConfigBySoftAP();
void startAP();
void scanWiFi();
void startServer();
void handleNotFound();
void handleRoot();
void handleConfigWifi();
void doClient();
void connectWiFi(int timeOut_s);
int getCityID();
int getWeather();
int getAir();
bool wifiConnected();
void disconnectWiFi();
String urlEncode(const String& text);
extern String ssid;
extern String pass;
extern String city;
extern String adm;
extern String location;
extern String lat; // 经度
extern String lon; // 纬度
extern bool connected;
extern NTPClient timeClient;
extern Weather weather;
extern bool queryWeatherSuccess;
extern bool queryAirSuccess;
extern NTPClient timeClient;


#endif