#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "Config.h"
class AndamNetwork {
public:
 void begin(const char*s,const char*p){ssid=s;pass=p;started=true;WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.begin(s,p);last=millis();}
 void loop(){if(!started)return;if(WiFi.status()!=WL_CONNECTED&&millis()-last>=Config::WIFI_RETRY_MS){last=millis();WiFi.disconnect();WiFi.begin(ssid,pass);}
 if(WiFi.status()==WL_CONNECTED&&!ntp){configTzTime(Config::TZ,Config::NTP1,Config::NTP2);ntp=true;}}
 bool connected()const{return WiFi.status()==WL_CONNECTED;}
 uint32_t epoch()const{if(!connected())return 0;time_t t=time(nullptr);return t>1700000000?(uint32_t)t:0;}
private:const char*ssid=nullptr;const char*pass=nullptr;bool started=false,ntp=false;unsigned long last=0;
};
