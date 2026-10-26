#pragma once

#include <Arduino.h>

// Forced Light-Sleep bei ausgeschaltetem WLAN
namespace Sleep {

// Der SDK-Timer für Forced Light-Sleep ist auf ca. 268 s begrenzt
constexpr uint32_t MAX_LIGHT_SLEEP_MS = 260UL * 1000UL;

// Schläft durationMs (höchstens MAX_LIGHT_SLEEP_MS) oder bis wakePin auf LOW
// geht. Gibt true zurück, wenn der Taster geweckt hat.
bool light(uint32_t durationMs, uint8_t wakePin);

}  // namespace Sleep
