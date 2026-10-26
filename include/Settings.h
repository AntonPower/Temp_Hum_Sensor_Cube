#pragma once

#include <Arduino.h>

// Dauerhaft im Flash gespeicherte Einstellungen
struct Settings {
    char wifiSsid[33];
    char wifiPassword[65];
    char vzUrl[129];          // z. B. http://192.168.178.10/middleware.php
    char vzUuidTemp[37];
    char vzUuidHum[37];
    uint32_t measureIntervalS;
    float targetTemperature;  // Wunschtemperatur = grün
    float temperatureSpan;    // ab Wunsch ± Spanne voll blau bzw. rot

    void setDefaults();
    // Lädt aus dem Flash, bei ungültigen Daten Standardwerte
    void load();
    void save() const;
};

extern Settings settings;
