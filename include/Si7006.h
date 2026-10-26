#pragma once

#include <Arduino.h>
#include <Wire.h>

// Minimaler Treiber für TH06 / Si7006 (I2C-Adresse 0x40)
class Si7006 {
public:
    explicit Si7006(TwoWire &wire = Wire) : _wire(wire) {}

    bool begin();
    // Misst Feuchte und liest die dabei gemessene Temperatur aus.
    bool read(float &temperature, float &humidity);

private:
    static constexpr uint8_t ADDRESS          = 0x40;
    static constexpr uint8_t CMD_MEASURE_RH   = 0xF5;  // No Hold Master Mode
    static constexpr uint8_t CMD_READ_PREV_T  = 0xE0;  // Temperatur aus letzter RH-Messung
    static constexpr uint8_t CMD_RESET        = 0xFE;

    bool command(uint8_t cmd);
    bool readRaw(uint16_t &raw, bool withCrc, uint16_t timeoutMs);
    static uint8_t crc8(const uint8_t *data, uint8_t len);

    TwoWire &_wire;
};
