#include "TimeSync.h"

#include "Clock.h"

namespace TimeSync {
namespace {

constexpr uint32_t PLAUSIBLE_EPOCH = 1700000000UL;  // Nov. 2023

bool haveTime = false;
uint32_t epochAtSync = 0;
uint32_t clockAtSync = 0;

}  // namespace

bool known() {
    return haveTime;
}

uint32_t epochAt(uint32_t clockMs) {
    if (!haveTime) {
        return 0;
    }
    return epochAtSync + int32_t(clockMs - clockAtSync) / 1000;
}

uint32_t epochNow() {
    return epochAt(Clock::now());
}

bool set(uint32_t epoch) {
    if (epoch < PLAUSIBLE_EPOCH) {
        return false;
    }
    epochAtSync = epoch;
    clockAtSync = Clock::now();
    haveTime = true;
    Serial.printf("Uhrzeit: %lu\n", (unsigned long)epoch);
    return true;
}

}  // namespace TimeSync
