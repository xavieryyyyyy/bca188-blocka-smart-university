#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <mbedtls/md.h>
class AndamSecurity {
public:
 bool begin(const char* h) {
  if(!h||strlen(h)!=64)return false;
  for(int i=0;i<32;i++){int a=nib(h[2*i]),b=nib(h[2*i+1]);if(a<0||b<0)return false;key[i]=(a<<4)|b;}
  ready=true;return true;
 }
 String sign(const String&s)const{
  if(!ready)return "";const auto*info=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);if(!info)return "";
  uint8_t d[32];if(mbedtls_md_hmac(info,key,32,(const unsigned char*)s.c_str(),s.length(),d))return "";
  const char*x="0123456789abcdef";char o[65];for(int i=0;i<32;i++){o[2*i]=x[d[i]>>4];o[2*i+1]=x[d[i]&15];}o[64]=0;return String(o);
 }
 bool verify(const String&s,const String&sig)const{
  if(sig.length()!=64)return false;String e=sign(s);if(e.length()!=64)return false;
  volatile uint8_t d=0;for(int i=0;i<64;i++)d=d|(uint8_t)(e[i]^sig[i]);return d==0;
 }
 bool beginCounters(const char*ns){
  if(!prefs.begin(ns,false))return false;uint32_t hi=prefs.getUInt("seqhi",0);
  if(hi>UINT32_MAX-32)return false;next=hi+1;reserved=hi;return true;
 }
 uint32_t nextSequence(){
  if(!next||next==UINT32_MAX)return 0;
  if(next>reserved){uint32_t hi=next+31;if(hi<next||prefs.putUInt("seqhi",hi)!=4)return 0;reserved=hi;}
  return next++;
 }
 bool beginRequestCounter(){if(!reqPrefs.begin("andam-req",false))return false;if(!reqPrefs.isKey("req"))reqPrefs.putUInt("req",0);return true;}
 uint32_t nextRequest(){uint32_t r=reqPrefs.getUInt("req",0);if(r==UINT32_MAX)return 0;++r;return reqPrefs.putUInt("req",r)==4?r:0;}
private:
 static int nib(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}
 uint8_t key[32]{};bool ready=false;Preferences prefs,reqPrefs;uint32_t next=1,reserved=0;
};
