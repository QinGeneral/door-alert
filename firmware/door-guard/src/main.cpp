#include <Arduino.h>
#include "radio.h"
#ifdef ROLE_REAR
#include "rear.h"
#else
#include "front.h"
#endif

void setup(){
  Serial.setTxBufferSize(4096);
  Serial.begin(115200);
  // Debug logging must never stall ranging or radio when no serial reader is open.
  Serial.setTxTimeoutMs(0);
  bootSession=esp_random();setupRole();
}
void loop(){loopRole();delay(5);}
