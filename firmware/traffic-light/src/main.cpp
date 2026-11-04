#include <Arduino.h>

// Common-cathode module with onboard current-limiting resistors.
// XIAO: R=D6/GPIO21, Y=D7/GPIO20, G=D10/GPIO10, common=GND.
constexpr uint8_t pins[] = {21, 20, 10};
constexpr const char *names[] = {"RED", "YELLOW", "GREEN"};
constexpr uint32_t intervalMs = 1000;
uint8_t phase = 0;
uint32_t changedAt = 0;
uint32_t cycles = 0;

void showPhase() {
  for (auto pin : pins) digitalWrite(pin, LOW);
  digitalWrite(pins[phase], HIGH);
  Serial.printf("LIGHT=%s GPIO=%u UPTIME_MS=%lu CYCLE=%lu\n",
                names[phase], pins[phase], (unsigned long)millis(),
                (unsigned long)cycles);
}

void setup() {
  for (auto pin : pins) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  Serial.begin(115200);
  // No wait for USB: standalone operation starts immediately as well.
  changedAt = millis();
  showPhase();
}

void loop() {
  const uint32_t now = millis();
  if (now - changedAt >= intervalMs) {
    changedAt = now;
    phase = (phase + 1) % 3;
    if (phase == 0) ++cycles;
    showPhase();
  }
  delay(1);
}
