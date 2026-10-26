#pragma once

#include <Arduino.h>

// RGB-LED an R/G (PWM) und B (GPIO16, nur an/aus möglich)
class RgbLed {
public:
    RgbLed(uint8_t pinR, uint8_t pinG, uint8_t pinB, bool activeLow)
        : _pinR(pinR), _pinG(pinG), _pinB(pinB), _activeLow(activeLow) {}

    void begin();
    // r und g: 0..255, b: an/aus
    void set(uint8_t r, uint8_t g, bool b);
    void off() { set(0, 0, false); }

    // Farbverlauf blau → cyan → grün → gelb → rot um die Wunschtemperatur
    void showTemperature(float temperature, float target, float span);

private:
    void writePwm(uint8_t pin, uint8_t value);

    uint8_t _pinR, _pinG, _pinB;
    bool _activeLow;
};
