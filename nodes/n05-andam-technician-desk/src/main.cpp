#include <Arduino.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "Config.h"
#include "NodeConfig.h"
#include "AndamSecurity.h"
#include "AndamNetwork.h"
#include "MqttBase.h"
AndamNetwork net;AndamSecurity sec;MqttBase mqtt;
bool r1=false,r2=false,muted=false,greenState=false,buzzState=false;
uint32_t lastSeq=0;unsigned long snapAt=0,lastBeat=0,lastBlink=0,lastReminder=0,nextBeep=0;
uint8_t beeps=0;bool muteLast=HIGH,muteStable=HIGH;unsigned long muteChanged=0;
void beepQueue(uint8_t n){beeps=n;nextBeep=millis();buzzState=false;}
void callback(char*,uint8_t*p,unsigned int n){
 JsonDocument d;if(deserializeJson(d,p,n))return;
 const char*node=d["node"]|"";const char*st=d["state"]|"";const char*rooms=d["rooms"]|"";const char*sig=d["sig"]|"";
 uint32_t s=d["seq"]|0U,t=d["ts"]|0U,now=net.epoch();
 if(strcmp(node,"n05")||strcmp(st,"open_tickets")||!s||s<=lastSeq||!t||!now)return;
 if((now>t?now-t:t-now)>Config::MAX_AGE_S)return;
 String c=String("n05|open_tickets|")+rooms+"|"+s+"|"+t;if(!sec.verify(c,String(sig)))return;
 bool nr1=String(rooms).indexOf("n03")>=0,nr2=String(rooms).indexOf("n04")>=0;
 bool appeared=(!r1&&nr1)||(!r2&&nr2);r1=nr1;r2=nr2;lastSeq=s;snapAt=millis();
 if(appeared){muted=false;beepQueue(3);}if(!r1&&!r2)muted=false;
}
void heartbeat(){
 uint32_t s=sec.nextSequence(),t=net.epoch(),up=millis()/1000;int r=WiFi.RSSI();if(!s||!t)return;
 String c=String("n05|heartbeat|")+up+"|"+r+"|"+s+"|"+t;
 JsonDocument d;d["node"]="n05";d["uptime_s"]=up;d["rssi_dbm"]=r;d["seq"]=s;d["ts"]=t;d["sig"]=sec.sign(c);
 String out;serializeJson(d,out);mqtt.publish(NodeConfig::HEARTBEAT,out,false);
}
void setup(){
 Serial.begin(115200);pinMode(NodeConfig::ROOM1,OUTPUT);pinMode(NodeConfig::ROOM2,OUTPUT);pinMode(NodeConfig::GREEN,OUTPUT);pinMode(NodeConfig::BUZZER,OUTPUT);pinMode(NodeConfig::MUTE,INPUT_PULLUP);
 digitalWrite(NodeConfig::ROOM1,LOW);digitalWrite(NodeConfig::ROOM2,LOW);digitalWrite(NodeConfig::GREEN,LOW);digitalWrite(NodeConfig::BUZZER,LOW);
 if(!sec.begin(HMAC_KEY_HEX)||!sec.beginCounters("andam-n05")){Serial.println("SECURITY/FLASH INIT FAILED");return;}
 net.begin(WIFI_SSID,WIFI_PASS);mqtt.begin("n05",NodeConfig::ALERT,callback);beepQueue(1);
}
void loop(){
 net.loop();mqtt.loop(net.connected()&&net.epoch());unsigned long now=millis();
 bool mv=digitalRead(NodeConfig::MUTE);if(mv!=muteLast){muteLast=mv;muteChanged=now;}
 if(now-muteChanged>=Config::DEBOUNCE_MS&&mv!=muteStable){muteStable=mv;if(!mv)muted=true;}
 bool fresh=mqtt.connected()&&net.epoch()&&snapAt&&now-snapAt<=30000;
 if(!fresh){r1=false;r2=false;}
 digitalWrite(NodeConfig::ROOM1,r1?HIGH:LOW);digitalWrite(NodeConfig::ROOM2,r2?HIGH:LOW);
 if(!fresh){if(now-lastBlink>=500){lastBlink=now;greenState=!greenState;digitalWrite(NodeConfig::GREEN,greenState);}}
 else digitalWrite(NodeConfig::GREEN,HIGH);
 if(fresh&&(r1||r2)&&!muted&&lastReminder&&now-lastReminder>=60000){lastReminder=now;beepQueue(3);}
 if(!r1&&!r2)lastReminder=now;
 if(beeps&&now>=nextBeep){buzzState=!buzzState;digitalWrite(NodeConfig::BUZZER,buzzState);nextBeep=now+120;if(!buzzState)--beeps;}
 else if(!beeps&&buzzState){buzzState=false;digitalWrite(NodeConfig::BUZZER,LOW);}
 if(mqtt.connected()&&net.epoch()&&now-lastBeat>=Config::HEARTBEAT_MS){lastBeat=now;heartbeat();}
}
