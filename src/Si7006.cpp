#include "Si7006.h"

bool Si7006::begin() {
    if (!command(CMD_RESET)) {
        return false;
    }
    delay(15);  // Reset dauert laut Datenblatt max. 15 ms
    return true;
}

bool Si7006::read(float &temperature, float &humidity) {
    uint16_t raw;

    if (!command(CMD_MEASURE_RH) || !readRaw(raw, true, 50)) {
        return false;
    }
    humidity = 125.0f * raw / 65536.0f - 6.0f;
    humidity = constrain(humidity, 0.0f, 100.0f);

    if (!command(CMD_READ_PREV_T) || !readRaw(raw, false, 10)) {
        return false;
    }
    temperature = 175.72f * raw / 65536.0f - 46.85f;
    return true;
}

bool Si7006::command(uint8_t cmd) {
    _wire.beginTransmission(ADDRESS);
    _wire.write(cmd);
    return _wire.endTransmission() == 0;
}

bool Si7006::readRaw(uint16_t &raw, bool withCrc, uint16_t timeoutMs) {
    const uint8_t len = withCrc ? 3 : 2;
    const uint32_t start = millis();

    // Im No-Hold-Modus quittiert der Sensor nicht, solange er noch misst.
    while (_wire.requestFrom(ADDRESS, len) != len) {
        if (millis() - start > timeoutMs) {
            return false;
        }
        delay(2);
    }

    uint8_t buf[3];
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = _wire.read();
    }
    if (withCrc && crc8(buf, 2) != buf[2]) {
        return false;
    }
    raw = (uint16_t(buf[0]) << 8) | buf[1];
    return true;
}

// CRC-8, Polynom x^8 + x^5 + x^4 + 1, Startwert 0x00
uint8_t Si7006::crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0x00;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
        }
    }
    return crc;
}
