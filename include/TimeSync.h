#pragma once

#include <Arduino.h>

// Echte Uhrzeit per NTP, weitergezählt mit Clock::now()
namespace TimeSync {

bool known();
// Unix-Zeit zu einem Zeitpunkt von Clock::now(), 0 wenn unbekannt
uint32_t epochAt(uint32_t clockMs);
uint32_t epochNow();

// Bei bestehender Heim-WLAN-Verbindung aufrufen
bool sync();
// true, wenn ein neuer Abgleich fällig ist (täglich, ohne Uhrzeit stündlich)
bool due();

}  // namespace TimeSync
