#pragma once
#include <Wire.h>
#include <Preferences.h>
#include <SparkFun_VL53L5CX_Library.h>
#include <driver/gpio.h>
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
static bool gridRequested=false,buttonLatched=false,radioAllowed=false;
static uint32_t buttonAt=0,lastRadioTry=0;
static uint8_t readFailures=0;

static void scanBus() {
  Serial.printf("{\"role\":\"rear\",\"event\":\"i2c_scan\",\"sda_level\":%d,\"scl_level\":%d,\"addresses\":[",gpio_get_level(GPIO_NUM_6),gpio_get_level(GPIO_NUM_7));
  bool first=true;
  for(uint8_t address=8;address<120;++address) {
    Wire.beginTransmission(address);
    if(Wire.endTransmission()==0){if(!first)Serial.print(',');Serial.print(address);first=false;}
  }
  Serial.println("]}");
}

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
    const Zone zone=classifyZone(measurements.nb_target_detected[i],status,d);
    const bool usable=zone==Zone::Valid || zone==Zone::TooClose;
    zoneValid[i]=usable;distances[i]=usable?d:0;
    if(!usable)continue;
    if(calActive && calCollecting && zone==Zone::Valid) {
      calSum[i]+=d;++calCount[i];calMin[i]=min(calMin[i],uint16_t(d));calMax[i]=max(calMax[i],uint16_t(d));
    }
    if(zone==Zone::TooClose){++valid;nearest=min(nearest,uint16_t(d));hits[i]=true;++hitCount;continue;}
    if(calibrated && !background.used[i])continue;
    ++valid;nearest=min(nearest,uint16_t(d));
    hits[i]=calibrated ? int(background.mm[i])-d>=FOREGROUND_MARGIN_MM : d<=DESK_NEAR_MM;
    if(hits[i])++hitCount;
  }
  if(!radioAllowed){radioAllowed=true;event("sensor_frame_before_radio");}
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
static void attachWire() {
  Wire.begin(6,7);Wire.setClock(400000);Wire.setTimeOut(100);
}
static bool probeSensor() {
  Wire.beginTransmission(0x29);uint8_t ack=Wire.endTransmission();
  Serial.printf("{\"role\":\"rear\",\"event\":\"i2c_probe\",\"address\":41,\"error\":%u}\n",ack);
  return ack==0;
}
static void sensorLost(const char *why) {
  ranging=false;calActive=false;radioAllowed=false;readFailures=0;everFrame=false;
  decision.state=Condition::SensorError;
  if(radioReady)stopRadio();
  initAt=millis();
  event(why);
}
// True when ESP-NOW came up but the sensor no longer answers and must be reinitialized.
static bool attachRadio() {
  if(!startRadio())return false;
  Wire.end();attachWire();
  if(probeSensor()){event("radio_started_after_sensor");return false;}
  return true;
}
static void trySensor(uint32_t now) {
  if(radioReady)stopRadio();
  radioAllowed=false;readFailures=0;
  initAt=now;outgoing.valid=0;everFrame=false;decision=Decision{};
  attachWire();
  if(!probeSensor()){ranging=false;return;}
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
      else if(command=="SCAN")scanBus();
      else if(command=="STATUS")lastLog=millis()-1000;
      else if(command.length())event("commands_CAL_DESK_GRID_STATUS_SCAN");
      command="";
    } else if(command.length()<32)command+=c;
  }
}
static void setupRole() {
  outgoing.kind=Kind::Status;
  pinMode(9,INPUT_PULLUP);
  stopRadio();
  attachWire();
  prefs.begin("door-guard",false);
  if(prefs.getBytesLength("background")==sizeof(background)) {
    prefs.getBytes("background",&background,sizeof(background));
    if(background.tag==0x42473031) {
      for(int i=0;i<64;++i)if(background.used[i] && background.mm[i]>=40 && background.mm[i]<=4000)++baselineZones;
      calibrated=baselineZones>=MIN_VALID_ZONES;
    }
  }
  initAt=millis()-5000;
  Serial.printf("{\"role\":\"rear\",\"fw\":\"door-guard-1.0\",\"mode\":\"%s\"}\n",calibrated?"CALIBRATED":"DESK");
}
static void loopRole() {
  uint32_t now=millis();commands();
  if(radioReady){
    Received r;
    while(pollRadio(r)){sendPacket(rearStatus(now));}
    if(now-lastSend>=200){sendPacket(rearStatus(now));lastSend=now;}
    peerWatchdog(now,5000);
  }
  bool pressed=digitalRead(9)==LOW;
  if(pressed && !buttonLatched) {
    if(!buttonAt)buttonAt=now;
    if(now-buttonAt>=2000){buttonLatched=true;startCalibration();}
  } else if(!pressed){buttonAt=0;buttonLatched=false;}
  if(calActive && !calCollecting && now-calAt>=10000){calCollecting=true;event("calibration_collecting_keep_area_empty");}
  if(calActive && now-calAt>20000){calActive=false;event("calibration_timeout");}
  if(!ranging && now-initAt>=5000)trySensor(now);
  if(ranging && imager.isDataReady()) {
    if(imager.getRangingData(&measurements)){readFailures=0;processFrame(millis());}
    else if(++readFailures>=3)sensorLost("sensor_read_failed");
  }
  now=millis();
  if(ranging && radioAllowed && !radioReady && now-lastRadioTry>=1000){
    lastRadioTry=now;
    if(attachRadio())sensorLost("i2c_lost_after_radio");
  }
  if(ranging && now-frameAt>5000)sensorLost("sensor_frame_timeout");
  if(now-lastLog>=1000) {
    lastLog=now;Packet p=rearStatus(now);
    Serial.printf("{\"role\":\"rear\",\"ms\":%lu,\"state\":\"%s\",\"mode\":\"%s\",\"valid\":%u,\"hits\":%u,\"nearest_mm\":%u,\"frame_age\":%lu,\"frames\":%lu,\"rx\":%lu,\"tx_errors\":%lu}\n",(unsigned long)now,conditionName(p.state),calibrated?"CALIBRATED":"DESK",p.valid,p.foreground,p.nearest,(unsigned long)p.frameAge,(unsigned long)frameCount,(unsigned long)rxCount,(unsigned long)txErrors);
  }
}
