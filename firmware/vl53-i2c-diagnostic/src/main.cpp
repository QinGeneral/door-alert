#include <Arduino.h>
#include <Wire.h>
#include <esp_mac.h>
#include <driver/gpio.h>
#ifdef ENABLE_RANGING
#include <SparkFun_VL53L5CX_Library.h>
SparkFun_VL53L5CX imager;
VL53L5CX_ResultsData measurement;
bool ready=false;
uint32_t frames=0;
#endif

struct Test {uint8_t sda,scl;uint32_t hz;};
constexpr Test tests[]={{6,7,100000},{6,7,400000},{7,6,100000},{7,6,400000}};
uint8_t indexTest=0;
uint32_t lastTest=0;

bool writeRegister(uint16_t reg,uint8_t value) {
  Wire.beginTransmission(0x29);Wire.write(reg>>8);Wire.write(reg&0xff);Wire.write(value);
  return Wire.endTransmission()==0;
}
int readRegister(uint16_t reg) {
  Wire.beginTransmission(0x29);Wire.write(reg>>8);Wire.write(reg&0xff);
  if(Wire.endTransmission(false)!=0)return -1;
  if(Wire.requestFrom(uint8_t(0x29),uint8_t(1))!=1)return -1;
  return Wire.read();
}
void scan(const Test &t) {
  Wire.end();delay(10);
  bool begun=Wire.begin(t.sda,t.scl,t.hz);Wire.setTimeOut(50);delay(10);
  Serial.printf("{\"event\":\"scan\",\"sda_gpio\":%u,\"scl_gpio\":%u,\"hz\":%lu,\"bus_started\":%s,\"sda_level\":%d,\"scl_level\":%d,\"addresses\":[",t.sda,t.scl,(unsigned long)t.hz,begun?"true":"false",gpio_get_level(gpio_num_t(t.sda)),gpio_get_level(gpio_num_t(t.scl)));
  bool first=true,found=false;
  if(begun)for(uint8_t address=8;address<120;++address) {
    Wire.beginTransmission(address);
    if(Wire.endTransmission()==0) {
      if(!first)Serial.print(',');Serial.print(address);first=false;
      if(address==0x29)found=true;
    }
  }
  Serial.println("]}");
  if(found) {
    int id=-1,revision=-1;
    if(writeRegister(0x7fff,0x00)){id=readRegister(0);revision=readRegister(1);}
    bool restored=writeRegister(0x7fff,0x02);
    Serial.printf("{\"event\":\"sensor_id\",\"id\":%d,\"revision\":%d,\"matches_vl53l5cx\":%s,\"bank_restored\":%s}\n",id,revision,(id==0xf0&&revision==2)?"true":"false",restored?"true":"false");
  }
}
void setup() {
  Serial.setTxBufferSize(4096);Serial.begin(115200);Serial.setTxTimeoutMs(0);
  uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_STA);
  Serial.printf("{\"fw\":\"vl53-i2c-only-v1\",\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"wifi_enabled\":false}\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
  delay(1000);lastTest=millis()-3000;
#ifdef ENABLE_RANGING
  Wire.begin(6,7,400000);Wire.setTimeOut(100);
  Serial.println("{\"event\":\"initializing_ranging_without_wifi\"}");
  ready=imager.begin(0x29,Wire);
  if(ready)ready=imager.setResolution(64)&&imager.setRangingFrequency(10)&&imager.startRanging();
  Serial.printf("{\"event\":\"ranging_init\",\"ok\":%s}\n",ready?"true":"false");
#endif
}
void loop() {
#ifdef ENABLE_RANGING
  if(ready && imager.isDataReady() && imager.getRangingData(&measurement)) {
    ++frames;
    if(frames%10==0) {
      uint16_t nearest=65535;uint8_t valid=0;
      for(int i=0;i<64;++i) {
        int d=measurement.distance_mm[i];uint8_t st=measurement.target_status[i];
        if(measurement.nb_target_detected[i] && (st==5||st==9) && d>=40 && d<=4000){++valid;nearest=min(nearest,uint16_t(d));}
      }
      Serial.printf("{\"event\":\"ranging_data\",\"frames\":%lu,\"valid\":%u,\"nearest_mm\":%u}\n",(unsigned long)frames,valid,nearest==65535?0:nearest);
      if(frames%30==0) {
        uint8_t statuses[256]{};uint8_t withTarget=0;
        int16_t rawMin=32767,rawMax=-32768;
        for(int i=0;i<64;++i){++statuses[measurement.target_status[i]];if(measurement.nb_target_detected[i])++withTarget;rawMin=min(rawMin,measurement.distance_mm[i]);rawMax=max(rawMax,measurement.distance_mm[i]);}
        Serial.printf("{\"event\":\"raw_status\",\"zones_with_target\":%u,\"raw_min_mm\":%d,\"raw_max_mm\":%d,\"status_counts\":{",withTarget,rawMin,rawMax);
        bool first=true;
        for(int i=0;i<256;++i)if(statuses[i]){if(!first)Serial.print(',');Serial.printf("\"%d\":%u",i,statuses[i]);first=false;}
        Serial.println("}}");
      }
    }
  }
#else
  uint32_t now=millis();
  if(now-lastTest>=3000){scan(tests[indexTest]);indexTest=(indexTest+1)%4;lastTest=millis();}
#endif
  delay(5);
}
