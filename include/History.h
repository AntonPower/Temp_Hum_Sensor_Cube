#pragma once

#include <Arduino.h>

// Verlauf der letzten 7 Tage, ein Mittelwert alle 10 Minuten. Liegt im RAM und
// wird stündlich in den Flash gesichert, damit er einen Stromausfall übersteht.
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

// Beim Start: gesicherten Verlauf aus dem Flash laden
void restore();
// Sobald die Uhrzeit bekannt ist: Lücke seit dem letzten Sichern auffüllen
void fillGapSinceRestore();
// Regelmäßig aufrufen, sichert höchstens einmal pro Stunde
void saveIfDue(uint32_t nowMs);

}  // namespace History
