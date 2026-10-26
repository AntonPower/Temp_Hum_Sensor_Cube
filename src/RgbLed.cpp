#include "RgbLed.h"

void RgbLed::begin() {
    analogWriteRange(255);
    pinMode(_pinR, OUTPUT);
    pinMode(_pinG, OUTPUT);
    pinMode(_pinB, OUTPUT);
    off();
}

void RgbLed::set(uint8_t r, uint8_t g, bool b) {
    writePwm(_pinR, r);
    writePwm(_pinG, g);
    digitalWrite(_pinB, b != _activeLow ? HIGH : LOW);
}

void RgbLed::writePwm(uint8_t pin, uint8_t value) {
    if (_activeLow) {
        value = 255 - value;
    }
    if (value == 0) {
        analogWrite(pin, 0);
        digitalWrite(pin, LOW);
    } else if (value == 255) {
        analogWrite(pin, 0);
        digitalWrite(pin, HIGH);
    } else {
        analogWrite(pin, value);
    }
}

void RgbLed::showTemperature(float temperature, float target, float span) {
    // x = -1 (kalt, blau) .. 0 (Wunsch, grün) .. +1 (warm, rot)
    float x = constrain((temperature - target) / span, -1.0f, 1.0f);

    if (x < -0.5f) {
        // blau → cyan: Grün blendet ein
        set(0, uint8_t((x + 1.0f) * 2.0f * 255.0f), true);
    } else if (x < 0.0f) {
        // cyan → grün: Blau kann an GPIO16 nicht gedimmt werden, daher bis
        // kurz vor dem Wunschwert cyan
        set(0, 255, x < -0.1f);
    } else if (x < 0.5f) {
        // grün → gelb: Rot blendet ein
        set(uint8_t(x * 2.0f * 255.0f), 255, false);
    } else {
        // gelb → rot: Grün blendet aus
        set(255, uint8_t((1.0f - x) * 2.0f * 255.0f), false);
    }
}
