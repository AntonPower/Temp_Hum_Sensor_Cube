# Volkszähler einrichten

Der Cube schickt seine Messwerte per HTTP an die Volkszähler-Middleware. Dafür
braucht es einen Rechner im Heimnetz, der dauerhaft läuft, z. B. einen
Raspberry Pi.

## 1. Volkszähler installieren

Am einfachsten ist das fertige Image für den Raspberry Pi:

1. Das Raspberry-Pi-Image aus dem Volkszähler-Wiki (https://wiki.volkszaehler.org) herunterladen.
2. Mit dem Raspberry Pi Imager oder balenaEtcher auf eine SD-Karte schreiben.
3. Pi per LAN anschließen und starten. Die Weboberfläche ist danach unter
   `http://<IP-des-Pi>/` erreichbar.

Alternativ lässt sich Volkszähler auf einem vorhandenen Linux-Server oder per
Docker installieren, siehe die Installationsanleitung im Wiki.

## 2. Kanäle anlegen

1. Die Weboberfläche öffnen und auf **Kanal hinzufügen** klicken.
2. Einen Kanal **Temperatur** anlegen (Kanaltyp für Temperatur).
3. Einen Kanal **Luftfeuchte** anlegen (Kanaltyp für Luftfeuchtigkeit).
4. Bei jedem Kanal unter den Details die **UUID** kopieren.

## 3. Cube einstellen

1. Taster am Cube 3 s halten, die LED blinkt langsam violett.
2. Mit dem Handy ins WLAN `SensorCube` (Passwort `sensorcube`).
3. `http://192.168.4.1` öffnen und eintragen:
   - WLAN-Name und Passwort des Heimnetzes
   - Middleware-URL: `http://<IP-des-Pi>/middleware.php`
   - die beiden UUIDs
   - Messintervall, z. B. 300 s
4. Speichern. Der Cube startet neu und sendet ab dann bei jeder Messung.

## Testen ohne Cube

Mit curl lässt sich prüfen, ob die Middleware Werte annimmt:

```sh
curl -X POST "http://<IP-des-Pi>/middleware.php/data/<UUID>.json?operation=add&value=21.5"
```

Die Antwort sollte `"rows":1` enthalten, und der Wert taucht im Diagramm auf.
