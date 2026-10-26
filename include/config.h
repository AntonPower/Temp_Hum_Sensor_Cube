#pragma once

#include <Arduino.h>

// Pinbelegung ESP-12 (siehe "Infos Temp. Hum. Sensor V2.docx")
constexpr uint8_t PIN_I2C_SDA = 4;   // GPIO4, Standard-I2C des ESP8266
constexpr uint8_t PIN_I2C_SCL = 5;   // GPIO5
constexpr uint8_t PIN_LED_R   = 12;
constexpr uint8_t PIN_LED_G   = 14;
constexpr uint8_t PIN_LED_B   = 16;
constexpr uint8_t PIN_BUTTON  = 13;  // low-aktiv
constexpr uint8_t PIN_TM_CLK  = 2;   // TM1637, in V1 nicht verwendet
constexpr uint8_t PIN_TM_DIO  = 0;

// RGB-LED: true, wenn die LED eine gemeinsame Anode hat (Pin LOW = an)
constexpr bool LED_ACTIVE_LOW = true;

// Temperaturanzeige (Wunschtemperatur und Spanne stehen in den Einstellungen)
constexpr uint32_t DISPLAY_DURATION_MS = 10UL * 1000UL;

// Taster
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 3000;  // langer Druck = Einstellungsmodus

// Einstellungsmodus
constexpr const char *SETTINGS_AP_SSID = "SensorCube";
constexpr const char *SETTINGS_AP_PASSWORD = "sensorcube";  // mind. 8 Zeichen
constexpr uint32_t SETTINGS_TIMEOUT_MS = 5UL * 60UL * 1000UL;  // ohne Aktivität auf der Seite zurück
constexpr uint32_t SETTINGS_MEASURE_INTERVAL_MS = 10UL * 1000UL;  // Live-Werte auf der Seite

// Light-Sleep zwischen den Messungen (Taster weckt auf)
constexpr bool USE_LIGHT_SLEEP = true;

// Heim-WLAN und Uhrzeit
constexpr const char *HOSTNAME = "sensorcube";  // erreichbar unter http://sensorcube.local
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15UL * 1000UL;
constexpr const char *NTP_SERVER = "pool.ntp.org";
constexpr const char *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";  // Deutschland
