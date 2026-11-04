#pragma once
#include <Wire.h>
#include <Preferences.h>
#include <SparkFun_VL53L5CX_Library.h>
#include "detection.h"

static SparkFun_VL53L5CX imager;
static VL53L5CX_ResultsData measurements;
static Preferences prefs;
static Decision decision;
static Packet outgoing;
static bool ranging=false,everFrame=false,allocated=false;
static uint32_t frameAt=0,initAt=0,lastSend=0,lastLog=0,frameCount=0;
static uint16_t distances[64];
static bool zoneValid[64];
struct Background { uint32_t tag;uint16_t mm[64];uint8_t used[64]; };
static Background background{};
static bool calibrated=false;
static uint8_t baselineZones=0;
static bool calActive=false,calCollecting=false;
static uint32_t calAt=0;
static uint8_t calFrames=0;
static uint32_t calSum[64];
static uint16_t calMin[64],calMax[64];
static uint8_t calCount[64];
static String command;
static bool gridRequested=false,buttonLatched=false;
static uint32_t buttonAt=0;

static void event(const char *name) {Serial.printf("{\"role\":\"rear\",\"event\":\"%s\"}\n",name);}
static Condition effectiveState(uint32_t now) {
  if(calActive)return Condition::Calibrating;
  if(!ranging)return Condition::SensorError;
  if(!everFrame)return Condition::Starting;
  if(now-frameAt>1000)return Condition::Stale;
  return decision.state;
}
static Packet rearStatus(uint32_t now) {
  Packet p=outgoing;p.state=effectiveState(now);p.mode=calibrated?1:0;
  p.frameAge=everFrame?now-frameAt:0xffffffff;p.kind=Kind::Status;return p;
}
static void startCalibration() {
  if(!ranging){event("calibration_rejected_sensor_offline");return;}
  calActive=true;calCollecting=false;calAt=millis();calFrames=0;
  memset(calSum,0,sizeof(calSum));memset(calCount,0,sizeof(calCount));
  for(int i=0;i<64;++i){calMin[i]=65535;calMax[i]=0;}
  event("calibration_countdown_10_seconds_clear_the_door_area");
}
static void finishCalibration() {
  Background next{};next.tag=0x42473031;uint8_t usable=0;
  for(int i=0;i<64;++i) {
    if(calCount[i]>=24 && calMax[i]-calMin[i]<=120) {
      next.used[i]=1;next.mm[i]=calSum[i]/calCount[i];++usable;
    }
  }
  calActive=false;
  if(usable<MIN_VALID_ZONES){event("calibration_failed_insufficient_stable_zones");return;}
  if(prefs.putBytes("background",&next,sizeof(next))!=sizeof(next)) {
    event("calibration_failed_storage");return;
  }
  background=next;calibrated=true;baselineZones=usable;
  decision=Decision{};event("calibration_saved");
}
static void processFrame(uint32_t now) {
  everFrame=true;frameAt=now;++frameCount;
  bool hits[64]{};uint8_t valid=0,hitCount=0;uint16_t nearest=65535;
  for(int i=0;i<64;++i) {
    const int j=i*VL53L5CX_NB_TARGET_PER_ZONE;
    const int d=measurements.distance_mm[j];
    const int status=measurements.target_status[j];
    const bool good=measurements.nb_target_detected[i]>0 && (status==5||status==9) && d>=40 && d<=4000;
    zoneValid[i]=good;distances[i]=good?d:0;
    if(!good)continue;
    if(calActive && calCollecting) {
      calSum[i]+=d;++calCount[i];calMin[i]=min(calMin[i],uint16_t(d));calMax[i]=max(calMax[i],uint16_t(d));
    }
    if(calibrated && !background.used[i])continue;
    ++valid;nearest=min(nearest,uint16_t(d));
    hits[i]=calibrated ? int(background.mm[i])-d>=FOREGROUND_MARGIN_MM : d<=DESK_NEAR_MM;
    if(hits[i])++hitCount;
  }
  outgoing.valid=valid;outgoing.foreground=hitCount;outgoing.nearest=nearest==65535?0:nearest;
  if(calActive && calCollecting && ++calFrames>=32)finishCalibration();
  if(!calActive) {
    const uint8_t required=calibrated ? max(uint8_t(MIN_VALID_ZONES),uint8_t((baselineZones*3+3)/4)) : MIN_VALID_ZONES;
    Condition candidate=adjacentHit(hits)?Condition::Occupied:(valid>=required?Condition::Clear:Condition::Invalid);
    decision.update(candidate,now);
  }
  if(gridRequested) {
    Serial.print("{\"role\":\"rear\",\"grid_mm\":[");
    for(int i=0;i<64;++i){if(i)Serial.print(',');Serial.print(distances[i]);}
    Serial.println("]}");gridRequested=false;
  }
}
static void trySensor(uint32_t now) {
  initAt=now;outgoing.valid=0;everFrame=false;decision=Decision{};
  Wire.beginTransmission(0x29);uint8_t ack=Wire.endTransmission();
  Serial.printf("{\"role\":\"rear\",\"event\":\"i2c_probe\",\"address\":41,\"error\":%u}\n",ack);
  if(ack!=0){ranging=false;return;}
  if(allocated){delete imager.Dev;delete imager.VL53L5CX_i2c;allocated=false;}
  event("sensor_initializing");
  bool ok=imager.begin(0x29,Wire);allocated=true;
  if(ok)ok=imager.setResolution(64)&&imager.setRangingFrequency(10)&&imager.startRanging();
  ranging=ok;
  if(ok){frameAt=millis();event("sensor_ready_8x8_10hz");}
  else Serial.printf("{\"role\":\"rear\",\"event\":\"sensor_init_failed\",\"code\":%lu,\"value\":%lu}\n",(unsigned long)imager.lastError.lastErrorCode,(unsigned long)imager.lastError.lastErrorValue);
}
static void commands() {
  while(Serial.available()) {
    char c=Serial.read();
    if(c=='\r')continue;
    if(c=='\n') {
      command.trim();command.toUpperCase();
      if(command=="CAL")startCalibration();
      else if(command=="DESK") {
        calActive=false;calibrated=false;prefs.remove("background");decision=Decision{};event("desktop_mode_1000mm");
      } else if(command=="GRID")gridRequested=true;
      else if(command=="STATUS")lastLog=millis()-1000;
      else if(command.length())event("commands_CAL_DESK_GRID_STATUS");
      command="";
    } else if(command.length()<32)command+=c;
  }
}
static void setupRole() {
  outgoing.kind=Kind::Status;
  pinMode(9,INPUT_PULLUP);
  Wire.begin(6,7);Wire.setClock(400000);Wire.setTimeOut(100);
  prefs.begin("door-guard",false);
  if(prefs.getBytesLength("background")==sizeof(background)) {
    prefs.getBytes("background",&background,sizeof(background));
    if(background.tag==0x42473031) {
      for(int i=0;i<64;++i)if(background.used[i] && background.mm[i]>=40 && background.mm[i]<=4000)++baselineZones;
      calibrated=baselineZones>=MIN_VALID_ZONES;
    }
  }
  startRadio();initAt=millis()-5000;
  Serial.printf("{\"role\":\"rear\",\"fw\":\"door-guard-1.0\",\"mode\":\"%s\"}\n",calibrated?"CALIBRATED":"DESK");
}
static void loopRole() {
  uint32_t now=millis();commands();
  Received r;
  while(pollRadio(r)){sendPacket(rearStatus(now));}
  if(now-lastSend>=200){sendPacket(rearStatus(now));lastSend=now;}
  if(!radioReady && now-lastLog>=1000)startRadio();
  bool pressed=digitalRead(9)==LOW;
  if(pressed && !buttonLatched) {
    if(!buttonAt)buttonAt=now;
    if(now-buttonAt>=2000){buttonLatched=true;startCalibration();}
  } else if(!pressed){buttonAt=0;buttonLatched=false;}
  if(calActive && !calCollecting && now-calAt>=10000){calCollecting=true;event("calibration_collecting_keep_area_empty");}
  if(calActive && now-calAt>20000){calActive=false;event("calibration_timeout");}
  if(!ranging && now-initAt>=5000)trySensor(now);
  if(ranging && imager.isDataReady()) {
    if(imager.getRangingData(&measurements))processFrame(millis());
    else{decision.state=Condition::SensorError;ranging=false;calActive=false;event("sensor_read_failed");}
  }
  now=millis();
  if(ranging && now-frameAt>5000){ranging=false;calActive=false;event("sensor_frame_timeout");}
  if(now-lastLog>=1000) {
    lastLog=now;Packet p=rearStatus(now);
    Serial.printf("{\"role\":\"rear\",\"ms\":%lu,\"state\":\"%s\",\"mode\":\"%s\",\"valid\":%u,\"hits\":%u,\"nearest_mm\":%u,\"frame_age\":%lu,\"frames\":%lu,\"rx\":%lu,\"tx_errors\":%lu}\n",(unsigned long)now,conditionName(p.state),calibrated?"CALIBRATED":"DESK",p.valid,p.foreground,p.nearest,(unsigned long)p.frameAge,(unsigned long)frameCount,(unsigned long)rxCount,(unsigned long)txErrors);
  }
}
