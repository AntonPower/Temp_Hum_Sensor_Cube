#include "SettingsPortal.h"

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

#include "Clock.h"
#include "History.h"
#include "TimeSync.h"
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

// Uhrzeit des Handys übernehmen, damit der Cube Stromausfälle im Verlauf erkennt
void handleTime() {
    const bool wasKnown = TimeSync::known();
    if (TimeSync::set(strtoul(server.arg("e").c_str(), nullptr, 10)) && !wasKnown) {
        History::fillGapSinceRestore();
    }
    server.send(204);
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
    // Manuelle Punkte als [Alter in s, Temperatur, Feuchte]
    chunk += F("],\"m\":[");
    for (size_t i = 0; i < History::manualSize(); i++) {
        uint32_t ageS;
        float t, h;
        History::manualAt(i, Clock::now(), ageS, t, h);
        if (i > 0) {
            chunk += ',';
        }
        chunk += '[';
        chunk += String(ageS);
        chunk += ',';
        chunk += String(t, 2);
        chunk += ',';
        chunk += String(h, 1);
        chunk += ']';
        if (chunk.length() > 1000) {
            server.sendContent(chunk);
            chunk = "";
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
        "<p><button id=\"b1\" onclick=\"show(1)\">1 Stunde</button> "
        "<button id=\"b24\" onclick=\"show(24)\">1 Tag</button> "
        "<button id=\"b168\" onclick=\"show(168)\">7 Tage</button></p>"
        "<h2>Temperatur</h2><canvas id=\"ct\"></canvas>"
        "<h2>Luftfeuchte</h2><canvas id=\"ch\"></canvas>"
        "<p class=\"hint\" id=\"hi\"></p>"
        "<script>fetch('/zeit?e='+Math.floor(Date.now()/1000)).catch(function(){});var D=null,hours=24;"
        // Zeitachse aus der Uhr des Handys: Punkt i endet vor age + (n-1-i)*step Sekunden
        "function draw(id,k,unit,col){var d=D,c=document.getElementById(id),w=c.clientWidth,H=160,"
        "r=window.devicePixelRatio||1;c.width=w*r;c.height=H*r;var g=c.getContext('2d');g.scale(r,r);"
        "g.font='11px sans-serif';var now=Date.now(),span=hours*36e5,t0=now-span,L=40,B=16,"
        "x=function(t){return L+(t-t0)/span*(w-L-4)},v=d[k],n=v.length,pts=[],man=[];"
        "v.forEach(function(a,i){var t=a===null?0:now-(d.age+(n-1-i)*d.step+d.step/2)*1000;"
        "pts.push(a===null||t<t0?null:[t,a])});"
        "d.m.forEach(function(p){var t=now-p[0]*1000;if(t>=t0)man.push([t,p[k=='t'?1:2]])});"
        // Achse vom niedrigsten bis zum höchsten Wert im sichtbaren Zeitraum
        "var lo=1e9,hi=-1e9;pts.concat(man).forEach(function(p){if(p){lo=Math.min(lo,p[1]);hi=Math.max(hi,p[1])}});"
        "if(lo>hi){g.fillStyle='#666';g.fillText('Noch keine Daten',L,H/2);return}"
        "var pad=hi-lo<.2?.1:0,a0=lo-pad,a1=hi+pad,y=function(a){return 6+(a1-a)/(a1-a0)*(H-B-12)};"
        // Raster: 1 Stunde alle 10 min, 1 Tag alle 3 h, 7 Tage um Mitternacht
        "g.strokeStyle='#ddd';g.fillStyle='#666';var gm=hours==1?10:hours==24?180:1440,m=new Date(t0);"
        "m.setSeconds(0,0);var mm=m.getHours()*60+m.getMinutes();m.setHours(0,Math.ceil(mm/gm)*gm);"
        "for(;m.getTime()<now;m.setMinutes(m.getMinutes()+gm)){var X=x(m.getTime());g.beginPath();"
        "g.moveTo(X,4);g.lineTo(X,H-B);g.stroke();if(X<w-30)g.fillText(hours==1?m.getHours()+':'+"
        "('0'+m.getMinutes()).slice(-2):hours==24?m.getHours()+' h':"
        "m.toLocaleDateString('de-DE',{weekday:'short'}),X+2,H-4)}"
        "var f=k=='t'?1:0;g.fillText(hi.toFixed(f)+unit,0,y(hi)+4);g.fillText(lo.toFixed(f)+unit,0,y(lo)+4);"
        "g.strokeStyle=col;g.lineWidth=1.5;g.beginPath();var on=false;"
        "pts.forEach(function(p){if(!p){on=false;return}"
        "on?g.lineTo(x(p[0]),y(p[1])):g.moveTo(x(p[0]),y(p[1]));on=true});g.stroke();"
        "g.fillStyle=col;man.forEach(function(p){g.beginPath();g.arc(x(p[0]),y(p[1]),3,0,7);g.fill()})}"
        "function show(n){hours=n;[1,24,168].forEach(function(b){"
        "document.getElementById('b'+b).disabled=b==n});if(!D)return;"
        "draw('ct','t','°','#d9480f');draw('ch','h','%','#1971c2')}"
        "fetch('/verlauf').then(function(r){return r.json()}).then(function(d){D=d;show(hours);"
        "document.getElementById('hi').textContent=d.t.length?'Linie: Mittelwerte alle '+d.step/60+"
        "' Minuten, Punkte: Messungen per Tastendruck. '+Math.round(d.t.length*d.step/3600)+' h gespeichert.':''})"
        "</script>");
    body += F("<form method=\"post\" action=\"/save\"><h2>Messung und Anzeige</h2>");
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
    // Eigener Access Point mit Captive Portal
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SETTINGS_AP_SSID, SETTINGS_AP_PASSWORD);
    dns.start(53, "*", WiFi.softAPIP());
    Serial.printf("Einstellungsmodus: WLAN \"%s\", http://%s\n", SETTINGS_AP_SSID,
                  WiFi.softAPIP().toString().c_str());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/werte", HTTP_GET, handleReadings);
    server.on("/verlauf", HTTP_GET, handleHistory);
    server.on("/zeit", HTTP_GET, handleTime);
    server.onNotFound(handleNotFound);
    server.begin();

    running = true;
    startedAt = millis();
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
