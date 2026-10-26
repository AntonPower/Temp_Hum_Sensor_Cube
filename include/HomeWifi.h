#pragma once

#include <Arduino.h>

// Verbindung zum Heim-WLAN, nur bei Bedarf eingeschaltet
namespace HomeWifi {

bool configured();
bool connect();
void disconnect();

}  // namespace HomeWifi
