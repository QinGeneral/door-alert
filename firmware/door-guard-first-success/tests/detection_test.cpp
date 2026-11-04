#include <assert.h>
#include "detection.h"
int main() {
  bool hit[64]{};
  assert(!adjacentHit(hit));
  hit[0]=true;assert(!adjacentHit(hit));
  hit[9]=true;assert(!adjacentHit(hit)); // diagonal does not qualify
  hit[8]=true;assert(adjacentHit(hit));
  for(auto &v:hit)v=false;
  hit[7]=true;hit[8]=true;assert(!adjacentHit(hit)); // no wrap across rows
  Decision d;
  assert(d.update(Condition::Clear,100)==Condition::Invalid);
  assert(d.update(Condition::Clear,200)==Condition::Invalid);
  assert(d.update(Condition::Clear,300)==Condition::Clear);
  assert(d.update(Condition::Occupied,400)==Condition::Invalid);
  assert(d.update(Condition::Occupied,500)==Condition::Occupied);
  assert(d.update(Condition::Clear,600)==Condition::Occupied);
  assert(d.update(Condition::Clear,700)==Condition::Occupied);
  assert(d.update(Condition::Clear,2499)==Condition::Occupied);
  assert(d.update(Condition::Clear,2500)==Condition::Clear);
  assert(d.update(Condition::SensorError,2600)==Condition::SensorError);
  assert(d.update(Condition::Clear,2700)==Condition::Invalid);
  assert(d.update(Condition::Stale,2800)==Condition::Stale);
  Decision wrap;
  wrap.update(Condition::Occupied,0xffffffe0);
  wrap.update(Condition::Occupied,0xfffffff0);
  assert(wrap.update(Condition::Clear,16)==Condition::Occupied);
  return 0;
}
