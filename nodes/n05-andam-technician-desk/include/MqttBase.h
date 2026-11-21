#pragma once
#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "secrets.h"
class MqttBase {
public:
 MqttBase():mqtt(plain){}
 void begin(const char*node,const char*sub,void(*cb)(char*,uint8_t*,unsigned int)){
  nodeId=node;subTopic=sub;
#if USE_TLS
  secure.setCACert(BROKER_CA_PEM);mqtt.setClient(secure);
#else
  mqtt.setClient(plain);
#endif
  mqtt.setServer(BROKER_HOST,BROKER_PORT);mqtt.setKeepAlive(30);mqtt.setBufferSize(768);mqtt.setCallback(cb);
 }
 void loop(bool ready){
  if(!ready)return;
  if(!mqtt.connected()&&millis()-lastTry>=5000){lastTry=millis();String cid=String("andam-")+nodeId+"-"+String((uint32_t)ESP.getEfuseMac(),HEX);
   String st=String("sentra/andam/")+nodeId+"/status";String will=String("{\"node\":\"")+nodeId+"\",\"status\":\"offline\"}";
   if(mqtt.connect(cid.c_str(),MQTT_USER,MQTT_PASS,st.c_str(),1,true,will.c_str())){
    if(subTopic.length())mqtt.subscribe(subTopic.c_str(),1);
    String online=String("{\"node\":\"")+nodeId+"\",\"status\":\"online\"}";mqtt.publish(st.c_str(),online.c_str(),true);
   }}
  if(mqtt.connected())mqtt.loop();
 }
 bool connected(){return mqtt.connected();}
 bool publish(const String&t,const String&p,bool retain=false){return mqtt.connected()&&mqtt.publish(t.c_str(),p.c_str(),retain);}
private:WiFiClient plain;WiFiClientSecure secure;PubSubClient mqtt;String nodeId,subTopic;unsigned long lastTry=0;
};
