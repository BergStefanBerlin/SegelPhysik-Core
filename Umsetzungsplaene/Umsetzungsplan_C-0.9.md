# Umsetzungsplan Core: dynamische Fremdkraft und Versionsvergleich

## Nutzerabnahme 04.11.2026

**Nutzerabnahmen: done.** Ausdruecklicher Nutzerauftrag vom 04.11.2026,
09:16 Uhr fuer den gelieferten Stand einschliesslich laufender Eingaben
U-C-000121. Dieser datierte Abschluss ersetzt alle aelteren offenen
Nutzerabnahmestaende in diesem Plan; historische Nachweise bleiben erhalten.
Technische Testluecken und nicht finalisierte Zusatzumfaenge werden dadurch
nicht als nachgewiesen ausgegeben. Kein Commit, Tag oder Push beauftragt.

## Datierter fachlicher Nachtrag 04.11.2026: laufende Eingaben

Dieser Nachtrag fuehrt das bestehende Fremdkraftthema fort; alte
Abschlussnachweise und der separate reine Performanceplan bleiben
unveraendert abgegrenzt.

**U-C-000121 / A-C-00135:** Nutzerfreigabe 08:52 Uhr.
Vor Implementierung CPPI-0119 vergeben; danach validierten
Kontextwechsel ohne Historienmutation implementieren, anschliessend
physikalische Wirkung und exakte Cacheinvalidierung pruefen, zuletzt
in den GUI-Schrittuebergang integrieren (U-G-000145).

Technisch umgesetzt: neuer unveraenderlicher Kontext durch
`dynamische_fremdkraft_aendern`, bisherige Modell-/Zeitvalidierung
und Endlichkeitspruefung erhalten. Ruder/Schwert verwenden unveraendert
die vorhandenen Bewegungsereignisse und selektive stellungsabhaengige
Massen-/Traegheits-/Meshaufbereitung. Keine neuen Modellnetze,
keine Aenderung an Integrator, Zeitschritt oder stationaerem Solver.
`simulation_gesamtlauf` (1,49 s) prueft geaenderte Translation/Rotation,
unveraenderte historische JSON-Ausgabe, Ablehnung nichtendlicher Werte
und exakte Gleichheit vorbereiteter/ungebundener Ausgabe nach Kraftwechsel.
`modellaufbereitung` (4,14 s) bestanden. GUI-Adapter bestaetigt zusaetzlich
laufende Ruder-/Schwerttransformationen und unveraenderte Historie.
Nutzerabnahme offen, keine Git-/Release-Aktion.

**Status:** C-0.9 Core-Mindestumfang technisch umgesetzt und gezielt
geprueft; priorisierte Ursachenanalyse und Messvergleich am 03.11.2026
abgeschlossen. GUI-Ergebnisse siehe G-0.10-Plan, Abschnitte 13.7–13.8.
Persoenliche GUI-Abnahme und weitergehende Physiknachweise offen.
**Arbeitszielversion:** C-0.9; keine Tag- oder Release-Freigabe.
Der abgeschlossene C-0.8-Plan bleibt unveraendert. Kein zweiter Plan fuer
denselben Versionsschritt.

Massgeblich ist die
[Spezifikation Dynamische Fremdkraft](../Spezifikationen/Spezifikation_Dynamische_Fremdkraft.md).
Herkunft und offene Entscheidungen:
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Dynamische_Fremdkraft.json).
Abhaengiger
[GUI-Plan, Abschnitt 13](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.10.md#13-planentwurf-einheitliche-rechenbedienung-und-performancevergleich).

## 1. Umfang und Entscheidungsgate

**Verbindlicher Haltepunkt, Nutzerpraezisierung 03.11.2026, 18:42 Uhr,
aktualisiert durch Go 19:23 Uhr:** Die Core-Mindestanpassungen und
technischen Integrationsnachweise sind abgeschlossen. Die priorisierte
Ursachenanalyse und Core-Messungen sind nach Abschluss der GUI-Messungen
und ausdruecklicher Freigabe des Messfensters erfolgt. Das ist
ausdruecklich keine Freigabe fuer Produktionscode-Aenderungen,
Optimierungen oder weitere Physikimplementierung. GUI- und Core-Messungen
konkurrierten nicht.

Die minimal erforderliche Core-Anpassung fuer dieselben relevanten
physikalischen Eingaben in Statik und Dynamik ist geliefert; vollstaendige
Ergebnis-/Physikparitaet ist nicht nachgewiesen. Die gemeinsame GUI-
Integration und alle noch offenen Physiknachweise bleiben separat. Das Go
von 19:23 erlaubte die Ursachenanalyse; nach ausdruecklichem Abschluss der
GUI-Messungen wurde das Core-Messfenster freigegeben und genutzt. Keine
Windphysik, kein neuer Integrator, kein
versteckter Gleichgewichtsstart und keine vorgezogene Optimierung auf
Kosten korrekter Physik.

Der Nutzer hat den begrenzten Core-Mindestvertrag in Spezifikation
Abschnitt 8 freigegeben. Kraftvektor und Ansatzpunkt sind modellfest;
die GUI-Kamerawahl ist kein Core-Kraftframe. Gleichzeitige aktive
Testantriebs- und Fremdkraftkontexte werden abgewiesen. Die API ist vor
der Implementierung mit CPPI-0105 bis CPPI-0112 registriert. Die
projektweiten maxima waren A-C-00125 und U-C-000111; die neuen Kennungen
wurden fortlaufend vergeben: A-C-00126 bis A-C-00128 und
U-C-000112 bis U-C-000114. Weitergehende Matrix und GUI-Abnahme bleiben
offen. Ursachenanalyse und Core-Messvergleich sind abgeschlossen;
weitergehende Physikmatrix bleibt offen.

## 2. Geordnete Arbeitspakete

| Umsetzungsschritt | Anforderung | Voraussetzung | Ergebnis und Abschlusskriterium | Status / Nachweis |
|---|---|---|---|---|
| U-C-000112: Dynamischen Eingangsvertrag liefern | A-C-00126 | Freigegebener Mindestvertrag; CPPI-0105 bis CPPI-0109 und CPPI-0111 registriert | Unveraenderliche Konfiguration, Startvalidierung, feste Bewegungsstellungen vor dem ersten Zustand und Batch-Eingang; Altaufrufe ohne Kontext unveraendert. | umgesetzt; `hydrodynamik_traegheit` und `simulation_gesamtlauf` |
| U-C-000113: Bewegtes physisches Modell konsistent integrieren | A-C-00128 | U-C-000112 | Kraftauswertung, Schwerpunkt, Traegheit und Modellursprung verwenden dieselben festen Stellungen. Physischer Massen-/Traegheitstest bestaetigt die Korrektur. | umgesetzt; `hydrodynamik_traegheit` |
| U-C-000114: Fremdkraft in Integrator und Ausgabe integrieren | A-C-00127 | U-C-000112; U-C-000113; CPPI-0110 und CPPI-0112 registriert | Kraft und Moment wirken in allen tatsaechlichen Kraftauswertungen; aktiver Null-/Inaktivfall, Beitrag/Summen/Snapshot und Testantrieb-Konflikt werden gezielt geprueft. | umgesetzt; `hydrodynamik_traegheit` und `simulation_gesamtlauf` |
| Weitergehende Statik-zu-Dynamik-Matrix | IDEE-C-00092, nicht finalisiert | Erneute Nutzerfreigabe | Ueber den Mindestvertrag hinausgehende Matrix und Konvergenznachweise. | blockiert |
| GUI-Integration und persoenliche Pruefung | GUI-Plan G-0.10 | Adapteranbindung und Mindest-Core | Echter dynamischer GUI-Lauf; technische Tests und Nutzerabnahme getrennt erfassen. | laufend ausserhalb dieses Core-Auftrags |
| Performance-Ursachenanalyse | IDEE-C-00092, weiterhin nicht finalisiert | Go 03.11.2026, 19:23 Uhr | Evidenzgebundene Inventur, Messwerte und Ursachenrangfolge gemaess Abschnitt 4; keine Implementierung. | abgeschlossen; Grenzen und Rohbelege in Abschnitt 4 |
| Core-Messlaeufe | IDEE-C-00092, weiterhin nicht finalisiert | GUI-Messungen abgeschlossen; Core-Messfenster ausdruecklich freigegeben | Reproduzierbare Messreihe gemaess Abschnitt 4; keine konkurrierende CPU-Last. | abgeschlossen; keine Optimierung freigegeben |

Die Quellen 88 bis 92 bezeichnen ausschliesslich die bereits vergebenen
Entwurfspunkte in der verlinkten Liste, nicht neue Anforderungen.

## 3. Meilensteine und Nachweise

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| Vertrag bereit | Core-Mindestumfang freigegeben | Spec Abschnitt 8, A-C-/U-C-Kennungen und CPPI-0105 bis CPPI-0112 registriert; Arbeitsziel C-0.9, kein Release. |
| Mindest-Core korrekt | U-C-000112 bis U-C-000114 implementiert | Kraft-/Momentreferenz, Pose-Transformation, bewegte Masse/Traegheit, Null/inaktiv/Fehler, Snapshots und Batchlauf bestanden. |
| GUI-Integration bereit | Core-Mindesttests bestanden | Parent-Adapterbuild und dynamischer GUI-Lauf; persoenliche Pruefung des Nutzers bleibt separat. |
| Ursachenanalyse dokumentiert | Go 03.11.2026, 19:23 Uhr | Evidenzgebundene Befunde in Abschnitt 4.2; keine Physikabnahme und keine Codeaenderung. |
| Messvergleich belastbar | GUI-Messungen abgeschlossen; ausdrueckliche Freigabe des Core-Messfensters | Identische Eingaben, Warmup + 5 Laeufe, Median/Spannweite; getrennte Modelllade-, Integrations-, JSON- und CLI-Messungen. Ergebnis siehe 4.3. |

Gezielt pruefen: Schwerpunktangriff ohne Moment, bekanntes Kreuzprodukt,
gekraengte Pose, wirkliche Translation/Rotation, dt/dt/2/dt/4,
0/+-12/+-24/+-36-Grad-Referenzfamilie soweit fuer den jeweiligen
Fall definiert, Flutung sowie Datenfehler. Ein stationaerer Referenzstart
ist nur ein Test, kein neuer Standardstart und kein Stabilitaetsbeweis.
Bestehende thematisch betroffene Core-Tests nutzen; Gesamtregression
erst bei Hinweisen auf breitere Auswirkungen.

## 4. Performance-Ursachenanalyse und historischer Vergleich

**Go vom 03.11.2026, 19:23 Uhr; Messfensterfreigabe nach GUI-Abschluss
und Messdurchfuehrung am 03.11.2026:** Ursachenanalyse und Benchmarks sind
abgeschlossen. Keine Codeaenderung, Optimierung, Modellmigration oder
Aenderung von Physik/Abnahmekriterien.

Nutzerhinweis vom 03.11.2026, 18:42 Uhr: Auch die Modellstruktur hat
sich geaendert. Komplexere Modelle/Ableitungen und haeufigere
Neuberechnungen sind zu pruefende Ursachenhypothesen, keine Befunde.
Fuer historische und heutige Modelle Element-, Quell-/Ableitungsmesh-,
Dreiecks- und Nasspanelzahlen erfassen. Ableitungsaufbau, Laden,
Validierung, Cache-Treffer/-Invalidierungen und Anzahl physischer
Modellaufbereitungen sowie Kraft-/Geometrieauswertungen je Zeitschritt
und Ergebnisausgabe messen. Softwarestand und Modelldaten als getrennte
Einflussgroessen behandeln; keine unterschiedliche Modellkomplexitaet
als reine Coderegression ausgeben. Originalmodelle nicht veraendern.

Nutzerfestlegung vom 03.11.2026, 18:41 Uhr: **C-0.7 ist die
bevorzugte historische Core-Vergleichsbasis.** Lokal verifiziert:
`C-0.7` verweist auf
`ebca994770b216a749e18917d2ae387de1612dff` vom
28.10.2026, 19:24:39 Uhr (+01:00).
Zuerst den nach Mindestintegration funktional geprueften aktuellen
Core gegen diese Basis vergleichen. Weitere Staende dienen bei Bedarf
der Ursachenabgrenzung. Daraus folgt keine Festlegung auf GUI-Tag G-0.7;
eine kompatible historische GUI-Zuordnung ist gesondert zu belegen.

Am 03.11.2026 lokal gefundene Kandidaten:

| Core-Stand | Datum | Anlass fuer Vergleich |
|---|---|---|
| `d6d57c7` | 02.11.2026 | Letzter veroeffentlichter Ausgangsstand mit stationaerer Fahrt/Inspektion/Druckpunkten |
| `C-0.7` / `ebca994` | 28.10.2026 | Vom Nutzer bevorzugte Basis: Zusatzmasse, Daempfung, Lateraldruckpunkt |
| `9bb7f04` | 28.10.2026 | Ruder-/Schwertbewegung, lokaler Profilrahmen |
| `28ee870` | 28.10.2026 | Dynamische ITTC-Reibung |
| `2959d62` | 27.10.2026 | Etwa eine Woche zurueck: temporaerer Bugantrieb |

Hashes beim Vergleich vollstaendig aufloesen. Exakte Reihenfolge und
Zusaetze durch Git-Diffs der dynamischen Aufrufkette verifizieren;
Commitbetreff allein ist kein Ursachennachweis.

Vorgehen:

1. Alte Quellen in getrennten temporaeren Worktrees/Buildordnern
   bereitstellen. Aktuelle Branches und Dirty-Arbeit nicht wechseln,
   zuruecksetzen, committen oder ueberschreiben.
2. Gleicher Rechner, Release-Konfiguration, Compiler/Optimierung,
   nicht konkurrierende Messlaeufe. Ein Warmlauf, danach mindestens
   fuenf Wiederholungen; Median und Streuung sowie ms/Schritt,
   Schritte/s und Rechenzeit/s Simulationszeit ausgeben.
3. 420er und Folkeboot, gleicher Start, Dauer und dt. Zuerst 0,5 s,
   dt=0,01 s als Anschluss an den aktuellen Audit; nach stabilem
   Kurzlauf ein laengerer gemeinsamer Fall. Ohne Echtzeit-Wartezeit.
4. Reine Zeitschritte, Start/Laden und JSON-/Kraftausgabe getrennt
   messen. Anzahl Kraft-/Geometrieauswertungen und Schrittzahl
   dokumentieren, damit Mehrarbeit von teureren Einzelaufrufen
   unterschieden wird.
5. Historische Schema-/API-Unterschiede vorab pruefen. Ein gemeinsames
   kontrolliertes Modell und Physik-Feature-Set fuer Direktvergleiche
   verwenden, Originaldateien nicht migrieren oder ueberschreiben.
   Modellfingerprints/Meshgroessen dokumentieren. Wo Gleichheit
   unmoeglich ist, den Vergleich als eingeschraenkt oder nicht
   vergleichbar markieren und zusaetzlich den damaligen
   Originalumfang getrennt messen.
6. Neue allgemeine Fremdkraft nicht in alte Versionen zurueckportieren.
   Historische Zeitreihe nutzt den gemeinsamen Altvertrag
   (zum Beispiel Testantrieb aus/ein, wenn dort vorhanden);
   neuer Fremdkraftfall als heutige Zusatzmessung, nicht als
   vermeintlich gleicher historischer Lauf.
7. Schrittzahlen und numerische Ausgaben vergleichen. Mehrzeit
   durch neu hinzugekommene Hydrodynamik/Traegheit von einer
   Regression bei gleichem Umfang unterscheiden. Bei einem
   klaren Zeitsprung den Zwischenstand derselben Aufrufkette
   gezielt eingrenzen.
8. Bericht: pro Version Datum/Hash/Modell/Featureumfang,
   Rohzeiten und Resultate, erklaerte Codeaenderungen,
   Einschraenkungen und priorisierte Vorschlaege. Keine
   Core-Optimierung vor Ursachenbeleg und passendem Auftrag.

### 4.1 Modellinventar und Vergleichsgrenzen

Read-only-Inventar am 03.11.2026 aus `Modelle\420` und
`Modelle\folkeboot`; Eingabedateien wurden nicht geaendert. SHA-256 bindet
die gezählten Geometrien an konkrete Dateien. OBJ-Zahlen sind
Quellmeshwerte; Ableitungen koennen die resultierende Dreieckszahl
veraendern und werden nicht als identisch mit der Quellzahl behauptet.

| Modell | Schema | Elemente / Gruppen | OBJ-Bytes | Quellvertices / -dreiecke | Hydrostatik-Volumenmeshes (Quell-Dreiecke) | Hydrodynamikrollen (Quell-Dreiecke) | Modell- / OBJ-SHA-256 |
|---|---:|---:|---:|---:|---|---|---|
| 420 | 3 | 11 / 4 | 4,388,621 | 47,101 / 94,129 | Schwert 24 (mit `z_schnitt`), Ruder 16, Rumpf 69,056, Cockpit 24,460; zusammen 93,556 vor Ableitung | Kiel 24, Ruder 16, Rumpf 69,056; zusammen 69,096 vor Ableitung | `90383a6bc04305453598bcfa5207ec63cb35dd838e137849056044d01717914a` / `306af3f86c16efe0d5308116f30125d70fc70c6a2f4d314cbcaf6123f5ca01f3` |
| Folkeboot | 3 | 8 / 4 | 19,951 | 257 / 486 | Schwert 20, Ruder 16, Rumpf 374; zusammen 410 | Kiel 20, Ruder 16, Rumpf 374; zusammen 410 | `0a6314b9ab321796db5e393fdcc29bec0082dd269b86f4f932d12734cec16c2d` / `8bde5fa509461379aeb4bb7c49c797b4828431b9cc2fcf6bccf955c9b07f131b` |

Die aktuellen Modelle verwenden Schema 3. C-0.7 akzeptiert laut
`ebca994:Code/src/modell.cpp` nur `0.1-entwurf`; C-0.8 und C-0.9
akzeptieren `0.1-entwurf`, 2 und 3. Daher ist das aktuelle Modellpaar
kein direkter Drei-Versionen-Lauf: Ein C-0.7-Ergebnis damit existiert
nicht. Nicht stillschweigend konvertieren. Falls die unveraenderten
archivierten 0.1-Modelle als zusaetzliche kompatible Zeitreihe verwendet
werden, diese klar als andere Modelldaten/Physikfassung ausweisen und
nicht mit aktuellen Schema-3-Ergebnissen vermischen. Die C-0.8/0.9-
Messung mit dem aktuellen Schema-3-Paar bleibt direkt vergleichbar.

**Zusaetzlicher, getrennt ausgewiesener Dreiversionslauf:** Die
unveraenderten archivierten Modelle `Modelle alt\420` und
`Modelle alt\folkeboot` sind Schema `0.1-entwurf` und wurden von C-0.7,
C-0.8 und C-0.9 angenommen. Beide Dateien markieren `status=entwurf` und
`simulationsfreigabe=false`; der technische Lauf ist daher kein
physikalischer Abnahmenachweis. Diese Zeitreihe vergleicht dieselben
Modelldateien und OBJ-Dateien ueber alle drei Core-Staende, aber nicht die
aktuellen Schema-3-Modellstaende.

| Archivmodell | Gruppen / Elemente | Referenzierte eindeutige OBJ-Dateien | Summe Bytes / Vertices / Triangles dieser OBJ-Dateien | Modell-SHA-256 |
|---|---:|---:|---:|---|
| 420 (`0.1-entwurf`) | 4 / 11 | 19 | 8,372,470 / 124,531 / 248,581 | `fa9539de98d10305777ebeff05b1f2e8f60b95ecc6fdf0a4e1263dec29d1024e` |
| Folkeboot (`0.1-entwurf`) | 4 / 8 | 17 | 49,645 / 725 / 1,382 | `0043f85987ddc0af19fb3484d02df6a614f9ad28fdb6e647cc163fccf134045c` |

Die addierten OBJ-Zahlen umfassen verschiedene Anzeige-, Massen- und
Hydrostatikdateien und sind weder eine einzelne Laufzeit-Dreieckszahl noch
die Zahl der je Kraftauswertung verarbeiteten Nasspanels. Einzeldateien,
Hashes und fehlende Referenzen (keine) stehen im
[persistenten Modellinventar](Nachweise/Performance_Core_2026-11-03/legacy-model-inventory.json).

### 4.2 Evidenzgebundene Laufzeitbefunde

Messwerte und Grenzen stehen in Abschnitt 4.3. Statische
Aufruf-/Meshzahlen sind keine Laufzeitmessungen. Die Prioritaet bewertet
nur das qualitative Einsparpotenzial; sie ist weder eine Prognose in
Prozent noch eine Freigabe zur Umsetzung.

| Befund / Aenderung | Core-Pfad; statisch / dynamisch | Quelle, Zeilen und Versionshash | Zusaetzliche Zeit | Qualitatives Einsparpotenzial | Physikalisches Risiko bei spaeterer Aenderung |
|---|---|---|---|---|---|
| Schema-3-Modelle teilen OBJ-Geometrie ueber 11/8 Elemente, enthalten aber sehr unterschiedliche Meshgroessen. 420 hat 94,129 Quell-Dreiecke, Folkeboot 486; ein Laufzeitunterschied zwischen diesen Modellen waere nicht als reine Code-Regression zu lesen. Der 420-Rumpf hat 69,056 Triangles, der Folkeboot-Rumpf 374. | Laden und beide Rechenarten; Hydrostatik durchlaeuft Volumenmeshes, Hydrodynamik die Rollenmeshes. | `Code/src/modell.cpp:1354-1609`; `Code/src/hydrostatik.cpp:784-878, 2136-2225`; aktuelle Modelldateien/SHA-256 in 4.1. Schema-3-Lader: C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`; dynamischer Stand: C-0.9 `3fa197135a0858ee525d3e4bc8273242e0ee97c2`. | Gemessen: Schema-3-420 laden C-0.8 1.090 s [1.046-1.316], C-0.9 1.160 s [1.055-1.291]; Folkeboot 5.23/5.17 ms. Vollstaendige Werte und CLI-Zeiten in 4.3. Kein isolierter Dreieckskostenanteil. | **Hoch** fuer Geometriepfade, sofern Profilierung den erwarteten Meshbezug bestaetigt. | **Hoch** bei Approximation, Weglassen oder anderer Nass-/Schnittgeometrie; niedriger bei exakter Wiederverwendung mit korrekter Pose. |
| C-0.8 ergaenzt Shared-OBJ-Laden, Schema-3-Validierung, Ableitungen (`z_schnitt`, `ohne_deckflaeche`), Triangulations-/Topologiepruefung sowie Mesh-Integrale fuer Masse, Schwerpunkt und Elementtraegheit. Diese Arbeit faellt primaer beim Laden/Vorbereiten an, nicht als Massenintegral pro Zeitschritt. Abgeleitete Geometrie kann mehr/weniger Dreiecke als die Quelle haben. | Start/Laden fuer Statik und Dynamik; vorbereitete Massen-/Volumenmeshes bleiben danach im Modell. | `Code/src/modell.cpp:644-720, 841-958, 1072-1307, 1354-1609`; C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`. | Gemessen am identischen 0.1-420er: Lade-Median C-0.7/0.8/0.9 1.153/1.156/1.145 s (Ueberlappung); am Schema-3-420er C-0.8/C-0.9 1.090/1.160 s. Loaderzeit ist quantifiziert, aber nicht kausal in Validierung, Ableitung, Meshintegrale und Dateilesen zerlegt. | **Mittel bis hoch** fuer Startzeit grosser Modelle; der konkrete Loaderanteil ist nicht isoliert. | **Hoch** beim Umgehen von Validierung, Flutungsrand, Ableitungs- oder Massenintegralen; Wiederverwendung muss Version und Inhalt binden. |
| Katalog-/Inspektionspfad fuehrt eigenstaendige Vollaufbereitung aus: Katalogeinlesen liest Beschreibung/Mesh, berechnet Modellfassung und validiert per `lade_modell`; `readModelInspection` importiert Beschreibung und Modell, prueft Geometriedatei-Fingerprints vor/nach Anzeigeaufbereitung und baut mehrere Mesh-/Baumansichten. Beim Schema-3-420er verweisen 10 Anzeigeelemente auf dieselbe OBJ-Datei; die Vorher-/Nachher-Pruefung liest/hash't diese Datei statisch bis zu 20-mal pro Inspektionsaufbau (4,388,621 Byte je Pass), zusaetzlich zu getrennten Beschreibungs-/Modellaufbereitungen. Das ist Dateilesepfad-/Aufrufzaehlung, keine gemessene Platten-I/O. `ModelInspection` haelt Daten fuer seinen Snapshot; eine gemeinsam geladene Physikmodell-Cache ueber Katalog-/Inspektionsaufrufe ist im Pfad nicht sichtbar. Diese Arbeit ist nicht Teil von `simulieren()` selbst. | Modellbrowser/Vorschau/Inspektion; getrennt von Integrationszeit, kann aber GUI-Lade-/Bedienzeiten beeinflussen. JSON-Meshantworten kopieren/serialisieren selektierte Meshes. | `Code/src/katalog.cpp:121-172, 189-225`; `Code/src/inspektion.cpp:40-55, 700-748, 779-840, 996-1018, 1129-1138`; C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`. | Core-Katalog-/Inspektionszeit **nicht gemessen**; Benchmark deckte nur `lade_modell()`, dynamische API und `simulieren()` ab. Die GUI-Messung ist separat in G-0.10 §13.7-13.8 dokumentiert und kein Core-Inspektionsprofil. | **Hoch** fuer wiederholte Katalog-/Inspektionsstarts, falls die GUI dieselben Snapshots neu anfordert; keine Aussage zur Core-Zeitschrittzeit. | **Niedrig bis mittel** fuer exakt invalidierte, unveraenderliche Snapshots; stale Daten, Pfad-/Geometrieaenderungen und Diagnoseverhalten duerfen nicht verloren gehen. |
| C-0.8 fuehrt einen iterativen Hydrostatik-Gleichgewichtssolver ein; Residuen schneiden alle Volumenmeshes wiederholt. Der stationaere Kurvenfahrt-Solver hat zusaetzlich Start-/Fallback-Suchen, finite-difference-Jacobi und Liniensuche. Das sind legitime Kosten der statischen Gleichgewichtssuche, keine zusaetzlichen dynamischen Schritte und kein Beleg fuer dynamische Physikparitaet. | Statisch/stationaer: mehrere Kraft-/Schnittauswertungen je Solve. Dynamischer Zeitschritt ruft diese Solver nicht auf. | `Code/src/hydrostatik.cpp:995-1148, 1175-1590`; C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`. | **Nicht gemessen** (ausserhalb des dynamischen Benchmarks); diese Solver werden im gemessenen Dynamikpfad nicht aufgerufen. | **Hoch** innerhalb statischer Solverlaeufe; nicht auf dynamische Integration uebertragbar. | **Hoch**: weniger Iterationen, grobere Schnitte oder Abbruch-/Seed-Aenderungen koennen Loesung, Konvergenz und Flutungsstatus aendern. |
| Ergebnisserialisierung rechnet den gelieferten Zustand erneut aus: `zustand_json()` ruft `kraefte()` und `bilanz()` auf und schreibt Beitraege, Volumina und Summen; danach wird der Lateraldruckpunkt aus Kraftbeitraegen abgeleitet oder bei fehlenden verwendbaren Beitraegen geometrisch erneut aus Nasspanels bestimmt. Dynamische Laeufe schreiben initial, im Intervall und final; `ausgabe_intervall` ist standardmaessig 1. | Beide Rechenarten pro Ergebnisdatensatz; dynamisch haeufig pro Zeitschritt, statisch normalerweise ein Datensatz. | `Code/src/simulation.cpp:84-136, 180-250, 483-486, 529-684`; `Code/src/hydrostatik.cpp:2259-2317`; Default in `Code/include/segelphysik/simulation.hpp:9-25`. V5/V6-Ausgabe: C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`; Fremdkraft-Kontext: C-0.9 `3fa197135a0858ee525d3e4bc8273242e0ee97c2`. | Gemessen: einzelne V6-Serialisierung am 420er 5.97/7.11/6.93 ms fuer C-0.7/0.8/0.9 und Folkeboot 0.346/0.334/0.352 ms; CLI-Gesamtlauf 2.532/3.040/3.005 s fuer 51 Zustaende und 1.51 MB (420). JSON allein ist von Dateischreiben getrennt; volle Matrix in 4.3. Ausgabeintervall und Dateischreiben getrennt erfassen. | **Mittel bis hoch** bei dichter Ausgabe; nur exakt im selben Zustand wiederverwendbare Kraft-/Bilanzdaten sind Kandidat. | **Mittel bis hoch**: Auslassen oder Wiederverwenden eines nicht passenden Snapshots kann Kraftbilanz, Flutung, Kontext und historisches Ergebnis verfalschen. |
| C-0.8 fuehrt `bewegtes_modell()` in `kraefte()` ein: jede Kraftauswertung kopiert das Modell samt Meshdaten, transformiert bewegte Volumen-/Massenpunkte, rotiert Elementtraegheiten und aggregiert Masse/Schwerpunkt/Traegheit. Der dynamische Integrator nutzte zu diesem Zeitpunkt fuer seine eigene Beschleunigungs-/CG-Fortschreibung noch das Grundmodell; Kraftgeometrie und Integratortraegheit waren damit nicht vollstaendig konsistent. | Statisch: Bewegungsmodell bei entsprechenden Kraftauswertungen. Dynamisch: einmal pro `kraefte()`-Auswertung, einschliesslich Integratorstufen, Endpruefung und Ausgabe. | `Code/src/hydrostatik.cpp:77-126, 2035-2145` am C-0.8-Stand; eingefuehrt durch `d6d57c7dc5084dc3599f7208b3c63c35446b3345`. | Gemessen am identischen archivierten 0.1-420er: 50 Schritte ohne Zusatzhydrodynamik median 87.5 ms (C-0.7) -> 168.2 ms (C-0.8), mit Testantrieb 132.5 -> 222.2 ms. Beim Folkeboot free 0.845 -> 2.744 ms, drive 1.279 -> 2.875 ms. Die Versionsmehrzeit ist gemessen, umfasst aber weitere C-0.8-Aenderungen; kein isolierter Funktionsanteil von `bewegtes_modell()`. | **Hoch** fuer dieselbe unveraenderte Stellung innerhalb eines Rechenfalls; ein moeglicher Funktionsanteil ist nicht isoliert gemessen. | **Hoch**: Bewegung ohne passende Integrator-Masse/Traegheit ist selbst ein Physikproblem; kein Weglassen der bewegten Kraftgeometrie. |
| C-0.9 fuehrt `physisches_modell_fuer()` ein und stellt im `dynamikschritt()` einmal zusaetzlich das physische Modell fuer Masse, Schwerpunkt und Traegheit her; `kraefte()` bereitet es weiterhin je Kraftauswertung vor. Bei festen Stellungen wird dieselbe Geometrie damit fuer Integrator und Kraefte konsistent, aber ohne Cache je Aufruf erneut kopiert/aufbereitet. | Vor allem dynamisch: zusaetzlich eine Aufbereitung im Schritt; pro Schritt weiterhin zwei Integrator-Kraftauswertungen plus bei Hydrodynamikkontext eine Endzustandspruefung. Die Ergebnisaufzeichnung ruft `kraefte()` je Datensatz erneut auf. Fuer Modelle mit Bewegungsmetadaten und Ausgabeintervall 1 ergeben sich aus dem Kontrollfluss je Schritt eine Integratoraufbereitung plus zwei `kraefte()`-Aufbereitungen; Hydrodynamik-Endpruefung und positiver aktiver Testantrieb fuegen je eine hinzu. Damit sind es je nach Fall 3N, 4N oder 5N Schrittaufbereitungen plus N+1 Ausgabestaende. Hydrodynamik-/Testantriebs-/Fremdkraftstart rufen weitere Aufbereitungen ausserhalb der Zeitschrittschleife auf. Dies sind statische Kontrollflusszahlen; der Harness hat keine dynamischen Zaehler. | `Code/src/hydrostatik.cpp:77-164, 2136-2147, 2333-2445, 2480-2500`; C-0.9 `3fa197135a0858ee525d3e4bc8273242e0ee97c2`. | Gemessen C-0.8 -> C-0.9 fuer 50 Schritte: archivierter 420 hydro+Testantrieb 1.182 -> 1.525 s; Folkeboot 14.93 -> 20.16 ms; aktuelle Schema-3-Modelle 1.221 -> 1.554 s und 17.92 -> 23.14 ms. Zeitbereiche sowie weitere Versionseffekte siehe 4.3; nicht als isolierte Cachekosten interpretieren. Aufrufzahlen sind aus dem Kontrollfluss abgeleitet, keine Messungen. | **Hoch** als genau eingegrenzte Wiederverwendung desselben unveraenderten Stellungs-/Modellzustands. | **Hoch**: veraltete Meshpositionen oder falsche Massen-/Schwerpunkt-/Traegheitsbezuge aendern Physik; Bewegungsereignisse und mitbewegte Elemente muessen die Cachegrenze bilden. |
| Je Kraftauswertung schneidet Hydrostatik Volumenmesh-Dreiecke; Hydrodynamik erstellt fuer Rollenmeshes erneut nassbenetzte Panels. Fuer 420 sind 93,556 Volumen- und 69,096 hydrodynamische Quell-Dreiecke betroffen (vor Ableitung), fuer Folkeboot 410 bzw. 410. Rumpfmesh dominiert 420. Das sind physikalisch relevante Mesheingaben, nicht bloss Darstellung. | Statisch: je Residuum/Kraftauswertung. Dynamisch: Anfangs- und Mittelpunktkraft, hydrodynamische Endzustandspruefung und Ausgabeauswertung. Bewegte Hydrodynamikrollen erfordern zusaetzlich Meshtransformation. | `Code/src/hydrostatik.cpp:234-274, 784-878, 2136-2225`; `Code/src/hydrotraegheit.cpp:81-190`; Meshinventar 4.1. Grundpfade bereits C-0.7 `ebca994770b216a749e18917d2ae387de1612dff`; Umfeld C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`, C-0.9 `3fa197135a0858ee525d3e4bc8273242e0ee97c2`. | Die 6,001-Stuetzstellen-Aufbereitung beim `hydrodynamik_start()` ist nicht separat gestoppt. Gemessen wurde die anschliessende 50-Schritt-Hydrodynamik: auf identischem 0.1-420er 0.946/1.145/1.181 s fuer C-0.7/0.8/0.9 ohne Testantrieb; mit Testantrieb 0.995/1.182/1.525 s. Das sind Gesamtzeiten der Integrationsschleife, kein reiner Zusatzmassenanteil. | **Hoch**, sofern Profile den Dreiecksschnitt als Zeitanteil bestaetigen; Modelle getrennt betrachten. | **Hoch** bei Geometrievereinfachung, culling oder geringerer Schnittgenauigkeit; niedriger bei exakt pose-/konfigurationsgebundener Wiederverwendung. |
| Zusatzmasse/Daempfung ist bereits C-0.7-Bestand und daher fuer sich keine C-0.8/0.9-Regression. Startaufbereitung durchlaeuft Rumpfmesh/Wasserlinie; die Ellipsoid-Zusatzmasse integriert mit 6,001 Stuetzstellen je Aufruf, gemeinsam fuer die drei Achsen. C-0.9 berechnet sie bei Bewegungsereignissen neu, nicht bei unveraenderten festen Stellungen je Schritt. Der stationaere Solver kann sie innerhalb seiner Iterationen wiederholt aufrufen. | Dynamisch typischerweise beim Start und nach geaenderter Stellung; statischer Kurvenfahrtssolver iterativ. | `Code/src/hydrotraegheit.cpp:10-28, 81-190`; Aufrufstellen `Code/src/hydrostatik.cpp:1328, 1384, 1558, 2076, 2404-2410`. Zusatzmassen: C-0.7 `ebca994770b216a749e18917d2ae387de1612dff`; bewegungsabhaengige Erneuerung: C-0.9 `3fa197135a0858ee525d3e4bc8273242e0ee97c2`. | **Nicht gemessen.** 6,001 ist ein statischer Schleifenwert, keine Laufzeit. | **Mittel** fuer Start/Ereignis; als Ursache fuer jeden Schritt nicht belegt. | **Hoch**: Pose, Wasserlinie, Benetzung, Schwerpunkt, Stellung und Fluidparameter bestimmen das Ergebnis. |
| C-0.9 fuegt die dynamische Fremdkraft als Pose-Transformation, Hebelarmmoment, Bilanzbeitrag und Kontextfelder im JSON hinzu. Der Zusatzaufwand ist konstant je Kraftauswertung und klein gegenueber Meshedurchlaeufen; Kontextfelder werden pro Ergebniszustand geschrieben. | Dynamisch; keine statische Aktivierung. Integrator wertet Anfang und Mittelpunkt aus, Ergebnisaufzeichnung berechnet den gelieferten Zustand separat neu. | `Code/src/hydrostatik.cpp:2227-2252, 2333-2480`; `Code/src/simulation.cpp:104-118, 434-449, 556-658`; C-0.9 `3fa197135a0858ee525d3e4bc8273242e0ee97c2`. | C-0.9-only, aktuelle Schema-3-Modelle, 10 N modellfest am Schwerpunkt, ohne Zusatzhydrodynamik; Kontextstart ausserhalb der Integrationszeit: 50 Schritte 420 253.0 ms [226.6-261.7] gegen free 239.4 [188.3-246.6], Folkeboot 3.43 [1.86-4.31] gegen 2.36 [1.49-2.56] ms. Bereiche ueberlappen; anderer Kraftzustand, kein isolierter kausaler Aufpreis. | **Niedrig** gegenueber geometriemengenabhaengigen Befunden. | **Hoch** fuer Frame, Momentbezug und Bilanz; Vereinfachung nur unter Erhalt des Vertrags. |

**Einordnung:** Ein dynamischer Schritt ist physikalisch nicht mit genau
einer statischen Kraftauswertung gleichzusetzen. Zwei Integrator-
Kraftauswertungen, hydrodynamische Endzustandspruefung und explizite
Zustandssnapshots sind nachvollziehbare dynamische Mehrarbeit. Die
statische Gleichgewichts-/Kurvenfahrtsuche darf umgekehrt nicht in den
dynamischen Schritt hineingerechnet werden. Eine Statik-Dynamik-Paritaet
der noch offenen Rotations-, Bewegungs-, Flutungs- und
Integrationskonvergenzfaelle ist mit dieser Laufzeitanalyse nicht
nachgewiesen.

### 4.3 Messungen, Methodik und Grenzen

**Status:** GUI-Matrix `gui-perf-final3` ist abgeschlossen; Details und
GUI-Rohmessungen stehen im G-0.10-Plan, Abschnitte 13.7-13.8. Nach deren
Abschluss wurde der Core-Messslot ausdruecklich freigegeben und am
03.11.2026 genutzt. Die voneinander getrennten Laeufe konkurrierten nicht.
Keine Produktionsdatei oder Modelldatei wurde fuer die Messung geaendert.

Alle drei Core-Staende wurden aus getrennten Quellstaenden gebaut und
isoliert gemessen: C-0.7 `ebca994770b216a749e18917d2ae387de1612dff`,
C-0.8 `d6d57c7dc5084dc3599f7208b3c63c35446b3345`, C-0.9
`3fa197135a0858ee525d3e4bc8273242e0ee97c2`. Einheitlich: Visual Studio
17 2022 x64, MSVC 19.44.35229, Windows SDK 10.0.26100.0, `Release`,
AMD Ryzen 5 8600G (6 Kerne/12 logische Prozessoren); Messungen
seriell/unpaced. Testinstrumentierung war ausgeschaltet. Je Zelle ein
Warmlauf und fuenf Messlaeufe; Tabellen zeigen Median und Min-Max-Spanne.

Die direkte API-Messung stoppt nur 50 Aufrufe `dynamikschritt()` mit
`dt=0,01 s` (Horizont 0,5 s); Laden und Zustands-/Kontextaufbau liegen
ausserhalb dieses Zeitfensters. Faelle: `free` ohne dynamischen Zusatz-
kontext; `drive` mit gemeinsamem Testantrieb-Default (200 N); `hydro` mit
Default-Hydrodynamikkontext; `hydro_drive` mit beiden. C-0.9 zusaetzlich:
aktive 10-N-Fremdkraft modellfest am Schwerpunkt, ohne Hydrodynamik oder
Testantrieb. Der Start des jeweiligen Kraftkontexts liegt ausserhalb der
gestoppten Integrationsschleife. Die `ms/Schritt` und Schritte/s lassen
sich aus den 50-Schritt-Zeiten ableiten; keine Echtzeit-Wartezeit.

Eine eigene Messung wiederholt `lade_modell()` fuenfmal und stoppt
`zustand_json()` fuer je einen fertigen Zustand (V6, mit erneuter
Kraftauswertung) fuenfmal. Der CLI-Gesamtlauf umfasst Laden,
Integrationsschritte, JSONL-Serialisierung und Dateischreiben gemeinsam:
`--dynamisch MODELL AUSGABE 0.5 0.01`, Standard-Ausgabeintervall 1,
51 Zustaende einschliesslich Start und Ende, ohne Testantrieb und ohne
angeforderten Hydrodynamikkontext. Er ist keine reine Integrationszeit.
Die identischen aktuellen Schema-3-Modelle sind mit C-0.8/C-0.9 direkt
vergleichbar. C-0.7 weist sie bereits bei `--pruefen` mit
`Unbekanntes Feld: geometrie` ab; dafuer gibt es keinen Messwert. Der
Dreiversionslauf verwendet deshalb zusaetzlich und getrennt die exakten
archivierten `0.1-entwurf`-Modelldateien aus 4.1.

**Direkte Integrationszeit, Median [Min-Max] ms je 50 Schritte:**

| Modelldaten | Modell | Fall | C-0.7 | C-0.8 | C-0.9 |
|---|---|---|---:|---:|---:|
| Archiv `0.1-entwurf` | 420 | free | 87.55 [86.94-87.87] | 168.24 [147.75-189.67] | 221.39 [173.92-247.99] |
| Archiv `0.1-entwurf` | 420 | drive | 132.47 [130.68-133.13] | 222.22 [216.73-279.72] | 271.91 [262.10-342.70] |
| Archiv `0.1-entwurf` | 420 | hydro | 946.23 [934.06-961.52] | 1,145.00 [1,094.03-1,210.16] | 1,181.49 [1,174.71-1,223.64] |
| Archiv `0.1-entwurf` | 420 | hydro_drive | 995.46 [987.11-999.26] | 1,182.11 [1,159.72-1,248.71] | 1,525.22 [1,509.70-1,598.06] |
| Archiv `0.1-entwurf` | Folkeboot | free | 0.845 [0.840-0.847] | 2.7438 [1.9074-2.8858] | 2.6510 [1.7416-3.1751] |
| Archiv `0.1-entwurf` | Folkeboot | drive | 1.2788 [1.2749-1.3005] | 2.8754 [2.1453-3.5264] | 3.3977 [2.0051-4.0554] |
| Archiv `0.1-entwurf` | Folkeboot | hydro | 15.567 [14.556-15.679] | 15.437 [14.528-18.438] | 15.016 [14.700-19.432] |
| Archiv `0.1-entwurf` | Folkeboot | hydro_drive | 16.021 [15.137-16.879] | 14.929 [14.687-19.399] | 20.162 [19.665-24.808] |
| Schema 3 | 420 | free | inkompatibel | 179.64 [142.45-197.60] | 239.38 [188.32-246.58] |
| Schema 3 | 420 | drive | inkompatibel | 236.39 [224.79-290.49] | 258.52 [247.43-340.78] |
| Schema 3 | 420 | hydro | inkompatibel | 1,113.48 [1,104.07-1,162.96] | 1,142.29 [1,135.06-1,224.24] |
| Schema 3 | 420 | hydro_drive | inkompatibel | 1,221.16 [1,175.37-1,294.26] | 1,553.53 [1,521.95-1,676.23] |
| Schema 3 | Folkeboot | free | inkompatibel | 2.5527 [2.3257-3.1527] | 2.3609 [1.4924-2.5633] |
| Schema 3 | Folkeboot | drive | inkompatibel | 3.1056 [2.5400-3.9605] | 4.5295 [2.2123-4.8049] |
| Schema 3 | Folkeboot | hydro | inkompatibel | 17.369 [16.876-19.859] | 17.768 [16.266-22.292] |
| Schema 3 | Folkeboot | hydro_drive | inkompatibel | 17.917 [16.842-21.146] | 23.137 [22.051-30.568] |

**Modellladen / Einzelzustand-JSON, Median [Min-Max] ms:**

| Modelldaten / Modell | Messung | C-0.7 | C-0.8 | C-0.9 |
|---|---|---:|---:|---:|
| Archiv 420 | `lade_modell()` | 1,153 [1,135-1,250] | 1,156 [1,097-1,217] | 1,145 [1,103-1,310] |
| Archiv 420 | ein `zustand_json()` | 5.973 [5.949-6.044] | 7.113 [7.018-7.482] | 6.927 [6.853-7.094] |
| Archiv Folkeboot | `lade_modell()` | 5.522 [5.464-5.634] | 5.553 [5.516-5.582] | 5.737 [5.552-5.901] |
| Archiv Folkeboot | ein `zustand_json()` | 0.346 [0.326-0.441] | 0.334 [0.328-0.350] | 0.352 [0.347-0.406] |
| Schema-3-420 | `lade_modell()` | inkompatibel | 1,090 [1,046-1,316] | 1,160 [1,055-1,291] |
| Schema-3-420 | ein `zustand_json()` | inkompatibel | 7.011 [6.954-7.134] | 7.078 [7.028-7.157] |
| Schema-3-Folkeboot | `lade_modell()` | inkompatibel | 5.232 [5.140-5.287] | 5.169 [5.136-5.243] |
| Schema-3-Folkeboot | ein `zustand_json()` | inkompatibel | 0.334 [0.327-0.340] | 0.372 [0.366-0.384] |

**CLI-Gesamtlauf: Median [Min-Max] s; 51 Zustaende, Median-JSONL-Bytes:**

| Modelldaten / Modell | C-0.7 | C-0.8 | C-0.9 |
|---|---|---|---|
| Archiv 420 | 2.532 [2.510-2.553], 1,512,830 B | 3.040 [2.959-3.393], 1,515,654 B | 3.005 [2.982-3.148], 1,515,915 B |
| Archiv Folkeboot | 0.0640 [0.0630-0.0704], 1,386,966 B | 0.0751 [0.0707-0.0877], 1,390,310 B | 0.0784 [0.0777-0.0802], 1,390,455 B |
| Schema-3-420 | inkompatibel | 2.930 [2.914-2.953], 1,516,113 B | 2.940 [2.927-2.942], 1,516,349 B |
| Schema-3-Folkeboot | inkompatibel | 0.0710 [0.0682-0.0717], 1,392,188 B | 0.0730 [0.0712-0.0749], 1,392,187 B |

C-0.9-only-Fremdkraftmessung (10 N am Schwerpunkt, Schema 3,
50 Schritte): 420 Median 253.01 ms [226.65-261.74], gegen `free`
239.38 [188.32-246.58]; Folkeboot 3.432 ms [1.861-4.308], gegen
`free` 2.361 [1.492-2.563]. Bereiche ueberlappen. Das ist ein
End-to-End-Schrittvergleich bei veraenderter Kraft und kein isolierter
Fremdkraft-Aufpreis; C-0.7/C-0.8 besitzen diesen Vertrag nicht.

**Ursacheneinordnung:** Die groesste beobachtete Modellabhaengigkeit ist
der 420er: das Laden liegt bei rund 1.1 s und der CLI-Lauf mit
51 Ausgaben bei rund 2.9-3.0 s; Folkeboot-Laden liegt bei rund 5 ms
und der CLI-Lauf bei rund 0.07 s. Beim identischen archivierten
0.1-420er steigt der direkte `free`-Schrittmedian von C-0.7 zu C-0.8
von 87.5 auf 168.2 ms und `drive` von 132.5 auf 222.2 ms. Das ist
konsistent mit zusaetzlicher bewegter Modell-/Meshaufbereitung, aber
nicht kausal nur `bewegtes_modell()` zuzuschreiben; C-0.8 enthaelt
weitere Core-Aenderungen. Von C-0.8 zu C-0.9 steigt `hydro_drive` beim
identischen 420er von 1.182 auf 1.525 s (Archiv) bzw. 1.221 auf
1.554 s (Schema 3); beim Folkeboot von 14.93 auf 20.16 ms (Archiv)
bzw. 17.92 auf 23.14 ms (Schema 3). Das passt zur zusaetzlichen
physikalischen Modellaufbereitung pro Schritt, belegt ohne Messzaehler
aber keinen exklusiven Kostenanteil. Dagegen bleiben Schema-3-CLI-
Gesamtmediane C-0.8/C-0.9 nahe beieinander: 420 2.930/2.940 s,
Folkeboot 0.0710/0.0730 s. Laden und Ergebnisaufzeichnung ueberdecken
hier offenbar kleinere Integratorunterschiede.

In den archivierten 0.1-Laeufen stimmen die ausgegebenen Endwerte von
Position X/Z und Betrag der Geschwindigkeit zwischen C-0.7 bis C-0.9
bei denselben vier Testfaellen bis auf etwa 2e-15 m bzw. 1.3e-14 m/s
ueberein; fuer Schema 3 stimmen die C-0.8/C-0.9-Endwerte ebenso nahe
ueberein. Das ist nur ein 0.5-s-Kurzlauf der benannten Komponenten,
keine Vollparitaet, Stabilitaets- oder Physikabnahme; die archivierten
Eingaben sind ausdruecklich nicht simulationsfreigegebene Entwuerfe.

**Nicht gemessen / Grenzen:** Core-Katalog-/Inspektionspfad,
Platten-I/O getrennt vom `lade_modell()`-Aufruf, reine Dateischreibzeit,
statische Gleichgewicht-/Kurvenfahrt-Solver, kalte Betriebssystem-
Dateicaches, lange Horizonte, dynamische Aufruf-/Cache-Zaehler und
tatsaechliche Nasspanel-/abgeleitete-Dreieckszahlen je Schritt. Der
6,001-Stuetzstellen-Aufbau der Zusatztraegheit wurde nicht als eigener
Zeitblock gestoppt; `hydro`-API-Zeiten umfassen nur die darauffolgende
Schrittfolge. Fuer C-0.7 wurde kein aktuelles Schema-3-Ergebnis
erzeugt. Die Messdaten schliessen keine der offenen Rotations-,
Bewegungs-, Flutungs-, Frame- oder Integrationskonvergenzpruefungen.

Die kompakten reproduzierbaren Belege sind im Core-Repository unter
[Nachweise/Performance_Core_2026-11-03](Nachweise/Performance_Core_2026-11-03/)
abgelegt: Manifest, alle API-Roh-/Normdaten, CLI-Rohdaten,
Modellinventar, abgeleitete Zusammenfassung und Buildmetadaten. Die
SHA-256-Werte der Manifestdateien wurden gegen die abgelegten Dateien
geprueft. Grosse einzelne alte 420/Folkeboot-CLI-Beispiele
(`legacy-420-c07.jsonl`, `legacy-folk-c07.jsonl`) sowie Build- und
Quellverzeichnisse wurden bewusst nicht uebernommen. Keine Messung
autorisiert Codeaenderungen oder Optimierungen; deren moegliche Einsparung
bleibt qualitativ einzuschaetzen und muesste nach separater Freigabe mit
Physik-/Regressionstests abgesichert werden.

## 5. Aktueller Stand

U-C-000112 bis U-C-000114 und die technischen Core-Mindesttests sind
abgeschlossen. Release-Build: `build\standard`, Konfiguration Release.
Gezielte CTest-Faelle `hydrodynamik_traegheit` und
`simulation_gesamtlauf` bestanden. Im vollstaendigen Core-CTest-Lauf
bestanden nach dem Integrator-Fix auch die Hydrodynamik-Referenzen;
19/21 Core-CTest-Faelle bestanden. Die beiden Fehler wurden am
03.11.2026 mit unveraenderten Quellen von HEAD
`d6d57c7dc5084dc3599f7208b3c63c35446b3345`, denselben aktuellen
Modelldaten, demselben Compiler und Release-Konfiguration in einem
getrennten Build reproduziert. Das war eine begrenzte
Regressionsabgrenzung, kein historischer Performancevergleich:

- `modellkatalog_schnittstelle`: `Darstellungsgeometrie weicht ab`.
  `katalog_tests.cpp:123-126` vergleicht den geladenen Quader mit der
  fest kodierten alten Datei `meshes\gui\rumpf.obj`. Der aktuelle Quader
  referenziert dagegen `geometrie.datei = boot.obj` und
  `Darstellung.mesh = element_mesh_1`; die alte Datei existiert nicht.
- `modell_geometrie_v2`: `CORE-MOD-0002`, Element
  `Schwert.volumen`: `Unbekanntes Feld: ableitung`. Der Test konvertiert
  den aktuellen Optimisten mit dem alten Schema-2-Konverter, dessen
  Ergebnis die Schema-3-Ableitung `z_schnitt` im Volumenblock behaelt.
  Bereits der unveraenderte HEAD lehnt diese Schema-2-Datei identisch ab.

Beide Fehler sind damit bestehende Test-/Modelldaten-
Kompatibilitaetsprobleme, keine Regression der dynamischen Fremdkraft.
Keine Originalmodelle oder fachfremden Konverter-/Katalogtests wurden
geaendert. Sie werden nicht als erfolgreich ausgewiesen.
GUI-Adapter-Integration und Nutzerabnahme sind
separat; GUI-Messungen und Core-Messvergleich sind abgeschlossen und
referenziert. Jede weitergehende Implementierung und Optimierung bleibt
BLOCKIERT bis zu einem separaten ausdruecklichen Auftrag. Kein Commit,
Tag, Push oder Release beauftragt.
