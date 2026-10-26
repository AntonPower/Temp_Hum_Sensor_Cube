#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "Button.h"
#include "Clock.h"
#include "History.h"
#include "RgbLed.h"
#include "Settings.h"
#include "SettingsPortal.h"
#include "Si7006.h"
#include "Sleep.h"
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
        SettingsPortal::setReadings(t, h);
        Serial.printf("Temperatur: %.2f °C, Feuchte: %.1f %%\n", t, h);
        return true;
    }
    Serial.println("Fehler beim Lesen des Sensors");
    return false;
}

// Regelmäßige Messung, die in den Verlauf eingeht
void measureAndRecord() {
    if (measure()) {
        History::add(lastTemperature, lastHumidity, Clock::now());
    }
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

    measureAndRecord();
    lastMeasurement = Clock::now();
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

    if (SettingsPortal::active()) {
        // Häufiger messen für die Live-Werte auf der Seite, der Verlauf mittelt
        if (Clock::now() - lastMeasurement >= SETTINGS_MEASURE_INTERVAL_MS) {
            lastMeasurement = Clock::now();
            measureAndRecord();
        }
        delay(1);
        return;
    }

    const uint32_t interval = settings.measureIntervalS * 1000UL;
    const uint32_t elapsed = Clock::now() - lastMeasurement;
    if (elapsed >= interval) {
        lastMeasurement = Clock::now();
        measureAndRecord();
        return;
    }

    const uint32_t remaining = interval - elapsed;
    if (USE_LIGHT_SLEEP && !displayActive && button.idle() && remaining >= 100) {
        const uint32_t sleepMs = min(remaining, Sleep::MAX_LIGHT_SLEEP_MS);
        const uint32_t before = millis();
        const bool byButton = Sleep::light(sleepMs, PIN_BUTTON);
        const uint32_t counted = millis() - before;

        // Beim Wecken per Taster ist die tatsächliche Schlafzeit unbekannt.
        // Dann geht der Uhr höchstens ein Messintervall verloren.
        if (!byButton && counted < sleepMs) {
            Clock::addSleep(sleepMs - counted);
        }
        return;
    }
    delay(10);
}
