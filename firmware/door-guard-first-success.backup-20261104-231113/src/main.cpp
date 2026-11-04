#include <Arduino.h>
#include "radio.h"
#ifdef ROLE_REAR
#include "rear.h"
#else
#include "front.h"
#endif

void setup(){Serial.begin(115200);bootSession=esp_random();setupRole();}
void loop(){loopRole();delay(5);}
