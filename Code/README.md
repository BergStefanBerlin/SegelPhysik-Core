# SegelPhysik-Core: nativer Build

## Temporärer Testantrieb (C-0.5)

Bibliotheksaufruf: `testantrieb_start(modell,start,konfiguration,aktiv)`,
danach `dynamikschritt(modell,zustand,dt,ereignisse)`.
`TestantriebKonfiguration` enthält `kraft_n` (200 N) und `grenze_m_s`
(exakt 463/90 m/s); `TestantriebEreignis` enthält `kennung` und `aktiv`.
Ereignisse werden vor dem Schritt eingefroren und nach erfolgreichem Schritt
im unveränderlichen `Zustand::testantrieb` bestätigt.
Der gleiche Kontext begleitet gespeicherte Zustände bis zur Serialisierung.
Testzustände liefern Ergebnisversion 3, kontextfreie Altaufrufe Version 2.
Die GUI muss Version 3 ausdrücklich unterstützen; es gibt keine neuen
CLI-Schalter und keine statische Schubregelung.

Horizontaler Bugschub verwendet nur physische Importgeometrie und den echten
Hebelarm. Der aktive XY-Endgeschwindigkeitsbegrenzer (auch bei 0 N) weist
Impuls und Energieänderung getrennt aus. Konfiguration-/Kontextfehler,
fehlende positive Schubgeometrie und singuläre Richtung:
`CORE-SIM-0001`. Keine Widerstandsphysik oder Kursstabilisierung.
Vollständiger API-/Feldvertrag:
[CORE → GUI](../Dokumentationen/Schnittstelle_Core_GUI.md#c-05-temporärer-testantrieb-ergebnisversion-3).

Gezielter Nachweis im Release-Build:
`ctest --test-dir build\c05 --output-on-failure -R "temporaerer_testantrieb|hydrostatik_referenzen|kraft_momentenbilanz|modell_import"`.
`temporaerer_testantrieb` erzeugt im Buildordner die reproduzierbare
`testantrieb-420-referenz.jsonl` einschließlich Ein/Aus-Ereignissen und
Begrenzung. Technisch getestet, Nutzerabnahme weiterhin offen.

Eigenständige C++17-Bibliothek und CLI ohne Python, pybind11 oder GUI.
Implementiert sind Modellimport/-prüfung, Massenintegration,
Wasserebenenschnitt, gemeinsame Hydrostatik, statisches Gleichgewicht,
Starrkörperdynamik, Aufzeichnung und abfragbarer JSON-Fehlerkatalog.
Es gelten die ausdrücklich dokumentierten
[numerischen und geometrischen Grenzen](../Dokumentationen/Umsetzungsnachweise_Hydrostatikkern.md).

## Lizenzstatus

Die gewünschte Lizenzrichtung für den eigenen CORE-Code ist nicht-kommerziell:
Bildungs-/Vereinsausnahme und kostenlose Weitergabe einschließlich Ableitungen
mit Quellcode und gleichen Bedingungen. Die
[Lizenzdokumentation](../Dokumentationen/Lizenzdokumentation.md) beschreibt die
getrennte CORE-Prüfung. Die
[vorgeschlagenen Bedingungen](../Dokumentationen/Lizenzbedingungen_Entwurf.md)
sind ein **nicht in Kraft gesetzter Entwurf**, keine Nutzungserlaubnis und
keine freie Open-Source-Lizenz. Rechte fremder Komponenten bleiben unverändert;
eine Verteilungsfreigabe liegt nicht vor.

## Bauen und prüfen

Mit CMake ab 3.20 und einem C++17-Compiler, vom Repository aus:

```powershell
cmake -S Code -B build\standard -DBUILD_TESTING=ON
cmake --build build\standard --config Release
ctest --test-dir build\standard -C Release --output-on-failure
.\build\standard\Release\segelphysik_cli.exe --build-info
```

Die Beispielpfade zur EXE gelten für Visual Studio als
Mehrkonfigurationsgenerator. Bei Einkonfigurationsgeneratoren wird
`-DCMAKE_BUILD_TYPE=Release` beim Konfigurieren benötigt und die EXE
liegt direkt im Buildverzeichnis.

Für den Build ohne Testprogramme `-DBUILD_TESTING=OFF` verwenden.
Die Option `-DSEGELPHYSIK_TEST_INSTRUMENTATION=ON` bereitet einen
gesonderten Testbuild vor und erfordert `BUILD_TESTING=ON`.
Detailtimer benötigen zusätzlich `testmessungen_aktivieren(true)`.
Die CLI aktiviert sie nicht; die Gesamtlaufzeitmessung ist immer aktiv.
Die [Messliste](../Dokumentationen/Zeitmessungen.md) beschreibt Grenzen,
Verschachtelungen und zugehörige Tests.

## CLI und Bibliothek

Die CLI unterstützt `--help`, `--build-info`, `--fehlerdoku`,
`--pruefen MODELL`, `--statisch MODELL AUSGABE` und
`--dynamisch MODELL AUSGABE DAUER_S DT_S`.
`--fehlerdoku` gibt den eingebetteten
[Fehlerkatalog](ressourcen/fehlerkatalog.json) als UTF-8-JSON aus,
ohne ein Modell zu laden. Der Katalog beschreibt die tatsächlich
implementierten Prüfungen innerhalb ihrer dokumentierten Grenzen.
Der [Ergebnis-/Fehlervertrag](../Dokumentationen/Schnittstelle_Core_GUI.md)
beschreibt die fachliche Anfrage „gib mir Fehlerdoku“ und ihren aktuellen
CLI-Abfrageweg; eine Live-Verbindung ist noch nicht implementiert.
Fehlende, zusätzliche oder unbekannte Argumente führen zu Exitcode 2
und einer Fehlermeldung auf stderr. Tests prüfen auch diese Fehlerpfade.

Beispiele, vom Repository aus (Ausgabedateien dürfen noch nicht existieren):

```powershell
$cli = ".\build\standard\Release\segelphysik_cli.exe"
$modell = "..\Modelle\quader\modell.json"
& $cli --pruefen $modell
& $cli --statisch $modell ".\build\statik.json"
& $cli --dynamisch $modell ".\build\dynamik.jsonl" 1 0.01
```

Die Beispielmodelle liegen außerhalb des Repositories im gemeinsamen
Ordner `Segelphysik\Modelle` neben `CORE` und `GUI`. CMake-Variable
`SEGELPHYSIK_MODELLE_DIR` (Standard `../../Modelle` relativ zu `Code`)
legt den Ordner für Tests und GUI fest.

`.jsonl` liefert vollständige Zustände pro Zeile, andere Endungen ein
inkrementell geschriebenes JSON-Array. Laufstatus und Gesamtdauer
erscheinen auf stdout, Diagnosen auf stderr. Ein Modellprüfaufruf
liefert dagegen ein strukturiertes JSON-Prüfergebnis.
Ctrl+C fordert den Abbruch laufender Simulationen an.
Erfolg: Exitcode 0; abgelehnte Modelle, Solverfehler oder Abbruch:
Exitcode 1; falsche CLI-Verwendung: Exitcode 2.

Die Bibliotheks-API steht in [modell.hpp](include/segelphysik/modell.hpp),
[hydrostatik.hpp](include/segelphysik/hydrostatik.hpp) und
[simulation.hpp](include/segelphysik/simulation.hpp).
Vorbereitete Modelle können für wiederholte Auswertungen und Schritte
wiederverwendet werden, ohne Dateien pro Schritt neu einzulesen.

## Reproduzierbare Benchmarks

`core_benchmark MODELL` misst ohne GUI und ohne aktive Detailtimer.
Der optionale PowerShell-Treiber benötigt keine Python-Installation:

```powershell
.\Code\tests\benchmark_suite.ps1 build\standard\Release build\performance.json
```

Er erzeugt kleine, zusammengesetzte und große Referenzläufe, prüft deren
JSON-Ausgaben und berichtet Aufwärmung, Wiederholungen, Median und
Streuung. Build, Tests, Benchmarks und produktive Berechnungen
verwenden kein Python.

Der [Umsetzungsplan](../Umsetzungsplaene/Umsetzungsplan_Hydrostatikkern.md)
enthält Umfang, Abhängigkeiten und tatsächlichen Fortschritt.
