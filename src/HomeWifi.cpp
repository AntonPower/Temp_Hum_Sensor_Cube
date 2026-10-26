#include "HomeWifi.h"

#include <ESP8266WiFi.h>

#include "Settings.h"
#include "config.h"

namespace HomeWifi {

bool configured() {
    return settings.wifiSsid[0] != '\0';
}

bool connect() {
    WiFi.persistent(false);  // Zugangsdaten nicht bei jedem Verbinden in den Flash schreiben
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(HOSTNAME);
    WiFi.begin(settings.wifiSsid, settings.wifiPassword);

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("WLAN: keine Verbindung");
            disconnect();
            return false;
        }
        delay(50);
    }
    Serial.printf("WLAN: verbunden, IP %s\n", WiFi.localIP().toString().c_str());
    return true;
}

void disconnect() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
}

}  // namespace HomeWifi
