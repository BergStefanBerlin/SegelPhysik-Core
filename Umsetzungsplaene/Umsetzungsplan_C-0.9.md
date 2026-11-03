# Umsetzungsplan Core: dynamische Fremdkraft und Versionsvergleich

**Status:** C-0.9 Core-Mindestumfang technisch umgesetzt und gezielt
geprueft; Auftrag vom 03.11.2026, 18:43 Uhr. Persoenliche GUI-Abnahme offen.
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

**Verbindlicher Haltepunkt, Nutzerpraezisierung 03.11.2026, 18:42 Uhr:**
Zunaechst nur die fuer den GUI-Umbau notwendigen korrekten
Core-Mindestanpassungen und technischen Integrationsnachweise liefern.
Sobald die GUI zur persoenlichen Pruefung bereitsteht, die Umsetzung
stoppen. Historischer Performancevergleich, weitergehende Optimierungen
und weitere Umsetzung erst nach erneuter Nutzerfreigabe. Kein
automatisches Weiterarbeiten nach technischem Funktionsgate.

Zuerst die minimal erforderliche korrekte Core-Anpassung fuer dieselben
physikalischen Eingaben in Statik und Dynamik liefern. Danach gemeinsam
mit der neuen GUI ordentlich testen und erst anschliessend die
historische Performance vergleichen. Keine Windphysik, kein neuer
Integrator, kein versteckter Gleichgewichtsstart und keine vorgezogene
Optimierung auf Kosten korrekter Physik.

Der Nutzer hat den begrenzten Core-Mindestvertrag in Spezifikation
Abschnitt 8 freigegeben. Kraftvektor und Ansatzpunkt sind modellfest;
die GUI-Kamerawahl ist kein Core-Kraftframe. Gleichzeitige aktive
Testantriebs- und Fremdkraftkontexte werden abgewiesen. Die API ist vor
der Implementierung mit CPPI-0105 bis CPPI-0112 registriert. Die
projektweiten maxima waren A-C-00125 und U-C-000111; die neuen Kennungen
wurden fortlaufend vergeben: A-C-00126 bis A-C-00128 und
U-C-000112 bis U-C-000114. Weitergehende Matrix, GUI-Abnahme und
Performancevergleich sind nicht freigegeben.

## 2. Geordnete Arbeitspakete

| Umsetzungsschritt | Anforderung | Voraussetzung | Ergebnis und Abschlusskriterium | Status / Nachweis |
|---|---|---|---|---|
| U-C-000112: Dynamischen Eingangsvertrag liefern | A-C-00126 | Freigegebener Mindestvertrag; CPPI-0105 bis CPPI-0109 und CPPI-0111 registriert | Unveraenderliche Konfiguration, Startvalidierung, feste Bewegungsstellungen vor dem ersten Zustand und Batch-Eingang; Altaufrufe ohne Kontext unveraendert. | umgesetzt; `hydrodynamik_traegheit` und `simulation_gesamtlauf` |
| U-C-000113: Bewegtes physisches Modell konsistent integrieren | A-C-00128 | U-C-000112 | Kraftauswertung, Schwerpunkt, Traegheit und Modellursprung verwenden dieselben festen Stellungen. Physischer Massen-/Traegheitstest bestaetigt die Korrektur. | umgesetzt; `hydrodynamik_traegheit` |
| U-C-000114: Fremdkraft in Integrator und Ausgabe integrieren | A-C-00127 | U-C-000112; U-C-000113; CPPI-0110 und CPPI-0112 registriert | Kraft und Moment wirken in allen tatsaechlichen Kraftauswertungen; aktiver Null-/Inaktivfall, Beitrag/Summen/Snapshot und Testantrieb-Konflikt werden gezielt geprueft. | umgesetzt; `hydrodynamik_traegheit` und `simulation_gesamtlauf` |
| Weitergehende Statik-zu-Dynamik-Matrix | IDEE-C-00092, nicht finalisiert | Erneute Nutzerfreigabe | Ueber den Mindestvertrag hinausgehende Matrix und Konvergenznachweise. | blockiert |
| GUI-Integration und persoenliche Pruefung | GUI-Plan G-0.10 | Adapteranbindung und Mindest-Core | Echter dynamischer GUI-Lauf; technische Tests und Nutzerabnahme getrennt erfassen. | laufend ausserhalb dieses Core-Auftrags |
| Historischer Core-Performancevergleich | IDEE-C-00092, nicht finalisiert | Persoenliche GUI-Pruefung und erneutes ausdrueckliches Go | Reproduzierbare Versions-/Modellvergleichsreihe gemaess Abschnitt 4. | BLOCKIERT |

Die Quellen 88 bis 92 bezeichnen ausschliesslich die bereits vergebenen
Entwurfspunkte in der verlinkten Liste, nicht neue Anforderungen.

## 3. Meilensteine und Nachweise

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| Vertrag bereit | Core-Mindestumfang freigegeben | Spec Abschnitt 8, A-C-/U-C-Kennungen und CPPI-0105 bis CPPI-0112 registriert; Arbeitsziel C-0.9, kein Release. |
| Mindest-Core korrekt | U-C-000112 bis U-C-000114 implementiert | Kraft-/Momentreferenz, Pose-Transformation, bewegte Masse/Traegheit, Null/inaktiv/Fehler, Snapshots und Batchlauf bestanden. |
| GUI-Integration bereit | Core-Mindesttests bestanden | Parent-Adapterbuild und dynamischer GUI-Lauf; persoenliche Pruefung des Nutzers bleibt separat. |
| Versionsvergleich belastbar | GUI persoenlich geprueft und neues ausdrueckliches Go | Vergleich nur nach erneutem Auftrag. Bis dahin BLOCKIERT. |

Gezielt pruefen: Schwerpunktangriff ohne Moment, bekanntes Kreuzprodukt,
gekraengte Pose, wirkliche Translation/Rotation, dt/dt/2/dt/4,
0/+-12/+-24/+-36-Grad-Referenzfamilie soweit fuer den jeweiligen
Fall definiert, Flutung sowie Datenfehler. Ein stationaerer Referenzstart
ist nur ein Test, kein neuer Standardstart und kein Stabilitaetsbeweis.
Bestehende thematisch betroffene Core-Tests nutzen; Gesamtregression
erst bei Hinweisen auf breitere Auswirkungen.

## 4. Historischer Performancevergleich, erst nach Integration

**Zusaetzliches Eintrittsgate:** persoenliche Pruefung der umgebauten GUI
durch den Nutzer und anschliessendes ausdrueckliches Go. Bis dahin
diesen Vergleich nicht ausfuehren.

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
separat; der Versionsvergleich und jede weitergehende Optimierung bleiben
BLOCKIERT bis zur persoenlichen GUI-Pruefung und erneutem ausdruecklichen
Go. Kein Commit, Tag, Push oder Release beauftragt.
