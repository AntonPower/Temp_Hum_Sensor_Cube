#include "SettingsPortal.h"

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include "Clock.h"
#include "History.h"
#include "HomeWifi.h"
#include "TimeSync.h"
#include "Settings.h"
#include "config.h"

namespace SettingsPortal {
namespace {

ESP8266WebServer server(80);
DNSServer dns;
bool running = false;
bool homeNetwork = false;  // im Heim-WLAN statt eigenem Access Point
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
        "h2{margin-top:1.5em;font-size:1.1em}.now{font-size:2em;margin:.5em 0}"
        "canvas{width:100%;height:160px;display:block}.hint{color:#666}</style></head><body>"
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
    startedAt = millis();  // offene Seite hält den Modus am Leben
    server.send(200, F("application/json"), readingsJson());
}

// Verlauf als JSON, in Stücken gesendet, damit kein großer String im RAM entsteht
void handleHistory() {
    startedAt = millis();
    const size_t n = History::size();

    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, F("application/json"), "");

    String chunk = F("{\"step\":");
    chunk += String(History::STEP_S);
    chunk += F(",\"age\":");
    chunk += String(History::ageOfNewestS(Clock::now()));
    for (uint8_t series = 0; series < 2; series++) {
        chunk += series == 0 ? F(",\"t\":[") : F("],\"h\":[");
        for (size_t i = 0; i < n; i++) {
            const float v = series == 0 ? History::temperatureAt(i) : History::humidityAt(i);
            if (i > 0) {
                chunk += ',';
            }
            chunk += isnan(v) ? String("null") : String(v, series == 0 ? 2 : 1);
            if (chunk.length() > 1000) {
                server.sendContent(chunk);
                chunk = "";
            }
        }
    }
    chunk += F("]}");
    server.sendContent(chunk);
    server.sendContent("");
}

void handleRoot() {
    startedAt = millis();
    String body = F(
        "<p class=\"now\"><span id=\"t\">–</span> °C &nbsp; <span id=\"h\">–</span> %</p>"
        "<script>function u(d){document.getElementById('t').textContent="
        "d.t===null?'–':d.t.toFixed(1);document.getElementById('h').textContent="
        "d.h===null?'–':d.h.toFixed(0)}u(");
    body += readingsJson();
    body += F(");setInterval(function(){fetch('/werte').then(function(r){return r.json()})"
              ".then(u).catch(function(){})},10000)</script>");
    body += F(
        "<h2>Temperatur, letzte 7 Tage</h2><canvas id=\"ct\"></canvas>"
        "<h2>Luftfeuchte, letzte 7 Tage</h2><canvas id=\"ch\"></canvas>"
        "<p class=\"hint\" id=\"hi\"></p>"
        "<script>"
        // Zeitachse aus der Uhr des Handys: Punkt i endet vor age + (n-1-i)*step Sekunden
        "function draw(id,v,unit,col,d){var c=document.getElementById(id),w=c.clientWidth,H=160,"
        "r=window.devicePixelRatio||1;c.width=w*r;c.height=H*r;var g=c.getContext('2d');g.scale(r,r);"
        "g.font='11px sans-serif';var now=Date.now(),span=6048e5,t0=now-span,L=34,B=16,"
        "x=function(t){return L+(t-t0)/span*(w-L-4)};"
        "var lo=1e9,hi=-1e9;v.forEach(function(a){if(a!==null){lo=Math.min(lo,a);hi=Math.max(hi,a)}});"
        "if(lo>hi){g.fillStyle='#666';g.fillText('Noch keine Daten',L,H/2);return}"
        "lo=Math.floor(lo-.5);hi=Math.ceil(hi+.5);var y=function(a){return 4+(hi-a)/(hi-lo)*(H-B-8)};"
        "g.strokeStyle='#ddd';g.fillStyle='#666';var m=new Date(t0);m.setHours(24,0,0,0);"
        "for(;m.getTime()<now;m.setDate(m.getDate()+1)){var X=x(m.getTime());g.beginPath();"
        "g.moveTo(X,4);g.lineTo(X,H-B);g.stroke();g.fillText(m.toLocaleDateString('de-DE',"
        "{weekday:'short'}),X+2,H-4)}"
        "g.fillText(hi+unit,0,12);g.fillText(lo+unit,0,H-B);"
        "g.strokeStyle=col;g.lineWidth=1.5;g.beginPath();var on=false,n=v.length;"
        "v.forEach(function(a,i){if(a===null){on=false;return}"
        "var t=now-(d.age+(n-1-i)*d.step+d.step/2)*1000;if(t<t0)return;"
        "on?g.lineTo(x(t),y(a)):g.moveTo(x(t),y(a));on=true});g.stroke()}"
        "fetch('/verlauf').then(function(r){return r.json()}).then(function(d){"
        "draw('ct',d.t,'°','#d9480f',d);draw('ch',d.h,'%','#1971c2',d);"
        "document.getElementById('hi').textContent=d.t.length?'Mittelwerte alle '+d.step/60+"
        "' Minuten, '+Math.round(d.t.length*d.step/3600)+' h gespeichert.':''})"
        "</script>");
    body += F("<form method=\"post\" action=\"/save\"><h2>Heim-WLAN</h2>"
              "<p class=\"hint\">Damit ist der Cube nach langem Tastendruck unter "
              "http://sensorcube.local erreichbar.</p>");
    body += field("Name (SSID)", "ssid", escape(settings.wifiSsid));
    body += field("Passwort (leer lassen = unverändert)", "pass", "", "password");
    body += F("<h2>Messung und Anzeige</h2>");
    body += field("Messintervall in Sekunden (10 bis 600)", "interval",
                  String(settings.measureIntervalS), "number", "min=\"10\" max=\"600\"");
    body += field("Wunschtemperatur in °C (grün)", "target",
                  String(settings.targetTemperature, 1), "number", "step=\"0.5\"");
    body += field("Spanne in °C (ab Wunsch ± Spanne voll blau/rot)", "span",
                  String(settings.temperatureSpan, 1), "number", "step=\"0.5\" min=\"0.5\"");
    body += F("<button type=\"submit\">Speichern</button></form>");
    sendPage(body);
}

void handleSave() {
    strlcpy(settings.wifiSsid, server.arg("ssid").c_str(), sizeof(settings.wifiSsid));
    if (server.arg("pass").length() > 0) {
        strlcpy(settings.wifiPassword, server.arg("pass").c_str(), sizeof(settings.wifiPassword));
    }
    // Höchstens 10 Minuten, damit jeder Punkt im Verlauf mindestens eine Messung hat
    settings.measureIntervalS = constrain(server.arg("interval").toInt(), 10L, 600L);
    settings.targetTemperature = server.arg("target").toFloat();
    settings.temperatureSpan = max(0.5f, server.arg("span").toFloat());
    settings.save();

    // Kein Neustart, sonst ginge der Verlauf im RAM verloren
    sendPage(F("<p>Gespeichert.</p><p><a href=\"/\">Zurück</a></p>"));
    Serial.println("Einstellungen gespeichert");
}

void handleNotFound() {
    if (homeNetwork) {
        server.send(404, F("text/plain"), F("Nicht gefunden"));
        return;
    }
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
    homeNetwork = HomeWifi::configured() && HomeWifi::connect();
    if (homeNetwork) {
        MDNS.begin(HOSTNAME);
        MDNS.addService("http", "tcp", 80);
        TimeSync::sync();
        History::fillGapSinceRestore();
        Serial.printf("Seite: http://%s.local oder http://%s\n", HOSTNAME,
                      WiFi.localIP().toString().c_str());
    } else {
        // Ohne Heim-WLAN eigener Access Point mit Captive Portal
        WiFi.forceSleepWake();
        delay(1);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(SETTINGS_AP_SSID, SETTINGS_AP_PASSWORD);
        dns.start(53, "*", WiFi.softAPIP());
        Serial.printf("Einstellungsmodus: WLAN \"%s\", http://%s\n", SETTINGS_AP_SSID,
                      WiFi.softAPIP().toString().c_str());
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/werte", HTTP_GET, handleReadings);
    server.on("/verlauf", HTTP_GET, handleHistory);
    server.onNotFound(handleNotFound);
    server.begin();

    running = true;
    startedAt = millis();
}

void stop() {
    server.stop();
    if (homeNetwork) {
        MDNS.end();
        HomeWifi::disconnect();
    } else {
        dns.stop();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
    }
    running = false;
    Serial.println("Einstellungsmodus beendet");
}

bool active() {
    return running;
}

void handle() {
    if (homeNetwork) {
        MDNS.update();
    } else {
        dns.processNextRequest();
    }
    server.handleClient();
    if (millis() - startedAt >= SETTINGS_TIMEOUT_MS) {
        stop();
    }
}

}  // namespace SettingsPortal
