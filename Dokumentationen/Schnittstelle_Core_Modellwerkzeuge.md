# Schnittstelle Core → Modellwerkzeuge: Modell-Fingerprints

**Status:** Normativer Schnittstellenvertrag für C-0.21, fachlich finalisiert
am 10.11.2026; Implementierung vorhanden, vollständige Abnahme offen. Anforderungen:
A-C-00149 bis A-C-00151. Der Vertrag steht in der
[Modellbeschreibung, Modellformat 0.3](../Spezifikationen/Spezifikation_Modellbeschreibung.md#modellformat-03-fingerprints-und-modellpfade).

## Zweck

Modellwerkzeuge wie Segelmacher prüfen und aktualisieren die Fingerprints
eines Modellordners über denselben Core-Code. Die aufrufende Anwendung
übergibt einen Pfad; sie implementiert weder Hashverfahren noch das Schreiben
von `modells_validation.json` selbst. Es gibt keinen Netzwerk- oder CLI-
Zweitvertrag: Der vorgesehene Integrationsweg ist die öffentliche C++-
Bibliothek des Core.

## Öffentliche C++-API

Die öffentliche Deklaration liegt in
`segelphysik/modell_fingerprints.hpp`:

```cpp
#include <filesystem>
#include <string>
#include <vector>

namespace segelphysik {

enum class ModellFingerprintBereich {
    visualisation,
    mass,
    hydrostatics,
    aerodynamics,
    modelData
};

struct ModellFingerprintWert {
    ModellFingerprintBereich bereich;
    std::string hash;
    std::string modified_at;
};

struct ModellFingerprintErgebnis {
    std::vector<ModellFingerprintWert> werte;
    bool validierungsdatei_aktualisiert;
};

ModellFingerprintErgebnis aktualisiere_modell_fingerprints(
    const std::filesystem::path& modellordner,
    const std::vector<ModellFingerprintBereich>& bereiche);

}
```

Die Signatur ist der verbindliche API-Vertrag. Ein Aufruf mit allen fünf Bereichen eignet sich für ein
Modellwerkzeug; die GUI übergibt bei „Visualisation-Hash erneuern“
ausschließlich `ModellFingerprintBereich::visualisation`.

## Verhaltensvertrag

1. Der Core prüft beide Modelldateien und alle Ressourcen/Schemas vor
   Verwendung, prüft dateiübergreifende Identität und Ressourcenreferenzen
   und berechnet die angeforderten SHA-256-Fingerprints.
2. Der übergebene Pfad bezeichnet genau einen Modellordner mit den beiden
   Modelldateien. `schemaRoot` wird aus der gemeinsamen Laufzeitkonfiguration
   aufgelöst, nicht aus dem Arbeitsverzeichnis. Für jeden Bereich sortiert
   der Core alle regulären Dateien einschließlich versteckter Dateien nach
   normalisiertem relativem UTF-8-Pfad (`/`) byteweise. Für jede Datei
   gelangen nacheinander eine 64-Bit-unsigned little-endian Pfadlänge, die
   UTF-8-Pfadbytes, eine 64-Bit-unsigned little-endian Inhaltslänge und die
   exakten Inhaltsbytes in den Hashstrom. Leere Bereiche hashen den leeren
   Eingabestrom. Symlinks werden abgelehnt; absolute Pfade und Zeitstempel
   sind ausgeschlossen.
3. Ein erfolgreicher Aufruf schreibt nur die angeforderten Hashfelder.
   `modifiedAt` wird für einen Bereich nur aktualisiert, wenn dessen Hashwert
   sich ändert. Andere Bereiche und sonstige Validierungsmetadaten bleiben
   unverändert.
4. Vorhandene Fingerprints werden bei normalen Katalog-/Ladevorgängen
   geprüft, aber nicht automatisch überschrieben. Das Aktualisieren ist
   immer eine ausdrückliche Aktion des Aufrufers.
5. Die Ausgabe wird vor dem atomaren Ersetzen erneut schema-validiert. Bei
   Lese-, Schema-, Schreib- oder Konfliktfehlern liefert die Funktion keinen
   Erfolg und lässt die bestehende Validierungsdatei unverändert. Fehler
   werden mit den dokumentierten Core-Fehlerkennungen gemeldet.
6. Unbekannte Bereiche, Duplikate oder eine leere Bereichsliste sind
   Eingabefehler. Bei parallelen Schreibern wird keine Validierungsdatei
   teilweise oder unbemerkt überschrieben; Konflikte werden als Fehler
   gemeldet.
7. Fehler werfen `std::runtime_error` mit einer dokumentierten,
   eindeutigen Core-Fehlerkennung und Datei-/Feldkontext. Ein Fehler liefert
   keinen erfolgsförmigen Ergebniswert.

`CORE-FPR-0001` kennzeichnet veraltete Fingerprints beim Lesen,
`CORE-FPR-0002` eine nicht verfügbare Schreibsperre oder erkannte
Paralleländerung und `CORE-FPR-0003` einen fehlgeschlagenen Schreib- oder
atomaren Austauschvorgang. Schema-/Eingabefehler behalten den vorhandenen
Modellfehlervertrag. Alle Kennungen stehen im eingebetteten Core-Fehlerkatalog.

## Beispiel für Segelmacher

```cpp
const auto ergebnis = segelphysik::aktualisiere_modell_fingerprints(
    modellordner,
    {segelphysik::ModellFingerprintBereich::visualisation,
     segelphysik::ModellFingerprintBereich::mass,
     segelphysik::ModellFingerprintBereich::hydrostatics,
     segelphysik::ModellFingerprintBereich::aerodynamics,
     segelphysik::ModellFingerprintBereich::modelData});
```

Das Modellwerkzeug verwendet die zurückgegebenen Hashwerte für seine
Anzeige oder Erfolgsmeldung. Es muss `modells_validation.json` nicht selbst
lesen oder schreiben. Für GUI-Aktionen gelten dieselben Regeln; die GUI darf
nur den explizit gewählten Bereich erneuern.

## Laufzeitpfade

`segelphysik/defaults.hpp` stellt `laufzeit_defaults_datei()`,
`modellordner_standard(expliziter_ordner)` und
`schemaordner_fuer_modellordner(modellordner, expliziter_schemaordner)`
bereit. Die APIs lesen die Konfiguration bei jedem Aufruf neu; es gibt
keinen kompilierten Modellpfad und keinen Rückfall auf das Arbeitsverzeichnis.

Ohne explizite Pfadangabe gelten in dieser Reihenfolge:

1. `SEGELPHYSIK_TEST_MODEL_ROOT` bzw. `SEGELPHYSIK_TEST_SCHEMA_ROOT`;
2. `SEGELPHYSIK_MODEL_ROOT` bzw. `SEGELPHYSIK_SCHEMA_ROOT`;
3. `modelRoot` bzw. `schemaRoot` der gemeinsamen Defaults-Datei.

Die Datei liegt unter `%APPDATA%\SegelPhysik\defaults.json`.
`SEGELPHYSIK_TEST_DEFAULTS_FILE` hat Vorrang vor
`SEGELPHYSIK_DEFAULTS_FILE` und diesem Standardort.
Relative Dateifelder beziehen sich auf das Konfigurationsverzeichnis.
Die Datei muss ein Objekt mit nichtleeren Stringfeldern `modelRoot`,
`schemaRoot` und einem Objekt `defaults` sein. Weitere Schlüssel sind
zulässig. Diese APIs schreiben keine Konfiguration.

## Build, Provider und Bereitstellung

Core deklariert die beiden Header-only-Provider in `Code/CMakeLists.txt`
mit festen Versionen und geprüften Download-Hashes:

| Aufgabe | Provider | Lizenz |
|---|---|---|
| JSON Schema Draft 2020-12 einschließlich Formatprüfung | jsoncons v1.10.0 | Boost Software License 1.0 |
| SHA-256, inkrementell in 64-KiB-Leseblöcken | PicoSHA2 v1.0.1 | MIT |

Beide werden privat in Core eingebunden; Verbraucher linken
`segelphysik_core` und benötigen weder zusätzliche Providerheader noch
Provider-DLLs. Die Lizenztexte liegen in den unveränderten
FetchContent-Quellarchiven und müssen bei einer Weitergabe dieser Quellen
erhalten bleiben; Lizenzhinweise sind auch im Auslieferungspaket zu
berücksichtigen.

Beim ersten Konfigurieren werden die Archive heruntergeladen. Für einen
Offline-Build müssen die geprüften Quellen vorher bereitliegen. Die
üblichen CMake-Overrides
`FETCHCONTENT_SOURCE_DIR_CORE_JSONCONS` und
`FETCHCONTENT_SOURCE_DIR_CORE_PICOSHA2` zeigen dann auf die entpackten
Quellverzeichnisse. Die Daten-Schemas sind weiterhin Laufzeitdateien aus
`schemaRoot`, keine eingebetteten Ersatzschemata.
