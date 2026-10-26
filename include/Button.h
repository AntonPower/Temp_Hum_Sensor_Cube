#pragma once

#include <Arduino.h>

// Entprellter, low-aktiver Taster mit kurzem und langem Druck
class Button {
public:
    enum class Event { None, ShortPress, LongPress };

    Button(uint8_t pin, uint32_t debounceMs, uint32_t longPressMs)
        : _pin(pin), _debounceMs(debounceMs), _longPressMs(longPressMs) {}

    void begin();
    // Regelmäßig aus loop() aufrufen
    Event poll();
    // true, solange der Taster losgelassen ist und kein Druck läuft
    bool idle() const { return !_rawPressed && !_pressed; }

private:
    uint8_t _pin;
    uint32_t _debounceMs;
    uint32_t _longPressMs;

    bool _rawPressed = false;
    bool _pressed = false;
    bool _longReported = false;
    uint32_t _lastChange = 0;
    uint32_t _pressStart = 0;
};
