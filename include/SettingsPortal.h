#pragma once

#include <Arduino.h>

// Eigener Access Point mit Einstellungsseite unter http://192.168.4.1
namespace SettingsPortal {

void start();
void stop();
bool active();
// Regelmäßig aus loop() aufrufen, solange active()
void handle();

}  // namespace SettingsPortal
