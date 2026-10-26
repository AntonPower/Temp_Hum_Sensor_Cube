#pragma once

#include <Arduino.h>

// Dauerhaft im Flash gespeicherte Einstellungen
struct Settings {
    uint32_t measureIntervalS;
    float targetTemperature;  // Wunschtemperatur = grün
    float temperatureSpan;    // ab Wunsch ± Spanne voll blau bzw. rot

    void setDefaults();
    // Lädt aus dem Flash, bei ungültigen Daten Standardwerte
    void load();
    void save() const;
};

extern Settings settings;
