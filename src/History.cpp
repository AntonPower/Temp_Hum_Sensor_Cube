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
constexpr uint32_t FILE_MAGIC = 0x56455232;  // "VER2"

struct Point {
    int16_t temperature;  // °C × 100
    int16_t humidity;     // % × 10
};

Point points[CAPACITY];
size_t head = 0;   // nächster Schreibplatz
size_t count = 0;
uint32_t totalSteps = 0;  // Anzahl aller je abgeschlossenen Schritte

// Manuelle Punkte merken sich ihren Schritt, damit sie auch nach einem
// Neustart an der richtigen Stelle im Verlauf liegen
struct ManualPoint {
    uint32_t step;     // Wert von totalSteps während der Messung
    uint16_t offsetS;  // Sekunden seit Beginn dieses Schritts
    int16_t temperature;
    int16_t humidity;
};

ManualPoint manual[MANUAL_CAPACITY];
size_t manualHead = 0;
size_t manualCount = 0;

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
    uint32_t totalSteps;
    uint32_t manualCount;
};

void push(Point p) {
    points[head] = p;
    head = (head + 1) % CAPACITY;
    if (count < CAPACITY) {
        count++;
    }
    totalSteps++;
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

const ManualPoint &manualAtIndex(size_t i) {
    return manual[(manualHead + MANUAL_CAPACITY - manualCount + i) % MANUAL_CAPACITY];
}

void pushManual(const ManualPoint &p) {
    manual[manualHead] = p;
    manualHead = (manualHead + 1) % MANUAL_CAPACITY;
    if (manualCount < MANUAL_CAPACITY) {
        manualCount++;
    }
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

void addManual(float temperature, float humidity, uint32_t nowMs) {
    if (!started) {
        started = true;
        stepStart = nowMs;
    }
    closeSteps(nowMs);
    pushManual({totalSteps, uint16_t((nowMs - stepStart) / 1000UL),
                int16_t(lroundf(temperature * 100.0f)), int16_t(lroundf(humidity * 10.0f))});
    saved = false;  // beim nächsten saveIfDue() sofort sichern
}

size_t manualSize() {
    return manualCount;
}

void manualAt(size_t i, uint32_t nowMs, uint32_t &ageS, float &temperature, float &humidity) {
    const ManualPoint &p = manualAtIndex(i);
    ageS = ageOfNewestS(nowMs) + (totalSteps - p.step) * STEP_S - p.offsetS;
    temperature = p.temperature / 100.0f;
    humidity = p.humidity / 10.0f;
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
            totalSteps = header.totalSteps;
            restoredEndEpoch = header.endEpoch;
            Serial.printf("Verlauf: %u Punkte geladen\n", unsigned(count));

            ManualPoint p;
            for (uint32_t i = 0; i < min<uint32_t>(header.manualCount, MANUAL_CAPACITY); i++) {
                if (size_t(f.read(reinterpret_cast<uint8_t *>(&p), sizeof(p))) != sizeof(p)) {
                    break;
                }
                pushManual(p);
            }
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
    // Manuelle Punkte, die älter als der gespeicherte Verlauf sind, fallen weg
    while (manualCount > 0 && totalSteps - manualAtIndex(0).step > count) {
        manualCount--;
    }
    FileHeader header = {FILE_MAGIC, uint32_t(count), TimeSync::epochAt(stepStart), totalSteps,
                         uint32_t(manualCount)};
    f.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header));
    for (size_t i = 0; i < count; i++) {
        f.write(reinterpret_cast<const uint8_t *>(&at(i)), sizeof(Point));
    }
    for (size_t i = 0; i < manualCount; i++) {
        f.write(reinterpret_cast<const uint8_t *>(&manualAtIndex(i)), sizeof(ManualPoint));
    }
    f.close();
}

}  // namespace History
