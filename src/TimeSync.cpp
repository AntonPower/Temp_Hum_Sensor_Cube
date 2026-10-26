#include "TimeSync.h"

#include <time.h>

#include "Clock.h"
#include "config.h"

namespace TimeSync {
namespace {

constexpr uint32_t PLAUSIBLE_EPOCH = 1700000000UL;  // Nov. 2023
constexpr uint32_t SYNC_TIMEOUT_MS = 10UL * 1000UL;
constexpr uint32_t RESYNC_MS = 24UL * 3600UL * 1000UL;
constexpr uint32_t RETRY_MS = 3600UL * 1000UL;

bool haveTime = false;
uint32_t epochAtSync = 0;
uint32_t clockAtSync = 0;
bool attempted = false;
uint32_t lastAttempt = 0;

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

bool sync() {
    attempted = true;
    lastAttempt = Clock::now();

    configTime(TIMEZONE, NTP_SERVER);
    const uint32_t start = millis();
    while (uint32_t(time(nullptr)) < PLAUSIBLE_EPOCH) {
        if (millis() - start > SYNC_TIMEOUT_MS) {
            Serial.println("Uhrzeit: kein NTP");
            return false;
        }
        delay(100);
    }
    epochAtSync = time(nullptr);
    clockAtSync = Clock::now();
    haveTime = true;
    Serial.printf("Uhrzeit: %lu\n", (unsigned long)epochAtSync);
    return true;
}

bool due() {
    if (!attempted) {
        return true;
    }
    return Clock::now() - lastAttempt >= (haveTime ? RESYNC_MS : RETRY_MS);
}

}  // namespace TimeSync
