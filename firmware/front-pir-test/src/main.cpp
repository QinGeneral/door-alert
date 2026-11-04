#include <Arduino.h>

// Bench test only: these colors describe the FRONT PIR, not occupancy behind a door.
constexpr uint8_t RED_PIN = 21;     // D6 -> R
constexpr uint8_t YELLOW_PIN = 20;  // D7 -> Y
constexpr uint8_t GREEN_PIN = 10;   // D10 -> G
constexpr uint8_t PIR_PIN = 4;      // D2 <- AM312 OUT
constexpr uint32_t WARMUP_MS = 30000;
constexpr uint32_t DEBOUNCE_MS = 100;
constexpr uint32_t HOLD_MS = 2000;

enum class State { Warmup, Idle, Motion };
State state = State::Warmup;
bool raw = false;
bool stable = false;
bool haveMotion = false;
uint32_t startedAt = 0;
uint32_t rawChangedAt = 0;
uint32_t lastHighAt = 0;
uint32_t lastLogAt = 0;
bool red = false, yellow = false, green = false;

void setLights(bool r, bool y, bool g) {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(YELLOW_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  if (r) digitalWrite(RED_PIN, HIGH);
  if (y) digitalWrite(YELLOW_PIN, HIGH);
  if (g) digitalWrite(GREEN_PIN, HIGH);
  red = r; yellow = y; green = g;
}

void report(uint32_t now) {
  const char *name = state == State::Warmup ? "WARMUP" :
                     state == State::Motion ? "MOTION" : "IDLE";
  Serial.printf("{\"fw\":\"front-pir-test-v2\",\"ms\":%lu,\"state\":\"%s\","
                "\"raw\":%u,\"stable\":%u,\"r\":%u,\"y\":%u,\"g\":%u}\n",
                (unsigned long)(now - startedAt), name,
                unsigned(raw), unsigned(stable), unsigned(red),
                unsigned(yellow), unsigned(green));
}

void setup() {
  for (uint8_t pin : {RED_PIN, YELLOW_PIN, GREEN_PIN}) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  // AM312 drives OUT. Avoid loading it with the ESP32's internal pulldown.
  // An unplugged OUT can float: IDLE does not prove that sensor wiring is sound.
  pinMode(PIR_PIN, INPUT);
  Serial.begin(115200);
  startedAt = millis();
  rawChangedAt = startedAt;
  raw = digitalRead(PIR_PIN) == HIGH;
  setLights(false, true, false);
  report(startedAt);
}

void loop() {
  uint32_t now = millis();
  bool input = digitalRead(PIR_PIN) == HIGH;
  if (input != raw) {
    raw = input;
    rawChangedAt = now;
  }
  bool changed = false;
  if (stable != raw && now - rawChangedAt >= DEBOUNCE_MS) {
    stable = raw;
    changed = true;
  }
  State previous = state;
  if (now - startedAt < WARMUP_MS) {
    state = State::Warmup;
    haveMotion = false;
    setLights(false, ((now - startedAt) / 500) % 2 == 0, false);
  } else {
    if (stable) {
      haveMotion = true;
      lastHighAt = now;
    }
    bool active = haveMotion && now - lastHighAt < HOLD_MS;
    state = active ? State::Motion : State::Idle;
    setLights(active, false, !active);
  }
  if (changed || state != previous || now - lastLogAt >= 1000) {
    report(now);
    lastLogAt = now;
  }
  delay(10);
}
