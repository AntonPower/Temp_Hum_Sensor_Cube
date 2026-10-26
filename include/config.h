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
