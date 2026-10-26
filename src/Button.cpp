#include "Button.h"

void Button::begin() {
    pinMode(_pin, INPUT_PULLUP);
}

Button::Event Button::poll() {
    const uint32_t now = millis();
    const bool raw = digitalRead(_pin) == LOW;

    if (raw != _rawPressed) {
        _rawPressed = raw;
        _lastChange = now;
    }
    if (now - _lastChange < _debounceMs) {
        return Event::None;
    }

    if (_rawPressed && !_pressed) {
        _pressed = true;
        _longReported = false;
        _pressStart = now;
    } else if (!_rawPressed && _pressed) {
        _pressed = false;
        if (!_longReported) {
            return Event::ShortPress;
        }
    } else if (_pressed && !_longReported && now - _pressStart >= _longPressMs) {
        // Langer Druck wird schon beim Halten gemeldet, nicht erst beim Loslassen
        _longReported = true;
        return Event::LongPress;
    }
    return Event::None;
}
