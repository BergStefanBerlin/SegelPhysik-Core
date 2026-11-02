# Umsetzungsplan C-0.8: Modellinspektion und stationaeres Fahrtgleichgewicht

## Korrekter LDP und Ersatz-SDP

**Status:** Spezifikation am 02.11.2026 finalisiert und Umsetzung
ausdrücklich freigegeben; Core-Implementierung und gezielte technische
Abnahme abgeschlossen. Die gemeldeten GUI-Integrationstests sind grün;
EXE-Link, Smoke-Test und Nutzerabnahme stehen aus.
Ausschließliche fachliche Grundlage:
[Spezifikation Lateraldruckpunkt, Abschnitt 5](../Spezifikationen/Spezifikation_Lateraldruckpunkt.md#5-c-08-korrekter-ldp-und-ersatz-sdp).
Dieser Abschnitt erweitert den einzigen C-0.8-Plan; C-0.7 wird nicht
nachträglich geändert. Die GUI bleibt außerhalb dieses Core-Auftrags.

### Freigegebene Umsetzungsschritte

| U-ID | Anforderung | Ergebnis und Nachweis | Abhängigkeit | Status |
|---|---|---|---|---|
| U-C-000106 | A-C-00123–00125 | Finalisierte Spezifikation, IDEE-zu-A-Zuordnung und Registereinträge CPPI-0101..0104 | keine | abgeschlossen |
| U-C-000107 | A-C-00123 | Panelbilanz Q/B/X/Z für Druck und beide Seitenkraftmethoden, vor Bauteilaggregation und ohne Physikrückwirkung | U-C-000106 | abgeschlossen |
| U-C-000108 | A-C-00124 | Ersatz-SDP-Status und Modellposition aus passendem eingefrorenem stationärem Ergebnis; ohne Lastkontext kein Block, veralteter Snapshot wird abgelehnt | U-C-000106 | abgeschlossen |
| U-C-000109 | A-C-00125 | Additive JSON-Felder, LDP-Version 2, Rückwärtskompatibilität und gleiche Live-/Batch-Serialisierung | U-C-000107, U-C-000108 | abgeschlossen |
| U-C-000110 | A-C-00123–00125 | Analytische Panel-, Schwellen-, Frame-, Pose-, Snapshot- und Legacytests; bestehende Physik-/Bilanzregression und echter 420er-Nachweis; kein expliziter Vorher-nachhervergleich | U-C-000109 | gezielte Tests bestanden; Nachweisgrenze siehe unten |
| U-C-000111 | A-C-00125 | Core-Daten und erwartete Punktpositionen an GUI-Integration übergeben; [GUI-Vertrag, Abschnitt 5](../../GUI/Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md#5-ldp-und-ersatz-sdp-als-punkte-02112026), [Ansichtsfensterkonfiguration, Abschnitt 10](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.9.md#10-darstellung-im-ansichtsfenster-konfigurieren-02112026); keine GUI-Änderung in diesem Schritt | U-C-000110 | GUI-Integrationstests grün; EXE-Link/Smoke ausstehend |

### Meilensteine und Abnahmekriterien

- **Vertrag bereit:** A-/U-Zuordnung und CPPI-Register sind konsistent;
  CPPI-0031 und V6 bleiben erhalten.
- **Panelauswertung bereit:** `+100 N bei x=+1 m` und
  `-80 N bei x=-1 m` ergeben `Q=20 N`, `X=180 Nm`, `x=9 m`.
  Panelbetragssumme und signierte Momente bleiben bei innerer und
  bauteilübergreifender Aufhebung erhalten. Beide Schwellen sind
  exakt und beidseits geprüft. Modell-X-Kräfte verfälschen das
  Seitenkraftmoment nicht.
- **Snapshot bereit:** aktive, inaktive und Nullkraft sowie beide
  Kraftframes sind geprüft; Daten ohne stationären Kontext enthalten
  keinen Ersatz-SDP. Der Ergebnis-Snapshot ist unabhängig späterer
  Eingabeänderungen.
- **Core-Abnahme bereit:** geometrischer Altstatus und kraftbasierter
  LDP sind unterscheidbar; Pose, bewegte Geometrie, beide
  Seitenkraftmethoden inklusive Kennfeld und 420er-Referenz sind
  abgedeckt. Die geforderte Unverändertheit von Kräften, Momenten,
  Solver und Zustandsfolge ist durch bestehende Regressionen abgesichert,
  aber nicht durch einen expliziten Vorher-nachhervergleich nachgewiesen.
  GUI- und Nutzerabnahme bleiben getrennt.

### Tatsächliche technische Nachweise (02.11.2026)

- Standard-Releasebuild mit MSVC 2022 erfolgreich; erzeugte Core-Ausgaben:
  `build/standard/Release/segelphysik_core.lib`,
  `segelphysik_cli.exe` und `segelphysik_konvertieren.exe`.
- Gezielter CTest-Lauf vor dem Test-Nachtrag: **6/6 bestanden**, 40,41 s gesamt:
  `lateraldruckpunkt`, `hydrodynamik_referenzen`,
  `hydrodynamik_referenzmodelle` (einschließlich echtem 420er),
  `stationaere_kurvenfahrt`, `kraft_momentenbilanz` und
  `simulation_gesamtlauf`.
- Der LDP-Test enthält die unabhängige rotierende Panelreferenz mit
  analytischem Q/B/X/Z, das Handbeispiel `+100/-80 N`, Schwellen,
  Frame-/Snapshot-, Legacy- und Vertragsprüfungen.
- Test-Nachtrag: Beide tatsächlichen Seitenkraftmethoden
  (`geschaetzte_endliche_streckungspolare`, `polare_2d`) sind mit einer
  unabhängigen analytischen Referenz im Hydrodynamik-CTest geprüft,
  jeweils für Kiel/Schwert und Ruder. Vollständig getauchter Quader:
  Sehnenlänge 1 m, Dicke 0,02 m, Spannweite 2 m, Z-Versatz 0,25 m.
  Die vier Seitenflächendreiecke haben je 1 m² Fläche und 0,5 m²
  projizierte Fläche; Schwerpunkte bei x=±1/6 m,
  y=±0,01 m und z=0,25±1/3 m. Bei Giergeschwindigkeit 6 rad/s
  und Querfahrt 0 bzw. 0,5 m/s folgen lokale Normalgeschwindigkeiten
  n=-(v_y±1) und Sehnengeschwindigkeitsbetrag 0,06 m/s.
  Für die geschätzte Methode gilt damit der vollständig abgelöste
  Plattenzweig (AR=2, cn=1,136); die konstante Testpolare verwendet
  Cl=0,4 und Gesamt-Cd=0,1. Die erwarteten Panelkräfte werden direkt
  aus diesen Handwerten berechnet, nicht aus Akkumulatoren oder
  abgefragten Panels. Mit f+ und f- an den beiden X-Positionen gilt
  Q=2(f++f-), B=2(|f+|+|f-|), X=(f+-f-)/3 und Z=0,25Q.
  Alle vier Akkumulatoren sind numerisch geprüft (Toleranz 1e-8).
  Sowohl vollständige als auch teilweise Gegenkraftaufhebung
  unterscheiden explizit den alten Betragsschwerpunktfehler.
  Bewegte Geometrie und das echte 420er-Modell bleiben zusätzlich geprüft.
- Frisch gebauter Standard-Release-Test-Nachtrag: **4/4 bestanden**,
  **3,12 s**: `hydrodynamik_referenzen`, `hydrodynamik_referenzmodelle`,
  `lateraldruckpunkt`, `kraft_momentenbilanz`.
- **Nachweisgrenze Physik:** Der frühere Vergleich der Beitragsanzahl
  prüft ausschließlich diese Anzahl. Er belegt weder numerische
  Reproduzierbarkeit noch Physikinvarianz und ist im Test entsprechend
  umbenannt. Bestehende Kraft-/Momentenbilanz-, Simulations- und
  Solvertests sind Regressionen, kein expliziter Vorher-nachhervergleich
  der Kräfte, Momente oder Zustandsfolge. Ein solcher Vergleich wurde
  nicht ausgeführt; die Metadatenimplementierung ändert keine
  physikalischen Berechnungsformeln.
- Ein erster isolierter Ninja-Build scheiterte zunächst an fehlender
  `nlohmann_json`-CMake-Paketregistrierung und nicht initialisiertem
  MSVC-Standardumfeld; mit vorhandener GUI-vcpkg-Abhängigkeit und
  `vcvars64` war der Standardbuild erfolgreich. Kein verbleibender
  Fehler im gezielten Testsatz.
- GUI-Integration laut Auftraggeber mit frisch eingebundenem Core:
  **7/7** Settings-/Parser-/Core-Integration-/Workspace-DPI-Tests
  (148,55 s) sowie final **4/4** verschärfte Rasterprüfungen
  beider Marker bei 100/150/200 % (100,06 s). EXE-Link und Smoke-Test
  sind noch ausstehend und wurden hier nicht ausgeführt.

Die GUI-Integration bezieht sich auf den
[GUI-Vertrag, Abschnitt 5](../../GUI/Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md#5-ldp-und-ersatz-sdp-als-punkte-02112026)
und die
[Ansichtsfensterkonfiguration, Abschnitt 10](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.9.md#10-darstellung-im-ansichtsfenster-konfigurieren-02112026).

### Rückverfolgbarkeit

| Entwurfspunkt | Umsetzung | Nachweis |
|---|---|---|
| IDEE-C-00085 → A-C-00123 | U-C-000107, U-C-000109, U-C-000110 | Q/B/X/Z, Gegenkräfte, Schwellen, Pose und kein Klemmen |
| IDEE-C-00086 → A-C-00124 | U-C-000108–000110 | eingefrorener Snapshot, aktiv/null/inaktiv, beide Frames, kein Kontext |
| IDEE-C-00087 → A-C-00125 | U-C-000106, U-C-000109–000111 | CPPI/JSON/Legacy, Live-/Batch-Parität, bestehende Physikregression (kein Vorher-nachhervergleich) und GUI-Übergabe |

Technische Abnahme, Nutzer-Sichtabnahme und Git-Veröffentlichung bleiben
getrennt. Kein Commit, Push, Tag oder Release beauftragt.

## Stationaeres Fahrtgleichgewicht

### Freigegebene stationaere Dreh- und Kurvenfahrt (C-0.8)

**Status:** Spezifikation Abschnitt 7 am 02.11.2026 finalisiert; Umsetzung
und gezielte technische Abnahme abgeschlossen. Die gerade Solverspur
(U-C-000096 bis U-C-000100) bleibt als
unveraenderter Default erhalten. Neue `kurvenfahrt`-Eingabe ist opt-in und
hat `false` als Standard. Neue Anforderungen IDEE-C-00080..84 sind
A-C-00118..122; neue Umsetzungsschritte U-C-000101..105.

#### Verbindliche numerische Strategie

Der Kurvenmodus loest sechs skalierte Bewegungsresiduen in den sechs
Unbekannten `Z`, Roll, Stampf, `u_x`, `u_y` und signiertem `r`. Die horizontale
wasserrelative Schwerpunktgeschwindigkeit bleibt im bootsfesten Rahmen
konstant; `u_z` wird aus der kinematischen Bedingung `v_S,W,z=0` eliminiert.
Die Pose behaelt den Startkurs als Referenz, weil zugelassene stationaere
Drehfaelle nur mit kursinvarianten Lasten loesbar sind. Vor jeder Iteration wird
eine nichtinvariante horizontale weltfeste Last bei `r != 0` als
`last_invariant` ausgeschlossen; keine phasenabhaengige Einmalbilanz wird als
Kreisloesung ausgegeben.

Numerik: zentrale finite Differenzen, residualweise Skalierung mit den
angeforderten Kraft-/Momenttoleranzen, spaltennormierte gedaempfte
Least-Squares-Suche (Levenberg-Marquardt mit monotoner Trust-Region-
Anpassung), Rangdiagnose und komponentenweise Konvergenz. Suchbereiche relativ
zum Start: Z +/-100 m, Roll/Stampf +/-85 Grad, `u_x/u_y` +/-50 m/s, `r` +/-1 rad/s;
Maximalschritte 0,5 m, 0,1 rad, 5 m/s, 0,05 rad/s. Suchgrenztreffer,
Rangdefizit, Abbruch, Flutung und Stagnation haben eigene Statuswerte. Kein
Erfolg aus kleinem Schritt oder kleinem Merit ohne alle sechs Toleranzen.

Startwertkorrektur nach dem 24-Grad-Nutzerfall vom 02.11.2026:
Wenn die vorgeschaltete Geradeaus-Startwertsuche aus Ruhe nicht konvergiert,
wird eine erfolgreiche hydrostatische Lage als numerischer Startwert
verwendet, statt mit ungetrimmter Identitaetspose weiterzusuchen.
Bei der anschliessenden Fahrtinitialisierung wird die Drehrichtung nicht
allein aus der Seitenkraft geraten: 40 Intervalle im bestehenden
Drehraten-Suchbereich [-1,1] rad/s werden nach Vorzeichenwechseln des
Gier-Bewegungsresiduums um Welt-Z durchsucht. Klammern werden mit maximal
12 Bisektionen verfeinert; unter den so gefundenen Startwerten entscheidet
das vollstaendige skalierte Sechs-Residuen-Merit. Ohne geklammerte Nullstelle
dient die kleinste verfuegbare Gierabweichung als Startwert.
Dies ist keine physikalische Teilkonvergenz und kein globaler
Loesungsbeweis: Erfolg wird weiterhin ausschliesslich anhand aller sechs
Residuen gemeldet. Kraft, Ansatzpunkt und Elementstellungen bleiben bei
saemtlichen Auswertungen unveraendert. Kein Ruder-/Lasthochlauf und keine
geheimen Eingabeaenderungen; explizite bewegte Startzustaende bleiben erhalten.
Abbruch wird auch waehrend der begrenzten Initialsuche geprueft.

#### Verbindlicher Ergebnisvertrag

C++: `StationaereGleichgewichtKonfiguration::kurvenfahrt=false`;
`ErgebnisJsonFormat::{Kompatibel,V6}`; `Laufoptionen::ergebnis_json_format`
standardmaessig `V6`; `zustand_json(..., ErgebnisJsonFormat)` mit Standard
`V6`. `Kompatibel` erhaelt automatische V1-V5-Ausgabe bit-/feldsemantisch;
V5-Nullraten werden niemals umgedeutet. V6 hat die top-level Kursrate und
Status auch in dynamischen Zeilen. Stationaer liefert `loesungstyp`, Modell-
omega, aeussere Summen separat von Bewegungsresiduen, Radius oder null und
Status. `kurvenfahrt` fehlt in alten Eingaben = `false`.

#### Geordnete Umsetzungsschritte

| U-ID | Anforderungen / Ergebnis | Abhaengigkeit | Status |
|---|---|---|---|
| U-C-000101 | A-C-00118, A-C-00121; additive API und V6-/Kompatibilitaetsvertrag mit CPPI-0088..0100 vor Umsetzung registriert, V1-V5 erhalten | finalisierte Spezifikation | implementiert; Defaults, Kompatibilitaetsprojektion und CPPI-Vertrag getestet |
| U-C-000102 | A-C-00119; konstante Relativkinematik und sechs physikalische Bewegungsresiduen einschliesslich Schwerpunktzentripetalbeschleunigung, Rotations-/Fluidzusaetztraegheit, Kreiselterm und feste Elementstellungen | U-C-000101 | implementiert; Kreis- und Kreiselreferenzen sowie alle sechs Residuen bestanden |
| U-C-000103 | A-C-00118, A-C-00120, A-C-00122; begrenzter Kurvenloeser, frameinvariante Lastpruefung, Kinematik- und Fehlerstatus | U-C-000102 | implementiert; positive/negative Rate, geneigte Lage, Rangdefizit, Flutungswiederholung ohne Scheinerfolg, inkompatible Weltlast, ungueltige Eingabe und Abbruch bestanden |
| U-C-000104 | A-C-00121; dynamische signierte Kursrate und V6-JSON, V5/V1-V4-Kompatibilitaetsregression | U-C-000101, U-C-000103 | implementiert; dynamischer V6-Default und explizite Kompatibilitaetsprojektionen bestanden |
| U-C-000105 | A-C-00118..00122; analytische Gerade-/Kreis-/Gyro-/Frame-/Vorzeichen-/Tilt-/Dynamik-/Fehler-/Legacytests; gezielter CTest | U-C-000102..000104 | implementiert; gezielter CTest 6/6 bestanden |

#### Meilensteine und Rueckverfolgbarkeit

- **Vertrag:** U-C-000101; vor Implementierung alle oeffentlichen APIs und
  CPPI-IDs festgelegt, numerische Strategie und GUI-Feldnamen eindeutig.
- **Physik:** U-C-000102/103; analytische Kreis- und Gyroreferenzen, lokale
  Rotationsanstroemung, unveraenderte Stellungen sowie Weltlast-Inkompatibilitaet
  geprueft; sechs Residuen innerhalb 0,1 N/Nm.
- **Lieferung:** U-C-000104/105; Referenzfehler Drehrate <=0,01 Grad/s,
  Roll/Pitch <=0,1 Grad, Schwerpunkt-Vz <=0,001 m/s, Roll/Pitch-Raten
  <=0,01 Grad/s und Fahrt <=0,1 m/s; V6 dynamisch/stationaer und V1-V5
  kompatibel; targeted CTest protokolliert.

| Anforderung | Umsetzung | Nachweis |
|---|---|---|
| A-C-00118 | U-C-000101, U-C-000103, U-C-000105 | gerade/rund/reine Drehung, fester Rechenfall und Stellung |
| A-C-00119 | U-C-000102, U-C-000103, U-C-000105 | alle sechs Bewegungsgleichungen, Beschleunigung, Gyro, Zusatztraegheit |
| A-C-00120 | U-C-000103, U-C-000105 | Frames, Invarianz und explizite Inkompatibilitaet |
| A-C-00121 | U-C-000101, U-C-000104, U-C-000105 | V6 dynamisch/stationaer, V1-V5 unveraendert |
| A-C-00122 | U-C-000105 | numerische Grenzen, Referenz-Istwerte und Fehlerfaelle |

**Ist-Nachweise und offene Luecken**

- Nachkorrektur grosser Ruderausschlaege (U-C-000103/105, bestehende
  A-C-00118/00122): exakter Screenshotfall 420, Kraft [150,10,0] N
  modellfest am Punkt [-0,05,0,3] m, Ruder 24 Grad, Schwert 100 Prozent
  aus Ruhe reproduziert. Vorher 100 Iterationen ohne Konvergenz,
  21,483 s, max|R_F|=704,346 N und max|R_M|=137,638 Nm.
  Ursache: ungeeigneter Lage-/Drehratenstart im lokalen Suchzweig;
  modellfeste Last war nicht physikalisch unvereinbar.
  Nach verbesserter Initialisierung konvergiert derselbe eingefrorene Fall
  in 4 Iterationen: v ca. 2,1244 m/s, r=-0,30153826 rad/s
  (ca. -17,28 Grad/s), Radius ca. 7,05 m,
  max|R_F|=0,016035 N, max|R_M|=0,006048 Nm.
  Einzelmessung 4,27 s; parallele GUI/Core-Testlast erhoeht die Laufzeit,
  keine Zeitgarantie.
- `stationaere_kurvenfahrt` bindet das reale 420-Modell jetzt regulaer
  ueber CTest ein, nicht nur ueber einen optionalen manuellen Aufruf.
  Ruder 0/+1/-1 Grad mit altem 100-N-Fall und +/-12, +/-24, +/-36 Grad
  mit der Screenshotlast konvergieren aus Ruhe; alle sechs Residuen
  <=0,05 N/Nm, maximal 7 Iterationen. Die Gegenrichtung ist gesondert
  geprueft. Analytische, Singularitaets-, Fehler- und Altvertragsfaelle
  im gleichen Test bleiben erhalten.
- Nach vollstaendigem Neubau der sechs betroffenen Testprogramme:
  **6/6 gezielte Core-CTest bestanden**, 35,47 s. Dabei wurden zwei
  alte V2-/V4-Ergebnispruefungen erstmals gegen den aktuellen V6-Default
  neu uebersetzt; die Versionsannahme war nicht mehr passend.
  Diese Legacytests verlangen nun ausdruecklich
  `ErgebnisJsonFormat::Kompatibel` und pruefen weiterhin V2/V4, waehrend
  die Kurvenfahrttests den neuen V6-Default pruefen. Kein Lockern der
  physikalischen Akzeptanzkriterien.
- GUI-Nachweis desselben 24-Grad-Falls und des -24-Grad-Gegenfalls im
  echten Adapter bestanden; Hauptfenster mit unveraendertem Ruder 24
  bei 100/150/200 Prozent bestanden. Standard-EXE neu gebaut.
  Diese Nachkorrektur aendert keine exportierte Schnittstelle oder
  Vertragsversion und erlaubt weiterhin keine Aussage globaler
  Eindeutigkeit oder dynamischer Stabilitaet.

- Gemeinsame persistente Kreisreferenz unter
  `Code/tests/testdata/stationaere_kurvenreferenz/`: `modell.json` und
  `box.obj` bilden Masse 4000 kg, Schwerpunkt null und
  `I=diag(5000,7000,9000) kg*m^2` identisch zur In-Memory-Referenz ab.
  `kreisfall.json` enthaelt Last, vollstaendigen Startzustand,
  Hydrodynamikoptionen und Sollwerte (`u=[2,0,0] m/s`, `r=0,4 rad/s`,
  Roll=0,2 rad, Pitch=0, Radius=5 m). Das Modell deklariert alle sechs
  Zusatzmassen und Daempfungen explizit als null; der Lauf nutzt unveraenderte
  `Laufoptionen`-Wasserdefaults einschliesslich aktivierter Zusatzmasse.
  Kein GUI-Testschalter und keine manuelle Modellmutation erforderlich.
  Ohne Widerstand ist das Produkt u*r nicht eindeutig: die Referenz ist
  bewusst ein vorbesetztes relatives Gleichgewicht, kein Suchtest aus Ruhe.
  `stationaere_kurvenfahrt` prueft den unveraenderten Dateiimport,
  Massentraegheit, Solver und `simulieren` inklusive V6-Ausgabe:
  **1/1 gezielter CTest bestanden** nach Fixture-Ergaenzung.
- Reales Standardmodell `Modelle/420/modell.json`, aus Ruhe mit Identitaetspose,
  modellfeste Kraft `[100,0,0] N` bei `[1,0,0] m`, Schwert 100 Prozent,
  Standardwasser und Kurvenmodus: bei Ruder 0/+1/-1 Grad jeweils in
  **5 Iterationen konvergiert**. Gemessene Kursraten
  `0,000132566 / -0,0108649 / 0,0111162 rad/s`, Laufzeiten
  `2,456 / 2,314 / 2,762 s`; groesste Kraftresiduen
  `0,035281 / 0,000015 / 0,001496 N`, groesste Momentresiduen
  `0,015079 / 0,000016 / 0,000606 Nm` bei Solltoleranzen 0,05 N/Nm.
  Reproduzierbar mit `stationaere_kurvenfahrt_tests.exe <Kreisfixture>
  <420-Modell>`; optionaler zweiter Modellpfad aktiviert diese Messung.
  Dies ist ein konkreter Konvergenznachweis, kein Stabilitaetsnachweis.
- Gezielter Release-CTest in `CORE\build\standard` nach Build der drei
  geaenderten Testziele: **6/6 bestanden** (`stationaere_kurvenfahrt`,
  `simulation_gesamtlauf`, `hydrostatik_referenzen`,
  `hydrodynamik_referenzen`, `kraft_momentenbilanz`,
  `temporaerer_testantrieb`). Die Kurvenreferenz prueft positive/negative
  RHR-Drehrate, geneigte Lage, Radius, Fahrtfehler <=0,1 m/s,
  Roll/Pitch <=0,1 Grad, Schwerpunkt-Vz <=0,001 m/s, Euler-Roll-/Pitchraten
  <=0,01 Grad/s sowie Kraft-/Momentresiduen <=0,05 N/Nm. Zusaetzliche Tests
  decken Zusatzmasse/Kreiselterm, Rangdefizit, Flutungswiederholung ohne
  Scheinerfolg, Weltlast-Inkompatibilitaet, Abbruch, ungueltige Eingabe,
  dynamischen V6-Default, singularen Kursratenstatus und V1-V5-Kompatibilitaet ab.
- Vollstaendiger CTest-Lauf: **19/21 bestanden**. Offen bleiben nur die
  bereits bekannten, fachfremden Bestandsfehler `modellkatalog_schnittstelle`
  (Kataloggeometrie 0/0 statt 8/12) und `modell_geometrie_v2` (Schema-3-Feld
  `ableitung` im Schema-2-Konvertierungstest); beide wurden nicht geaendert.
- Mit der analytischen Solverreferenz wurden die numerischen Obergrenzen in
  den aktuellen Tests eingehalten. Kein Nachweis globaler Loesungseindeutigkeit
  oder dynamischer Stabilitaet; mehrere-Loesungen-/globale Stabilitaetsanalyse
  bleibt ausserhalb des freigegebenen Umfangs.

**Planerweiterung vom 02.11.2026:** finalisierte Spezifikation und Plan
freigegeben; die Implementierung wurde ausdrücklich beauftragt. Der
technische Umsetzungsstand und die noch offenen Abnahmenachweise stehen
nachfolgend. Frühere Inspektionsarbeiten und deren historische
Statusangaben weiter unten bleiben unverändert; sie belegen keinen
stationären Solver.

Massgeblich:
[Spezifikation Stationaeres Fahrtgleichgewicht](../Spezifikationen/Spezifikation_Stationaeres_Fahrtgleichgewicht.md).
GUI-Integration:
[G-0.9](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.9.md).
Dieser bestehende Plan bleibt der einzige C-0.8-Versionsplan.

### Geordnete Arbeitspakete

| Schritt | Anforderungen | Umsetzung und erwarteter Nachweis | Voraussetzung | Status |
|---|---|---|---|---|
| U-C-000096 | A-C-00114, A-C-00115, A-C-00116, A-C-00117 | Typisierte Eingabe-/Ergebnis-API, Koordinaten und Fehlerstatus implementiert; CPPI-0080 bis CPPI-0087 registriert. Begrenzte skalierte Suche, endliche Suchbereiche, Rangdiagnose, Abbruchbudget und Referenzkursregelung stehen in `hydrostatik.cpp`; Toleranzen sind auf max. 0,1 N/Nm begrenzt. Analytisches Fixture liegt unter `Code/tests/testdata/stationaere_referenz/`. | Spezifikation finalisiert | implementiert; technische Vertragsprüfung bestanden |
| U-C-000097 | A-C-00114, A-C-00115 | Unveränderlicher Einzelfall mit einer modell- oder weltfesten Kraft und Elementstellungen; gemeinsamer Kraft-/Momentpfad nutzt Hydrodynamik und bewegte Masse-/Volumenmesh-, Schwerpunkt- und Trägheitseigenschaften. Stationäre Rudereingaben ohne deklarierte Modellgrenzen verwenden nur die technische Endlichkeitsgrenze ±1e6 Grad; Prozentstellungen bleiben 0–100. | U-C-000096 | implementiert; Welt-/Modellframe, exzentrischer Hebelarm und simultane Ruder20/Schwert50-Eingaben regressionsgeprüft |
| U-C-000098 | A-C-00115, A-C-00116 | Solver für Z, 3D-Orientierung und horizontale Fahrt prüft alle sechs Kraft-/Momentkomponenten; Vz und Drehraten sind null. Rangdefizit, Suchgrenze, Abbruch und Flutungswiederholung werden explizit ausgewiesen. | U-C-000097 | implementiert; erfolgreiche Flutungswiederholung (1 Wiederholung), Rangdefizit, Suchgrenze, Abbruch und alle Residuenfälle regressionsgeprüft |
| U-C-000099 | A-C-00114, A-C-00116 | Lauf-/Serialisierungsweg nutzt v5 nur stationär, mit eingefrorenem Eingabesnapshot, Pose/Geschwindigkeit, Beiträgen, Residuen, Toleranzen und Status; Versionen 1–4 bleiben unverändert. API-/JSON-Verträge und CPPI-Register aktualisiert. | U-C-000098 | implementiert; Core-JSON-Test und GUI-Agent-Integrationstest bestanden |
| U-C-000100 | A-C-00114, A-C-00115, A-C-00116, A-C-00117 | Analytische Widerstandsreferenz, Nullast, horizontaler Lastfall, nicht ausgleichbare Last, NaN-Eingabe, Abbruch, feste Bewegungs-Massenlage, v5-Ausgabe sowie Hydrostatik-/Dynamikregression ausgeführt. | U-C-000099 | fachliche stationäre Testmatrix technisch bestanden; zwei unabhängige Katalog-/Schema-3-Bestandsregressionen bleiben offen |

### Meilensteine und Gates

- **C-S1 Vertrag:** Eintritt: Finalisierung; Abnahme U-C-000096:
  konkrete Ein-/Ausgaben, Interface-IDs, Solverstrategie, endliche Suchbereiche,
  Rangdiagnose und benannte Testfixtures dokumentiert. Bei Aenderung des
  fachlichen Vertrags vorher Nutzerentscheidung.
- **C-S2 Solver:** Eintritt C-S1 und U-C-000097; Abnahme U-C-000098:
  alle sechs unabhaengig nachgerechneten Residuen <= 0,1 N bzw. 0,1 Nm,
  analytischer Geschwindigkeitsfehler <= 0,1 m/s. Intern engere Ziele
  anstreben und Istwerte protokollieren; Grenzen niemals still lockern.
- **C-S3 Lieferung:** Eintritt C-S2; Abnahme U-C-000099/100:
  aktuelle API und Serialisierung, alle Referenz-/Negativtests bestanden,
  bestehende Hydrostatik/Dynamik unveraendert, GUI-Integrationsfixture
  lieferbar. Fachliche Nutzerabnahme separat.

### Rueckverfolgbarkeit und aktueller Nachweisstand

| Anforderung / Spezifikationsabschnitt | Schritte | Abnahme | Status / Beleg |
|---|---|---|---|
| A-C-00114 / 3 | U-C-000096, U-C-000097, U-C-000099, U-C-000100 | Typisierte Eingaben, fester Rechenfall und v5-Snapshot | technische Core-Abnahme bestanden; GUI meldet zusätzlich reales Modell 420 mit Ruder20/Schwert50 und sechs Residuen ≤0,1 |
| A-C-00115 / 4 | U-C-000096, U-C-000097, U-C-000098, U-C-000100 | Sechs Residuen und bewegte physische Geometrie | Galilei-, beide Frame-, exzentrische Kraft-, Stellungs- und Flutungswiederholungsregressionen bestanden |
| A-C-00116 / 5 | U-C-000096, U-C-000098, U-C-000099, U-C-000100 | Status/Diagnose, Referenzkurs, Nullraten und Fehlerfälle | JSON-/Negativtests einschließlich Rangdefizit, Suchgrenze und Abbruch bestanden; Stabilität/global eindeutige Lösung ausdrücklich nicht nachgewiesen |
| A-C-00117 / 6 | U-C-000096, U-C-000100 | Geschwindigkeit und alle sechs Residuen gegen unabhängige Referenz | `v=1,118035272 m/s` (Fehler 0,000001283 m/s); max. Kraftresiduum 0,003443046 N, max. Momentresiduum 0,001032914 Nm bei Solltoleranzen 0,05 N/Nm |

### Historischer Validierungsstand des geradlinigen Solvers (U-C-000096..100)

Nachpruefung des 420er-Nutzerfalls am 02.11.2026:
Ruder 0 Grad, Schwert 100 Prozent, modellfeste Kraft [100,0,0] N
bei [2,0,0] m. Vorher 100 Iterationen / 15,36 s ohne Konvergenz.
Ursache der langsamen Suche: Bei modellfester Last ist eine gemeinsame
Drehung von Kurs und wasserrelativer Fahrt physikalisch unbestimmt;
die bisherige Erkennung betrachtete nur eine isolierte Kursaenderung.
Der Solver behaelt nun den Startkurs als explizite Referenz und loest
fuenf Parameter unter weiterhin sechs Residuenbedingungen. Keine
kuenstliche Giermomentkraft, keine Lockerung der Abnahmekriterien.
Die Liniensuche nutzt passend zum reduzierten Kleinste-Quadrate-Schritt
die Summe der quadrierten skalierten Residuen; die Konvergenzpruefung
bleibt komponentenweise bei 0,05 N/Nm.

Nachher: 6 Iterationen / 1,99 s bis zur ehrlichen Stagnationsdiagnose,
Versuchsfahrt 1,987110111 m/s, Rest-Fy 0,102321154 N und
Rest-Mz 0,128377476 Nm. Das ist weiterhin **kein nachgewiesenes
Gleichgewicht**, auch nicht mit 0,1-N/Nm-Obergrenzen. Daraus folgt
weder ein Beweis der Nicht-Existenz noch eine garantierte Fahrtgenauigkeit
von 0,1 m/s. Ohne aktivierte Kraft konvergiert derselbe Fall in 1,18 s
bereits im hydrostatischen Startwert. Beide Faelle sind im GUI-Adaptertest
als Regression enthalten; der belastete Fall darf sein Budget nicht
wieder aufbrauchen und darf keine falsche Erfolgsmeldung liefern.
Release-Build und 5/5 gezielte Tests im Build
`C:\spdeps\gui-workspace-tabs` bestanden (23,29 s):
`gui_core_integration`, `gui_workspace_tabs_100`,
`hydrostatik_referenzen`, `hydrodynamik_referenzen`,
`simulation_gesamtlauf`. Die analytische Fahrt bleibt unveraendert
innerhalb 0,1 m/s. Kein neuer Schnittstellenvertrag.

- Release-Build in `build/standard`; gezielt
  `ctest --test-dir build/standard -C Release -R
  "simulation_gesamtlauf|hydrostatik_referenzen|hydrodynamik_referenzen|kraft_momentenbilanz"
  --output-on-failure`: **4/4 bestanden**. `simulation_gesamtlauf`
  prüft Nullast, analytische `F=c*v²`-Referenz, beide Kraftrahmen,
  Rotations-Framewechsel, exzentrischen Momentenhebelarm, Galilei-
  Verschiebung (Relative-Abweichung `1,281879e-6 m/s`; verglichene
  Hydrodynamik-Kraft-/Momentbeiträge innerhalb der vorhandenen
  0,05-N-/0,05-Nm-Toleranzen),
  Mehrfachstellung Ruder20/Schwert50, v5-JSON, fehlende Lösung, Suchgrenze,
  ungültige Kraft und Abbruch. `hydrostatik_referenzen` prüft die tatsächliche stationäre
  Flutungswiederholung: `flutungswiederholungen=1`, `geflutet=["Rumpf"]`.
  Die Solver-Solltoleranzen betragen 0,05 N und 0,05 Nm (Grenzwert laut
  Spezifikation: 0,1); stationäre technische Suchbereiche sind Z ±100 m,
  drei Rotationssuchparameter ±31,4159265 (Pose-Rotationsvektor dadurch
  komponentenweise ±π rad) und Vx/Vy ±50 m/s. Eine wegen dieser Grenzen
  stagnierende Suche liefert `suchgrenze`.
- Analytisches Core-Fixture:
  `Code/tests/testdata/stationaere_referenz/modell.json`, Mesh `fin.obj`,
  Kraft weltfest `[1500,0,0] N`, Angriffspunkt modellfest
  `[0,0,-0.4] m`, Start Pose/Tempo null, Wasser `rho=1000 kg/m³`,
  `nu=1e-6 m²/s`, keine Bewegungsstellungen.
  `v_erwartet=sqrt(2*1500/(1000*1*2.4))=1.118033989 m/s`;
  Core gemessen `1.118035272 m/s`, `max|F|=0.00344304548 N`,
  `max|M|=0.001032913644 Nm`. Die exzentrische Fremdkraft trägt um S
  `[6.074006715e-10,450,0] Nm` bei. Solltoleranzen: 0,05 N und 0,05 Nm.
- Historischer vollständiger Core-CTest des Geradeausumfangs: **18/20 bestanden**. Die
  reproduzierbaren Bestandsfehler sind unabhängig von stationärer Lösung
  und bewegter Physikgeometrie:
  - `modellkatalog_schnittstelle` → `katalog_tests.cpp:124–126` vergleicht
    die aktuelle Schema-2-Quellgeometrie (8 Punkte/12 Dreiecke aus
    `element_mesh_1` in `boot.obj`) mit `quader/meshes/gui/rumpf.obj`.
    Diese erwartete Datei ist im aktuellen Modellinventar nicht vorhanden,
    also zählt der Test 0/0; Core liefert 8/12. Reproduktion:
    `ctest --test-dir build/standard -C Release -R modellkatalog_schnittstelle
    --output-on-failure`.
  - `modell_geometrie_v2` → Quellmodell `Modelle/optimist/modell.json` ist
    bereits Schema 3 und enthält `Hydrostatik.volumen.ableitung`; der
    `konvertiere_modell_v2`-Pfad schreibt in
    `build/standard/v2-modelle-test/optimist/modell.json` ein von Schema-2-
    Import erwartetes Ergebnis, lässt aber die nur in Schema 3 erlaubte
    `ableitung` stehen. Der Importer weist diese als unbekannt zurück.
    Reproduktion: `ctest --test-dir build/standard -C Release -R
    modell_geometrie_v2 --output-on-failure`. Beide Ursachen liegen in
    Katalog-/Schema-3-Migrationstestdaten und wurden nicht durch Änderungen
    am stationären Solver oder dessen bewegter physischer Geometrie
    verursacht; diese fremde Arbeit wurde nicht umgeschrieben.
- Die frühere GUI-Integration für den Geradeausumfang meldete
  `gui_core_integration` einschließlich stationärem Nullast- und Abbruchfall
  bestanden; sie nutzte Core-Vertragsversion 5 und belegt nicht den neuen
  V6-Kurvenfahrtvertrag. Die V6-GUI-Integration ist separat abzunehmen.
- Der beauftragte Core-Nachweisumfang für die offenen Fälle ist damit
  geschlossen. GUI meldet den realen 420-Lauf Ruder20/Schwert50 ebenfalls
  konvergiert mit allen sechs Residuen ≤0,1; GUI-Integration ist kein
  Ersatz für die hier gelisteten Core-Tests. Persönliche Nutzerabnahme,
  dynamische Stabilität und globale Eindeutigkeit bleiben außerhalb dieses
  technischen Nachweises.

Git-Commit, Tag, Push und Release sind nicht Bestandteil dieses Plans.

---

## Bisheriger Inspektionsumfang und Nachweise

**Status:** Core implementiert und technisch in GUI integriert; persönliche GUI-Abnahme offen.
**Stand:** 02.11.2026; alle elf Modelle migriert; explizite Ableitungen implementiert und technisch geprüft; persönliche GUI-Abnahme separat offen.
**Zielversion:** C-0.8.

Maßgeblich ist ausschließlich die
[Erweiterung der Modellkatalog-Spezifikation, Abschnitt 3](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#3-c-08-objektbäume-eigenschaften-und-mesh-abfragen).
Die [Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Modellinspektion.json)
enthält die Herkunftszuordnung. C-0.8 ergänzt den Vertrag ausdrücklich
für die neue Inspektionsschnittstelle; bestehende Endpunkte bleiben unverändert.

Dieser Plan ist C-0.8 zugeordnet. Er ist der maßgebliche Plan für diesen
Versionsschritt; weitere C-0.8-Arbeiten sind hier einzugliedern.
Kein paralleler Versionsplan, keine Änderungen historischer Pläne.
Die Schritte U-C-000078 bis U-C-000085 setzen A-C-00098 bis A-C-00104 um.
Die Interfaces CPPI-0032 bis CPPI-0059 stehen im Register; die 21
Inspektions-Exports CPPI-0032 bis CPPI-0052 sowie zwei Snapshotprojektionen
und die notwendigen Importdatenfelder sind implementiert.

## Finalisierte Planerweiterung 02.11.2026: Einzelquelle und Gesamtinspektion

Der Nutzer hat Finalisierung und Umsetzung der Anforderungen A-C-00110 bis
A-C-00113 ausdrücklich beauftragt. Die maßgeblichen Verträge stehen in der
[Modellbeschreibung](../Spezifikationen/Spezifikation_Modellbeschreibung.md)
und in
[Modellkatalog-Schnittstelle, Abschnitt 4](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#4-c-08-erweiterung-objektzentrierte-gesamtinspektion).
Herkunftszuordnungen sind in
[Modellformat.json](../Spezifikationen/Entwurfspunkte/Modellformat.json)
und
[Modellinspektion.json](../Spezifikationen/Entwurfspunkte/Modellinspektion.json)
nachvollziehbar. Historische Schritte U-C-000078 bis U-C-000089 bleiben
unverändert.

### Geordnete Umsetzungsschritte

| U-ID | Ergebnis / Prüfauftrag | Anforderungen | Abhängigkeit | Status |
|---|---|---|---|---|
| U-C-000090 Referenzinventar und Konfliktnachweis | Alle elf Bestandsmodelle geometrisch verglichen; offene/geschlossene Topologie und Quellenkonflikte im Spezifikationsinventar dokumentiert. | A-C-00110, A-C-00111 | U-C-000078 | Abgeschlossen; Konflikte über freigegebene Ableitungen aufgelöst |
| U-C-000091 Schema-3-Importer | Eine Mesh-ID je geometrischem Element; keine unbenutzten oder elementübergreifend geteilten Quellnetze. Legacy/Schema 2 bleiben lesbar. | A-C-00110 | U-C-000090 | Implementiert und getestet |
| U-C-000092 Sichere Schema-3-Migration | Strikter Aufruf bleibt unverändert; explizites `--schema3-ableitungen` migriert auch die drei Boote über geometrisch geprüfte Schnitte/Flächenauswahl. Vollständige Vorabvalidierung vor Zielanlage. | A-C-00111 | U-C-000090, U-C-000091 | Alle elf Modelle geliefert; Original-/Überschreibschutz erhalten |
| U-C-000093 Kanonischer Objektbaum und Properties | Modell/Baugruppe/Element einmal; vollständige Eigenschaften mit `area`, Fachabfragen kompatibel. | A-C-00112 | U-C-000078 | Implementiert, Core-/GUI-Tests bestanden |
| U-C-000094 Typisierte Bewegungs-Vorschau | Core-transformierte Referenz-/Grenzposen und Achse; undefinierte Grenzen bleiben leer. | A-C-00113 | U-C-000079 | Implementiert, Core-/GUI-Tests bestanden |
| U-C-000095 Regression und technische Lieferung | Historische 36 Tests sowie neue Physik-/Flutungs-/Dynamikvergleiche und GUI-Nachweise nach Ableitungsfreigabe. | A-C-00110 bis A-C-00113 | U-C-000091 bis U-C-000094 | Technische Migration/Integration nachgewiesen; persönliche GUI-Abnahme separat |

### Tatsächliche Lieferung dieser Erweiterung, 02.11.2026

```powershell
cmake --build C:\spdeps\gui-workspace-tabs --config Release --parallel 4
ctest --test-dir C:\spdeps\gui-workspace-tabs -C Release -E "^gui_graphics_render_" --output-on-failure
```

Integrierter Lauf: **36/36 bestanden, 212,59 s**, davon 20 Core-Fälle.
`modellinspektion_c08` 6,81 s, `modell_geometrie_v2` 50,90 s; letzterer
deckt auch Schema 3 und die acht erfolgreichen/drei abgelehnten Migrationen
ab. Die bekannte breite GUI-Grafikbaseline bleibt ausgeschlossen und wird
nicht als behoben ausgegeben.

Zusätzlicher `gui_model_catalog`-Lauf nach Schema-3-Adapterergänzung:
**1/1 bestanden, 29,66 s**. Acht Modelle werden über denselben GUI-Adapter
geladen und auf Masse/Hüllgrenzen sowie eine Quelle je Element geprüft;
drei Konfliktmodelle erzeugen keinen Zielordner.

Finale GUI-Inspektionsprüfung bei 100/150/200 % einschließlich echten
Schema-3-Ladens über MainWindow: **3/3 bestanden, 37,49 s**.

CLI-Smokeprüfung erfolgreich: `--schema3` erzeugt das Quader-Beispiel unter
`C:\spdeps\gui-workspace-tabs\single-source-models\quader`.
Folkeboot wird mit CORE-MOD-0009 für die abweichenden Quellen des Schwerts
abgelehnt, ohne Zielanlage. Ohne Flag bleibt die Schema-2-CLI kompatibel.
CPPI-0074 bis CPPI-0079 im Register dokumentieren die additive Lieferung.

**Historischer Sperrstand vor Ableitungsfreigabe:** `420`, `folkeboot`, `optimist` blieben
unverändert lesbar, aber nicht als vollständig migriert ausgegeben.
Eine Nutzerentscheidung wurde über das Rückfragetool angefordert; es war
keine Antwort verfügbar. Deshalb keine eigenmächtige Quellwahl,
keine geschlossenen Ersatzflächen und keine Physikänderung. Die vollständige
Einzelquellenabnahme dieser drei Modelle bleibt blockiert.
Keine Änderungen an Originalmodellen und keine Git-Veröffentlichung.
Dieser Sperrstand ist durch die nachfolgend dokumentierte Nutzerentscheidung
und nachgewiesene Ableitungslieferung aufgehoben, nicht durch eine stille Quellwahl.

### Abschließende Ableitungslieferung nach Nutzerfreigabe

Der Nutzer wählt ausdrücklich Option 1: bisherige Physik erhalten, mit
ausreichender statt bitgenauer Übereinstimmung; vorläufige Genauigkeitsgrenzen
darf die Umsetzung fachlich begründet selbst bestimmen. Die Präzisierung
von A-C-00110/A-C-00111 in der bestehenden Spezifikation dokumentiert Vertrag,
Abgrenzung und Grenzen; keine zweite Spezifikation und keine neue Physik.

- 420/Cockpit: bestehendes geschlossenes Volumennetz als Quelle; Masseschale
  durch Weglassen vorhandener Deckdreiecke bei Z=0,475 m.
- Optimist/Rumpf: geschlossenes Volumennetz als Quelle; Masseschale und
  Darstellung ohne vorhandene Deckdreiecke bei Z=0,314342 m.
- Schwert/Ruder: vollständiges Massennetz als Quelle, Auftriebsteil durch
  geometrisch bestimmte, explizit gespeicherte Ebene. Bei 420/Schwert geneigt,
  sonst horizontal. Alle Schnittdeckel werden aus dem Schnitt erzeugt,
  nicht als unabhängige Quelldatei gespeichert.
- 420/Rumpf: vorhandenes gemeinsames Massen-/Volumennetz als Quelle; keine
  Neuberechnung/Vereinfachung. Darstellungsfläche 16,0740466 → 16,0669159 m²
  (−0,0444 %), Hüllgrenzen unverändert; Physikquelle unverändert.
- Keine künstlichen Gegengewichte, Parameterkorrekturen oder Namensregeln.
  Flutränder, Massenstützstellen, Massenintegration und Bewegungsalgorithmen
  bleiben erhalten. GUI kennzeichnet die tatsächlichen Ableitungen.
- Sämtliche elf Schema-3-Modelle dauerhaft unter
  `C:\spdeps\gui-workspace-tabs\single-source-models`; ein OBJ und ein JSON
  je Modell, eine Quelle je geometrischem Element. Originale unverändert.

Erster gemeinsamer Nachweis nach dieser Erweiterung: **6/6 bestanden,
163,13 s** (`modell_geometrie_v2`, `modellinspektion_c08`,
`gui_model_catalog`, `gui_boat_inspection_100/150/200`).
Der GUI-Katalog prüft nun alle elf migrierten Modelle; strikte Konfliktprüfung
bleibt separat für den bisherigen Aufruf erhalten.

Tatsächliche maximale Abweichungen des 0,2-s-Dynamikvergleichs:

| Boot | Teilvolumen über vier Wasserlagen, m³ | Position, m | Geschwindigkeit, m/s | Drehrate, rad/s |
|---|---|---|---|---|
| 420 | 4,34e-19 | 1,19e-6 | 1,19e-5 | 2,59e-6 |
| Folkeboot | 6,65e-12 | 7,30e-7 | 7,89e-6 | 2,02e-6 |
| Optimist | 1,71e-14 | 1,98e-7 | 1,83e-6 | 2,92e-7 |

Die geringfügigen dynamischen Unterschiede sind mit den bestehenden
flächenbezogenen Auswertungen und geänderter Triangulierung vereinbar;
die spezifizierten Regressionstoleranzen werden eingehalten.
Die Tests prüfen zusätzlich Masse, Schwerpunkt, Trägheit, offene
Flutränder, Auftriebsschwerpunkte, Kräfte/Momente und Flutungsentscheidungen.
Keine Aussage über beliebig lange dynamische Integrationen.
Negativtests prüfen ungültige/fehlende Schnitte und die Verweigerung einer
Zielanlage bei unzulässigen Ableitungen.
Der Standard-GUI-Release-Build mit diesen Core-Änderungen ist erfolgreich.
Abschließende Stützstellen-/Negativtests und GUI-Verwendungszeile erneut
geprüft: **4/4 bestanden, 115,14 s** (`modell_geometrie_v2` sowie drei
DPI-Inspektionsprüfungen). Massenstützstellenkoordinaten als Menge exakt
erhalten; ungültige Ableitungen erzeugen keinen Zielordner.
Bekannte breite Grafikbaseline bleibt separat; keine Git-Veröffentlichung.

### Vertiefter Konfliktnachweis und persistierte Migration

Die zunächst acht konfliktfreien Modelle wurden zusätzlich dauerhaft in neue
Unterordner von `C:\spdeps\gui-workspace-tabs\single-source-models`
konvertiert: `bleistift`, `kegel`, `kugel`, `platte`, `pyramide`, `quader`,
`rolle`, `unregelmaessig`. Jeder Ordner enthält Schema-3-JSON und OBJ;
keine Änderung der Originale oder automatischer Wechsel des GUI-Modellordners.

Die sieben Konflikte wurden ergänzend direkt an den OBJ-Daten quantifiziert.
Messmethode: OBJ-Flächen als Dreiecksfächer, Dreiecksflächen über Kreuzprodukte,
orientierte Volumensumme über Ursprungstetraeder, offene Kanten über
Koordinatenpaare. Für geschlossene Netze wird der Betrag des orientierten
Volumens angegeben; für offene Schalen ist diese Summe ausdrücklich
**kein gültiges Körpervolumen**. Masseprognosen verwenden unveränderte
JSON-Dichte und -Wanddicke. Dies sind geometrische Entscheidungshilfen,
kein Ersatz für Core-Schwerpunkt-, Trägheits-, Flutungs- oder Simulationstests.

| Konflikt | Bisherige Physikquelle | Abweichende Kandidatenquelle | Quantitativer Effekt |
|---|---|---|---|
| 420 / Rumpf | Masse/Volumen: 16,0669159 m²; 1,13007290 m³ | Darstellung: 16,0740466 m²; 1,13032594 m³ | Schalenmasse 79,3705646 → 79,4057902 kg; identische Hüllgrenzen belegen keine Geometriegleichheit |
| 420 / Cockpit | Masseschale: 4,23672109 m², 374 offene Kanten | Darstellung/Volumen geschlossen: 6,60792109 m²; 0,690105464 m³ | Schalenmasse 0,00508406531 → 0,00792950531 kg; geschlossene Ersatzquelle verliert die aus offenen Massenkanten ermittelte Flutrandinformation |
| 420 / Schwert | Massen-/Darstellungsnetz: 0,007408350 m³ | Volumennetz: 0,0033615405 m³ | Volumennetz reicht nur bis Z=0,023786 statt 0,56 m; dessen Verwendung als Vollkörpermasse ergäbe 4,445010 → 2,0169243 kg |
| Folkeboot / Schwert | Massen-/Darstellungsnetz: 0,091496000 m³ | Volumennetz: 0,0719663694 m³ | Obergrenze Z=-0,47 statt -0,32 m; Massenersatz 1037,56464 → 816,098628 kg |
| Folkeboot / Ruder | Massen-/Darstellungsfläche: 0,680447661 m² | Volumenfläche: 0,380294669 m² | Obergrenze Z=-0,47 statt -0,16 m; Schalenmassenersatz 3,81050690 → 2,12965015 kg |
| Optimist / Rumpf | Masseschale: 4,08968375 m², 44 offene Kanten | Volumennetz geschlossen: 6,41660411 m²; 0,774649998 m³ | Schalenmasse 38,8519957 → 60,9577391 kg; Verlust der bisherigen offenen Flutrandkanten |
| Optimist / Schwert | Massen-/Darstellungsnetz: 0,00634543411 m³ | Volumennetz: 0,00239140748 m³ | Obergrenze Z=-0,081812 statt 0,2834 m; Vollkörpermassenersatz 3,80726046 → 1,43484449 kg |

**Technische Bewertung des Ausgangsbefunds:** Mehr Dreiecke oder ein geschlossenes Netz allein
ist kein belastbares Auswahlkriterium. Bei Schwert/Ruder liegen teils
unterschiedliche räumliche Ausschnitte vor, nicht lediglich verschiedene
Auflösungen. Bei den offenen Rumpf-/Cockpitschalen hängen vorhandene
Flutungsinformationen an den offenen Massenkanten. Einfach alle Verwendungen
auf ein geschlossenes Netz umzubiegen würde diese Semantik verändern.
Eine andere Quelle, eine Flächen-/Volumenteilauswahl oder eine neue
Ableitungsregel benötigte deshalb eine explizite fachliche Freigabe mit
Core-Vorher-/Nachherprüfung. Diese Freigabe liegt inzwischen vor; die
abschließende Lieferung oben setzt sie ohne Austausch der Massenphysik um.

### Rückverfolgbarkeit der Erweiterung

| Anforderung / Spezifikationsabschnitt | Umsetzung | Tatsächlicher Nachweis | Status |
|---|---|---|---|
| [A-C-00110](../Spezifikationen/Spezifikation_Modellbeschreibung.md#a-c-00110-ein-quellmesh-vertrag-schema-3) | U-C-000090, U-C-000091, U-C-000095 | Schema-3-Positiv-/Negativfälle, Legacy/Schema 2, eine Quelle mit definierten Ableitungen, GUI-Katalog aller elf Modelle | Technisch nachgewiesen |
| [A-C-00111](../Spezifikationen/Spezifikation_Modellbeschreibung.md#a-c-00111-nachweisbare-migration-und-konfliktbehandlung) | U-C-000090, U-C-000092, U-C-000095 | Acht identische Migrationen; drei freigegebene Ableitungsmigrationen mit Physik-/Flutungs-/Dynamikvergleich; strikter Aufruf verweigert Konflikte weiterhin | Technisch nachgewiesen; persönliche GUI-Abnahme separat |
| [A-C-00112](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#a-c-00112-kanonischer-objektbaum-und-bereichszugeordnete-eigenschaften) | U-C-000093, U-C-000095 | `modellinspektion_c08` und GUI: kanonische Identitäten, Bereiche, Felder, Deduplizierung, Snapshot-/JSON-Parität | Technisch nachgewiesen; persönliche GUI-Abnahme separat |
| [A-C-00113](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#a-c-00113-typisierte-bewegungs-vorschau-aus-dem-snapshot) | U-C-000094, U-C-000095 | `modellinspektion_c08`: bestehende Transformation, Grenzen/Referenz und JSON; GUI-Vorschau ohne Kamera-/Simulationsänderung | Technisch nachgewiesen; persönliche GUI-Abnahme separat |

### Meilensteine und Entscheidungstore

| Meilenstein | Eintritt | Überprüfbares Austrittskriterium |
|---|---|---|
| Fachvertrag final | Ausdrücklicher Auftrag | Vier A-Kennungen, IDEE-Zuordnung, Schema-3-Kompatibilität, Snapshot- und Bewegungsvertrag spezifiziert. |
| Inventar entschieden | Alle elf Referenzmodelle verglichen | Acht identische Migrationen; drei nach Nutzerfreigabe über explizite Ableitungen migriert, ohne Austausch offener Masseschalen oder neue Physik. |
| Core technisch geliefert | U-C-000091 bis U-C-000095 abgeschlossen | Gemeinsamer Importpfad, sichere Migration und Inspektionsverträge bestehen gezielte Tests sowie Core-Build/CTest. |
| GUI integriert | GUI nutzt den Core-Vertrag | GUI-Build-/Vertragstests mit denselben Daten bestanden; technische Integration ersetzt keine persönliche Sichtabnahme. |

### Verifikation und Grenzen

Vorher/Nachher-Nachweise erfassen Geometrie, Masse, Schwerpunkt, Trägheit,
Volumen, Stützstellen/Flutränder, Bewegung und repräsentative
Simulationszustände. Die vorhandenen Rechen- und Stützstellenalgorithmen
bleiben unverändert. Für die konfliktfreien Modelle sind bestehende
Gleichheitsprüfungen zu erhalten; für konfliktbehaftete Modelle werden keine
fachlichen Toleranzen oder maßgeblichen Quellen erfunden. Keine automatische
Schließung offener Schalen, kein Meshvereinfachen, Editor, Reparatur oder
neue Physik. Ergebnisse werden erst nach realem Test in der Statustabelle
als bestanden markiert.

## 1. Geordnete Umsetzungsschritte

Core-Build und die gesamte CTest-Suite bestehen. Die persönliche GUI-Abnahme
ist nicht Teil der Core-Lieferung.

Fachlich bestätigt am 01.11.2026: alle vorhandenen Objektbaumtypen und
Eigenschaften einschließlich Physiknetzen als lesbare Inspektionsdaten.
Die Vertragserweiterung, das vollständige Dateninventar und der API-Vertrag
sind final spezifiziert. Reparatur-/Schreibzugriffe
bleiben außerhalb des aktuellen Umfangs. Bestätigt ist ein einmal
eingelesener, unveränderlicher Inspektionsstand: Dateiänderungen
verändern oder entwerten ihn nicht; Detailabfragen verwenden denselben
Bestand ohne erneutes Dateilesen. Mesh-Updates, Mesh-Modifikationen und
weitergehende Neuladebedienung werden später besprochen.

| U-ID / Paket | Ergebnis | Anforderungen | Abhängigkeit |
|---|---|---|---|
| U-C-000078 Ausgangsstand und Inventar | Ausgangsrevision `ebca994770b216a749e18917d2ae387de1612dff`, Branch `main`, Release-Build und Bestands-Catalog-Test erfasst. Vertrags-/Altverhalten wird durch `katalog_tests` sowie die Feldabdeckung in `inspektion_tests.cpp` charakterisiert. | A-C-00098, A-C-00103, A-C-00104 | Abgeschlossen |
| U-C-000079 Snapshot und API-Typen | Unveränderlicher Besitzsnapshot, einmalige Query-Schnittstelle, gemeinsame `lies_beschreibung`-/`lade_modell`-/OBJ-Bausteine, Prüfung vor/nach Import und Speicherung der Massenmesh-/Massenartdaten. | A-C-00099, A-C-00102, A-C-00104 | U-C-000078; abgeschlossen |
| U-C-000080 Baumsichten | Fünf bedingte Sichten, Gruppen-/Element-/Fachhierarchie, JSON-Pointer-IDs und Elternreferenzen. | A-C-00098, A-C-00100 | U-C-000079; abgeschlossen |
| U-C-000081 Eigenschaften | Importierte Quellfelder und abgeleitete Gesamtmasse, Schwerpunkt, Trägheit, Hüllgrenzen und Bewegung mit Typen/Einheiten; überlappende Auswahl dedupliziert. | A-C-00098, A-C-00100 | U-C-000080; abgeschlossen |
| U-C-000082 Mesh-Arten | Sechs bedingte Geometriearten, echte Dreieck-/Punkttopologie, Elementfilter, Herkunft und Anzeigemesh-Projektion aus Snapshot. | A-C-00099, A-C-00101 | U-C-000079; abgeschlossen |
| U-C-000083 JSON und Diagnosen | Alle sechs Serializer, `inspektionsversion` und CORE-INSP-0001–0004 im Runtime-Fehlerkatalog; alte JSON-Ausgaben unverändert. | A-C-00102 | U-C-000081, U-C-000082; abgeschlossen |
| U-C-000085 Bestandsadapter zusammenführen | Bestehende Struktur-/Darstellungsaufrufe und neue Snapshotprojektionen nutzen gemeinsame Import-/Abfragebausteine; V1-Signaturen/Antworten und Altlebenszyklus erhalten. | A-C-00104 | U-C-000083; abgeschlossen |
| U-C-000084 Nachweise und Übergabe | Feldabdeckung, Snapshot-Mutation, Fehlerfälle, C++-/JSON-Parität und Bestandsregression; vollständiger Core-Build/CTest und GUI-Übergabedaten. | A-C-00098 bis A-C-00104 | U-C-000085; Core-Nachweise abgeschlossen, GUI-Abnahme offen |

Baum- und Mesh-Pakete können nach dem API-Vertrag unabhängig bearbeitet
werden. Keine GUI-Implementierung, keine neue Physik und kein
Netzwerktransport in diesem Core-Auftrag.

## 2. Meilensteine und Eintrittskriterien

| Meilenstein | Eintritt | Abnahmekriterium | Stand |
|---|---|---|---|
| Vertrag spezifiziert | Nutzerentscheidungen vorhanden | Inventar, Datenumfang, Versionierung und Zielversion final dokumentiert | Erfüllt |
| Umsetzung beauftragt | Finaler Vertrag liegt vor | Ausdrücklicher Implementierungsauftrag vorhanden; Baseline erfassen | Erfüllt; Implementierung freigegeben |
| Inspektion integriert | API freigegeben | Bäume, Eigenschaften und Meshes liefern widerspruchsfreie Identitäten und Fassung | Erfüllt; `modellinspektion_c08` bestanden |
| Technisch geliefert | Integration abgeschlossen | Core-Build und Vertragstests bestanden; GUI-Release-Build und integrierte Tests bestanden; Regressionen und Abweichungen dokumentiert | Erfüllt für Core und technische GUI-Integration |
| GUI-seitig abgenommen | Core-Lieferung und separate GUI-Integration vorhanden | Nutzer bestätigt die sichtbare Objektinspektion; keine Gleichsetzung mit Core-Testabschluss | Offen |

## 3. Prüfkonzept

Ausgangsbranch `main`, Revision und Release-Build sind in U-C-000078
festgehalten. Verwendet wurden die vorhandenen CMake-/CTest-Werkzeuge und
das konfigurierte Buildverzeichnis `CORE\build\standard`.

- Typauflistungen nur für tatsächlich unterstützte Sichten/Geometrie.
- Vollständige Hierarchie gemäß freigegebenem Inventar; keine Zyklen,
  ungültigen Referenzen oder Namensheuristiken.
- Einzel-, Gruppen- und Mehrfachauswahl mit überlappenden Nachkommen:
  jede Eigenschaft genau einmal, keine automatische Summe.
- Typisierte Werte und SI-/dokumentierte Einheiten, klare Fehlend- und
  Nicht-anwendbar-Statuswerte; keine Nullwert-Ersatzdaten.
- Meshpunkte und Dreiecke gegen die maßgebliche Core-Geometrie prüfen;
  Elementfilter, Herkunft, Referenzrahmen und leere Filter testen.
- Prüfung beim anfänglichen Einlesen nachweisen; danach Modell- und
  Darstellungsdateien ändern und nachweisen, dass Baum, Eigenschaften und
  Meshes unverändert abrufbar bleiben. Kein Nachladen aktueller Dateien
  durch Detailabfragen und keine Entwertung des bestehenden Stands.
- Unbekannte Typen/IDs, unlesbare Daten und fehlerhafte Modelle:
  eindeutige Diagnosen, keine Erfolgsvortäuschung.
- Keine Dateipfade oder internen Puffer in den Antworten; alle nun
  freigegebenen fachlichen Eigenschaften vorhanden; C++-/JSON-Parität.
- Bestehende Katalog-/Import-/Simulationsverträge regressionsprüfen;
  Inspektion ändert weder Modell noch laufende Simulation.
- A-C-00104: Der Aufrufgraph führt neue und alte Struktur-/Darstellungs-
  eintritte über `readModelInspection` und dieselben Snapshotprojektionen;
  dessen Einlesepfad verwendet die bestehenden Parser, den Modellimport,
  den OBJ-Leser und die Diagnose-/Versionsbausteine. `katalog_tests` und
  `modellinspektion_c08` bestehen. Altaufrufe behalten Katalogfassungs-
  prüfung und KAT-Fehler; Snapshotprojektionen lesen nach Rückgabe keine
  Dateien erneut.

## 4. Fortschrittsführung und Grenzen

**Nachweise:** Release-Build mit CMake unter
`CORE\build\standard\Release`; Core-Bibliothek
`CORE\build\standard\Release\segelphysik_core.lib`.
Vollständiger Befehl: `cmake --build CORE\build\standard --config Release`;
CTest: `ctest --test-dir CORE\build\standard -C Release --output-on-failure`.
Ergebnis: 19/19 CTest-Fälle bestanden, einschließlich
`modellinspektion_c08`, `modellkatalog_schnittstelle`, `modell_import`,
Hydrostatik-/Hydrodynamik-/Bilanz-/Simulationstests und CLI-Fehlerkatalog.
Feldabdeckung und Snapshotdateiänderung sind in
`Code\tests\inspektion_tests.cpp` belegt; fehlerhafte Physik liefert eine
Teilsicht/CORE-INSP-0004, unlesbare Gesamtstruktur wird mit
CORE-INSP-0003 abgelehnt.

Technische GUI-Integration am 01.11.2026 bestätigt: GUI-Release-Builds in
`C:\spdeps\gui-workspace-tabs` und `C:\spdeps\guibuild` erfolgreich; das
integrierte CTest-Set `-E '^gui_graphics_render_'` besteht mit 35/35 Fällen
(19 Core-, 16 GUI-Tests). Darin bestehen insbesondere die GUI-
Inspektions- und Workspace-DPI-Tests mit 100, 150 und 200 Prozent. Der
GUI-Ladeworker verwendet die Snapshotprojektionen
`modellstruktur(*snapshot)` und `darstellungsgeometrie(*snapshot)`.
Der ausgeschlossene, bereits zuvor fehlschlagende Test
`gui_graphics_render_100` meldet weiterhin, dass das Deaktivieren der
Kurslinie für Modell 420 das exakte Bild nicht wiederherstellt; dies ist
als bekannter, nicht durch diese Integration behobener Fehler festgehalten.
Die persönliche fachliche Nutzerabnahme bleibt offen.

Die Schnittstellenbeschreibung und das CPPI-Register kennzeichnen C-0.8
als Core-implementiert; die technische Auslieferung ersetzt keine
persönliche fachliche Abnahme. Kein Commit, Push, Tag oder Release wurde
ausgeführt.

## 5. Freigegebene Erweiterung: konsolidierte Modellgeometrie

**Stand 02.11.2026:** Nutzerauftrag Alternative 2; keine Editor-,
Reparatur-, Wind- oder Speicherfunktion der GUI. Maßgeblich für dieses
Thema ist zusätzlich die bestehende
[Modellbeschreibung, C-0.8-Erweiterung](../Spezifikationen/Spezifikation_Modellbeschreibung.md#c-08-erweiterung-konsolidierte-quellgeometrie-02112026).
Die früheren Schritte und ihr Nachweis bleiben unverändert.

| Schritt | Anforderungen | Abhängigkeit | Ergebnis / tatsächlicher Status |
|---|---|---|---|
| U-C-000086 Schema 2 und gemeinsamer Import | A-C-00105, A-C-00106, A-C-00107 | U-C-000079 | Implementiert: ein OBJ, `o`-Auswahl, gemeinsame Parser-/Physikbausteine, explizite Rolle ohne Namensinferenz; Altmodus erhalten. |
| U-C-000087 Quellregister und Snapshotherkunft | A-C-00108 | U-C-000086 | Implementiert: `quelle`, alle Quellvarianten, stabile Quell-ID, alle Elementverwendungen, Herkunft/Ableitung und Knoten-Mesh-Verweise. |
| U-C-000088 Sichere Migration | A-C-00109 | U-C-000086 | Implementiert: öffentliche API und CLI, ausschließlich neuer Zielordner, kein Überschreiben/Ändern der Originale, keine Geometriefabrikation. |
| U-C-000089 Regression und Integrationsnachweise | A-C-00105, A-C-00106, A-C-00107, A-C-00108, A-C-00109 | U-C-000087, U-C-000088 | Core und GUI technisch nachgewiesen: Core-Suite 20/20; integrierte Suite ohne bekannte `gui_graphics_render_*` 36/36; abschließende Inspektionsrenderprüfung 3/3. Persönliche fachliche Abnahme offen. |

Meilenstein M4: Eintritt nach M1–M3 und den oben freigegebenen Verträgen.
Abnahme verlangt den vollständigen Alt-/Neuvergleich und bestehende
Core-Regressionen; technische GUI-Integration ersetzt keine persönliche
fachliche Nutzerabnahme. Schnittstellen CPPI-0060 bis CPPI-0073 sind im
vorhandenen Register ergänzt. Erzeugte neue Referenzmodelle sind lediglich
Build-/Testnachweise unter `build\standard\v2-modelle-test`, keine zweite
Spezifikation und keine Änderung der Geschwister-Originale.

### Tatsächlicher Nachweis M4, 02.11.2026

Release-Build aller Core-Ziele erfolgreich in `build\standard`.
Verwendete Werkzeuge: `cmake.exe` und `ctest.exe` aus
`C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`
(nicht auf dem Shell-PATH). Aufrufe: `cmake --build build\standard
--config Release` und `ctest --test-dir build\standard -C Release
--output-on-failure`. Für vorhandene Tests wurden `TEMP`/`TMP` auf
`CORE\build\standard\test-files` gesetzt, sodass Testdateien im Checkout
bleiben. Ergebnis des abschließenden Laufs: **20/20 bestanden in 63,11 s**;
neuer Fall `modell_geometrie_v2` 44,39 s.

Der neue Test vergleicht optimist, 420, folkeboot, bleistift, kegel,
kugel, platte, pyramide, quader, rolle und unregelmaessig exakt:
Element-/Gesamtmassen, Schwerpunkte, Trägheiten, physische und angezeigte
Meshes, Flutränder, Massenpunktprojektionen, Bewegungen/Hubwege,
Gruppen-/Elementstruktur, Fachblöcke/Materialmetadaten und Diagnose-IDs.
Die fünf dynamischen Quaderzustände für 0,04 s bei 0,01 s Schrittweite
sind nach Herausnahme ausschließlich der erwartbar unterschiedlichen
Modellfassungskennungen identisch.
Die wirklichen Varianten bleiben erhalten: optimist und 420 jeweils
19 Quellnetze, folkeboot 17, jeweils in genau einem OBJ.

Zusätzliche Negativ-/Vertragsfälle: fehlende/nichttextuelle Mesh-Referenz,
doppelte Element-/OBJ-ID, Flächen ohne `o`, nichtmetrische Einheit,
verbotene alte Einzeldateireferenz, fehlende/ungültige/doppelte Rolle,
keine Namensinferenz im Schema 2, globale/negative Indizes,
über mehrere Elemente geteilte und unbenutzte Quellen, alle
Elementfilter, direkte Knotenlinks, Herkunft sowie
Snapshotpersistenz nach Dateiveränderung. Nichttriangulierte
Schema-2-Flächen werden auch für die Teilsicht nicht repariert.
Bestehende Zielordner werden abgelehnt; Original-JSON und
Metadaten einschließlich Herkunftspfaden bleiben erhalten.

Die CLI-Konvertierung eines 420 wurde zusätzlich ausdrücklich ausgeführt
(`build\standard\v2-cli-demo`); ursprüngliche Modelle blieben unverändert.
`git diff --check` ohne Fehler. Alle bestehenden C-0.8-Arbeitsänderungen
sind erhalten; keine GUI-/Originalmodelldateien geändert und kein
Commit/Push/Tag/Release ausgeführt.

| Aktive Anforderung | Schritt(e) | Tatsächlicher Nachweis | Status |
|---|---|---|---|
| A-C-00105 | U-C-000086, U-C-000089 | `modell_geometrie_v2`: Ein-OBJ-Import, Referenzen, Indizes, Negativfälle | technisch erfüllt |
| A-C-00106 | U-C-000086, U-C-000089 | explizite Rollen und Namensunabhängigkeit, Rollen-Negativfälle | technisch erfüllt |
| A-C-00107 | U-C-000086, U-C-000089 | elf Alt-/Neuvergleiche, Simulation und bestehende 19 Regressionen | technisch erfüllt |
| A-C-00108 | U-C-000087, U-C-000089 | Quellregister, alle Filter, Herkunft, Knotenlinks, Snapshotdateiänderung; GUI-Integration und Inspektionsrenderprüfung laut folgendem Nachtrag | Core/GUI technisch erfüllt; persönliche Abnahme offen |
| A-C-00109 | U-C-000088, U-C-000089 | elf API-Migrationen, CLI-420, Original-/Überschreibschutz | technisch erfüllt |

### Integrationsnachtrag M4, 02.11.2026

Vom GUI-verantwortlichen Parent als ausgeführt und bestanden gemeldet:
Release-Build von GUI und sämtlichen Testzielen erfolgreich; auch die
Standard-GUI-EXE wurde neu gebaut. Die integrierte CTest-Suite ohne die
bekannten `gui_graphics_render_*` besteht mit **36/36 Tests in 216,29 s**
(20 Core-, 16 GUI-Tests). Die ausgeschlossenen bekannten Grafikfälle
werden dadurch nicht als behoben oder bestanden ausgewiesen.

Der GUI-Test `testUnifiedModels` konvertiert alle elf Referenzmodelle und
vergleicht Masse, Hüllgrenzen, Gruppen und Bewegungsanzahl sowie für die
sechs bisherigen Fach-Mesharten exakt Punkte, Dreiecke und Materialien.
Ein migriertes Element mit neutraler ID `teil_017`, expliziter Rolle
`ruder` und Bewegung besteht ebenfalls; die GUI benötigt dafür keine
Namensinferenz oder eigene Schemainterpretation.

Die abschließenden Inspektionsrendertests verwenden ein echtes
Schema-2-Folkeboot bei 100, 150 und 200 Prozent Skalierung:
**3/3 bestanden in 44,61 s**. Nachgewiesen sind der Baum mit
Quell-/Ableitungsgeometrien, Kamera, Checkboxen und der unveränderliche
Snapshot. Diese Integrationsnachweise ergänzen die zuvor lokal
ausgeführten Core-Tests; für diesen Dokumentationsnachtrag wurden keine
Tests erneut ausgeführt und keine Codeänderungen vorgenommen.

M4 ist damit technisch in Core und GUI nachgewiesen. Die persönliche
fachliche Nutzerabnahme bleibt ausdrücklich offen. Keine
Commit-/Push-/Tag-/Release-Aktion.
