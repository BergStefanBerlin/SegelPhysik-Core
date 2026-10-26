# Hydrostatikkern: Implementierungs- und Testnachweise

Stand: 26.10.2026. Belege zum
[Umsetzungsplan](../Umsetzungsplaene/Umsetzungsplan_Hydrostatikkern.md).
Dies ist kein zusätzlicher Plan und keine zusätzliche Spezifikation.

Technischer Sicherungsstand: **C-0.1**. Die produktiven Rechenpfade,
CTest und Benchmarksteuerung benötigen kein Python. Der ursprüngliche
optionale Python-Messtreiber wurde durch `benchmark_suite.ps1` ersetzt
und mit drei Modellfällen sowie beiden Aufzeichnungsformaten geprüft.
Die historischen Python-Testnamen im Inventar sind Quellenverweise,
keine auszuführenden Core-Tests.

## Implementierte Funktionalität

| Bereich | Implementierung | Nachweis |
|---|---|---|
| Build, Bibliothek und CLI | Eigenständiger C++17-/CMake-Build, ohne Python-/GUI-Laufzeit | Standard, Testinstrumentierung und Produktion mit MSVC Release x64 gebaut |
| Import | Striktes JSON, Unicode, doppelte Schlüssel, endliche Zahlen, Dreiecks-OBJ mit negativen Indizes | `modell_import` |
| Modellfehler | Katalogisierte IDs mit Dateikontext; maschinenlesbarer Prüfaufruf | `modell_import`, `simulation_gesamtlauf`, zusätzlicher CLI-/JSON-Abgleich |
| Masse | Punkt, homogener Vollkörper, dünne Materialfläche; Schwerpunkt, Trägheit und Steiner | Analytische Punkt-, Würfel-, Tetraeder- und Dreiecksflächenreferenzen in `modell_import` |
| Volumennetze | Topologie, Orientierung je Komponente, toleranzbasierte Selbstschnittprüfung | Fehlerfixtures in `modell_import` |
| Hydrostatik | Aktueller Ebenenschnitt, Volumenmoment, Trockenschnellpfad und Vollnasspfad | Analytische Quader, diagonaler Halbschnitt und kleine Tetraeder-Restvolumina bis etwa 1,67e-16 m³ |
| Statik | Höhe/Krängung/Trimm, getrennte Residuen, Überlast-/Nichtkonvergenz-/Abbruchstatus | Symmetrische/asymmetrische Referenzen in `hydrostatik_referenzen` |
| Dynamik | Schwerpunkttranslation und Quaternionrotation ohne Zusatzdämpfung | Ruhendes Gleichgewicht, freier Fall, Hauptachsenrotation, singuläre Trägheit, Wiederholbarkeit und Schrittweitenstudie |
| Ergebnisse | Modellfassung, Pose xyzw, Vektoren, Elementvolumina, Status und Zeit | `simulation_gesamtlauf` und unabhängige JSON-Prüfung der GUI-Beispiele |
| Zeitmessungen | Immer aktive Gesamtdauer, testexklusive Detailtimer | Alle sieben Detailmesspunkte im Instrumentierungslauf; keine im Normalbetrieb |
| Geschwindigkeit | Reproduzierbarer Baseline-/Vergleichstreiber, räumlicher Sweep statt vollständigem Paardurchlauf | JSON-Belege unten |

Die CTest-Suite enthält **11 Testprogramme/-fälle** mit zahlreichen
internen Referenz- und Negativprüfungen, nicht lediglich elf einzelne
physikalische Assertions. Standard- und Instrumentierungsbuild haben
jeweils 11/11 bestanden. Die genaue Befehlsfolge steht in der
[Buildanleitung](../Code/README.md).
Zusätzlich bestanden im Standard-Debugbuild ebenfalls 11/11 Fälle.
Das [historische Testinventar](Testinventar_Hydrostatikkern.json)
ordnet 107 Module mit 939 Testfunktionen nachvollziehbar zu;
es behauptet weder bestandene Alttests noch deren unveränderte Übernahme.

## Beispieldaten und tatsächliche Ergebnisse

Alle acht einfachen Modelle der Beispielsammlung wurden statisch
über die CLI berechnet: Bleistift, Kegel, Kugel, Platte, Pyramide,
Quader, Rolle und unregelmäßiger Körper; alle meldeten `konvergiert`.
Dies bestätigt nur den implementierten Lauf, keine unabhängige
physikalische Abnahme dieser historischen bzw. angenäherten Netze.

420 wird mangels Hydrostatikmasse mit `CORE-MOD-0004` abgelehnt.
Folkeboot und Optimist werden mit `CORE-MOD-0007` abgelehnt, weil
die aktuelle konservative Prüfung keine Überschneidungsfreiheit
nachweist. Modelle wurden nicht still korrigiert oder teilgerechnet.

Die [statische Referenz](Beispiele_Core_GUI/statisch.json) und
[dynamische Referenz](Beispiele_Core_GUI/dynamisch.json) sind tatsächlich
erzeugte Quaderläufe. Die Dateien wurden unabhängig als JSON gelesen:
normierte Quaternionen, gemeinsame Zustände, eindeutige Beitrags-IDs,
endliche Vektoren und Summe der Elementvolumina wurden geprüft.
Der dynamische Beispielzeitraum beträgt 0,1 s bei nominal 0,01 s.
Die Zeitpunkte werden an der Startzeit und Ticknummer verankert,
damit sich keine zusätzlichen Schritte durch kumulierte Zeitaddition
ergeben. Eine tatsächlich nicht ganzzahlige Laufdauer erhält einen
abschließenden Restschritt.

## Performancebelege und Entscheidungen

Vorher: [Performancebaseline](Performancebaseline.json).
Nach räumlichem Sweep: [PerformanceOptimiert](PerformanceOptimiert.json).
Gleiche Modellinhalte/Fingerprints, CPU, Compiler-/Buildart und
Ausgabeaufgabe; die Benchmarks wurden nicht parallel ausgeführt.
Es wurden keine universellen Zeitbudgets oder Echtzeitgarantien erfunden.

| Großer Kugelfall, gesamter Lauf | Vorher, Median | Nachher, Median | Verhältnis |
|---|---:|---:|---:|
| Statik | 0,410 s | 0,208 s | rund 1,97× schneller |
| Dynamik, 0,2 s Simulationszeit | 0,414 s | 0,214 s | rund 1,93× schneller |

Die Optimierung betrifft die Selbstschnitt-Vorauswahl beim Import:
Sortierung entlang der längsten Modellachse und aktive AABB-Liste
reduzieren Paarprüfungen, ohne deren Prädikate abzuschwächen.
Der Worst Case bleibt bei stark überdeckenden Boxen quadratisch.
Die Genauigkeits-/Negativtests bestehen nach der Änderung weiterhin.
Kleine Laufzeiten streuen; dort wird keine allgemeine Beschleunigung
aus einzelnen Medianverschiebungen behauptet.

Die kleine Zahlenformatprobe ergab rund 0,195 % relativen Fehler
für `float`, gegenüber etwa 1,8e-12 für `double`; ein verlässlicher
Geschwindigkeitsvorteil einfacher Genauigkeit zeigte sich nicht.
Deshalb bleibt `double` die technische Arbeitswahl. Das ist kein
vollständiger Mehrformatvergleich jedes Solvers.

Beim vollständigen kleinen Aufzeichnungslauf mit 1000 Schritten
lagen die Mediane bei etwa 33,8 ms für JSONL und 34,7 ms für JSON.
Die Streuungen überlappen; JSONL ist vorläufiger Standard, JSON-Array
bleibt verfügbar. Die Mikroprobe großer Zustände kann umgekehrt ausfallen.
Die Datei-/Prozessverträge sind in der
[Schnittstellenbeschreibung](Schnittstelle_Core_GUI.md) beschrieben.

## Bekannte Grenzen und nicht erteilte Abnahmen

- Volumenüberlappung wird konservativ über AABBs geprüft. Dies ist
  absichtlich fehlersicher gegen Doppelzählung, aber keine vollständige
  geometrische Überlappungsklassifikation. Disjunkte Körper mit
  überdeckenden AABBs können abgelehnt werden.
- Das Modellformat ist der bestätigte aktuelle Entwurfsaufbau.
  Andere Schemaversionen und OBJ-Polygone werden nicht still übernommen.
- Eine beliebige geometrische Doppelwand kann nicht automatisch als
  fehlerhafte Materialfläche erkannt werden. Die Schaleninterpretation
  muss zum Modell passen; es gibt keine automatische Umdeutung.
- Die Dynamik ist Hydrostatik plus Gewicht, keine Widerstands- oder
  vollständige Strömungsrechnung. Ein Gleichgewicht ist nicht automatisch
  als stabil klassifiziert.
- Die Faktor-10-Reserve ist für konkrete Referenzfälle geprüft, nicht
  für sämtliche denkbaren Modellmaßstäbe, Trägheiten und Zeitschritte.
  Quantitative Gesamtfehlerbudgets und Performance-Abnahmegrenzen
  benötigen weiterhin eine ausdrückliche fachliche Festlegung.
- Live-WebSocket, GUI-Implementierung und Git-Veröffentlichung wurden
  nicht implementiert bzw. ausgeführt und gehören nicht zur jetzigen
  headless Umsetzung.

Die wesentlichen Rechenpfade sind implementiert und technisch geprüft.
Eine vorbehaltlose Gesamtabnahme sämtlicher Anforderungen wird wegen
dieser Grenzen und offenen fachlichen Kriterien ausdrücklich nicht behauptet.
