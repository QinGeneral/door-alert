#pragma once
#include <stdint.h>

constexpr uint32_t MAGIC = 0x444f4f52;
constexpr uint8_t VERSION = 1;
constexpr uint8_t CHANNEL = 6;
constexpr uint8_t FRONT_MAC[6] = {0xac,0x27,0x6e,0x7f,0xd3,0x20};
constexpr uint8_t REAR_MAC[6] = {0xac,0x27,0x6e,0x7f,0xc3,0xb4};
enum class Kind : uint8_t { Status = 1, Hello = 2 };
enum class Condition : uint8_t {
  Starting=0, Clear=1, Occupied=2, SensorError=3, Stale=4,
  Calibrating=5, Invalid=6
};
struct __attribute__((packed)) Packet {
  uint32_t magic = MAGIC;
  uint8_t version = VERSION;
  Kind kind = Kind::Status;
  Condition state = Condition::Starting;
  uint8_t mode = 0; // 0: DESK <=1000mm, 1: calibrated background
  uint32_t session = 0;
  uint32_t sequence = 0;
  uint32_t uptime = 0;
  uint32_t frameAge = 0xffffffff;
  uint16_t nearest = 0;
  uint8_t valid = 0;
  uint8_t foreground = 0;
};
static_assert(sizeof(Packet)==28, "Wire protocol size must match both boards");
inline const char *conditionName(Condition s) {
  switch(s) {
    case Condition::Clear:return "CLEAR";
    case Condition::Occupied:return "OCCUPIED";
    case Condition::SensorError:return "SENSOR_ERROR";
    case Condition::Stale:return "STALE";
    case Condition::Calibrating:return "CALIBRATING";
    case Condition::Invalid:return "INVALID";
    default:return "STARTING";
  }
}

