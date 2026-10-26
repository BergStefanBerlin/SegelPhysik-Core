# Hydrostatikkern: Bestandsaufnahme und erste Buildnachweise

## Status

Dies ist die historische Erstaufnahme vor der Physikimplementierung.
Die folgenden damaligen Buildgrenzen beschreiben nicht den aktuellen
Core. Den aktuellen Implementierungs-, Test- und Messstand enthält der
[fortgeschriebene Nachweis](Umsetzungsnachweise_Hydrostatikkern.md).

Stand: 26.10.2026. Unterstützender Nachweis zu
[U-C-000001 und U-C-000004](../Umsetzungsplaene/Umsetzungsplan_Hydrostatikkern.md),
keine zusätzliche Spezifikation oder Planung.
Die Bestandsaufnahme ist noch nicht vollständig abgeschlossen.
Es wurde kein Altcode in den produktiven Core übernommen.

Die Quellverweise beziehen sich auf das benachbarte Projekt
`SegelPhysik3`, nicht auf eine Laufzeitabhängigkeit.

## Fortschreibung: vollständiges Testinventar und neue Zuordnung

Das [maschinenlesbare Testinventar](Testinventar_Hydrostatikkern.json)
erfasst inzwischen 103 historische `test*.py`-Module sowie vier native
Vertrags-/Paritätstreiber, zusammen 939 Testfunktionen.
Parametrisierte Ausführungen werden dabei nicht als zusätzliche
Funktionen gezählt. Die separate native `conftest.py` enthält nur
eine Referenzfixture.

Je Modul sind Quellenfingerprint, Testnamen, Zeilen, vorhandene
Zweckdokumentation, Übernahme-/Nichtübernahmeentscheidung, neue
Prüfprogramme und Anforderungsbezüge erfasst. Bei gemischten Dateien
bezieht sich die Zuordnung nur auf die ausdrücklich genannten
gemeinsamen Physikthemen, nicht auf sämtliche historischen Teilverträge.
Das ist eine vertragsbezogene Neubewertung, keine Behauptung
zeilenweiser semantischer Gleichwertigkeit oder bestandener Alttests.

| Historischer Themenbereich | Aktuelle Entscheidung / Nachweis |
|---|---|
| Massengeometrie, Punktmassen, Schalen und Steiner | Unabhängige analytische Referenzen in `modell_import` |
| Ebenenschnitt, Auftrieb und Pose | Neu definierte Volumen-/Schwerpunktreferenzen in `hydrostatik_referenzen` und `pose_transformations`; keine Wasserliniengrafik |
| Statische Solver | Drei Unbekannte und getrennte Residuen neu definiert; keine neutralen Erfolgsfallbacks oder automatische Stabilitätsauswahl |
| Starrkörperdynamik | Gleichgewicht, freier Fall, Rotation, Wiederholbarkeit und Schrittweitenstudie; keine Dämpfungs-/Einschwingverträge |
| Überlappung und Voxel-/Unionpfade | Fehlerablehnung statt Boolean-Union oder angenäherter Voxelphysik; Grenzen ausdrücklich dokumentiert |
| Modellimport und Paketmetadaten | Neues bestätigtes JSON-/Dreiecks-OBJ-Format statt altem Manifest-/Material-/Formkatalog; alter Pythonlader bewusst ersetzt |
| Native Bindings und Python-Parität | Eigenständiger C++-Build und neuer Ergebnisvertrag; kein Python-ABI-/Arraybesitz-Kompatibilitätsversprechen |
| Messung und Geschwindigkeit | Testgating und Gesamttimer separat geprüft; neue reproduzierbare native Benchmarks |
| GUI, Rendering, Worker, Replay, Gelenke, Migration, Hydrodynamik | Bewusst nicht in dieser headless Hydrostatikstufe übernommen |

Die folgenden Abschnitte bewahren die damalige Lesetiefe der
Erstaufnahme. Sie sind kein aktueller Fehlbestand des Testinventars.
Allgemeine Fehlerbudgets und fachliche Abnahme bleiben davon getrennt.

## Gelesene Rechenbausteine und Übernahmebewertung

| Quelle relativ zu SegelPhysik3 | Befund | Konsequenz |
|---|---|---|
| `code/cpp/CMakeLists.txt` | C++17, pybind11-Modul; Python inklusive Development.Module zwingend | Build nicht übernehmen; eigenständige Bibliothek und CLI neu aufsetzen |
| `code/cpp/src/ebenenschnitt.hpp` | Volumenmomente aus geschnittenen Dreiecken, paarweise Volumensumme, wiederverwendbarer Arbeitsbereich; feste Nullvolumenschwelle 1e-15; Ebenenapex setzt normierte Normale voraus | Algorithmischer Übernahmekandidat; neue toleranz- und maßstabsbezogene Grenzfalltests nötig; keine ungeprüfte Konstantenübernahme |
| `code/cpp/src/hydrostatik.hpp` | Transformation der Wasserfläche in Modellbezug, Bounding-Box-Schnellpfade, Weltkraft und Moment um transformiertes r_s | Geeignete Grundidee; neue Elementzuordnung, optionale Nullvolumen-Angriffspunkte und Modellvektoren erforderlich |
| `code/cpp/src/massen.hpp` | Flächen-/Volumenintegration und Trägheiten; Flächengewicht bzw. Dichteparameter muss im Aufrufer korrekt interpretiert werden | Formeln gegen unabhängige Referenzen prüfen; Schalenflächen und Dichte mal Dicke ausdrücklich festlegen |
| `code/cpp/src/statik.hpp` | Zwei Unbekannte z/phi; Kraft und Moment teilweise in derselben Norm und Toleranz; hydrodynamische und Anzeigeabhängigkeiten; robuste Fallbacks mit eigenen Erfolgskriterien | Nicht unverändert übernehmen: drei Unbekannte z/Krängung/Trimm, getrennte Residuen und keine erfolgsförmigen Stagnations-Fallbacks |
| `code/cpp/src/dynamik.hpp` | Quaternionintegration und lineares Lösen; Körpergeschwindigkeiten, gyroskopische Terme, zusätzliche Widerstands-/Added-Mass-/Fluidmodelle | Starrkörperideen prüfen, fremde Physik entfernen; Integrator und Fehlerbudget vor Implementierung abstimmen |
| `code/cpp/src/mesh_import.hpp` | Polygonprüfung, Ear-Clipping und OBJ-Punkttransformation; kein vollständiger eigenständiger JSON-/OBJ-Dateilader | Geometriehilfen als Kandidat; Dateiimport und neues Modellformat separat implementieren |
| `code/cpp/src/mesh_pruefung.hpp` | Dreiecksprüfungen mit räumlicher Vorauswahl; Selbstschnittprüfung überspringt Dreiecke mit gemeinsamem Vertexindex | Keine pauschale Aussage vollständiger Selbstschnittfreiheit daraus ableiten |
| `code/cpp/src/frames.hpp`, erste 145 Zeilen | Quaternionreihenfolge wxyz, Matrizen und Orientierungsschritt | Historische Konvention ist kein neuer GUI-Vertrag; öffentliche Reihenfolge vor Verwendung festlegen |
| `code/cpp/src/datentypen.hpp`, erste 180 Zeilen | Array-basierte double-Daten, Validierung, Massen-/Trägheitsprüfungen und bestehende Widerstandsparameter | Kein ungeprüftes Übernehmen des alten Zustandsschemas oder Zahlenformats |
| `code/python/segelphysik/core/modell_lader.py`, Symbol-/Importübersicht | Python-/NumPy-Importpfad mit Formen, Manifestbaum, Gelenken und individuellen OBJ-Transformationsangaben | Neues Importformat benötigt eigene Implementierung; vollständige Detailprüfung des alten Laders noch offen |

Die genannten Grenzen und Änderungen sind Bewertungen für die
Anforderungsumsetzung, keine Reparaturaufträge für das Altprojekt.

## Testsichtung

Vollständig inhaltlich gelesen:

| Quelle | Wiederverwendbarer fachlicher Bezug | Grenze |
|---|---|---|
| `tests/test_hydrostatik.py` | Analytische Ponton-Wasserlinien, Trimm-/Krängungstransformationen, tangentialer Kontakt | Wasserlinien-Flächenmomente sind nicht selbst der geforderte Volumenschnitt; alte automatische Orientierungsnormalisierung nicht ungeprüft übernehmen |
| `tests/test_massen_schale.py` | Offener Würfel aus fünf Platten, analytische Steiner-Referenz, Verschiebungsinvarianz kleiner Schalen | Neue Schalenflächenkonvention weiterhin abzustimmen |

Strukturell gesichtet wurden außerdem folgende Dateien anhand ihrer
Testfunktionsnamen und ausgewählter Vertragsabschnitte. Die Anzahl zählt
Testfunktionen, nicht parametrisierte Testfälle oder bestandene Tests:

| Datei unter `tests` | Funktionen | Zuordnung / Prüfbedarf |
|---|---:|---|
| `test_auftrieb.py` | 22 | Trockene/volle/teilweise Verdrängung, Momente und vorbereitete Schnitte |
| `test_massen_geometrie_referenz.py` | 2 | Analytische Geometrie und Flächen; Detailprüfung offen |
| `test_native_massen.py` | 6 | Massenintegration und Selbstschnittcache; Parität allein nicht ausreichend |
| `test_native_mesh_import.py` | 4 | Polygontriangulierung, Dateikontext und Importcache |
| `test_mesh_validierung.py` | 10 | Eingabefehler, Schalen, Orientierung, Trägheitsprüfung |
| `test_native_statik.py` | 7 | Solverstatus und Abbruch; alte hydrodynamische Fälle getrennt behandeln |
| `test_statisch_robust.py` | 13 | Schlechte Startlagen, Budget, Stagnation und Cache; Erfolgskriterien neu definieren |
| `test_statik_stabilitaet.py` | 3 | Gleichgewichtsstabilität; zusätzliche Dreifreiheitsgradfälle nötig |
| `test_dynamik_6dof.py` | 16 | Freier Fall, Rotation und Frameinvarianz; Dämpfungstests nicht Teil erster Physik |
| `test_hydrostatik_dynamik_integration.py` | 2 | Annäherung an statische Lage; kein zulässiger ungedämpfter Einschwingvertrag |
| `test_volumen_union.py` | 13 | Überschneidungen und vorbereitete Union; Strategie noch offen |
| `test_native_geometrie.py` | 3 | Schnittreferenz und Arrayvertrag; neue C++-API statt NumPy-Vertrag |
| `test_genauigkeit_v09.py` | 7 | Getrennte Genauigkeitsgrößen als Vorbild; alte Zahlenwerte nicht übernommen |

`code/cpp/tests/paritaet_frames.py` wurde bis einschließlich der
Posekompositions- und Fehlerprüfungen im ersten Dateiteil gelesen.
Quaternionvorzeichen, Punkt-/Vektortransformation und ungültige Eingaben
sind gute Testideen; Paritätstoleranz 1e-12 wird nicht als neues globales
Fehlerbudget übernommen.

Die Dateiliste enthält außerdem GUI-, Render-, Hydrodynamik-,
Migrations-, Worker- und Replaytests. Diese sind noch nicht vollständig
klassifiziert. Auch die übrigen nativen Paritäts-/Vertragstests und der
vollständige Modellimport sind noch zu prüfen. U-C-000001 ist deshalb
**in Arbeit**, nicht abgeschlossen. Kein Legacy-Test wurde hier ausgeführt.

## Tatsächlich implementiertes Build-Grundgerüst

Die [Builddatei](../Code/CMakeLists.txt) erzeugt eine eigenständige
C++17-Bibliothek, eine CLI und optionale CTest-Tests. Python, pybind11
und GUI-Pakete werden nicht gesucht oder benötigt.
Alle eigenen Targets werden mit hohen Warnstufen und Warnungen als
Fehler gebaut; MSVC verwendet präzise Gleitkommaoptionen.

Die CLI stellt Hilfe, Buildstand und den eingebetteten Fehlerkatalog
bereit. Sie lehnt Simulationsbefehle ausdrücklich ab, statt fingierte
Ergebnisse zu liefern. Eine Testinstrumentierungs-Buildoption existiert;
es gibt noch keine Timer, keine Detailmessungen und kein Messregister.

Dieser vorgezogene unabhängige Buildtest legt weder Modellvertrag,
Solververfahren, Quaternionkonvention noch Genauigkeit fest. Die
vollständige Abarbeitung und Abnahme von U-C-000004 bleibt an den
im Plan genannten Bestandsnachweis gebunden.

### Build- und Testergebnisse vom 26.10.2026

Umgebung: Windows x64, Visual Studio 2022 Build Tools,
MSVC 19.44.35229.0, CMake 3.31.6-msvc6, Windows SDK 10.0.26100.0.
Generator: Visual Studio 17 2022, Architektur x64, Konfiguration Release.

| Konfiguration | Ausführung | Tatsächliches Ergebnis |
|---|---|---|
| Standard mit Tests | Configure, Build, CTest | Build erfolgreich; aktuell 8/8 Tests bestanden |
| Testinstrumentierung vorbereitet | Configure mit beiden Testoptionen ON, Build, CTest | Build erfolgreich; aktuell 8/8 Tests bestanden |
| Ohne Testtargets | Configure mit BUILD_TESTING=OFF, Build, CLI `--build-info` | Build und CLI erfolgreich; Testinstrumentierung nicht vorbereitet |
| Ungültige Optionskombination | BUILD_TESTING=OFF bei SEGELPHYSIK_TEST_INSTRUMENTATION=ON | Konfiguration wie erwartet ausdrücklich abgelehnt |

Die acht Tests prüfen Bibliothekskonfiguration, CLI-Hilfe,
Buildinformation, Fehlerkatalog, fehlende/unbekannte/zusätzliche Argumente
und Pose-Transformationen. Der Produktionsabruf des Katalogs mit
13 Einträgen wurde zusätzlich aus einem anderen Arbeitsverzeichnis
gegen die UTF-8-Quelldatei geprüft.
Die Poseprüfungen umfassen Normierung großer/kleiner Quaternionen,
Punkt-/Vektorunterscheidung, Inverse und ausdrückliche Fehlerdiagnosen.
Die erste Katalogvergleichsprüfung scheiterte an Windows-Textverarbeitung
im CMake-Testtreiber; explizite UTF-8-Decodierung und Normalisierung der
Zeilenenden beseitigten den Testtreiberfehler. Die tatsächliche
CLI-Antwort wurde zusätzlich unabhängig mit Python als UTF-8-JSON geprüft.
Dies sind **keine Physik-, Genauigkeits- oder Performanceabnahmen**.

Reproduzierbare Kommandos stehen in der
[Buildanleitung](../Code/README.md).
Auf diesem Rechner liegt CMake nicht im normalen PATH, sondern unter
Visual Studio Build Tools im Unterverzeichnis
`Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`.
Die Kommandos wurden mit diesem expliziten CMake-/CTest-Pfad ausgeführt.
