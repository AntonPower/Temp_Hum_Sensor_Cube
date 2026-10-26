# Temp. Hum. Sensor Cube

Firmware für einen ESP-12 mit TH06/Si7006-Sensor. Der Cube misst Temperatur und
Luftfeuchtigkeit, speichert den Verlauf der letzten 7 Tage und zeigt die Temperatur auf
Tastendruck 10 s lang über eine RGB-LED an.

## Bauen und Flashen

Das Projekt nutzt [PlatformIO](https://platformio.org/) in VS Code.

```sh
pio run                 # bauen
pio run -t upload       # flashen
pio device monitor      # serielle Ausgabe (115200 Baud)
```

## Pinbelegung

| Funktion | GPIO |
|---|---|
| I2C SDA / SCL (Sensor) | 4 / 5 |
| LED R / G / B | 12 / 14 / 16 |
| Taster (low-aktiv) | 13 |
| TM1637 CLK / DIO (V1 ungenutzt) | 2 / 0 |

## Stand

- [x] Messung über I2C, Ausgabe über Serial
- [x] LED-Farbverlauf und Taster (kurz drücken: Temperatur 10 s anzeigen, 3 s halten: Einstellungsmodus)
- [x] Einstellungsmodus per Browser
- [x] Verlauf der letzten 7 Tage und Stromsparen

## Einstellungen

Taster 3 s halten: Die LED blinkt langsam violett, und der Cube öffnet das WLAN
`SensorCube` (Passwort `sensorcube`). Im Browser `http://192.168.4.1` öffnen,
Werte eintragen und speichern. Nochmals 3 s halten beendet den Modus ohne Speichern, ebenso 5 Minuten
ohne geöffnete Einstellungsseite.

## Verlauf und Stromsparen

Der Cube bildet alle 10 Minuten einen Mittelwert und hält die letzten 7 Tage im
RAM. Das WLAN bleibt dabei aus, zwischen den Messungen schläft er im
Light-Sleep, ein Tastendruck weckt ihn sofort auf.

Die Diagramme stehen oben auf der Einstellungsseite. Die Zeitachse rechnet die
Seite mit der Uhr des Handys aus, der Cube braucht keine eigene Uhr. Nach einem
Stromausfall oder Neu-Flashen beginnt der Verlauf von vorn.
