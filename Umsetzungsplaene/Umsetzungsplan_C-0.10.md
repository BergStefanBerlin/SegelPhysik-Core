# Core-Plan C-0.10: ausschliesslich Performance der Aufbereitung

## Nutzerabnahme 04.11.2026

**Nutzerabnahmen: done.** Ausdruecklicher Nutzerauftrag vom 04.11.2026,
09:16 Uhr fuer den gelieferten Stand einschliesslich Performance.
Dieser datierte Abschluss ersetzt alle aelteren offenen
Nutzerabnahmestaende in diesem Plan; historische Nachweise bleiben erhalten.
Technische Testluecken und nicht finalisierte Zusatzumfaenge werden dadurch
nicht als nachgewiesen ausgegeben. Kein Commit, Tag oder Push beauftragt.

## Revisionsvorbereitung 04.11.2026: vollstaendige Regression

Auf ausdruecklichen Nutzerauftrag wurden die vorhandenen vollstaendigen
nativen Testsuiten nach dem Neubau ausgefuehrt, ohne Testausschluesse:

| Konfiguration | Ergebnis | Laufzeit | Nachweis |
|---|---|---|---|
| Release, `build\standard`, Instrumentierung AUS | **22/22 bestanden**, 0 Fehler, 0 uebersprungen | 96,26 s | [CTest](Nachweise/Meilenstein_2026-11-04/core-tests.log), [Detailprotokoll](Nachweise/Meilenstein_2026-11-04/core-tests-detail.log), [JUnit](Nachweise/Meilenstein_2026-11-04/core-tests.xml) |
| Release, `build\c010`, Instrumentierung EIN | **22/22 bestanden**, 0 Fehler, 0 uebersprungen | 119,35 s | [CTest](Nachweise/Meilenstein_2026-11-04/core-instrumented-tests.log), [Detailprotokoll](Nachweise/Meilenstein_2026-11-04/core-instrumented-tests-detail.log), [JUnit](Nachweise/Meilenstein_2026-11-04/core-instrumented-tests.xml) |

Buildprotokolle: [Standard](Nachweise/Meilenstein_2026-11-04/core-build.log),
[instrumentiert](Nachweise/Meilenstein_2026-11-04/core-instrumented-build.log).
Reproduzierbarer Aufruf je Buildordner:
`cmake --build <Buildordner> --config Release --parallel 3`,
danach `ctest --test-dir <Buildordner> -C Release --output-on-failure`.
Beide Builds haben `BUILD_TESTING=ON`.

Die beiden zuvor bekannten Altfehler sind damit geschlossen:
`modellkatalog_schnittstelle` verwendete alte Dateipfade und
Darstellungs-/Fehlerfixtures. Diese sind auf die reale Einzelquelle
umgestellt; der Fehlfall mit fehlender Massendatei bleibt als explizites
Legacy-Modell mit weiterhin lesbarer Darstellung erhalten.
`modell_geometrie_v2` deckte einen echten Konverterfehler auf:
Schema-3-Ableitungen wurden unveraendert in Schema 2 kopiert.
CPPI-0065 materialisiert sie jetzt exakt als eigene Quellnetze.
Die Tests pruefen unveraenderte Originale, exakte Modellphysik und
Darstellung sowie Schema-2-zu-3-Migration mit/ohne Ableitungsfreigabe.
Dies ist eine Bestandsfehlerkorrektur, keine Erweiterung des
Performanceumfangs oder der Physikvertraege.

Gepruefte Git-Basis: `a8c1ddcdab75366f6eed16b91e672fd4168273ef`
auf `main`, mit den uncommittierten Aenderungen vom 04.11.2026.
Die aelteren Fehlerprotokolle weiter unten bleiben historische Nachweise,
nicht der aktuelle Teststatus. Gruene vorhandene Tests ersetzen keine
externen physikalischen Referenzmessungen oder die weiterhin nicht
finalisierte erweiterte Physikmatrix. Windphysik ist nicht Teil dieses
Revisionspunkts. Keine Git-Veroeffentlichung ausgefuehrt.

**Stand:** 03.11.2026, Nutzerfreigabe 20:45 Uhr.
**Status:** Finalisiert, Core technisch umgesetzt und nachgewiesen.
GUI-Integration G-0.11 technisch nachgewiesen (Abschnitt 9.4);
fachliche Nutzerabnahme bleibt ein getrennter Schritt.
**Zielversion:** C-0.10; keine Versionsvergabe
durch Git. Einziger Plan fuer diesen vorgeschlagenen Versionsschritt.
Die bisherigen C-0.9-Arbeiten und ihre offenen Nachweise bleiben im
[bisherigen Plan](Umsetzungsplan_C-0.9.md); keine Uebernahme als
zusaetzliche Arbeitspakete dieses Performanceplans.

Massgeblich: [Performance-Spezifikation](../Spezifikationen/Spezifikation_Performance_Modellaufbereitung.md).
Entwurfskennungen stehen ausschliesslich in der
[Liste](../Spezifikationen/Entwurfspunkte/Performance_Modellaufbereitung.json).
Nummern vor Vergabe projektweit geprueft: bisher A-C-00128,
U-C-000114 und CPPI-0112. Neue Nummern werden nicht wiederverwendet.

## 1. Grenzen und Eintritt

Nur Modellbasis, stellungsabhaengige Aufbereitung, exakte Meshvorarbeiten
und passende Kraft-/Ergebniswiederverwendung. Kein statischer
Solverumbau, keine neue Physik, kein groesseres dt, keine vereinfachten
Meshes und kein Weglassen von Ausgaben.
Gemeinsame Helpers duerfen statische Pfade beschleunigen, aber deren
Suchverfahren und Abnahmekriterien bleiben unveraendert.

Vor Umsetzung: ausdruecklich finalisierte Spezifikation, Versionsschritt
bestaetigt, neue A-/U-Kennungen mit Rueckverfolgbarkeit vergeben;
konkrete neue exportierte APIs gegebenenfalls zuvor CPPI-registrieren.
Bestehende Dirty-Dokumentation bewahren. Kein Git-Auftrag.

## 2. Geordnete Arbeitsabschnitte

Aktuelle Umsetzungsstaende und tatsaechliche Belege stehen in Abschnitt 5
und dem abschliessenden Nachweisbericht. Alte Messungen gelten nicht
als Umsetzungserfolg dieses Plans.

| Arbeitsabschnitt | Entwurfsbezug | Abhaengigkeit | Ergebnis / Nachweis |
|---|---|---|---|
| U-C-000115: Referenz und Zaehler | A-C-00134 | Finalisierungsgate | Unveraenderte Baseline, numerische Referenzen, getrennte Zaehler fuer Modellbasis/Stellung/Mesh/Kraefte/JSON; 420/Folkeboot und identische Testfaelle dokumentiert |
| U-C-000116: Unveraenderliche Modellbasis | A-C-00129, A-C-00132 | U-C-000115 | Gemeinsame exakte Modell-/Meshdaten, klare Lauflebensdauer, keine wiederholte tiefe Basiskopie; Fehler-/Paralleltests |
| U-C-000117: Stellungsabhaengige physische Daten | A-C-00130, A-C-00131 | U-C-000116 | Eine passende Vorbereitung fuer Integrator und Kraefte; gezielte Invalidierung samt Kindern/Aggregaten; identischer Stellwert ohne Neuaufbereitung |
| U-C-000118: Exakte zustandsabhaengige Vorarbeiten | A-C-00131, A-C-00132 | U-C-000117 | Nur identische Vorarbeiten teilen; Pose-/Fluid-/Integratorstufen-Grenzen korrekt; notwendige Nass-/Flutungsauswertung erhalten |
| U-C-000119: Kraft-/Ergebniswiederverwendung | A-C-00133, A-C-00131 | U-C-000118 | Auswertung desselben vollstaendigen Zustands fuer JSON/Bilanz nutzen; Batch/Live/Rueckblick und Fehlpfade testen |
| U-C-000120: Physik- und Performancenachweis | A-C-00134; A-C-00129 bis A-C-00133 | U-C-000115 bis U-C-000119 | Referenzvergleich, Release-Build, betroffene bestehende Tests, Zaehlerbericht und Rohmessmatrix; Grenzen/Restregressionen sichtbar |

GUI-Abschaltung und Profilabbruch sind unabhaengig von Core-Arbeiten
implementierbar. GUI-Integration neuer konkreter Core-Vertraege erst
nach deren Stabilisierung; kein neuer Vertrag ist schon vorausgesetzt.

## 3. Meilensteine

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| Referenz fixiert | Freigabegate passiert | Reproduzierbare Eingaben, Ausgangszeiten und numerische Referenzen gespeichert |
| Wiederholungsarbeit entfernt | Modellbasis und Stellungsdaten umgesetzt | Zaehler zeigen keine tiefe Wiederaufbereitung unveraenderter Modell-/Stellungsfassungen; Ereignis-/Invalidierungstests bestanden |
| Ergebnisweg korrekt | Zustands- und Ergebnisabschnitte umgesetzt | Endzustand nicht mit Mittelpunkt verwechselt; Live und Batch sowie Rueckblick numerisch kompatibel |
| Performance belastbar | Alle vorigen Meilensteine | 0,5-s-/5-s-Matrix, dt 0,01 s, Warmup+5 Messungen; verbesserte Zielpfade und keine reproduzierbare Regression ohne Entscheidung |

Timing ohne teure Testinstrumentierung, Zaehler separat.
Laden, Schritte, JSON und Gesamtlauf nicht vermischen.
Schema3-Vorher/Nachher am aktuellen Ausgangsstand ist die Abnahmebasis;
C-0.7-Archivmodelle sind nur historische Einordnung.

## 4. Abschluss und Halt

Jeden Entwurfspunkt nach Finalisierung A-/U-Schritten und tatsaechlichen
Tests/Belegen zuordnen. Bestehende zwei Core-Test-/Modellkompatibilitaets-
fehler nicht verschweigen oder fachfremd reparieren.
Nicht belegte Einsparungen und offene Physikmatrix sichtbar lassen.
Am Ende Bericht mit vorher/nachher, Aufrufzahlen, Speicherlebensdauer,
Regressionen und offenen Punkten; dann Nutzerpruefung.
Keine automatische Veroeffentlichung oder Uebernahme weiterer
Optimierungsfelder.

## 5. Tatsaechlicher Implementierungsstand

- **U-C-000115:** Referenz ist die bereits isoliert gebaute, unveraenderte
  C-0.9-Releasebibliothek aus `session-state/.../files/core-perf/build-c09`.
  Sie wird nicht neu gebaut oder ueberschrieben. Derselbe neue Runner
  wird einmal gegen diese Bibliothek und einmal gegen C-0.10 gelinkt;
  ABI der bestehenden Modell-/Zustandstypen bleibt unveraendert.
  Aktuelle Schema3-Eingaben und deren Modellfassungen stehen in den
  Rohdaten. Ein Pilot fuer beide Modelle, hydro_drive, 0,5 s ergab
  bitidentische vollstaendige JSON-Zustandsfolgen. Der Pilot ist keine
  Freigabe der Gesamtmatrix.
- **U-C-000116:** `Modellaufbereitung` besitzt eine eigene konstante
  Modellmomentaufnahme. Keine Dateinamen-/ID-Caches und kein globaler
  Cachebesitz. Der threadlokale Zeiger routet ausschliesslich einen
  begrenzten Aufrufstapel; Eigentum bleibt beim Laufkontext. Ein neuer
  Kontext ist bei Modell-/Geometrie-/Ableitungswechsel zwingend.
  Urspruengliche mutable Quelldaten, andere Laeufe und parallele
  Kontexte beeinflussen die Momentaufnahme nicht.
- **U-C-000117:** Eine physische Stellungsfassung je Kontext. Unveraenderte
  effektive Stellungen teilen sie zwischen Integrator, Kraeften und
  Ausgabe. Bei Wechsel werden nur betroffene Elemente aus der Basis
  erneuert, anschliessend globale Masse/CG/Traegheit in unveraenderter
  Reihenfolge aggregiert. Mitbewegte Elemente werden mit erneuert.
  Verschachtelte eigene Bewegungsdefinitionen mitbewegter Elemente sind
  wie bisher importer-seitig unzulaessig; keine neue Hierarchiephysik.
  Ereignisquittierung/Historie und die bisherige ereignisabhaengige
  Aktualisierung hydrodynamischer Zusatztraegheit bleiben erhalten.
- **U-C-000118:** Keine hydrodynamischen Element-/Meshkopien je Auswertung:
  Kraftmetadaten bleiben am Originalelement, Panels verwenden das
  bereits vorbereitete physische Volumenmesh. Originalprofil-Bounds
  werden einmal gehalten. Weltpunkte werden je Mesh nur fuer exakt
  passende Pose wiederverwendet; Stellungswechsel invalidiert die
  betroffenen Meshes. Es bleibt maximal ein Weltpunktestand je eigenem
  Mesh. Wasserlinienschnitt, Benetzung, Flutung und lokale
  Rotationsanstroemung werden nicht poseuebergreifend eingefroren.
- **U-C-000119:** Vollstaendige letzte Kraftauswertung mit exaktem,
  nicht verlustbehaftetem Schluessel fuer Pose, Zeit/Schritt,
  Geschwindigkeiten, Rotation, Fluidparameter, Flutung, Stellungen,
  Kraftkontexte und wirksame Zusatztraegheitswerte. Ausgabe erhaelt eine
  eigene Kraefte-Kopie. Der bisherige hydrodynamische Pruefpunkt vor
  Geschwindigkeitsbegrenzung bleibt fuer identisches Fehlerverhalten
  erhalten. Ausgabe teilt nur eine zur endgueltigen Geschwindigkeit
  und den wirksamen Kontexten passende Auswertung; eine Begrenzung
  invalidiert diese, niemals werden Mittelpunktkraefte ausgegeben. Batch nutzt den
  Kontext intern; Livevertrag CPPI-0113..0118 und Anleitung in
  `Code/README.md` sowie `Dokumentationen/Schnittstelle_Core_GUI.md`.
  Rueckblicke bereiten die eigenen historischen Stellungen auf.
- **U-C-000120:** Vollstaendige Release-Matrix, Endstands-Kontrollen und
  abschliessende Testergebnisse stehen in Abschnitten 6, 8 und 9.
  Erster Timinglauf nach anfaenglicher Slotfreigabe wurde auf Anweisung
  des koordinierenden GUI-Agents wegen dessen anschliessendem
  Baselinebuild gestoppt. Rohdaten unter
  `Nachweise/Performance_C-0.10_2026-11-03_Interrupted/` sind ausdruecklich
  **nicht** Bestandteil der Performanceabnahme; neue serielle Messung
  erst nach erneuter Slotfreigabe. Kein Ergebnis daraus wird ersetzt
  oder als vollstaendige Matrix ausgegeben.

## 6. Funktionale Nachweise und Zaehler

Eigener Build `build/c010`, Release, Testinstrumentierung **AN**.
Vollstaendige CTest-Suite: **20 von 22 bestanden**; Nachweis
[`core-tests.txt`](Nachweise/Performance_C-0.10_2026-11-03/core-tests.txt).
Die beiden bereits im C-0.9-Plan beschriebenen Altfehler bleiben sichtbar:
`modell_geometrie_v2` lehnt `ableitung` im Schema2-Fixture ab;
`modellkatalog_schnittstelle` meldet abweichende Darstellungsgeometrie.
Import-/Katalogimplementierung wurden nicht geaendert. Keine Behauptung
einer insgesamt gruenen Suite oder einer fachlichen Modellfreigabe.

`modellaufbereitung` besteht fuer 420 und Folkeboot mit
free/drive/hydro/hydro_drive und Fremdkraft aktiv/inaktiv, auch kombiniert
mit Hydrodynamik. Geprueft werden komplette JSON-Paritaet gegen die
ungepufferten APIs, Ereignisse, identische Stellwerte samt Quittierung,
selektive Kindererneuerung, historische Auswertung nach spaeteren
Ereignissen, Pose, Geschwindigkeit, Rotation, Flutung, Zeit, rho/g,
Fluid-/Testantriebs-/Fremdkraftkonfiguration, fehlende/falsche
Pflichtkontexte, Fehlererholung, parallele Kontexte sowie Austausch
tatsaechlichen Modellinhalts bei unveraendertem Fassungsstring.
Der letzte gezielte Aufbereitungslauf bestand nach Erweiterung aller
Invalidierungstests erneut.

Separater instrumentierter Zaehlerlauf: 420, Hydrodynamik, feste
Stellungen, vier Schritte, je zwei JSON-Anforderungen.
Kontrollpfad = **ungepufferte C-0.10-Kompatibilitaets-API**, nicht
nachtraeglich instrumentiertes C-0.9. Numerische Paritaet bleibt erhalten.
Der Release-Vorhervergleich verwendet unabhaengig davon echtes C-0.9.
[`preparation-counters-tests.txt`](Nachweise/Performance_C-0.10_2026-11-03/preparation-counters-tests.txt)
enthaelt die tatsaechlichen Werte und den erfolgreichen Testabschluss.

| Zaehler | Ungebunden | Laufkontext |
|---|---:|---:|
| Modellbasisaufbau | kein Kontext | 1 |
| Tiefe physische Modellkopie | 32 | 1 |
| Meshkopie (Punkt-/Indexarrays eines Meshes) | 320 | 20 einschliesslich Basis |
| Stellungsaufbereitung / Massenaggregation | 32 | 1 |
| Lokale Meshtransformation | 128 | 4 |
| Originalprofil-Bounds | 40 | 2 |
| Vollstaendige Kraftauswertung | 20 | 9 |
| Ergebniswiederverwendung | 0 | 11 |
| Wasserlinienschnitt | 80 | 36 |
| Nasspanelberechnung | 84 | 51 |
| Weltpunkttransformation je Mesh/Pose | 84 | 27 |

Die neun verbleibenden Kraftauswertungen und 36 Schnitte belegen
zustaendige Anfangs-/Mittelpunkt-/Endzustaende; es wurden nicht einfach
alle Integratorstufen gleichgesetzt. Weggefallene Schnitte gehoeren zu
identisch doppelt angeforderten Auswertungen. Der anschliessende
Ereignistest bestaetigt eine zweite Stellungsaufbereitung beim geaenderten
Wert und keine weitere beim identischen Folgewert. Ein Eigentuemereignis
kopiert genau Eigentuemer plus seine mitbewegten Elemente.

Batch-Zusatznachweis gegen die eingefrorene originale C-0.9-CLI:
beide Modelle, 0,03 s / dt 0,01 s, jeweils alle vier vollstaendigen
JSON-Datensaetze **exakt identisch**. Keine Timingaussage aus diesem
funktionalen Lauf. Nachweis
[`batch-parity.json`](Nachweise/Performance_C-0.10_2026-11-03/batch-parity.json)
mit den vier referenzierten JSONL-Dateien im selben Ordner.

### Speicher und Lebensdauer

Der Kontext besitzt Basis, eine physische Stellungsfassung und eine
letzte Kraftauswertung. Bounds koennen nur fuer Basisprofilmeshes
eingetragen werden; Weltpunkte nur fuer Meshes der eigenen Basis oder
der einen physischen Fassung. Historische Posen erzeugen keine neuen
Cache-Schluesselobjekte je Zeitschritt, sondern ersetzen den letzten
Weltpunktestand. Stellungswechsel entfernt betroffene Weltpunktdaten;
Fehler bei der Aktualisierung verwirft den unvollstaendigen physischen
Cache. Destruktion gibt den gesamten Besitz frei. Der threadlokale
Routingzeiger wird auch bei Exceptions wiederhergestellt. Getrennte
Worker erhalten getrennte Kontexte; gleiche Kontextinstanzen werden
zusaetzlich gegen gleichzeitige Aufrufe verriegelt.

## 7. Rueckverfolgbarkeit und Abnahme

| Aktive Anforderung | Spezifikation | Umsetzungsschritte | Tatsaechlicher Nachweis | Status |
|---|---|---|---|---|
| A-C-00129 | Abschnitt 2 | U-C-000115, U-C-000116, U-C-000120 | `modellaufbereitung`: Snapshot, gleiche ID/Fassung mit anderem Inhalt, Parallelitaet; Basiszaehler 1; Besitzgrenzen Abschnitt 6 | technisch nachgewiesen |
| A-C-00130 | Abschnitt 3 | U-C-000117, U-C-000120 | Ereignis-/Kinder-/Historientests, eine feste Vorbereitung, identisches Folgeereignis ohne Mesharbeit, JSON-Paritaet | technisch nachgewiesen |
| A-C-00131 | Abschnitt 4 | U-C-000117, U-C-000118, U-C-000119, U-C-000120 | Alle Invalidierungstests; wirkliche Anfangs-/Mittelpunkt-/Endauswertung; vorhandene Schnitt-/Flutungs- und Solverregressionen | technisch nachgewiesen |
| A-C-00132 | Abschnitt 5 | U-C-000116, U-C-000118, U-C-000120 | Meshkopien 320 -> 20, lokale Transformationen 128 -> 4 im Kontrolltest; unveraenderte Meshes/Topologie; messbarer 420-Nutzen in Abschnitt 8 | technisch nachgewiesen |
| A-C-00133 | Abschnitt 6 | U-C-000119, U-C-000120 | Vollstaendige JSON-Paritaet, 11 Wiederverwendungen, historische Kontexte, Fehlerpfade, echte Batch-C-0.9-Paritaet fuer beide Modelle; integrierte GUI-Live-/Historienparitaet siehe Abschnitt 9.4 | Core und GUI-Integration technisch nachgewiesen; fachliche Nutzerabnahme separat |
| A-C-00134 | Abschnitt 7 | U-C-000115, U-C-000120 | Persistierte Test-/Zaehler-/Batchbelege, 48er-Release-Matrix, gezielte Wiederholung der streuenden Zelle und acht Endstands-hydro_drive-Kontrollen | technisch nachgewiesen; keine reproduzierte Regression |

Die fachliche Nutzerabnahme sowie bestehende offene Physikabnahmen und
die Simulationsfreigabe der Modellautoren werden hierdurch nicht ersetzt.

## 8. Release-Vorher/Nachher: vollstaendige Hauptmatrix

Nach erneuter ausdruecklicher Freigabe exklusiver CPU-Slot; waehrenddessen
keine Core-Builds/Tests und keine GUI-Timings. Der Slot wurde unmittelbar
nach Abschluss des letzten Messprozesses an den GUI-Agent zurueckgegeben,
vor dieser Dokumentation. Eigener Build `build/c010-release`, Release,
MSVC 19.44.35229, x64, Testinstrumentierung **AUS**.
Unveraenderte Referenzbibliothek C-0.9:
`3fa197135a0858ee525d3e4bc8273242e0ee97c2`.
SHA256 von Bibliotheken, Runnern, Quellstaenden und beiden aktuellen
Schema3-Modellen samt OBJ:
[`build-manifest.json`](Nachweise/Performance_C-0.10_2026-11-03/build-manifest.json).

**48 Konfigurationen**, je zwei Versionen, ein Warmup und fuenf
gemessene Laeufe: 420/Folkeboot, 0,5 s/5 s, dt 0,01 s,
fixed/events, free/drive/hydro/hydro_drive/foreign_off/foreign_on.
Ereignisse: erster Bewegungseigentuemer auf 50 am Laufmittelpunkt,
derselbe effektive Wert erneut im Folgeschritt. Alle Faelle mit
identischen Ausgangsdaten, Antrieben und Kraeften. Es wurde kein
ungueltiger oder instabiler Fall ersetzt: **alle 480 gemessenen Laeufe
beendeten den vorgesehenen Horizont**. Warmups kommen hinzu.

Jeder Lauf serialisiert Initialzustand und jeden Endzustand
(51 bzw. 501 vollstaendige Datensaetze). Die vollstaendigen JSON-Folgen
mit 17-stelliger Double-Ausgabe haben in **48/48 Konfigurationen und
allen fuenf Wiederholungen exakt denselben FNV-1a-64-Fingerabdruck**;
zusaetzlich ist je Konfiguration/Version der volle letzte Datensatz
gespeichert. Position, Quaternion, Geschwindigkeiten, Kraefte/Momente,
CG-Bezug, Volumina/Nassgeometrieauswertung, Stellungen, Druckpunkte und
saemtliche Kontext-/Bilanzfelder bleiben damit numerisch kompatibel.
Keine Toleranzen wurden gelockert.

Rohwerte (480 Datensaetze):
[`release-matrix.jsonl`](Nachweise/Performance_C-0.10_2026-11-03/release-matrix.jsonl).
Jede Phase mit Rohwerten, Median, Min und Max sowie Status/Paritaet:
[`release-summary.json`](Nachweise/Performance_C-0.10_2026-11-03/release-summary.json).

Gemessen wird die API-Pipeline mit Laden, Startvorbereitung, allen
Schritten und allen JSON-Ausgaben. `json_ms` enthaelt erforderliche
Kraftarbeit; `force_ms` ist eine zusaetzliche explizite Anfrage fuer den
letzten Zustand nach dessen JSON. Der Gesamtwert enthaelt auch
Fingerabdruckbildung und einmalige Referenzdatenerfassung, aber keine
externen Prozessstart- oder Ergebnisdatei-Schreibzeiten. Dies ist kein
GUI-Zeittakt und kein CLI-Dateisystembenchmark.

### 8.1 Gesamtmediane in Millisekunden

Je Zelle **C-0.9 -> C-0.10**. Streuungen und Rohwerte stehen unverkuerzt
im verlinkten maschinenlesbaren Bericht.

| Modell | Fall | 0,5 s fixed | 5 s fixed | 0,5 s events | 5 s events |
|---|---|---:|---:|---:|---:|
| 420 | free | 1774,02 -> 1329,47 | 6618,60 -> 3288,28 | 1805,63 -> 1342,42 | 6662,99 -> 3397,05 |
| 420 | drive | 1994,43 -> 1865,19 | 7207,58 -> 3261,45 | 2339,18 -> 1732,74 | 7415,32 -> 3242,22 |
| 420 | hydro | 2931,53 -> 1836,45 | 18857,40 -> 5656,44 | 3530,51 -> 1845,35 | 18644,51 -> 5491,41 |
| 420 | hydro_drive | 4130,43 -> 1630,89 | 23045,37 -> 5771,54 | 3447,23 -> 1623,43 | 21728,02 -> 5378,36 |
| 420 | foreign_off | 2066,58 -> 1872,44 | 7423,41 -> 3384,98 | 1686,12 -> 1325,36 | 6139,02 -> 3079,58 |
| 420 | foreign_on | 2356,75 -> 1345,06 | 7536,18 -> 9162,79 (*) | 1680,71 -> 1378,18 | 6198,66 -> 3080,89 |
| Folkeboot | free | 23,58 -> 21,63 | 178,14 -> 158,07 | 23,53 -> 21,10 | 169,77 -> 165,09 |
| Folkeboot | drive | 25,69 -> 22,73 | 203,02 -> 166,92 | 27,12 -> 22,68 | 212,31 -> 168,07 |
| Folkeboot | hydro | 60,91 -> 48,86 | 565,22 -> 425,33 | 60,40 -> 50,38 | 560,83 -> 425,65 |
| Folkeboot | hydro_drive | 69,42 -> 50,40 | 632,99 -> 438,08 | 68,89 -> 51,61 | 642,79 -> 435,19 |
| Folkeboot | foreign_off | 24,17 -> 21,28 | 176,68 -> 156,12 | 25,21 -> 21,35 | 175,81 -> 157,03 |
| Folkeboot | foreign_on | 24,45 -> 22,21 | 183,78 -> 162,58 | 25,11 -> 21,89 | 182,61 -> 163,52 |

**44/48 Zellen** zeigen vollstaendig getrennte verbesserte
Gesamtzeitspannen. Drei weitere haben bessere Mediane, aber
ueberlappende Spannen; dort ist die Beschleunigung nicht belastbar
belegt. Die vierte ueberlappende Zelle (*) hat einen schlechteren
Median und besonders starke Streuung; sie wird nicht als Verbesserung
ausgegeben.

### 8.2 Getrennte Phasen, Beispiel hydro_drive/fixed

Mediane in ms, jeweils C-0.9 -> C-0.10:

| Modell / Horizont | Laden | Vorbereitung | Schritte | alle JSON | letzte Kraftanfrage | Gesamt |
|---|---:|---:|---:|---:|---:|---:|
| 420 / 0,5 s | 1420,452 -> 1179,946 | 20,898 -> 17,522 | 1859,600 -> 322,336 | 813,844 -> 92,991 | 10,374 -> 0,011 | 4130,433 -> 1630,888 |
| 420 / 5 s | 1103,181 -> 1112,829 | 18,790 -> 17,931 | 16667,880 -> 4031,543 | 5177,415 -> 593,677 | 8,111 -> 0,007 | 23045,371 -> 5771,537 |
| Folkeboot / 0,5 s | 5,107 -> 5,323 | 0,228 -> 0,237 | 21,775 -> 9,459 | 40,784 -> 34,327 | 0,123 -> 0,007 | 69,419 -> 50,401 |
| Folkeboot / 5 s | 5,269 -> 5,346 | 0,240 -> 0,255 | 219,473 -> 94,104 | 394,789 -> 326,522 | 0,106 -> 0,005 | 632,990 -> 438,084 |

Unveraendertes Laden ist keine neue Importeroptimierung. Unterschiede
dieser Phase werden als Messstreuung mitgefuehrt, nicht als
Implementierungserfolg reklamiert.

### 8.3 Streuungsfall und verbleibende Kontrolle

420 / 5 s / fixed / foreign_on:
Gesamt C-0.9 7190,056..24868,999 ms, Median 7536,178 ms;
C-0.10 4393,395..9898,876 ms, Median 9162,793 ms.
Bereits das unveraenderte Laden streut von 1153,599..4299,563 ms
bzw. 1264,584..3393,998 ms. Phasen sind unterschiedlich betroffen:
Schrittmedian 2291,093 -> 1305,254 ms, JSON-Median
4053,471 -> 4886,681 ms. Numerische Paritaet bleibt exakt.
Aus dieser einen stark ueberlappenden Messung ist weder Beschleunigung
noch eine reproduzierbare Regression ableitbar. Eine gezielte
Kontrollwiederholung nach Rueckgabe eines freien CPU-Slots ist
angefordert; Originalwerte bleiben unabgeaendert erhalten.

Bei der abschliessenden Fehlerpfadpruefung wurde die Originalreihenfolge
der hydrodynamischen Endkraftvalidierung vor dem Testantriebsbegrenzer
wiederhergestellt. Das vermeidet einen unbeabsichtigt verschobenen
Diagnosepunkt bei extremen, vor Begrenzung ungueltigen Auswertungen.
Normale doppelte Endanfragen werden vom exakten Kraftcache abgefangen.
Die Hauptmatrix beschreibt den im Manifest gespeicherten Messstand vor
dieser Reihenfolgesicherung; die betroffenen hydro_drive-Pfade werden
am endgueltigen Build nochmals kontrolliert. Keine unbemerkte Zuordnung
alter Zeiten zu einem nachtraeglich geaenderten Binaerstand.

## 9. Endstands-Kontrollen und Abschluss

Nach der Reihenfolgesicherung wurde der eigene instrumentierte Core
erneut gebaut und die **vollstaendige Suite erneut ausgefuehrt**:
**20/22 bestanden**, ausschliesslich die beiden bereits genannten
Import-/Katalogaltfehler. Auch der neue explizite hydro_drive-Fall mit
wirksamem Geschwindigkeitsbegrenzer (zulaessige Grenze 0,01 m/s,
Startgeschwindigkeit 0,1 m/s) ergibt zwischen ungebundener und
kontextgebundener Auswertung identische komplette JSON-Datensaetze.
Die urspruenglichen Diagnose-Pruefpunkte bleiben erhalten.

Aktuelle Belege:
[`core-tests-final.txt`](Nachweise/Performance_C-0.10_2026-11-03/core-tests-final.txt),
[`final-functional-details.txt`](Nachweise/Performance_C-0.10_2026-11-03/final-functional-details.txt),
[`preparation-counters-tests-final.txt`](Nachweise/Performance_C-0.10_2026-11-03/preparation-counters-tests-final.txt).
Ein zwischenzeitlicher Testfixture-Versuch mit Grenze 0,001 m/s wurde
korrekt vom bestehenden Validator abgelehnt; das Fixture verwendet
jetzt die erlaubte Untergrenze. Keine Testantriebsgrenzen wurden geaendert.

Anschliessend eigener Release-Build ohne Instrumentierung. Wegen der
ungeklaerten Slot-Rueckmeldung wurde vor Fortsetzung festgestellt, dass
kein GUI-/Build-/Messprozess des Projekts aktiv war; die kurze
Restkontrolle erfolgte im urspruenglich ausdruecklich freigegebenen
Core-Zeitraum. Keine Builds/Tests waehrend der eigentlichen Messprozesse.
Nach dem letzten Prozess wurde die Freigabe nochmals ausdruecklich
gemeldet. Keine weiteren Core-CPU-Arbeiten sind ausstehend.

### 9.1 Streuungsfall erneut, beide Versionen im gleichen Kontrolllauf

420, 5 s, fixed, foreign_on: ein Warmup und fuenf Wiederholungen
je Version, derselbe Runner und dieselben Eingaben.

| Version | Median gesamt ms | Min ms | Max ms |
|---|---:|---:|---:|
| C-0.9 | 6288,129 | 6162,233 | 6419,402 |
| endgueltiges C-0.10 | 3271,758 | 3254,449 | 3296,817 |

**1,92-facher Nutzen**, vollstaendig getrennte verbesserte Spannen und
exakte Paritaet aller vollstaendigen Zustandsfolgen. Der schlechtere
Median der urspruenglichen stark streuenden Zelle wurde **nicht
reproduziert**. Originalwerte bleiben sichtbar; die Kontrollmessung
wird nicht rueckwirkend in die Hauptmatrix hineineditiert.

### 9.2 Alle durch die Reihenfolgesicherung beruehrten hydro_drive-Zellen

Am endgueltigen Build erneut ein Warmup und fuenf Wiederholungen je
Zelle. Referenz bleibt jeweils das unveraenderte C-0.9 aus der
Hauptmatrix (kein neu gemessener Gleichzeitvergleich in dieser Tabelle).
Alle acht Kontrollen zeigen exakte komplette Zustandsparitaet und
vollstaendig getrennte verbesserte Gesamtzeitspannen.

| Modell | Horizont / Stellung | C-0.9 Median ms | Endstand Median ms | Faktor |
|---|---|---:|---:|---:|
| 420 | 0,5 s fixed | 4130,433 | 1536,710 | 2,69 |
| 420 | 0,5 s events | 3447,231 | 1540,122 | 2,24 |
| 420 | 5 s fixed | 23045,371 | 5741,497 | 4,01 |
| 420 | 5 s events | 21728,023 | 5823,331 | 3,73 |
| Folkeboot | 0,5 s fixed | 69,419 | 53,369 | 1,30 |
| Folkeboot | 0,5 s events | 68,887 | 54,182 | 1,27 |
| Folkeboot | 5 s fixed | 632,990 | 465,432 | 1,36 |
| Folkeboot | 5 s events | 642,792 | 515,462 | 1,25 |

Rohwerte (50 weitere gemessene Laeufe):
[`followup-release.jsonl`](Nachweise/Performance_C-0.10_2026-11-03/followup-release.jsonl).
Phasen, Rohwerte, Median/Min/Max und Herkunft der jeweiligen Referenz:
[`followup-summary.json`](Nachweise/Performance_C-0.10_2026-11-03/followup-summary.json).
Endgueltiger Binaer-/Quellstand:
[`followup-build-manifest.json`](Nachweise/Performance_C-0.10_2026-11-03/followup-build-manifest.json).

### 9.3 Schlussstatus und Grenzen

**U-C-000115 bis U-C-000120 technisch abgeschlossen.**
A-C-00129 bis A-C-00134 sind den konkreten Belegen zugeordnet.
Wiederholungsarbeit ist per Zaehler beseitigt, der grosse Meshfall
profitiert messbar, numerische Ergebnisse bleiben identisch, und es
liegt keine reproduzierte Regression vor. Drei Zellen mit
ueberlappenden Spannen bleiben ohne belastbare Beschleunigungsaussage;
es wird kein pauschaler Mindestfaktor fuer alle Modelle behauptet.

Keine Physik-, Toleranz-, dt-, Meshvereinfachungs-, Integrator- oder
statische Solveraenderung. Keine neue Modell-/Simulationsfreigabe.
Kein Commit, Tag oder Push; vorhandene dirty Analyse-Dokumente bleiben
erhalten. Standard-GUI-Build nicht angefasst. GUI-Liveintegration
verwendet den finalen stabilen Vertrag CPPI-0113..0118 und gehoert zum
getrennten GUI-Plan; dessen Integrationsnachweise stehen in Abschnitt 9.4.
Diese technischen Nachweise ersetzen keine fachliche Nutzerabnahme.

Reproduktion im Core-Repository (einen neuen Ausgabeordner fuer die
Hauptmatrix waehlen; vorhandene Rohbelege werden nicht ueberschrieben):

```powershell
python Code\tests\aufbereitung_matrix.py --build build\c010-release\Release --models ..\Modelle --output Umsetzungsplaene\Nachweise\NEUER_LAUF
python Code\tests\aufbereitung_matrix.py --build build\c010-release\Release --models ..\Modelle --output Umsetzungsplaene\Nachweise\NEUER_LAUF --followup
```

Fuer den Referenzrunner ist beim Konfigurieren
`SEGELPHYSIK_PERFORMANCE_BASELINE_LIBRARY` auf die gespeicherte originale
C-0.9-Releasebibliothek zu setzen. `SEGELPHYSIK_TEST_INSTRUMENTATION=OFF`
ist fuer Zeitmessungen zwingend; Instrumentierungs- und Zeitlaeufe bleiben
getrennt.

### 9.4 GUI-Integration G-0.11

Laut Abschlussmeldung des koordinierenden GUI-Agents vom 03.11.2026,
22:12 Uhr, ist die Integration des stabilen Vertrags abgeschlossen:
Live haelt getrennte persistente `Modellaufbereitung`-Kontexte fuer
Worker und Anzeige anstelle der bisherigen zwei Modelle; Worker ruft
`.schritt()`, Anzeige `.json()` auf. Modellwechsel erneuert beide
Kontexte, Batch nutzt weiterhin die interne Integration.

Gemeldete Nachweise: GUI-Release-Build erfolgreich, 8/8 gezielte Tests
bestanden und erweiterter `gui_core_integration` erfolgreich.
Vollstaendiges JSON stimmt fuer 420 und Folkeboot ueber alle Zustaende
von 0,03 s mit Hydro+Testantrieb exakt mit ungebundenem Core ueberein;
historische Reihenfolge 3/0/2/1/3 ebenfalls exakt. GUI-Replay umfasst
28 Faelle je Version, 16/16 Tabellen-/Bildvergleiche pixelgenau und
versteckte Szenen mit 0/0/0/0 Arbeit. Beispiel 420 dynamisch,
AUX aus/einfach an: 350,31 auf 23,86 ms.

Massgebliche GUI-Umsetzung und Ergebnisbelege:
[GUI-Plan G-0.11](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.11.md).
Hier nur projektuebergreifender Integrationsverweis fuer A-C-00133;
keine erneuten Core-Tests, Builds oder Messungen fuer diesen Nachtrag.

## 10. Nachpruefung: dynamisches Kraftinventar und Ersatz-SDP

Beim anschliessenden GUI-Nutzertest vom 03.11.2026 fehlten trotz
Auswahl Kraftpfeile. Zwei Core-Ausgabefehler wurden getrennt
reproduziert und am Ursprung korrigiert:

| Befund | Korrektur | Nachweis |
|---|---|---|
| Dynamischer Beitrag vorhanden, Bilanz erwartete aber `stationaer.fremdkraft` | Erwartete externe Einzelbeitraege aus ihren tatsaechlichen IDs bestimmen; keine statische ID fuer dynamische Kraefte einsetzen | Bilanztest fuer beide IDs und gueltig/null/deaktiviert/nicht_berechnet/ungueltig; fehlende oder ungueltige Werte bleiben sichtbar unvollstaendig |
| Dynamischer Ersatz-SDP fehlte | Stationaere und dynamische Snapshots serialisieren den eigenen Kraft-Ansatzpunkt nach demselben Statusvertrag | Simulationstest prueft ersten/letzten Batchzustand; GUI-Adapter prueft aktiven, inaktiven und Nullkraftkontext sowie eingefrorene Historie |

`druckpunkte.ersatz_segel.position_modell_m` ist nur bei aktiver
Nichtnullkraft vorhanden und exakt der konfigurierte modellfeste
Ansatzpunkt. Inaktiv/Nullkraft enthalten keine Position. Keine neue
Wind-/Segelphysik, keine Aenderung von Kraft, Moment, Integrator oder
statischem Solver und keine stillschweigende Akzeptanz fehlender
Kraftbeitraege. Der
[Schnittstellenvertrag](../Dokumentationen/Schnittstelle_Core_GUI.md)
ist entsprechend praezisiert.

Nach den Korrekturen **3/3 gezielte Core-Tests bestanden**:
`modellaufbereitung`, `simulation_gesamtlauf`, `kraft_momentenbilanz`.
[Testprotokoll](Nachweise/Kraftauswahl_2026-11-03/core-tests.log).
GUI-Integration zusaetzlich **8/8** mit echten Kraft-Actors,
Bildwechseln und 18 freien/einfachen Ansichtswechseln:
[GUI-Nachtrag](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.11.md#6-nachpruefung-kraftauswahl-und-ansichtswechsel).

Die korrigierten Bilanzmetadaten und der zusaetzliche dynamische
Ersatz-SDP unterscheiden sich absichtlich von der alten JSON-Ausgabe.
Die bisherige numerische Performance-Referenz und ihre Rohbelege werden
nicht nachtraeglich umgeschrieben. Nutzerabnahme und bekannte Altbefunde
bleiben offen; kein Commit/Push/Tag.
