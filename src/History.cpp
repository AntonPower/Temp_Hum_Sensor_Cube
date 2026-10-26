#include "History.h"

namespace History {
namespace {

constexpr int16_t GAP = INT16_MIN;
constexpr uint32_t STEP_MS = STEP_S * 1000UL;

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

}  // namespace History
