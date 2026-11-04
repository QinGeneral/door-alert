#include <Arduino.h>
#include <esp_task_wdt.h>
#include "radio.h"
#ifdef ROLE_REAR
#include "rear.h"
#else
#include "front.h"
#endif

constexpr uint32_t LOOP_WDT_MS=10000;  // sensor init blocks ~1-3 s; I2C ops are bounded by Wire timeout

void setup(){
  Serial.begin(115200);
  // HWCDC blocks up to 100 ms per write (and up to ~2 s per burst) when the USB host is
  // attached but nobody reads the port. That stalled the loop, starving ranging and ESP-NOW.
  // With 0 ms the log is dropped instead of blocking.
  Serial.setTxTimeoutMs(0);
  esp_task_wdt_config_t wdt{};wdt.timeout_ms=LOOP_WDT_MS;wdt.idle_core_mask=0;wdt.trigger_panic=true;
  if(esp_task_wdt_reconfigure(&wdt)!=ESP_OK)esp_task_wdt_init(&wdt);
  esp_task_wdt_add(NULL);
  bootSession=esp_random();setupRole();
}
void loop(){esp_task_wdt_reset();loopRole();delay(5);}
