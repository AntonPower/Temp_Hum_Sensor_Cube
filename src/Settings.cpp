#include "Settings.h"

#include <EEPROM.h>

namespace {

constexpr uint32_t MAGIC = 0x53434233;  // "SCB3", bei Strukturänderung erhöhen

struct Stored {
    uint32_t magic;
    Settings data;
    uint32_t checksum;
};

uint32_t checksumOf(const Settings &s) {
    // FNV-1a über die Rohdaten
    uint32_t hash = 2166136261u;
    const uint8_t *p = reinterpret_cast<const uint8_t *>(&s);
    for (size_t i = 0; i < sizeof(Settings); i++) {
        hash = (hash ^ p[i]) * 16777619u;
    }
    return hash;
}

}  // namespace

Settings settings;

void Settings::setDefaults() {
    memset(this, 0, sizeof(*this));
    measureIntervalS = 60;
    targetTemperature = 21.0f;
    temperatureSpan = 4.0f;
}

void Settings::load() {
    Stored stored;
    EEPROM.begin(sizeof(Stored));
    EEPROM.get(0, stored);
    EEPROM.end();

    if (stored.magic == MAGIC && stored.checksum == checksumOf(stored.data)) {
        *this = stored.data;
    } else {
        setDefaults();
    }
}

void Settings::save() const {
    Stored stored;
    stored.magic = MAGIC;
    stored.data = *this;
    stored.checksum = checksumOf(stored.data);

    EEPROM.begin(sizeof(Stored));
    EEPROM.put(0, stored);
    EEPROM.end();  // schreibt in den Flash
}
