#include "Uploader.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>

#include "Settings.h"
#include "config.h"

namespace Uploader {
namespace {

bool connectWifi() {
    WiFi.persistent(false);  // Zugangsdaten nicht bei jedem Verbinden in den Flash schreiben
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.wifiSsid, settings.wifiPassword);

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("WLAN: keine Verbindung");
            return false;
        }
        delay(50);
    }
    Serial.printf("WLAN: verbunden nach %lu ms\n", millis() - start);
    return true;
}

void disconnectWifi() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
}

bool sendValue(WiFiClient &client, const char *uuid, float value) {
    if (uuid[0] == '\0') {
        return true;  // Kanal nicht eingerichtet
    }

    // Ohne Zeitstempel nimmt die Middleware die Serverzeit
    String url = settings.vzUrl;
    if (url.endsWith("/")) {
        url.remove(url.length() - 1);
    }
    url += "/data/";
    url += uuid;
    url += ".json?operation=add&value=";
    url += String(value, 2);

    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT_MS);
    if (!http.begin(client, url)) {
        Serial.println("Volkszähler: ungültige URL");
        return false;
    }
    const int code = http.POST("");
    http.end();

    if (code != HTTP_CODE_OK) {
        Serial.printf("Volkszähler: Fehler %d für %s\n", code, uuid);
        return false;
    }
    return true;
}

}  // namespace

bool configured() {
    return settings.wifiSsid[0] != '\0' && settings.vzUrl[0] != '\0' &&
           (settings.vzUuidTemp[0] != '\0' || settings.vzUuidHum[0] != '\0');
}

bool send(float temperature, float humidity) {
    if (!connectWifi()) {
        disconnectWifi();
        return false;
    }

    WiFiClient client;
    const bool ok = sendValue(client, settings.vzUuidTemp, temperature) &&
                    sendValue(client, settings.vzUuidHum, humidity);
    disconnectWifi();

    if (ok) {
        Serial.println("Volkszähler: gesendet");
    }
    return ok;
}

}  // namespace Uploader
