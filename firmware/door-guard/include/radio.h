#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "protocol.h"
#include "pairing_keys.h"

struct Received { Packet packet; uint32_t at; };
static QueueHandle_t inbox;
static bool radioReady=false;
static bool nowInited=false;
static uint32_t bootSession=0, txSeq=0, rxCount=0, txErrors=0;
static volatile uint32_t lastAcceptedAt=0;
static uint32_t peerRefreshes=0;
#ifdef ROLE_REAR
static const uint8_t *peerMac=FRONT_MAC;
static const uint8_t *selfMac=REAR_MAC;
#else
static const uint8_t *peerMac=REAR_MAC;
static const uint8_t *selfMac=FRONT_MAC;
#endif

static void radioHardwareOff() {
  radioReady=false;
  if(nowInited){esp_now_deinit();nowInited=false;}
  WiFi.mode(WIFI_OFF);
  if(inbox)xQueueReset(inbox);
}

static void radioReceive(const esp_now_recv_info_t *info,const uint8_t *data,int length) {
  if (!info || memcmp(info->src_addr,peerMac,6)!=0 || length!=sizeof(Packet)) return;
  Received item{}; memcpy(&item.packet,data,sizeof(Packet)); item.at=millis();
  if(item.packet.magic!=MAGIC || item.packet.version!=VERSION) return;
#ifdef ROLE_REAR
  if(item.packet.kind!=Kind::Hello) return;
#else
  if(item.packet.kind!=Kind::Status || uint8_t(item.packet.state)>6) return;
#endif
  lastAcceptedAt=item.at;
  xQueueOverwrite(inbox,&item);
}

static bool addPeer() {
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr,peerMac,6); memcpy(peer.lmk,PAIR_LMK,16);
  peer.channel=CHANNEL; peer.ifidx=WIFI_IF_STA; peer.encrypt=true;
  return esp_now_add_peer(&peer)==ESP_OK;
}

static bool startRadio() {
  if(!inbox) inbox=xQueueCreate(1,sizeof(Received));
  if(!inbox) return false;
  xQueueReset(inbox);
  WiFi.mode(WIFI_STA); WiFi.setSleep(false);
  uint8_t own[6]; esp_wifi_get_mac(WIFI_IF_STA,own);
  if(memcmp(own,selfMac,6)!=0) {
    Serial.printf("{\"event\":\"wrong_board\",\"mac\":\"%s\"}\n",WiFi.macAddress().c_str());
    WiFi.mode(WIFI_OFF); return false;
  }
  if(esp_wifi_set_channel(CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK) {WiFi.mode(WIFI_OFF);return false;}
  if(esp_now_init()!=ESP_OK) {WiFi.mode(WIFI_OFF);return false;}
  nowInited=true;
  if(esp_now_set_pmk(PAIR_PMK)!=ESP_OK || !addPeer() || esp_now_register_recv_cb(radioReceive)!=ESP_OK) {
    radioHardwareOff();return false;
  }
  radioReady=true;
  lastAcceptedAt=millis();
  Serial.printf("{\"event\":\"radio_ready\",\"channel\":6,\"encrypted\":true,\"mac\":\"%s\"}\n",WiFi.macAddress().c_str());
  return true;
}
static void stopRadio() {radioHardwareOff();}
static void sendPacket(Packet p) {
  if(!radioReady)return;
  p.session=bootSession;p.sequence=++txSeq;p.uptime=millis();
  if(esp_now_send(peerMac,reinterpret_cast<uint8_t*>(&p),sizeof(p))!=ESP_OK)++txErrors;
}
static bool pollRadio(Received &r) {
  if(inbox && xQueueReceive(inbox,&r,0)==pdTRUE){++rxCount;return true;}
  return false;
}
// Encrypted ESP-NOW drops a peer that rebooted, because the CCMP packet number restarted.
// Reinstalling the peer resets that window. The first silenceMs after startRadio are quiet.
static void peerWatchdog(uint32_t now,uint32_t silenceMs) {
  static uint32_t lastRefresh=0;
  if(!radioReady)return;
  if(int32_t(now-lastAcceptedAt)<=int32_t(silenceMs) || now-lastRefresh<silenceMs)return;
  lastRefresh=now;
  esp_now_del_peer(peerMac);
  if(!addPeer()){Serial.println("{\"event\":\"peer_refresh_failed\"}");radioHardwareOff();return;}
  ++peerRefreshes;
  Serial.println("{\"event\":\"peer_refresh\",\"reason\":\"peer_silent\"}");
}
