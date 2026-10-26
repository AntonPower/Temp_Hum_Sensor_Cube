#pragma once

#include <Arduino.h>

// Verlauf der letzten 7 Tage im RAM, ein Mittelwert alle 10 Minuten.
// Nach einem Neustart beginnt er von vorn.
namespace History {

constexpr uint32_t STEP_S = 10UL * 60UL;
constexpr size_t CAPACITY = 7UL * 24UL * 3600UL / STEP_S;  // 1008 Punkte

// Jede Messung einreichen, nowMs aus Clock::now()
void add(float temperature, float humidity, uint32_t nowMs);

size_t size();
// i = 0 ist der älteste Punkt, NAN für Lücken
float temperatureAt(size_t i);
float humidityAt(size_t i);
// Sekunden seit dem Ende des jüngsten Punkts
uint32_t ageOfNewestS(uint32_t nowMs);

}  // namespace History
