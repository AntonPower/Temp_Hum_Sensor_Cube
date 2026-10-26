#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "Button.h"
#include "RgbLed.h"
#include "Settings.h"
#include "SettingsPortal.h"
#include "Si7006.h"
#include "config.h"

Si7006 sensor;
RgbLed led(PIN_LED_R, PIN_LED_G, PIN_LED_B, LED_ACTIVE_LOW);
Button button(PIN_BUTTON, BUTTON_DEBOUNCE_MS, BUTTON_LONG_PRESS_MS);

float lastTemperature = NAN;
float lastHumidity = NAN;
uint32_t lastMeasurement = 0;

bool displayActive = false;
uint32_t displayStart = 0;

bool measure() {
    float t, h;
    if (sensor.read(t, h)) {
        lastTemperature = t;
        lastHumidity = h;
        Serial.printf("Temperatur: %.2f °C, Feuchte: %.1f %%\n", t, h);
        return true;
    }
    Serial.println("Fehler beim Lesen des Sensors");
    return false;
}

void showTemperature() {
    // Frischer Wert für die Anzeige, sonst der letzte gültige
    measure();
    if (isnan(lastTemperature)) {
        return;
    }
    led.showTemperature(lastTemperature, settings.targetTemperature, settings.temperatureSpan);
    displayActive = true;
    displayStart = millis();
}

void toggleSettingsMode() {
    displayActive = false;
    if (SettingsPortal::active()) {
        SettingsPortal::stop();
        led.off();
    } else {
        SettingsPortal::start();
    }
}

// Langsames violettes Blinken, solange der Einstellungsmodus läuft
void updateSettingsLed() {
    const bool on = (millis() / 500) % 2 == 0;
    if (on) {
        led.set(255, 0, true);
    } else {
        led.off();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Temp. Hum. Sensor Cube");

    // WLAN bleibt aus, solange es nicht gebraucht wird
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

    settings.load();
    Serial.printf("Messintervall %u s, Wunschtemperatur %.1f °C\n",
                  settings.measureIntervalS, settings.targetTemperature);

    led.begin();
    button.begin();

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!sensor.begin()) {
        Serial.println("Sensor nicht gefunden (I2C 0x40)");
    }

    measure();
    lastMeasurement = millis();
}

void loop() {
    switch (button.poll()) {
        case Button::Event::ShortPress:
            if (!SettingsPortal::active()) {
                showTemperature();
            }
            break;
        case Button::Event::LongPress:
            toggleSettingsMode();
            break;
        case Button::Event::None:
            break;
    }

    if (SettingsPortal::active()) {
        SettingsPortal::handle();
        if (SettingsPortal::active()) {
            updateSettingsLed();
        } else {
            led.off();  // Zeitüberschreitung
        }
    }

    if (displayActive && millis() - displayStart >= DISPLAY_DURATION_MS) {
        displayActive = false;
        led.off();
    }

    if (millis() - lastMeasurement >= settings.measureIntervalS * 1000UL) {
        lastMeasurement = millis();
        measure();
    }
    delay(SettingsPortal::active() ? 1 : 10);
}
