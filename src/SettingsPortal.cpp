#include "SettingsPortal.h"

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

#include "Settings.h"
#include "config.h"

namespace SettingsPortal {
namespace {

ESP8266WebServer server(80);
DNSServer dns;
bool running = false;
uint32_t startedAt = 0;
float currentTemperature = NAN;
float currentHumidity = NAN;

String escape(const char *text) {
    String out;
    for (const char *c = text; *c; c++) {
        switch (*c) {
            case '&': out += F("&amp;"); break;
            case '<': out += F("&lt;"); break;
            case '>': out += F("&gt;"); break;
            case '"': out += F("&quot;"); break;
            default: out += *c;
        }
    }
    return out;
}

void copyArg(const char *name, char *dest, size_t size) {
    strlcpy(dest, server.arg(name).c_str(), size);
}

String field(const char *label, const char *name, const String &value,
             const char *type = "text", const char *extra = "") {
    String html = F("<label>");
    html += label;
    html += F("<input type=\"");
    html += type;
    html += F("\" name=\"");
    html += name;
    html += F("\" value=\"");
    html += value;
    html += F("\" ");
    html += extra;
    html += F("></label>");
    return html;
}

void sendPage(const String &body) {
    String html = F(
        "<!doctype html><html lang=\"de\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Sensor Cube</title><style>"
        "body{font-family:sans-serif;max-width:28em;margin:1em auto;padding:0 1em}"
        "label{display:block;margin:.8em 0}input{display:block;width:100%;"
        "box-sizing:border-box;padding:.4em;font-size:1em}"
        "button{margin-top:1em;padding:.6em 1.2em;font-size:1em}"
        "h2{margin-top:1.5em;font-size:1.1em}.now{font-size:2em;margin:.5em 0}</style></head><body>"
        "<h1>Sensor Cube</h1>");
    html += body;
    html += F("</body></html>");
    server.send(200, F("text/html; charset=utf-8"), html);
}

String readingsJson() {
    String json = F("{\"t\":");
    json += isnan(currentTemperature) ? String("null") : String(currentTemperature, 2);
    json += F(",\"h\":");
    json += isnan(currentHumidity) ? String("null") : String(currentHumidity, 1);
    json += '}';
    return json;
}

void handleReadings() {
    server.send(200, F("application/json"), readingsJson());
}

void handleRoot() {
    String body = F(
        "<p class=\"now\"><span id=\"t\">–</span> °C &nbsp; <span id=\"h\">–</span> %</p>"
        "<script>function u(d){document.getElementById('t').textContent="
        "d.t===null?'–':d.t.toFixed(1);document.getElementById('h').textContent="
        "d.h===null?'–':d.h.toFixed(0)}u(");
    body += readingsJson();
    body += F(");setInterval(function(){fetch('/werte').then(function(r){return r.json()})"
              ".then(u).catch(function(){})},10000)</script>");
    body += F("<form method=\"post\" action=\"/save\"><h2>WLAN</h2>");
    body += field("Name (SSID)", "ssid", escape(settings.wifiSsid));
    body += field("Passwort (leer lassen = unverändert)", "pass", "", "password");
    body += F("<h2>Volkszähler</h2>");
    body += field("Middleware-URL", "vzurl", escape(settings.vzUrl), "url",
                  "placeholder=\"http://192.168.178.10/middleware.php\"");
    body += field("UUID Temperatur", "uuidt", escape(settings.vzUuidTemp));
    body += field("UUID Feuchte", "uuidh", escape(settings.vzUuidHum));
    body += F("<h2>Messung und Anzeige</h2>");
    body += field("Messintervall in Sekunden", "interval",
                  String(settings.measureIntervalS), "number", "min=\"10\" max=\"86400\"");
    body += field("Wunschtemperatur in °C (grün)", "target",
                  String(settings.targetTemperature, 1), "number", "step=\"0.5\"");
    body += field("Spanne in °C (ab Wunsch ± Spanne voll blau/rot)", "span",
                  String(settings.temperatureSpan, 1), "number", "step=\"0.5\" min=\"0.5\"");
    body += F("<button type=\"submit\">Speichern und neu starten</button></form>");
    sendPage(body);
}

void handleSave() {
    copyArg("ssid", settings.wifiSsid, sizeof(settings.wifiSsid));
    if (server.arg("pass").length() > 0) {
        copyArg("pass", settings.wifiPassword, sizeof(settings.wifiPassword));
    }
    copyArg("vzurl", settings.vzUrl, sizeof(settings.vzUrl));
    copyArg("uuidt", settings.vzUuidTemp, sizeof(settings.vzUuidTemp));
    copyArg("uuidh", settings.vzUuidHum, sizeof(settings.vzUuidHum));
    settings.measureIntervalS = constrain(server.arg("interval").toInt(), 10L, 86400L);
    settings.targetTemperature = server.arg("target").toFloat();
    settings.temperatureSpan = max(0.5f, server.arg("span").toFloat());
    settings.save();

    sendPage(F("<p>Gespeichert. Der Cube startet neu und schaltet das WLAN ab.</p>"));
    Serial.println("Einstellungen gespeichert, Neustart");
    delay(1000);
    ESP.restart();
}

void handleNotFound() {
    // Captive Portal: jede Adresse führt auf die Einstellungsseite
    server.sendHeader(F("Location"), F("http://192.168.4.1/"));
    server.send(302);
}

}  // namespace

void setReadings(float temperature, float humidity) {
    currentTemperature = temperature;
    currentHumidity = humidity;
}

void start() {
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SETTINGS_AP_SSID, SETTINGS_AP_PASSWORD);

    dns.start(53, "*", WiFi.softAPIP());
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/werte", HTTP_GET, handleReadings);
    server.onNotFound(handleNotFound);
    server.begin();

    running = true;
    startedAt = millis();
    Serial.printf("Einstellungsmodus: WLAN \"%s\", http://%s\n", SETTINGS_AP_SSID,
                  WiFi.softAPIP().toString().c_str());
}

void stop() {
    server.stop();
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    running = false;
    Serial.println("Einstellungsmodus beendet");
}

bool active() {
    return running;
}

void handle() {
    dns.processNextRequest();
    server.handleClient();
    if (millis() - startedAt >= SETTINGS_TIMEOUT_MS) {
        stop();
    }
}

}  // namespace SettingsPortal
