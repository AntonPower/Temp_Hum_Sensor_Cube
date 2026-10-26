#pragma once

#include <Arduino.h>

// Echte Uhrzeit, vom Browser beim Öffnen der Seite übernommen und mit
// Clock::now() weitergezählt. Nach einem Stromausfall wieder unbekannt.
namespace TimeSync {

bool known();
// Unix-Zeit zu einem Zeitpunkt von Clock::now(), 0 wenn unbekannt
uint32_t epochAt(uint32_t clockMs);
uint32_t epochNow();

// Uhrzeit setzen (Unix-Zeit in Sekunden), ungültige Werte werden ignoriert
bool set(uint32_t epoch);

}  // namespace TimeSync
