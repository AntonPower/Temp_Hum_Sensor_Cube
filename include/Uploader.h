#pragma once

#include <Arduino.h>

// Schaltet das WLAN kurz ein, sendet die Messwerte an Volkszähler und
// schaltet es danach wieder aus.
namespace Uploader {

// true, wenn WLAN und Volkszähler eingerichtet sind
bool configured();
bool send(float temperature, float humidity);

}  // namespace Uploader
