#pragma once
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/usb_serial_jtag.h>

constexpr uint8_t R_PIN=21,Y_PIN=20,G_PIN=10,PIR_PIN=4;
constexpr uint32_t PIR_WARMUP_MS=30000;
constexpr uint32_t DISPLAY_HOLD_MS=15000;
constexpr uint32_t LINK_TIMEOUT_MS=1200;
static Packet latest;
static bool haveStatus=false,pirRaw=false,pirStable=false;
static uint32_t packetAt=0,peerSession=0,peerSequence=0;
static uint32_t pirChangedAt=0,awakeAt=0,motionAt=0,frontLogAt=0,helloAt=0;
static uint32_t txHelloCount=0;
static bool lampR=false,lampY=false,lampG=false;
static const char *displayName="UNKNOWN";
static String frontCommand;
static bool serialAwake=false;
static uint32_t serialAwakeAt=0;

static void lights(bool r,bool y,bool g) {
  if(r==lampR && y==lampY && g==lampG)return;
  digitalWrite(R_PIN,LOW);digitalWrite(Y_PIN,LOW);digitalWrite(G_PIN,LOW);
  if(r)digitalWrite(R_PIN,HIGH);
  if(y)digitalWrite(Y_PIN,HIGH);
  if(g)digitalWrite(G_PIN,HIGH);
  lampR=r;lampY=y;lampG=g;
}
static void hello() {Packet p;p.kind=Kind::Hello;sendPacket(p);++txHelloCount;}
static bool fresh(uint32_t now) {
  return radioReady && haveStatus && now-packetAt<=LINK_TIMEOUT_MS && latest.frameAge<=1000;
}
static void standbySleep() {
  // Generic GPIO LIGHT sleep preserves high-impedance INPUT configuration.
  // Deep-sleep GPIO wake can automatically add a pulldown, unsuitable for this AM312.
  if(digitalRead(PIR_PIN)!=LOW || usb_serial_jtag_is_connected())return;
  esp_err_t err=gpio_wakeup_enable(gpio_num_t(PIR_PIN),GPIO_INTR_HIGH_LEVEL);
  if(err==ESP_OK)err=esp_sleep_enable_gpio_wakeup();
  if(err!=ESP_OK){Serial.printf("{\"role\":\"front\",\"event\":\"sleep_setup_failed\",\"error\":%d}\n",err);return;}
  lights(false,false,false);
  Serial.println("{\"role\":\"front\",\"event\":\"standby_light_sleep\"}");Serial.flush();
  stopRadio();
  // Timer wakes only to check for a newly attached USB host; radio remains off.
  while(!usb_serial_jtag_is_connected() && digitalRead(PIR_PIN)==LOW) {
    esp_sleep_enable_timer_wakeup(30000000ULL);
    err=esp_light_sleep_start();
    if(err!=ESP_OK)break;
  }
  gpio_wakeup_disable(gpio_num_t(PIR_PIN));
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  pinMode(PIR_PIN,INPUT);
  haveStatus=false;packetAt=0;awakeAt=millis();
  pirRaw=digitalRead(PIR_PIN)==HIGH;pirChangedAt=millis();pirStable=false;
  startRadio();hello();helloAt=millis();
  Serial.printf("{\"role\":\"front\",\"event\":\"wake\",\"error\":%d}\n",err);
}
static void setupRole() {
  for(uint8_t pin:{R_PIN,Y_PIN,G_PIN}){digitalWrite(pin,LOW);pinMode(pin,OUTPUT);}
  pinMode(PIR_PIN,INPUT);pirRaw=digitalRead(PIR_PIN)==HIGH;pirChangedAt=millis();
  lights(false,true,false);awakeAt=millis();startRadio();hello();
  Serial.println("{\"role\":\"front\",\"fw\":\"door-guard-1.0\",\"pins\":\"R=D6,Y=D7,G=D10,PIR=D2\"}");
}
static void loopRole() {
  uint32_t now=millis();
  bool raw=digitalRead(PIR_PIN)==HIGH;
  if(raw!=pirRaw){pirRaw=raw;pirChangedAt=now;}
  if(now-pirChangedAt>=100)pirStable=pirRaw;
  if(pirStable && now>=PIR_WARMUP_MS)motionAt=now;
  bool usb=usb_serial_jtag_is_connected();
  while(Serial.available()) {
    char c=Serial.read();if(c=='\r')continue;
    if(c=='\n') {
      frontCommand.trim();frontCommand.toUpperCase();
      if(frontCommand=="WAKE"){serialAwake=true;serialAwakeAt=now;}
      if(frontCommand=="STATUS")frontLogAt=now-1000;
      frontCommand="";
    } else if(frontCommand.length()<32)frontCommand+=c;
  }
  if(serialAwake && now-serialAwakeAt>=DISPLAY_HOLD_MS)serialAwake=false;
  if(!radioReady && now-helloAt>=1000)startRadio();
  Received received;
  while(pollRadio(received)) {
    auto &p=received.packet;
    if(haveStatus && p.session==peerSession && int32_t(p.sequence-peerSequence)<=0)continue;
    latest=p;packetAt=received.at;peerSession=p.session;peerSequence=p.sequence;haveStatus=true;
  }
  if(now-helloAt>=1000){helloAt=now;hello();}
  bool warmup=now<PIR_WARMUP_MS;
  bool active=usb || warmup || serialAwake || now-awakeAt<DISPLAY_HOLD_MS || (motionAt && now-motionAt<DISPLAY_HOLD_MS);
  bool isFresh=fresh(now);
  Condition state=haveStatus?latest.state:Condition::Starting;
  if(!active) {displayName="STANDBY";lights(false,false,false);}
  else if(!isFresh || (state!=Condition::Clear && state!=Condition::Occupied)) {
    displayName="UNKNOWN";lights(false,(now/350)%2==0,false);
  } else if(state==Condition::Occupied){displayName="RED";lights(true,false,false);}
  else if(warmup && !usb){displayName="WARMUP";lights(false,(now/500)%2==0,false);}
  else{displayName="GREEN";lights(false,false,true);}
  if(now-frontLogAt>=1000) {
    frontLogAt=now;
    Serial.printf("{\"role\":\"front\",\"ms\":%lu,\"display\":\"%s\",\"remote\":\"%s\",\"mode\":\"%s\",\"fresh\":%s,\"packet_age\":%lu,\"frame_age\":%lu,\"nearest_mm\":%u,\"valid\":%u,\"hits\":%u,\"pir\":%u,\"usb\":%s,\"rx\":%lu,\"hello\":%lu,\"r\":%u,\"y\":%u,\"g\":%u}\n",(unsigned long)now,displayName,conditionName(state),latest.mode?"CALIBRATED":"DESK",isFresh?"true":"false",(unsigned long)(haveStatus?now-packetAt:0xffffffff),(unsigned long)latest.frameAge,latest.nearest,latest.valid,latest.foreground,unsigned(pirStable),usb?"true":"false",(unsigned long)rxCount,(unsigned long)txHelloCount,unsigned(lampR),unsigned(lampY),unsigned(lampG));
  }
  if(!active && !usb && !pirRaw)standbySleep();
}
