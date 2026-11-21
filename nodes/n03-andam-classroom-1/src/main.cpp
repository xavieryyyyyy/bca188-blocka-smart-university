#include <Arduino.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include "Config.h"
#include "NodeConfig.h"
#include "AndamSecurity.h"
#include "Network.h"
#include "MqttBase.h"
Network net; AndamSecurity sec; MqttBase mqtt;
enum State:uint8_t{IDLE,WAIT_TICKET,SAVED,ACKED}; State state=IDLE;
uint32_t req=0,lastCmdSeq=0;unsigned long startAt=0,lastSend=0,lastBeat=0,lastBlink=0;
bool helpLast=HIGH,helpStable=HIGH,resLast=HIGH,resStable=HIGH,redOn=false;
unsigned long helpChanged=0,resChanged=0;bool errorMode=false;uint8_t errorSteps=0;
String signDoc(const String&canonical,JsonDocument&d){String s=sec.sign(canonical);if(!s.length())return "";d["sig"]=s;String out;serializeJson(d,out);return out;}
bool publishEvent(const char*ev){
 uint32_t seq=sec.nextSequence(),ts=net.epoch();if(!seq||!ts||!req)return false;
 String c=String("n03|")+ev+"|"+req+"|"+seq+"|"+ts;
 JsonDocument d;d["node"]="n03";d["event"]=ev;d["req"]=req;d["seq"]=seq;d["ts"]=ts;
 String out=signDoc(c,d);String topic=!strcmp(ev,"help_request")?NodeConfig::REQUEST:NodeConfig::RESOLVE_TOPIC;
 return out.length()&&mqtt.publish(topic,out,false);
}
void fail(){errorMode=true;errorSteps=0;redOn=false;lastBlink=millis();}
void help(){
 if(state!=IDLE)return;if(!mqtt.connected()||!net.epoch()){fail();return;}
 req=sec.nextRequest();if(!req){fail();return;}state=WAIT_TICKET;startAt=lastSend=millis();
 if(!publishEvent("help_request")){state=IDLE;req=0;fail();}
}
void resolve(){
 if(state==IDLE)return;if(!mqtt.connected()||!net.epoch()){fail();return;}
 const char*ev=(state==SAVED||state==ACKED)?"resolved":"cancelled";
 if(publishEvent(ev)){state=IDLE;req=0;digitalWrite(NodeConfig::RED,LOW);digitalWrite(NodeConfig::GREEN,LOW);}else fail();
}
void callback(char*,uint8_t*p,unsigned int n){
 JsonDocument d;if(deserializeJson(d,p,n))return;
 const char*node=d["node"]|"";const char*st=d["state"]|"";const char*sig=d["sig"]|"";
 uint32_t r=d["req"]|0U,s=d["seq"]|0U,t=d["ts"]|0U,now=net.epoch();
 if(strcmp(node,"n03")||!r||r!=req||!s||s<=lastCmdSeq||!t||!now)return;
 if((now>t?now-t:t-now)>Config::MAX_AGE_S)return;
 String c=String("n03|")+st+"|"+r+"|"+s+"|"+t;if(!sec.verify(c,String(sig)))return;
 if(!strcmp(st,"ticket_saved")&&state==WAIT_TICKET){state=SAVED;lastCmdSeq=s;}
 else if(!strcmp(st,"acknowledged")&&(state==SAVED||state==WAIT_TICKET)){state=ACKED;lastCmdSeq=s;}
}
void heartbeat(){
 uint32_t s=sec.nextSequence(),t=net.epoch(),up=millis()/1000;int rssi=WiFi.RSSI();if(!s||!t)return;
 String c=String("n03|heartbeat|")+up+"|"+rssi+"|"+s+"|"+t;
 JsonDocument d;d["node"]="n03";d["uptime_s"]=up;d["rssi_dbm"]=rssi;d["seq"]=s;d["ts"]=t;d["sig"]=sec.sign(c);
 String out;serializeJson(d,out);mqtt.publish(NodeConfig::HEARTBEAT,out,false);
}
void button(uint8_t pin,bool&last,bool&stable,unsigned long&changed,void(*fn)()){
 bool v=digitalRead(pin);unsigned long now=millis();if(v!=last){last=v;changed=now;}
 if(now-changed>=Config::DEBOUNCE_MS&&v!=stable){stable=v;if(!stable)fn();}
}
void setup(){
 Serial.begin(115200);pinMode(NodeConfig::HELP,INPUT_PULLUP);pinMode(NodeConfig::RESOLVE,INPUT_PULLUP);
 pinMode(NodeConfig::RED,OUTPUT);pinMode(NodeConfig::GREEN,OUTPUT);digitalWrite(NodeConfig::RED,LOW);digitalWrite(NodeConfig::GREEN,LOW);
 if(!sec.begin(HMAC_KEY_HEX)||!sec.beginCounters("andam-n03")||!sec.beginRequestCounter()){Serial.println("SECURITY/FLASH INIT FAILED");return;}
 net.begin(WIFI_SSID,WIFI_PASS);mqtt.begin("n03",NodeConfig::CMD,callback);
}
void loop(){
 net.loop();mqtt.loop(net.connected()&&net.epoch());button(NodeConfig::HELP,helpLast,helpStable,helpChanged,help);button(NodeConfig::RESOLVE,resLast,resStable,resChanged,resolve);
 unsigned long now=millis();
 if(state==WAIT_TICKET){
  if(now-lastSend>=5000){lastSend=now;if(!publishEvent("help_request"))Serial.println("Request retry failed");}
  if(now-startAt>=15000){state=IDLE;req=0;fail();}
  if(now-lastBlink>=200){lastBlink=now;redOn=!redOn;digitalWrite(NodeConfig::RED,redOn);digitalWrite(NodeConfig::GREEN,LOW);}
 }
 if(errorMode&&now-lastBlink>=100){lastBlink=now;redOn=!redOn;digitalWrite(NodeConfig::RED,redOn);if(++errorSteps>=6){errorMode=false;digitalWrite(NodeConfig::RED,LOW);}}
 if(state==SAVED){digitalWrite(NodeConfig::RED,HIGH);digitalWrite(NodeConfig::GREEN,LOW);}
 if(state==ACKED){digitalWrite(NodeConfig::RED,LOW);digitalWrite(NodeConfig::GREEN,HIGH);}
 if(state==IDLE&&!errorMode){digitalWrite(NodeConfig::RED,LOW);digitalWrite(NodeConfig::GREEN,LOW);}
 if(mqtt.connected()&&net.epoch()&&now-lastBeat>=Config::HEARTBEAT_MS){lastBeat=now;heartbeat();}
}
