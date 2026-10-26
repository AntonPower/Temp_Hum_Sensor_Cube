#pragma once

#include <Arduino.h>

// Laufzeit in ms, die auch die Zeit im Light-Sleep enthält. millis() allein
// läuft im Forced Light-Sleep nicht (vollständig) weiter.
namespace Clock {

uint32_t now();
// Schlafzeit nachtragen, die millis() nicht mitgezählt hat
void addSleep(uint32_t ms);

}  // namespace Clock
