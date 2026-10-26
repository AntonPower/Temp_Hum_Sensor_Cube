#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

#include "Si7006.h"
#include "config.h"

Si7006 sensor;

float lastTemperature = NAN;
float lastHumidity = NAN;
uint32_t lastMeasurement = 0;

void measure() {
    float t, h;
    if (sensor.read(t, h)) {
        lastTemperature = t;
        lastHumidity = h;
        Serial.printf("Temperatur: %.2f °C, Feuchte: %.1f %%\n", t, h);
    } else {
        Serial.println("Fehler beim Lesen des Sensors");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("Temp. Hum. Sensor Cube");

    // WLAN bleibt aus, solange es nicht gebraucht wird
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!sensor.begin()) {
        Serial.println("Sensor nicht gefunden (I2C 0x40)");
    }

    measure();
    lastMeasurement = millis();
}

void loop() {
    if (millis() - lastMeasurement >= MEASURE_INTERVAL_MS) {
        lastMeasurement = millis();
        measure();
    }
    delay(10);
}
