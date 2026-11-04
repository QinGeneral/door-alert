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
static uint32_t bootSession=0, txSeq=0, rxCount=0, txErrors=0;
// Counted in the Wi-Fi task; rxCount (queue pops) undercounts because the queue overwrites.
static volatile uint32_t rxAccepted=0, txAcked=0, txNoAck=0;
static uint32_t peerRefreshes=0;
static volatile uint32_t lastAcceptedAt=0;
#ifdef ROLE_REAR
static const uint8_t *peerMac=FRONT_MAC;
static const uint8_t *selfMac=REAR_MAC;
#else
static const uint8_t *peerMac=REAR_MAC;
static const uint8_t *selfMac=FRONT_MAC;
#endif

static void radioReceive(const esp_now_recv_info_t *info,const uint8_t *data,int length) {
  if (!info || memcmp(info->src_addr,peerMac,6)!=0 || length!=sizeof(Packet)) return;
  Received item{}; memcpy(&item.packet,data,sizeof(Packet)); item.at=millis();
  if(item.packet.magic!=MAGIC || item.packet.version!=VERSION) return;
#ifdef ROLE_REAR
  if(item.packet.kind!=Kind::Hello) return;
#else
  if(item.packet.kind!=Kind::Status || uint8_t(item.packet.state)>6) return;
#endif
  rxAccepted=rxAccepted+1;lastAcceptedAt=item.at;
  xQueueOverwrite(inbox,&item);
}
static void radioSent(const esp_now_send_info_t *,esp_now_send_status_t status) {
  if(status==ESP_NOW_SEND_SUCCESS)txAcked=txAcked+1; else txNoAck=txNoAck+1;
}
static bool addPeer() {
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr,peerMac,6); memcpy(peer.lmk,PAIR_LMK,16);
  peer.channel=CHANNEL; peer.ifidx=WIFI_IF_STA; peer.encrypt=true;
  return esp_now_add_peer(&peer)==ESP_OK;
}
// Re-installing the encrypted peer resets its CCMP replay state. Without this, a peer that
// rebooted (packet number restarted) can be silently dropped until both boards reset.
static void refreshPeer(const char *why) {
  if(!radioReady)return;
  esp_now_del_peer(peerMac);
  bool ok=addPeer();++peerRefreshes;
  Serial.printf("{\"event\":\"peer_refresh\",\"reason\":\"%s\",\"ok\":%s}\n",why,ok?"true":"false");
}

static bool startRadio() {
  if(!inbox) inbox=xQueueCreate(1,sizeof(Received));
  if(!inbox) return false;
  xQueueReset(inbox);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA); WiFi.setSleep(false);
  WiFi.disconnect(false,false); // never roam to a stored AP channel
  uint8_t own[6]; esp_wifi_get_mac(WIFI_IF_STA,own);
  if(memcmp(own,selfMac,6)!=0) {
    Serial.printf("{\"event\":\"wrong_board\",\"mac\":\"%s\"}\n",WiFi.macAddress().c_str());
    WiFi.mode(WIFI_OFF); return false;
  }
  if(esp_wifi_set_channel(CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK) {WiFi.mode(WIFI_OFF);return false;}
  if(esp_now_init()!=ESP_OK) {WiFi.mode(WIFI_OFF);return false;}
  if(esp_now_set_pmk(PAIR_PMK)!=ESP_OK || !addPeer() ||
     esp_now_register_recv_cb(radioReceive)!=ESP_OK || esp_now_register_send_cb(radioSent)!=ESP_OK) {
    esp_now_deinit();WiFi.mode(WIFI_OFF);return false;
  }
  uint8_t ch=0; wifi_second_chan_t sc; esp_wifi_get_channel(&ch,&sc);
  radioReady=true;lastAcceptedAt=millis();
  Serial.printf("{\"event\":\"radio_ready\",\"channel\":%u,\"encrypted\":true,\"mac\":\"%s\"}\n",ch,WiFi.macAddress().c_str());
  return true;
}
static void stopRadio() {
  radioReady=false; esp_now_deinit(); WiFi.mode(WIFI_OFF);
  if(inbox)xQueueReset(inbox);
}
static void sendPacket(Packet p) {
  if(!radioReady)return;
  p.session=bootSession;p.sequence=++txSeq;p.uptime=millis();
  if(esp_now_send(peerMac,reinterpret_cast<uint8_t*>(&p),sizeof(p))!=ESP_OK)++txErrors;
}
static bool pollRadio(Received &r) {
  if(inbox && xQueueReceive(inbox,&r,0)==pdTRUE){++rxCount;return true;}
  return false;
}
// Packets/second over the last log interval.
static float rxRate(uint32_t now) {
  static uint32_t lastN=0,lastT=0;
  uint32_t n=rxAccepted;float r=lastT&&now>lastT?(n-lastN)*1000.0f/(now-lastT):0;
  lastN=n;lastT=now;return r;
}
// If the peer has been silent for silenceMs, re-install it (at most once per silenceMs).
static void peerWatchdog(uint32_t now,uint32_t silenceMs) {
  static uint32_t lastRefresh=0;
  if(!radioReady)return;
  if(int32_t(now-lastAcceptedAt)>int32_t(silenceMs) && now-lastRefresh>=silenceMs) {
    lastRefresh=now;refreshPeer("peer_silent");
  }
}
