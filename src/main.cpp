#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "Button.h"
#include "RgbLed.h"
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
    led.showTemperature(lastTemperature, TARGET_TEMPERATURE, TEMPERATURE_SPAN);
    displayActive = true;
    displayStart = millis();
}

void enterSettingsMode() {
    // Platzhalter für Schritt 3: Access Point mit Weboberfläche
    Serial.println("Einstellungsmodus (folgt in Schritt 3)");
    for (uint8_t i = 0; i < 3; i++) {
        led.set(255, 0, true);  // violett blinken
        delay(150);
        led.off();
        delay(150);
    }
    displayActive = false;
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Temp. Hum. Sensor Cube");

    // WLAN bleibt aus, solange es nicht gebraucht wird
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

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
            showTemperature();
            break;
        case Button::Event::LongPress:
            enterSettingsMode();
            break;
        case Button::Event::None:
            break;
    }

    if (displayActive && millis() - displayStart >= DISPLAY_DURATION_MS) {
        displayActive = false;
        led.off();
    }

    if (millis() - lastMeasurement >= MEASURE_INTERVAL_MS) {
        lastMeasurement = millis();
        measure();
    }
    delay(10);
}
