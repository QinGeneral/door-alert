#pragma once
#include <stdint.h>
#include "protocol.h"

constexpr uint16_t DESK_NEAR_MM=1000;
constexpr uint16_t FOREGROUND_MARGIN_MM=300;
constexpr uint8_t MIN_VALID_ZONES=48;
constexpr uint32_t OCCUPIED_HOLD_MS=2000;

inline bool adjacentHit(const bool hit[64]) {
  for(int i=0;i<64;++i) {
    if(!hit[i])continue;
    if(i%8<7 && hit[i+1])return true;
    if(i<56 && hit[i+8])return true;
  }
  return false;
}
struct Decision {
  Condition state=Condition::Starting;
  uint8_t hitFrames=0,clearFrames=0;
  uint32_t lastHit=0;
  Condition update(Condition candidate,uint32_t now) {
    if(candidate==Condition::Occupied) {
      if(hitFrames<2)++hitFrames;
      clearFrames=0;lastHit=now;
      // During the first possible target frame, never report CLEAR.
      state=hitFrames>=2 ? Condition::Occupied : Condition::Invalid;
    } else if(candidate==Condition::Clear) {
      hitFrames=0;if(clearFrames<3)++clearFrames;
      if(state==Condition::Occupied && now-lastHit<OCCUPIED_HOLD_MS)return state;
      state=clearFrames>=3 ? Condition::Clear : Condition::Invalid;
    } else {
      state=candidate;hitFrames=0;clearFrames=0;
    }
    return state;
  }
};

