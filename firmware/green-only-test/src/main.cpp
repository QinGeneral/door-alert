#include <Arduino.h>
// Front lamp test: GREEN steady on D10/GPIO10, R/Y off.
constexpr uint8_t R_PIN=21,Y_PIN=20,G_PIN=10;
void setup(){
  Serial.begin(115200);
  for(uint8_t p:{R_PIN,Y_PIN,G_PIN}){digitalWrite(p,LOW);pinMode(p,OUTPUT);}
  digitalWrite(G_PIN,HIGH);
}
void loop(){
  Serial.printf("{\"test\":\"green_only\",\"ms\":%lu,\"g_pin\":10,\"g_level\":%d}\n",(unsigned long)millis(),digitalRead(G_PIN));
  delay(1000);
}
