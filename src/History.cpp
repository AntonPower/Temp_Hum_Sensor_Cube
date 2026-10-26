#include "History.h"

#include <LittleFS.h>

#include "Clock.h"
#include "TimeSync.h"

namespace History {
namespace {

constexpr int16_t GAP = INT16_MIN;
constexpr uint32_t STEP_MS = STEP_S * 1000UL;
constexpr uint32_t SAVE_INTERVAL_MS = 3600UL * 1000UL;
constexpr const char *FILE_NAME = "/verlauf.bin";
constexpr uint32_t FILE_MAGIC = 0x56455231;  // "VER1"

struct Point {
    int16_t temperature;  // °C × 100
    int16_t humidity;     // % × 10
};

Point points[CAPACITY];
size_t head = 0;   // nächster Schreibplatz
size_t count = 0;

bool started = false;
uint32_t stepStart = 0;
float sumT = 0;
float sumH = 0;
uint16_t samples = 0;

uint32_t restoredEndEpoch = 0;  // Ende des jüngsten Punkts beim letzten Sichern
bool saved = false;
uint32_t lastSave = 0;

struct FileHeader {
    uint32_t magic;
    uint32_t count;
    uint32_t endEpoch;  // 0 = Uhrzeit war unbekannt
};

void push(Point p) {
    points[head] = p;
    head = (head + 1) % CAPACITY;
    if (count < CAPACITY) {
        count++;
    }
}

void closeSteps(uint32_t nowMs) {
    while (nowMs - stepStart >= STEP_MS) {
        if (samples > 0) {
            push({int16_t(lroundf(sumT / samples * 100.0f)),
                  int16_t(lroundf(sumH / samples * 10.0f))});
        } else {
            push({GAP, GAP});
        }
        sumT = sumH = 0;
        samples = 0;
        stepStart += STEP_MS;
    }
}

const Point &at(size_t i) {
    return points[(head + CAPACITY - count + i) % CAPACITY];
}

}  // namespace

void add(float temperature, float humidity, uint32_t nowMs) {
    if (!started) {
        started = true;
        stepStart = nowMs;
    }
    closeSteps(nowMs);
    sumT += temperature;
    sumH += humidity;
    samples++;
}

size_t size() {
    return count;
}

float temperatureAt(size_t i) {
    const Point &p = at(i);
    return p.temperature == GAP ? NAN : p.temperature / 100.0f;
}

float humidityAt(size_t i) {
    const Point &p = at(i);
    return p.humidity == GAP ? NAN : p.humidity / 10.0f;
}

uint32_t ageOfNewestS(uint32_t nowMs) {
    return (nowMs - stepStart) / 1000UL;
}

void restore() {
    if (!LittleFS.begin()) {
        Serial.println("Verlauf: Dateisystem nicht verfügbar");
        return;
    }
    File f = LittleFS.open(FILE_NAME, "r");
    if (!f) {
        return;
    }
    FileHeader header;
    if (f.read(reinterpret_cast<uint8_t *>(&header), sizeof(header)) == sizeof(header) &&
        header.magic == FILE_MAGIC && header.count <= CAPACITY) {
        // Punkte liegen in der Datei vom ältesten zum jüngsten
        const size_t bytes = header.count * sizeof(Point);
        if (size_t(f.read(reinterpret_cast<uint8_t *>(points), bytes)) == bytes) {
            count = header.count;
            head = count % CAPACITY;
            restoredEndEpoch = header.endEpoch;
            Serial.printf("Verlauf: %u Punkte geladen\n", unsigned(count));
        }
    }
    f.close();
}

void fillGapSinceRestore() {
    if (restoredEndEpoch == 0 || !TimeSync::known()) {
        return;
    }
    // Die Zeit, in der der Cube aus war, als Lücke eintragen
    const uint32_t nowEpoch = TimeSync::epochAt(started ? stepStart : Clock::now());
    if (nowEpoch > restoredEndEpoch) {
        const uint32_t missing = min<uint32_t>((nowEpoch - restoredEndEpoch) / STEP_S, CAPACITY);
        for (uint32_t i = 0; i < missing; i++) {
            push({GAP, GAP});
        }
        Serial.printf("Verlauf: %u Punkte Lücke\n", unsigned(missing));
    }
    restoredEndEpoch = 0;
}

void saveIfDue(uint32_t nowMs) {
    if (saved && nowMs - lastSave < SAVE_INTERVAL_MS) {
        return;
    }
    if (count == 0) {
        return;
    }
    saved = true;
    lastSave = nowMs;

    File f = LittleFS.open(FILE_NAME, "w");
    if (!f) {
        Serial.println("Verlauf: Sichern fehlgeschlagen");
        return;
    }
    FileHeader header = {FILE_MAGIC, uint32_t(count), TimeSync::epochAt(stepStart)};
    f.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header));
    for (size_t i = 0; i < count; i++) {
        f.write(reinterpret_cast<const uint8_t *>(&at(i)), sizeof(Point));
    }
    f.close();
}

}  // namespace History
