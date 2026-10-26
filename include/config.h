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

// Messintervall (wird später über die Weboberfläche einstellbar)
constexpr uint32_t MEASURE_INTERVAL_MS = 60UL * 1000UL;

// RGB-LED: true, wenn die LED eine gemeinsame Anode hat (Pin LOW = an)
constexpr bool LED_ACTIVE_LOW = true;

// Temperaturanzeige (wird später über die Weboberfläche einstellbar)
constexpr float TARGET_TEMPERATURE = 21.0f;   // Wunschtemperatur = grün
constexpr float TEMPERATURE_SPAN = 4.0f;      // ab Wunsch ± Span voll blau bzw. rot
constexpr uint32_t DISPLAY_DURATION_MS = 10UL * 1000UL;

// Taster
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
constexpr uint32_t BUTTON_LONG_PRESS_MS = 3000;  // langer Druck = Einstellungsmodus
