# Core-Umsetzungsplan C-0.25: sailforce

**Stand:** 17.11.2026 · **Status:** FINAL fachlich; umgesetzt und gezielt getestet.
Reale Laufzeitintegration, GUI-Historie sowie Headless- und integrierte
Desktop-Echtzeit fuer alle drei Boote nachgewiesen (Abschnitt 9).
Bestands-Gesamtregression und persoenliche GUI-/DPI-Abnahme bleiben
offen; kein pauschaler Gesamtabschluss. Einziger Plan fuer C-0.25.
Erweiterte Nutzerfreigabe:
notwendige Aenderungen in SegelMacher und Modellrepository sind nun
beauftragt; Modellkorrekturen nur nach gepruefter Sicherung, fachliche
Vertraege bleiben massgeblich. Keine Git-/Release-Aktion.

Massgeblich sind [sailforce](../Spezifikationen/Spezifikation_sailforce.md)
mit A-C-00169 bis A-C-00177 und A-C-00179 bis A-C-00184 sowie
[Seitenkraft-SDP A-C-00178](../Spezifikationen/Spezifikation_Lateraldruckpunkt.md#6-c-025-aerodynamischer-seitenkraft-sdp).
Die GUI fuehrt ihren eigenen
[Plan G-0.25](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.25.md).
Bestehende Windarbeit verbleibt im
[Plan C-0.23](Umsetzungsplan_C-0.23.md); deren offene Nachweise werden
nicht durch die neue Planung als bestanden umgedeutet.

## 1. Bestand, Reihenfolge und externe Gates

Vor Vergabe am 17.11.2026 projektweit geprueft: A-C-00168,
U-C-000148, IDEE-C-00124. Vor Codebeginn vorhandene Interfaces bis
CPPI-0172 beruecksichtigt; konkrete neue Grundlagen als CPPI-0173 bis
CPPI-0185 vor ihrer Implementierung registriert.
Lokales HEAD `63beaaffa7a12013d9807348b579567c5b41447f` entsprach
GitHub-main; C-0.24 ist bereits als Tag vorhanden. C-0.25 ist die
ausdruecklich gewaehlte neue Arbeitszielversion, kein neu gesetzter Tag.
Neue Anforderungen A-C-00169 bis A-C-00184, Schritte U-C-000149 bis
U-C-000158. Vor tatsaechlicher Implementierung den dann aktuellen
Bestand und parallel hinzugekommene Schnittstellen erneut pruefen,
ohne diese vergebenen Kennungen umzunummerieren.

| Voraussetzung | Pruefung / Konsequenz | Aktueller Stand |
|---|---|---|
| Bestehender Wind- und Zustandsvertrag | Lokale Windfunktion, Eingabefassung, Punktkinematik und Historie fuer sailforce nachweisen. Nur benoetigte Gates aus C-0.23 uebernehmen, keine Behauptung dessen Gesamtabschlusses. | Windvertrag und reale sailforce-Nutzung einschliesslich paralleler Historienleser gezielt bestanden |
| SegelMacher-Basisbibliothek | Typisiertes Laden, Punkte und gekoppelte Lasten je Segel; konkrete Build-/Bibliotheksfassung und portable Abhaengigkeiten belegen. | `SegelMacher::Modell` eingebettet, rekursives Windows-DLL-Deployment ohne Producer-PATH getestet |
| SDP-Zusatzvertrag | `berechnen_mit_sdp` liefert transformierte Q/B/X/Z aus Einzellasten vor Streifenaggregation. Streifenlasten bleiben im lokalen Frame; SDP-Summen nicht erneut drehen. | Produzentenvertraege Release 4/4 bestanden; reale Core-Anbindung fuer fuenf Segel bestanden |
| Explizite Modellreferenzen | Vorhandene alte Lookup-Referenzen sind kein neuer Segelmodellvertrag. Zuordnung, Quelle, Geometrie, Manifest und Modellformat gegen reale Pakete pruefen. | Fuenf `sailModel`-Zuordnungen, Quellen-SHAs und kanonische OBJ-Geometrien geliefert; Modellformatregression bestanden |
| Formparitaet und Quellenkonsistenz | Montage-/Modellachsen und gespiegelte Form pruefen. Nutzerentscheidung 17.11.2026: bevorzugt nachgewiesene Laufzeitspiegelung durch SegelMacher mit gleichem Datensatz; falls nicht nachweisbar, passende Seitenvarianten. Wind, Kraefte, axiale Momente und SDP gemeinsam pruefen. | Explizite Produzentenspiegelung geliefert; Core-Darstellung nutzt dieselbe intrinsische x/z-Ebene, auch bei verwundenem Schothorn |
| Hardware und Leistungsziel | Ryzen 5 8600G / Radeon 760M, Windows; Messrahmen aus A-C-00175 und A-G-00172. | CPU per Windows CIM bestaetigt; je 6000 gueltige Headless-Schritte und G25-08 im echten Desktop fuer alle drei Boote bestanden |

**Geometrieentscheidung 17.11.2026:** Aktuelle Quellen erneut vergleichen.
Bei bestaetigtem Formkonflikt gilt die dem vorbereiteten Segelmodell
explizit zugeordnete intrinsische aerodynamische Quellgeometrie;
Darstellung daran angleichen. Kein Skalieren der Aerodynamik auf ein
abweichendes Darstellungsnetz, keine stille Rigg-/Montageaenderung.
Riggkonflikte gesondert klaeren. Die erweiterte Nutzerfreigabe liegt vor;
13 Modelldateien wurden vor Bearbeitung unter
`C:\spdeps\sailforce-model-backup-20261117` gesichert und verifiziert.

Fehlende Produzentenlieferungen und Modellschema-Abstimmung sind echte
Integrationsabhaengigkeiten. Ihre notwendigen Aenderungen sind durch den
erweiterten Nutzerauftrag inzwischen freigegeben und geliefert; die
Repositories und Versionsverlaeufe bleiben getrennt. Ersatzdaten oder
eine Naeherung gelten weiterhin nicht als bestandene reale Integration.

## 2. Umsetzungsschritte

**Montageentscheidung 17.11.2026:** Vorhandenen Segelhals und gerichtete
Mast-/Vorstagachse erhalten; die aerodynamische Form starr daran
platzieren. Abweichende Kopf-/Schothornlagen und konkrete Mast-/Baum-/
Sprietkonflikte vor Aenderungen einzeln zeigen und klaeren. Keine
automatische Skalierung, Streckung oder Riggkorrektur.

U-C-000149 bis U-C-000155 sind implementiert und integriert gezielt
getestet; U-C-000156 hat den Headless-Nachweis bestanden.
U-C-000157/158 haben die gezielte technische Regression, Historie und
integrierte Echtzeit bestanden; Bestands-Gesamtregression und
persoenliche Abnahme bleiben als getrennte Restanteile offen.
Einzelne Testanteile und verbleibende Grenzen sind in Abschnitt 9 getrennt
ausgewiesen. Abhaengigkeiten beziehen sich auf fertige, getestete Ergebnisse.

| U-ID | Arbeitspaket | Anforderungen | Abhaengigkeiten | Fertigkriterium |
|---|---|---|---|---|
| U-C-000149 | Vorhandene Kraft-, Wind-, Stellungs- und Ergebniswege inventarisieren; Produzenten-/Modellvertrag und Spiegelabbildung vereinbaren. Thread-/Ownership- und Versionsmatrix konkretisieren; jedes geplante neue oeffentliche Interface vor Implementierung einzeln im bestehenden Register kennzeichnen. Externe Lieferluecken sichtbar festhalten. | A-C-00177, A-C-00184 | keine; benoetigte Windnachweise als Eingang pruefen | C1: gepruefter Vertrag, konkrete IDs, Zuordnungen und explizite Liefergates; kein behaupteter SDP-Datenbestand |
| U-C-000150 | Vorbereitete Modelle laden, Integritaet und Form-/Quellenzuordnung pruefen; Bibliothek portabel anbinden. Explizite Aktivierung und konstante Luftparameter je Lauf validieren. Unveraenderliche Modelle/Vorbereitung wiederverwenden. | A-C-00169, A-C-00181, A-C-00183, A-C-00184 | U-C-000149, Basisbibliothek und Modellzuordnung | C25-01, C25-02, C25-03; fehlende oder unpassende Quelle wird diagnostiziert |
| U-C-000151 | Lokale Anstroemung aus bestehendem Wind und Starrkoerperpunktgeschwindigkeit mit voller 3D-Lage bilden. Live-quasistatische Stellungsfassungen an Schrittgrenzen anbinden, keine Stellgeschwindigkeiten erfinden. | A-C-00170, A-C-00172, A-C-00182, A-C-00184 | U-C-000150, benoetigte Wind-/Paritaetsnachweise | C25-04, C25-05, C25-09 |
| U-C-000152 | Ganze Segel gekoppelt auswerten, vollstaendige Streifenmomente nach S transformieren, Summen genau einmal in Dynamik und stationaere Suche integrieren; Fremdkraft erhalten. Bereichs-/Numerikfehler durch alle Integratorstufen weitergeben. | A-C-00171, A-C-00173, A-C-00179, A-C-00180 | U-C-000151 | C25-06 bis C25-08, C25-10; gemeinsame Bilanz und Hydrostatikregression |
| U-C-000153 | Konsistente unveraenderliche Ergebnisse mit IDs, Zeit/Fassung, Teil-/Summenlasten und expliziten Darstellungsbezuegen bereitstellen. Typisierte/serialisierte Projektionen, asynchronen Abruf und Historie erhalten; neue Vertragsversion gemaess C1 dokumentieren. | A-C-00174, A-C-00177, A-C-00182 | U-C-000152 | C25-09, C25-12, C25-14; Kernlastvertrag fuer GUI nutzbar |
| U-C-000154 | Produzenten-Zusatzdaten im Modellframe fuer signierte SDP-Rohsummen und Aufhebung auswerten; je Segel und gemeinsam liefern. Giermomentanteile getrennt erhalten und Druckpunkt-Snapshots integrieren. | A-C-00174, A-C-00178, A-C-00184 | U-C-000152, U-C-000153, gepruefter SDP-Zusatzvertrag und Formparitaet | C25-11, C25-12; ohne reale Zusatzdaten kein C3 |
| U-C-000155 | Tatsaechliche Streifengrenzlinien auf kompatibler Segelgeometrie mit stabiler Zuordnung erzeugen/cachen; Linienfassung und Transformation im Ergebnis verbinden. | A-C-00174, A-C-00176, A-C-00184 | U-C-000150, U-C-000151, U-C-000153, gepruefte Geometriezuordnung | C25-13; reale Linien und konsistente Lage statt Ersatzmesh |
| U-C-000156 | Gesamtlaufzeit mit allen benoetigten Auswertungen und Ergebnissen headless messen; Engpaesse ohne Vertragsverlust optimieren. Alle drei Referenzmodelle pruefen. | A-C-00175 | U-C-000153, U-C-000154, U-C-000155 | C25-15 headless; vollstaendige Messkonfiguration und Rohwerte, noch kein GUI-Nachweis |
| U-C-000157 | Neue Tests, nativen Build/CTest, Bestandsregression und Vertrags-/Registerdokumentation fuer die Core-Uebergabe abschliessen. Anforderungen gegen Core-Belege abgleichen, fehlende Gesamtintegration sichtbar halten. | A-C-00169 bis A-C-00184 | U-C-000149 bis U-C-000156 | C25-14/C25-16 ohne GUI-Anteil und C4; GUI-Integration bleibt fuer U-C-000158 offen |
| U-C-000158 | Nach Core-Uebergabe mit dem getrennten GUI-Stand die drei integrierten Echtzeitlaeufe und Live-/Wiedergabepruefung abschliessen. Gesamtbelege verknuepfen und Nutzerabnahme getrennt fuehren. Keine GUI-Codeaenderung in diesem Schritt. | A-C-00174, A-C-00175, A-C-00177, A-C-00178, A-C-00182 | U-C-000157 und GUI G3/G4 | C25-15/C25-16 mit GUI, G25-08 und C5; kein Gesamtabschluss bei fehlenden Gates |

## 3. Verbindliche Testfaelle

Fuer rein algebraische Last-/Momenten-/Snapshotparitaet gelten
`rtol=1e-10`, `atol=1e-9` in SI gemaess Bilanzvertrag. Referenzen,
Einheiten und verwendete Toleranzen je Test dokumentieren.
Produzenten-Modellabweichungen sind davon getrennt: dessen eigene
H1-/Aufloesungsnachweise bleiben sichtbar und werden nicht als
erfuellte Core-Transformationstoleranz ausgegeben.

| Fall | Eingabe und erwartetes Ergebnis |
|---|---|
| C25-01 Laden/Identitaet | Fuenf reale Segelpakete von 420/Folkeboot/Optimist, eindeutige Element-/Quellen-/Modellzuordnung; fehlendes Manifest, SHA-Fehler, unbekannte Version, doppelte/fremde Streifen-ID explizit ablehnen. |
| C25-02 Aktivierung | Kein/ein/zwei Segel, ausdrueckliche Aktivierung/deaktiviert, Voreinstellung an bei Zuordnung, vorhandene Wahl hat Vorrang; fehlende/defekte Referenz keine stille Nullkraft. Fremdkraft bleibt unabhaengig. |
| C25-03 Luft | Defaults 1.225 und 1.5e-5, geaenderte Werte, manifestierte Grenzen und Schnitt mehrerer Modelle, NaN/Inf/nichtpositiv; konstante Werte im Lauf und identische Historie. |
| C25-04 Wind/Kinematik | Gleiche Position wie Windfunktion; Translation und omega-kreuz-Hebelarm analytisch, gekraengtes Boot/geneigtes Vorstag; voller Vektor und korrekter Herkunft-/Stroemungsunterschied. |
| C25-05 Frames/Paritaet | Einbau-/Modellachsen explizit pruefen; beide Stellvorzeichen, Nullstellung und Formseiten. Vergleiche mit passendem Erzeugermodell; polare Kraefte und axiale Momente unter Spiegelung. Unbewiesene Paritaet liefert Diagnose. |
| C25-06 Lasten | Direkter Bibliotheksvergleich, volle Segelkopplung, Nullwind ueberall und einzelner windstiller Abfragepunkt; kein Nullsetzen eines Streifens gegen das gekoppelte Ergebnis. |
| C25-07 Bilanz | S-Bezugswechsel, freie Momente, Gegenkraefte, ein/zwei Segel plus Fremdkraft; Streifen-/Segel-/Gesamtsummen rekonstruieren genau dieselbe Solverbilanz ohne Doppelzaehlung. |
| C25-08 Rechenarten | Lasten in jedem Integrator-/Solverzustand aktuell; 6D-stationaere Residuen, kein neuer Kraengungssolver. Reine Hydrostatik unveraendert; aktives sailforce im ausdruecklichen Hydrostatikmodus wird abgelehnt. Keine universelle Konvergenzbehauptung. |
| C25-09 Live/Zeit | Winkelbestaetigung an naechster Schrittgrenze, alle Zwischenstufen gleiche Fassung; schnelle Aenderungsfolge, Pause/Weiterlauf und Laufende mit ausstehender Eingabe. Keine Stellkinetik, unveraenderte alte Snapshots. |
| C25-10 Fehler | Modellbereich 0/50 m/s und knapp ausserhalb, Luftgrenzen, ungueltige Hoehe, NaN/Inf und Bibliotheksnumerikfehler auch in Zwischenstufe. Letzter gueltiger Dynamikzustand erhalten; kein falscher Solvererfolg. |
| C25-11 SDP | A-C-00178 vollstaendig: Q/B/X/Z aus tatsaechlichen Lasten vor Streifenkompensation, 1-N-/5-%-Schwellen exakt und beidseitig, Zweilastreferenz x=9 m, reines Moment, Teil-/Gesamtgueltigkeit, Rotation und `-y*Fx`-Gegenbeispiel. Fremdkraft nie direkte Quelle. |
| C25-12 Abruf | Verzoegerter/fehlender Leser und parallele Abfragen bei weiterlaufendem Core; alle Felder eines Ergebnisses zustandskonsistent. Druckpunkte fehlen bei Datenfehler nicht als scheinbar gueltige Null. |
| C25-13 Linien | Echte horizontale Streifengrenzen im Segelmodellframe auf passender Flaeche, Dreieck/Viereck, beide Seiten, Lageaenderung ohne neuen Schnitt, Quellenwechsel mit Invalidierung. Keine Physikaenderung durch Geometrieabruf. |
| C25-14 Version/Historie | Typisierte/serialisierte Paritaet und Feature-/Versionsmatrix; Altdaten ohne Segelphysik lesbar, fehlende neue Daten explizit; neue Pflichtdatenfehler und fremde Modell-/Lauffassungen abweisen. |
| C25-15 Leistung | Drei Modelle, je 6000 gueltige 0.01-s-Schritte in <=60 s nach Vorbereitung; Endzustandsauswertung und Ergebnisbereitstellung inklusive. Median/p95/Maximum und Rueckstand berichten. GUI-Lauf ebenfalls Echtzeit; Ladezeit und Speicher separat. |
| C25-16 Regression/Integration | Bestehende Hydrostatik, Hydrodynamik, Fremdkraft, Wind, Montage, Import, stationaere Fahrt und Wiedergabe; vollstaendiger nativer CTest. Realer integrierter GUI-Lauf und fachliche Abnahme getrennt. |

## 4. Meilensteine und Abschluss

| Gate | Eintritt | Abnahme |
|---|---|---|
| C1 Vertrag | U-C-000149 | Bestehende Vertrage und neue konkrete CPPI-IDs geprueft; Produzenten-/Modell-/Paritaetslieferungen mit explizitem Status und Verantwortungsgrenze dokumentiert. |
| C2 Lastkern | C1; U-C-000150 bis U-C-000153 | C25-01 bis C25-10 und benoetigte C25-12/C25-14 bestanden; Kernvertrag fuer GUI vorhanden. Ein C2 ersetzt nicht den fehlenden SDP. |
| C3 Vollstaendiger Datenvertrag | C2; U-C-000154/U-C-000155 und reale Zusatzdaten/Geometrie/Paritaet | C25-11 bis C25-14 bestanden; alle Punkte, Lasten und Linien an reale Pakete gebunden. Externe Lieferung inzwischen vorhanden. |
| C4 Technische Core-Abnahme | C3; U-C-000156/U-C-000157 | C25-15 headless und Core-Anteil C25-16 bestanden, Core-Rueckverfolgung vollstaendig; GUI-Anteil explizit offen. Kein GUI-Gate als Voraussetzung. |
| C5 Gesamtintegration | C4, GUI G3/G4 und U-C-000158 | Gemeinsamer Echtzeitnachweis C25-15/G25-08 und GUI-Anteil C25-16, konsistente Live-/Wiedergabedaten; Nutzerabnahme separat ausweisen. |

Nach Umsetzungsauftrag zuerst gezielte neue Tests, dann vom Core-Stamm:

```powershell
cmake -S Code -B build\standard -DBUILD_TESTING=ON
cmake --build build\standard --config Release
ctest --test-dir build\standard -C Release --output-on-failure
```

Bei Aenderungen am Messgating zusaetzlich instrumentierten Build mit
`SEGELPHYSIK_TEST_INSTRUMENTATION=ON` pruefen. Konkrete Testselektoren,
Build-/Bibliotheksfassung und numerische Rohdaten hier nachtragen.
Dokumentfinalisierung ist kein bestandener Build oder Test.

## 5. Anforderungsabdeckung

| A-ID | Umsetzung | Tests | Status |
|---|---|---|---|
| A-C-00169 | U-C-000150, U-C-000157 | C25-01, C25-14 | implementiert; reale fuenf Pakete und Modellformatregression bestanden |
| A-C-00170 | U-C-000151, U-C-000157 | C25-04, C25-16 | implementiert; Wind-/Punktkinematikvertrag und reale Kopplung bestanden |
| A-C-00171 | U-C-000152, U-C-000157 | C25-07, C25-16 | implementiert; Streifen-/Solverbilanz und parallele Fremdkraft bestanden |
| A-C-00172 | U-C-000151, U-C-000157 | C25-04, C25-05 | implementiert; 3D-Punktwind und beide Formseiten gezielt getestet |
| A-C-00173 | U-C-000152, U-C-000157 | C25-05, C25-06, C25-07 | implementiert; S-Transformation, Eigenmomente und reale Summen getestet |
| A-C-00174 | U-C-000153, U-C-000154, U-C-000155, U-C-000157, U-C-000158 | C25-09, C25-12, C25-14 | implementiert; paralleler historischer Core-Abruf und GUI-Historie bestanden; C5 offen |
| A-C-00175 | U-C-000156, U-C-000157, U-C-000158 | C25-15, G25-08 | erfuellt: Headless und regulaerer echter Desktoplauf fuer alle drei Boote, je 6000 gueltige Schritte; Framegrenzen ebenfalls bestanden |
| A-C-00176 | U-C-000155, U-C-000157 | C25-13 | implementiert; reale gecachte Schnitte, stabile Grenzen/Nachbar-IDs und Spiegelparitaet getestet |
| A-C-00177 | U-C-000149, U-C-000153, U-C-000157, U-C-000158 | C25-12, C25-14 | implementiert; Version 8, Pflichtfeldparser, CPPI-0173 bis 0208 und asynchroner Ergebnissnapshot; Gesamtregression offen |
| A-C-00178 | U-C-000154, U-C-000157, U-C-000158 | C25-11 | Rohsummen-/Schwellenvertrag und reale Produzenten-SDPs integriert getestet; C5 offen |
| A-C-00179 | U-C-000152, U-C-000157 | C25-08, C25-16 | implementiert; Dynamik und stationaere Fahrt angebunden, reine Hydrostatik getrennt; Gesamtregression offen |
| A-C-00180 | U-C-000152, U-C-000157 | C25-01, C25-03, C25-10 | implementiert; Bereichs-/Kontext-/Numerikfehler explizit, letzte gueltige Snapshots unveraendert |
| A-C-00181 | U-C-000150, U-C-000157 | C25-02 | implementiert; Voreinstellung an, explizite Auswahl und inaktive Segel ohne Wind getestet |
| A-C-00182 | U-C-000151, U-C-000153, U-C-000157, U-C-000158 | C25-09, C25-12 | implementiert; immutable Winkelrevisionen, GUI-Livebestaetigung und Historie bestanden |
| A-C-00183 | U-C-000150, U-C-000157 | C25-03 | implementiert; inklusive Manifestgrenzen/Defaults und unveraenderliche Laufdaten getestet |
| A-C-00184 | U-C-000149 bis U-C-000155, U-C-000157 | C25-01, C25-05, C25-11, C25-13 | implementiert; Quellen-SHAs, starre Montage, Produzentenspiegelung und reale Flaechen-/Linienparitaet getestet |

## 6. Konsistenz- und Umsetzbarkeitspruefung 17.11.2026

- Wind bleibt bestehende Physik; neue Punkte nutzen denselben Vektorvertrag.
- Streifen-/Summenmomente haben S-Bezug; Abfragepunkt, Darstellungsbezug
  und Seitenkraft-SDP werden nicht verwechselt.
- SDP verwendet echte Seitenkraftmomente und B; weder volles Giermoment
  noch Streifensummen liefern einen unerkannten Ersatz.
- Livewinkel sind quasistatisch. Keine unbestimmte Stellgeschwindigkeit,
  keine neue laufende Aktivierungs- oder Luftdatensteuerung.
- Anforderungsabdeckung und Reihenfolge sind geplant; die Umsetzung
  ist mit expliziten Liefergates strukturiert; die erweiterten Lieferungen
  ermoeglichen inzwischen die reale Integration.
- CPU/GPU-Abschaetzung, Produzentenvorpruefung, Numeriktest, reale
  Integration, Echtzeitleistung und Nutzerabnahme bleiben getrennte Belege.

**Tatsaechliche C-0.25-Implementierungsnachweise:** Historischer
Grundlagenstand in Abschnitt 8, aktueller Integrationsstand in Abschnitt 9.
Kein pauschaler Gesamtabschluss C1-C5.

## 7. Nachweis der Dokumentfinalisierung

Am 17.11.2026 strukturell geprueft: beide finalisierten JSON-Listen
parsebar, 15 IDEE-zu-A-Zuordnungen gueltig, alle 23 neuen Anforderungen
projektuebergreifend genau einmal definiert und in ihren Plaenen
abgedeckt. 35 Markdown-Dateiverweise/Anker in den neuen Abschnitten
sowie die neuen JSON-Bezuege aufloesbar. Die 10 Core- und 7 GUI-Schritte
einschliesslich der projektuebergreifenden Liefergates sind azyklisch.
`git diff --check` in beiden Projekten und Whitespacepruefung der neuen
Dateien bestanden. Das prueft Dokumentkonsistenz, keine Implementierung,
physikalische Numerik, Echtzeitleistung oder Nutzer-Sichtabnahme.

## 8. Historischer erster Teilstand vor erweiterter Freigabe

Nach ausdruecklichem Umsetzungsauftrag in Core/GUI begonnen. Ohne
separate Freigabe keine Aenderung an SegelMacher oder Modelldateien.
Dieser erste Teilstand dokumentiert die damalige Freigabegrenze;
die anschliessend erteilte Erweiterung ist im Dokumentkopf festgehalten.
Die lesende Quellenpruefung hat weiterhin echte Liefergates ergeben:

- SegelMacher-`Transformation` erlaubt eigentliche Rotationen, keine
  Formspiegelung. Nachgewiesene Laufzeitspiegelung beziehungsweise
  passende Seitenvarianten fehlen weiter.
- 420: gepruefte Eckabstaende passend, aber noch keine vollstaendige
  Flaechen-/Quellen-/Laufzeitmodellzuordnung nachgewiesen.
- Folkeboot-Fock Vorliek Darstellung/Aero 9.696453/5.5 m,
  Gross 9.42/8.75 m; Optimist Vorliek 1.628736/1.73 m.
  Aerozuordnung und Originalseite dort nicht hinreichend vorhanden.
  Keine stillen Modell-/Riggkorrekturen; separate Freigabe erforderlich.
- Portable Produzenten-Bibliotheksanbindung und integrierter Laufvertrag
  bleiben offen. Deshalb noch kein Lastkern C2 und kein GUI-Snapshotvertrag.

Unabhaengig implementiert, **nicht in die Solverbilanz eingeschaltet**:

| Baustein / Beleg | Zugehoeriger Schritt / Testanteil | Grenzen des Nachweises |
|---|---|---|
| [sailforce.hpp](../Code/include/segelphysik/sailforce.hpp), [sailforce.cpp](../Code/src/sailforce.cpp), CPPI-0173 bis CPPI-0183 | U-C-000150/152/154: Luftdaten gegen explizite inklusive Modellgrenzen pruefen; volle Kraft/Eigenmomente mit starrer Pose nach S uebertragen; Q/B/X/Z signiert summieren und SDP mit 1 N/5 % bestimmen | Keine eigene Ersatz-Aerodynamik, kein Loader, keine Bilanzintegration; Schnitt mehrerer Modellbereiche durch Pruefung jedes aktiven Modells bei spaeterer Anbindung |
| `wind_am_punkt`, CPPI-0184/0185; gemeinsamer Windvektor-Rechenweg mit Mastauswertung | U-C-000151 / Teil C25-04: beliebige starr mitbewegte Punkte ohne Mastvoraussetzung; bestehendes Hoehenprofil, Weltwind und volle 3D-Punktgeschwindigkeit | Noch keine Segelstellungs-/Modellkopplung |
| [sailforce_tests.cpp](../Code/tests/sailforce_tests.cpp), CTest `sailforce_contract` | Teile C25-03/04/07/11: inklusive und benachbarte Schwellen, x=9-m-Gegenkraftbeispiel, Einzel-/Gesamtgueltigkeit, reine Momente, Rotation/Translation/S-Bezug, Fehler/Overflow, Nullwind, geneigte Lage und Mastparitaet | Algebraische Referenzen, kein direkter Produzentenvergleich in Core, keine Echtzeitabnahme |

Build mit bestehender nativer Release-Konfiguration `C:\spdeps\guibuild`
(GUI addiert den aktuellen Core-Quellbaum). Erstes Configure ueber das
PATH-CMake 4.4 scheiterte am Versionsmix mit vorhandenen CMake-3.31-Modulen
(`No preprocessor test for Renesas`). Mit dem vorhandenen Visual-Studio-
CMake 3.31 explizit konfiguriert und gebaut, keine Abhaengigkeiten installiert.
Der alte Core-Cache `build\standard` verweist noch auf den frueheren
Python-Projektpfad und wurde nicht geloescht oder umgebogen.

Tatsaechliche Pruefungen:

- Release-Ziele `sailforce_tests`, `wind_tests`,
  `lateraldruckpunkt_tests`, `bilanz_tests` gebaut.
- CTest `^(sailforce_contract|wind_contract|lateraldruckpunkt|kraft_momentenbilanz)$`:
  **4/4 bestanden**.
- Produktive GUI `C:\spdeps\guibuild\Release\segelphysik_gui.exe` und
  `segelphysik_gui_core_tests` gegen den geaenderten Core gebaut.
- `gui_core_integration`: **fehlgeschlagen**, `Core rejected the reference model`.
  Der konfigurierte Legacy-Pfad
  `Modelle_alt\quader\modells_information.json` existiert nicht;
  bereits der gesamte konfigurierte Ordner `Modelle_alt` fehlt.
  Keine Fixture erfunden, kein Test abgeschwaecht und keinen Vollregressions-
  erfolg behauptet. Dieser Nachweis bleibt offen.
- Kein vollstaendiger CTest-Lauf und kein realer 60-s-/GUI-Leistungsnachweis.
  Aenderungen uncommittiert.

## 9. Aktuelle reale Integration und Headless-Nachweis

Nach erweiterter Freigabe sind die zuvor fehlenden Lieferungen vorhanden:
SegelMacher-Laufzeitspiegelung mit korrekter Axialbehandlung, exakte
Streifengrenzen, einbettbare Bibliothek und rekursives Windows-DLL-Deployment.
Die fuenf Modellquellen sind explizit zugeordnet; Darstellung und
Montagepunkte wurden nach verifizierter Sicherung aus SHA-geprueften
intrinsischen Quellen erzeugt. Die Modellmigration dokumentiert konkrete
Geometrieabweichungen, ohne Riggachsen, Segelhals oder Aerodynamik zu skalieren.

Die verwundene 420-Fock besitzt ein reales Schothorn mit y=0.034534 m.
Dieses ist kein Beleg fuer eine verschobene intrinsische Spiegelebene:
Aeroformen werden explizit an y=0 gespiegelt; GUI-Geometrie und
Overlay verwenden dieselbe Spiegelung und starre Pose. Die Vorliekachse
bleibt in dieser Ebene. Ein Integrationstest vergleicht jeden gespiegelten
Darstellungspunkt mit dieser Abbildung.

**Implementierung:** `sailforce_start` bereitet Modelle, Integritaet,
Luftbereiche und echte Schnittlinien einmal vor. Alle Integratorstufen,
Endzustaende und stationaeren Auswertungen erhalten den historischen
Kontext. Quellen-SHAs und Manifeste gehen in die Cacheidentitaet ein.
Version 8 liefert volle Lasten, Rohsummen-SDPs und stabile Linien-/Nachbar-IDs.
Keine Segelmassen- oder Stellgeschwindigkeitsimpulse werden erfunden.

**Gezielte Nachweise, Release /WX, VS-CMake 3.31:**

- `sailforce_integration`: reale Zuordnungen 2/2/1; volle Bilanz neben
  Fremdkraft, SDP, Nullwind, aktive/inaktive Segel, Kontextfehler,
  Dynamik und stationaere Auswertung, Spiegelung, gecachte Grenz-IDs,
  vier parallele historische Leser und unveraenderte alte Snapshots.
  Erweiterter Lauf nach Integration ebenfalls bestanden (18.02 s):
  Bereichsfehler erst in einer Integratorzwischenstufe bei gueltigem
  Start, unveraenderter letzter gueltiger Zustand, wirklicher
  Kurvenfahrtpfad mit Paritaet der aeusseren Sechskomponentenlast und
  Datei-Simulator mit expliziter deaktivierter Segelauswahl fuer alle
  drei Modelle (je Start-/Endzustand, Version 8).
- 12/12 kombinierte Tests bestanden:
  `wind_contract`, `sailforce_contract`, `hydrostatik_referenzen`,
  `lateraldruckpunkt`, `kraft_momentenbilanz` sowie alle sieben
  `modellformat_03_*`-Tests.
- GUI `gui_force_display` mit striktem Version-8-Pflichtfeldparser
  einschliesslich Grenzzuordnung bestanden; separater echter
  `--sailforce-history-only`-Test mit 420 bestanden.
- Produzent: 4/4 Vertrags-/Paritaetstests bestanden. Optimierung der
  typisierten Geometrie im Lastkernel und bedarfsweiser Fehlertexte
  ohne Moden-/Physikreduktion; 24 Vorher/Nachher-Pruefsummenpaare identisch.
  Core-Profil pro Auswertung: 420 6.79 -> 0.95 ms, Folkeboot 17.58 ->
  2.03 ms, Optimist 1.88 -> 0.32 ms. Das sind Profile, keine 60-s-Belege.

### C25-15: vollstaendige Headless-Dynamik

Ryzen 5 8600G / Radeon 760M, Windows, Release, nicht instrumentiert.
Je Boot 6000 gueltige Schritte dt=0.01 s, Start aus hydrostatischem
Gleichgewicht mit Geschwindigkeiten null, Hydrodynamik aktiv, alle Segel
aktiv bei 0 Grad, Wind 3 m/s aus 65 Grad bei h0=10 m, Luft 1.225 kg/m3
und 1.5e-5 m2/s, keine Fremdkraft. Einschliesslich interner
Integratorauswertungen und vollstaendiger Endzustandsauswertung je Schritt.
Vorbereitung separat; keine gleichzeitigen Builds.

| Boot | 6000 Schritte Wandzeit | Median / p95 / Maximum je Schritt | Vorbereitung | Rueckstand |
|---|---:|---|---:|---:|
| 420 | 59.0723 s | 9.7355 / 10.6166 / 14.8984 ms | 3.6407 s | 0 s |
| Folkeboot | 27.3152 s | 4.3366 / 5.6152 / 26.9433 ms | 1.2577 s | 0 s |
| Optimist | 6.0425 s | 0.9893 / 1.1081 / 3.1710 ms | 0.5151 s | 0 s |

Alle drei Headless-Grenzen <=60 s bestanden. 420 hat nur geringe
Leistungsreserve; daraus folgt ausdruecklich kein bestandener GUI-Lauf.
Working Set nach Vorbereitung: 45,969,408 / 48,726,016 / 28,012,544 Bytes.
Peak Working Set des gemeinsamen Prozesses: 87,257,088 Bytes; dies ist
ein kumulierter Prozesspeak, kein isolierter Peak je Modell.

Unveraenderte [Rohdaten](../Dokumentationen/Nachweise_C-0.25/headless.json)
mit 18,000 Einzelzeiten, Konfiguration, Speicherwerten und vollstaendigen
Endzustaenden; SHA-256
`b39b2141dfeb5ece3ec237641401dea768df5ec5e8531e3c3e65cfcd7f76f23c`.
Die Ergebniszustaende enthalten die jeweiligen Modell- und Quellenfassungen.

### Offene Gesamtnachweise und Bestandsgrenzen

Die wiederhergestellten echten Legacy-Fixtures aus Modellcommit
`be18d6b920ece661066f5c5748f37db576e6b020` enthalten elf Modelle/38 Objekte.
Der aktuelle Core akzeptiert jedoch ausschliesslich Format 0.3
`modells_data.json`, waehrend zahlreiche alte CTest-Aufrufe Format 0.2
`modells_information.json` uebergeben. Daher scheitert unter anderem
`gui_core_integration` weiterhin schon am Import. Der erste Gesamtlauf
wurde nach wiederholten gleichartigen Fixture-/Katalogfehlern gestoppt.
Keine Tests entfernt, keine Formatunterstuetzung vorgetaeuscht und kein
vollstaendig gruener CTest behauptet. Die moderne Modellformatregression
ist davon getrennt bestanden.

GUI-Desktopmessung G25-08/C5 und fachliche Nutzer-Sichtabnahme sind hier
noch nicht geschlossen. Alle Aenderungen bleiben uncommittiert; kein
Push, Tag oder Release beauftragt oder ausgefuehrt.

**Erster echter Desktop-Gesamtlauf:** Start-Race im Testharness behoben
(Start erst nach vorbereiteten Reitern). Anschliessend je 6001 gueltige
Zustaende ohne Core-/VTK-Fehler, aber G25-08 nicht bestanden:

| Boot | Core-Wandzeit im GUI-Lauf | Frame p95 / Maximum |
|---|---:|---|
| 420 | 78.84 s | 140.95 / 186.32 ms |
| Folkeboot | 39.54 s | 93.59 / 149.80 ms |
| Optimist | 10.59 s | 73.74 / 95.11 ms |

Die drei echten vorbereiteten Desktopaufnahmen wurden visuell geprueft:
leichte Schnittlinien und gemeinsamer SDP sind sichtbar. Das ist keine
persoenliche Nutzerabnahme und ersetzt den fehlenden Fluessigkeitsnachweis
nicht. GUI-Hotspotanalyse/Optimierung wird fortgesetzt; keine Lockerung
der Frame- oder Echtzeitkriterien.

### Weitere GUI-Optimierung und Wegfall doppelter Lastberechnung

Ein spaeterer echter 420-Desktoplauf vor Snapshot-Handoff liefert
6001 gueltige Zustaende, Core 79.280 s und Wandzeit 79.307 s.
Frame-p95 38.396 ms, Maximum 54.659 ms: das Maximum besteht,
p95 <=33.3 ms und Echtzeit noch nicht. Die Mediananteile sind Abruf
16.365 ms (Core-Auswertung/JSON 8.978, Decode 1.479, Motion 4.296 ms)
und Szene 21.667 ms (darin VTK-Render 20.327 ms). Das belegt keine
alleinige GPU-Ursache; ein vorheriger 63.326-s-Lauf zeigt zudem
klaerungsbeduerftige Streuung. Rohdaten im GUI-Nachweispfad G25-08.

Die Aufbereitung wird bereits getrennt fuer Worker und Reader gehalten;
gegenseitige Kraftcache-Verdraengung war dort keine Ursache. Bestaetigte
Doppelarbeit ist die erneute Kraftauswertung im Reader fuer einen bereits
vom Worker vollstaendig ausgewerteten Endzustand.

CPPI-0206..0208 sind deshalb implementiert: `ErgebnisSnapshot` friert
Zustand und Last mit gemeinsam besessener Modellbasis ein;
`Modellaufbereitung::snapshot` uebernimmt den passenden Endzustandscache,
`snapshot_json` serialisiert ihn ohne neue Kraftberechnung. Keine
Modellkopie je Snapshot, keine GUI-Abhaengigkeit, kein Worker-Mutex beim
Lesen. Darstellungsableitungen wie geometrische Lateralpunkte bleiben
moeglich. GUI-Handoff mit begrenzten 64 Snapshots und unveraenderter
Alt-Historie integriert; Winkel-/Historientest einschliesslich
64/65-Verdraengungsgrenze bestanden.

Standard-CTest `modellaufbereitung` und `sailforce_integration` nach
Erweiterung **2/2 bestanden** (3.34/16.16 s), einschliesslich
byteidentischem JSON, kompatiblem Format, ungueltigem Pflichtkontext,
parallelen Lesern und Modellbesitz nach Freigabe des Erzeugers.
Separater nativer instrumentierter Build ohne Qt/VTK erfolgreich;
`modellaufbereitung` bestanden (4.74 s), neue Zaehlerpruefung:
zweimalige Snapshot-Serialisierung erhoeht weder `kraftauswertung`
noch `modellbasis_aufbau`. Bestehende absolute Geometrie-/Kopierzaehler
und deren Assertions unveraendert. Instrumentierte reale
`sailforce_integration` ebenfalls bestanden (18.27 s).

Diagnostisches Release-Kurzprofil, Mittel aus 20 verschiedenen gueltigen
Zustandspositionen je Boot; gleiche Lasten, byteidentisches JSON:

| Boot | Reader: neue Kraefte + JSON | Snapshot-JSON | Cacheuebernahme |
|---|---:|---:|---:|
| 420 | 7.539 ms | 5.728 ms | 0.0091 ms |
| Folkeboot | 5.259 ms | 3.112 ms | 0.0088 ms |
| Optimist | 1.683 ms | 1.265 ms | 0.0049 ms |

Unveraenderte [Profilrohdaten](../Dokumentationen/Nachweise_C-0.25/snapshot-profile.json);
SHA-256 `09d9f58ebbc3309c9f69ac7034a1b65303146ae0591f061ae34ecc8684383bc3`.
Das isoliert die vermiedene Doppelarbeit, ersetzt aber keinen echten
60-s-GUI-Nachweis. Segelbahnen, Moden, Physik und Abnahmeschwellen bleiben
unveraendert. Kontrollmessung mit/ohne Anzeige und G25-08 bleiben offen.

### Kontrolllauf nach Snapshot-/GUI-Wiederverwendung

Nach Snapshot-Handoff und exakter Motion-Broadphase/Konturbuffer-
Wiederverwendung liefert der kontrollierte 420-Vergleich:

| Variante | Corezeit | Wandzeit | Frame-p95 / Maximum |
|---|---:|---:|---|
| ohne Anzeige | 59.4348 s | 60.0150 s | nicht anwendbar |
| echte Anzeige | 61.2069 s | 61.3730 s | 30.0578 / 49.7771 ms |

Je 6001 Zustaende; Endphysik exakt identisch. 2194/2194 Snapshot-Treffer,
kein historischer Fallback. Motion-Vorbereitung in der Kurzprobe
3.969 -> 3.252 ms, Kontaktparitaet zum ungefilterten Pfad einschliesslich
trocken/getaucht geprueft. Gesamtes Worker-Mutexwarten 1.54 ms,
Snapshot-Capture 83.23 ms; 61.112 s stecken in `schritt`.
Kein belegter Worker-Pacing-/Mutexstau und keine bewiesene alleinige
GPU-Ursache. Die Framekriterien bestehen, G25-08 verfehlt die
60-s-Coregrenze aber noch um 1.2069 s.

Rohdaten: `C:\spdeps\guibuild\evidence\G25-snapshot-420-motion-comparison\sailforce-performance.json`.
Die anschliessende Core-Schnittoptimierung war in diesem Lauf noch nicht
enthalten.

### Punktabstaende im exakten Ebenenschnitt nur einmal berechnen

In `wasseranteil` werden dieselben vorzeichenbehafteten Punkthoehen
zunaechst fuer die Hoehengrenzen und danach fuer jede Dreieckskante
wiederholt bestimmt. Nun werden die Werte aus dem vorhandenen ersten
Durchlauf je Schnitt gespeichert und fuer alle Kanten wiederverwendet.
Sie werden bei jeder neuen Pose vollstaendig neu berechnet: kein
veralteter Wasserstandscache, keine ausgelassene Integratorstufe, keine
Geometrie-, Segelbahn-, Moden-, Physik- oder Toleranzaenderung.

Bestehender 420-Hydrobenchmark, 9 Wiederholungen mit je 500 Auswertungen:
Median 0.2385225 -> 0.2069175 s (**13.25 % weniger Zeit**),
Bereiche 0.2337644..0.2643904 -> 0.2032119..0.2098805 s.
Pruefsumme exakt unveraendert `28.076378071359326`.
[Vorher](../Dokumentationen/Nachweise_C-0.25/waterplane-before.json),
[nachher](../Dokumentationen/Nachweise_C-0.25/waterplane-after.json).

Nach jeweils 100 echten Dynamikschritten sind die vollstaendigen
Endzustaende aller drei Boote gegen das vorherige Snapshot-Kurzprofil
exakt identisch. 420-Kurzprofil 0.992031 -> 0.9677286 s;
[neue Rohdaten](../Dokumentationen/Nachweise_C-0.25/waterplane-profile-after.json).
Dies ist weiterhin nur ein Kurzprofil, kein neuer 60-s-Nachweis.

Nach dieser Aenderung Standard- und unabhaengig instrumentierter nativer
Build jeweils mit `modellaufbereitung`, `sailforce_integration` und
`hydrostatik_referenzen` **3/3 bestanden**. Der instrumentierte Test
prueft jetzt auch die Uebernahme selbst: weder Snapshot-Capture aus dem
fertigen Endzustand noch anschliessende Serialisierung erhoehen
`kraftauswertung` oder `modellbasis_aufbau`.

### Desktoplauf nach Schnittoptimierung, vor Release-IPO

Produktive GUI `C:\spdeps\guibuild\Release\segelphysik_gui.exe`,
Renderharness und Adapter-/Parserziele gegen diesen Core mit Release /WX
erfolgreich gebaut. Parser: 766 Pruefungen bestanden; echte
420-Snapshot-Historie, Livewinkel, 64er-Cache, JSON-Paritaet sowie
Desktop-Reflexions-/Wasserlinienparitaet bestanden.

Kontrollierter 420-Vergleich: ohne Anzeige Core 58.2297756 s,
Wand 60.0110846 s; mit Anzeige Core 59.7495093 s, Wand 60.0663862 s.
Frame-p95 29.9420 ms, Maximum 51.3272 ms; 2160/2160 Snapshot-Treffer,
kein Fallback, exakt identische Endphysik. Dieser einzelne
Kontrolllauf besteht die Core- und Framekriterien.

Der darauf folgende regulaere Dreiboottest liefert jedoch:

| Boot | Corezeit | Wandzeit | Frame-p95 / Maximum |
|---|---:|---:|---|
| 420 | 60.7687345 s | 61.0248788 s | 31.5942 / 46.9498 ms |
| Folkeboot | 30.1871174 s | 60.0057377 s | 26.3708 / 37.3823 ms |
| Optimist | 9.0541307 s | 60.0032377 s | 28.6827 / 93.1439 ms |

Je 6001 gueltige Zustaende, Snapshot-Treffer 2107/2480/2434,
kein Fallback. Alle Framekriterien bestehen. Folkeboot und Optimist
bestehen auch die Corezeit; **420 verfehlt sie um 0.7687345 s
(1.28 %)**. Der fruehere erfolgreiche Kontrolllauf wird deshalb
nicht als bestandener regulaerer Dreiboottest ausgegeben:
**G25-08 insgesamt weiterhin offen**, G25-09/Nutzerabnahme ebenfalls.

Im regulaeren 420-Lauf stecken 60.6719582 s in `schritt`, Capture
83.7503 ms, Mutexwarten insgesamt 1.7709 ms. Kein belegter Mutex-
oder Worker-Pacing-Stau; keine alleinige GPU-Ursache behauptet.
Keine weiteren unprofilierten Wiederholungen und keine Reduktion der
Physik, Moden oder Segelbahnen.

Rohdaten und drei echte Desktopaufnahmen:
`C:\spdeps\guibuild\evidence\G25-final-three-boats\sailforce-performance.json`;
Kontrollvergleich:
`C:\spdeps\guibuild\evidence\G25-final-core-420-comparison\sailforce-performance.json`.
GUI-Plan Abschnitt 13 fuehrt die ausfuehrlichen Teilnachweise.
Alle Messprozesse beendet; Aenderungen uncommittiert.

### Weitere gemessene Eingrenzung und Release-Linkzeitoptimierung

Instrumentiertes 420-Kurzprofil, 100 Dynamikschritte:
Nasspanels 291.116 ms (davon Welttransformation 121.589 ms),
Reibungsbeitraege 212.762 ms, Hydrostatik 166.119 ms
(davon Ebenenschnitt 164.917 ms), Druck 74.262 ms,
Seitenkraft 3.232 ms. Die enthaltenen Unterzeiten sind keine
additiven unabhaengigen Anteile. Das grenzt die eigentlichen Hotspots
ein; die unveraenderten Polardaten sind nicht der 420-Engpass.
[Instrumentierte Phasenrohdaten](../Dokumentationen/Nachweise_C-0.25/hydro-phases.json).

Ein Polaren- und ein zustandslokaler Panelcache brachten in den
Release-Kurzproben keinen belastbaren Gewinn. Beide Versuche wurden
vollstaendig entfernt, statt zusaetzliche Speicher-/Cachekomplexitaet
ohne Nachweis beizubehalten.

Fuer den Core ist jetzt nach `CheckIPOSupported` ausschliesslich im
Release-Build interprozedurale Optimierung aktiv. MSVC kompiliert den
Core mit `/GL`; Konsumenten erhalten `/LTCG`. So kann der Linker unter
anderem die haeufigen kleinen Pose-/Vektoraufrufe ueber
Uebersetzungseinheiten hinweg optimieren. Keine Fast-Math-Option,
`/fp:precise` und alle Validierungen unveraendert; Debug unveraendert.
Fehlender Toolchain-Support wird explizit gemeldet.

Nicht instrumentiertes Kurzprofil, vorherige Schnittoptimierung als
Vergleich: 420 100 Schritte 0.9677286 -> 0.9416165 s,
Snapshot-JSON 5.81908 -> 5.42575 ms. Die vollstaendigen Endzustaende
aller drei Boote nach 100 Dynamikschritten sind exakt identisch.
Dies ist noch kein integrierter Echtzeitnachweis.
[Nicht instrumentierte IPO-Kurzprofilrohdaten](../Dokumentationen/Nachweise_C-0.25/ipo-profile-after.json).

Finaler Core ohne experimentelle Caches gebaut:
Standard und eigenstaendiger nativer instrumentierter Build jeweils
**6/6 bestanden** (`modellaufbereitung`, `pose_transformations`,
`sailforce_integration`, `hydrostatik_referenzen`,
`hydrodynamik_referenzen`, `hydrodynamik_referenzmodelle`).
Die letzte Pruefung umfasst alle elf modernen Referenzmodelle.
Testlaufzeiten unter parallelen Test-/Buildjobs werden nicht als
Leistungsnachweis verwendet.

Ein regulaerer Dreiboottest der neu gegen diesen Core gebauten GUI
ist beauftragt. G25-08 bleibt bis zum tatsaechlichen Ergebnis offen.

### IPO-Dreiboottest und anschliessende exakte Dreiecksvorpruefung

Der IPO-Dreiboottest besteht G25-08 nicht:

| Boot | Corezeit | Frame-p95 / Maximum |
|---|---:|---|
| 420 | 60.8379435 s | 46.3195 / 107.8527 ms |
| Folkeboot | 30.8363189 s | 45.6269 / 72.9476 ms |
| Optimist | 9.3988859 s | 44.9073 / 76.3327 ms |

Render-Median bei allen drei Booten nun 33.6 ms statt zuvor
15..19 ms, obwohl JSON-Zeiten sinken. Kein Beweis fuer eine
GPU- oder IPO-Ursache; eine kurze Frozen-Presenter-Diagnose ohne Corelast
ist gesondert beauftragt. Keine weitere 60-s-Wiederholung ohne Eingrenzung.
Rohdaten: `C:\spdeps\guibuild\evidence\G25-final-ipo-three-boats\sailforce-performance.json`.

Der reale 420-Rumpf besitzt 34530 unterschiedliche Punkte und
69056 Dreiecke; eine Punktdeduplizierung waere dort kein Hebel.
Die meisten Dreiecke liegen jedoch vollstaendig unter oder ueber Wasser
und wurden trotzdem durch den allgemeinen Polygonclipper gefuehrt.

`wasseranteil` und `nasspanels` pruefen jetzt die drei tatsaechlichen
vorzeichenbehafteten Punkthoehen: strikt trockene Dreiecke ueberspringen;
vollstaendig nasse Dreiecke direkt mit derselben Rechenreihenfolge
integrieren. Nur wasserlinienkreuzende Dreiecke durchlaufen den
unveraenderten Clipper. Nullhoehen bleiben inklusive nass; keine
Boundingbox-Naeherung, Toleranz, Dreiecksreduktion oder geaenderte Physik.

Unabhaengig instrumentierter nativer Build, fuenf gezielte Tests
einschliesslich elf realer Hydromodelle bestanden. Vollstaendige
Endzustaende aller drei Boote nach 100 Dynamikschritten exakt identisch
zum Stand vor den Schnellpfaden. Instrumentierter 420-Zwischenstand:
100 Schritte 0.8078162 s, Ebenenschnitte 97.931 ms,
Nasspanels 214.178 ms. Vergleich zum frueheren instrumentierten Stand
0.9921638 s / 164.917 ms / 291.116 ms enthaelt auch die IPO-Aenderung;
kein isolierter Prozentgewinn allein fuer die Dreiecksvorpruefung behauptet.
Noch kein neuer 60-s-Gesamtnachweis.
[Instrumentierter Schnellpfad-Nachweis](../Dokumentationen/Nachweise_C-0.25/triangle-fastpath-instrumented.json).

**Abschliessende Core-Standardpruefung:** Release ohne Instrumentierung
6/6 gezielte Tests bestanden, einschliesslich aller elf modernen
Hydromodelle, Pose, Aufbereitung und realer sailforce-Integration.
Alle drei vollstaendigen 100-Schritt-Endzustaende exakt identisch
zum IPO-Stand ohne Dreiecksschnellpfad.
420-Release-Kurzprofil 0.9416165 -> 0.8029448 s
(14.73 % weniger Zeit bei gleicher IPO-Konfiguration).
[Release-Rohdaten](../Dokumentationen/Nachweise_C-0.25/triangle-fastpath-release.json).

### Isolierter GUI-Praesentationsbefund ohne Corelast

Echter unveraenderter 420-Snapshot, 1280x720 Technik, Corelauf bereits
beendet, null Zustands-/Meshupdates, Qt und Windows melden 100 Hz.
Auf demselben Build globale Swapvorgabe A/B/A = 1/0/1:

| Vorgabe | Render-Median | Frame-p95 |
|---|---:|---:|
| Standard 1 | 46.0411 ms | 48.4759 ms |
| 0 | 4.4027 ms | 5.4982 ms |
| erneut 1 | 46.0244 ms | 48.3817 ms |

Damit ist eine Swap-/Praesentationsbindung ohne Corelast reproduzierbar
belegt, kein Physik-/IPO-Engpass. Widget-/Unterkontext-Metadaten melden
trotzdem immer 1; die alleinige Abfrage dieser Werte waere irrefuehrend.
Die GUI setzt nun gezielt die globale Swapvorgabe vor QApplication auf
0, sonstiges Format, Qualitaet, 30-fps-/Ein-Render-/Abrufdrosseln
bleiben unveraendert. Keine ungebremste Frozen-Diagnoseschleife im Produkt.

Rohdaten jeweils unter `C:\spdeps\guibuild\evidence\G25-frozen-default`,
`G25-frozen-swap0` und `G25-frozen-swap1-confirm`.
Abschliessender regulaerer Dreiboottest gegen diese GUI und den
fertig geprueften Core laeuft; kein Gate vor dessen Ergebnis geschlossen.

### Finaler regulaerer Dreiboottest: Echtzeit und Bildgrenzen bestanden

Finaler unveraenderter Release-Build mit allen Segelbahnen und voller
Physik, globaler Swapvorgabe 0 und unveraenderten Praesentationsdrosseln.
Vorbereitung ausserhalb der Messung; je Boot 6000 gueltige Schritte
dt=0.01 s, 6001 Zustaende, echter Desktop 1280x720 Technik, Livefaktor 1,
alle Segel/SDPs/Grenzlinien aktiv. Keine parallelen Builds oder Tests.

| Boot | Corezeit | Wandzeit | Frame-p95 | Frame-Maximum |
|---|---:|---:|---:|---:|
| 420 | 51.7658714 s | 60.0113410 s | 31.2732 ms | 44.0536 ms |
| Folkeboot | 30.3125725 s | 60.0063603 s | 26.6654 ms | 32.8688 ms |
| Optimist | 8.7310538 s | 60.0030026 s | 26.5658 ms | 33.2296 ms |

**G25-08 und der integrierte Leistungsanteil C25-15 bestanden.**
Grenzen fuer jeden Fall explizit aus den Rohdaten geprueft:
Corezeit <=60 s, p95 <=33.3 ms, Maximum <=100 ms, 6001 Zustaende,
alle Segel aktiv, keine Fehler oder Snapshot-Fallbacks. Die um wenige
Millisekunden laengere Wandzeit enthaelt die absichtliche Livefaktor-1-
Praesentation; Coreberechnung beendet sich rechtzeitig.

420 besitzt 8.2341286 s Core-Reserve. Die nur bei weniger als 2 s
Reserve vorgesehene Bestaetigungsrunde ist nicht erforderlich.
Snapshot-Treffer 2092/2469/2499, kein Fallback.

Parser 766 Pruefungen, echte History/Livewinkel/64er-Cache/JSON-Paritaet
und echte Desktop-Reflexions-/Wasserlinienparitaet bestanden.
Produktive GUI `C:\spdeps\guibuild\Release\segelphysik_gui.exe` gebaut
(17.11.2026 23:12:07, 4042240 Bytes); alle Messprozesse beendet.

Vergleich gegen die gespeicherten alten 6000-Schritte-Daten aller drei
Boote: Anfangszustand, vorbereiteter Wind/Luft sowie vollstaendige
gespeicherte Sailforce-/Wind-Enddaten inhaltlich identisch. Die GUI-
Rohberichte enthalten keinen vollstaendigen Root-Endzustand mit Pose,
Geschwindigkeiten und Hydrostatik; deshalb keine vollstaendige
6000-Schritte-Physikparitaet behaupten. Vollstaendige Endzustandsparitaet
nach 100 echten Schritten ist separat im Core-Kurzprofil nachgewiesen.

[Vollstaendiger komprimierter Rohbeleg](../Dokumentationen/Nachweise_C-0.25/gui-final-swap0-three-boats.zip),
enthaelt unveraendert `sailforce-performance.json` (14109893 Bytes),
SHA-256 der enthaltenen JSON-Datei:
`76f4fe9fbf47729faa824366ab17151cbf8f2e3caaf433dcbce7f9e901fc4a32`.
Original und drei echte Desktopaufnahmen:
`C:\spdeps\guibuild\evidence\G25-final-swap0-three-boats`.
GUI-Plan Abschnitt 16 fuehrt die korrespondierenden Belege.

**Verbleibende Grenzen:** G25-09, persoenliche GUI-/DPI-Abnahme und die
dokumentierte Bestands-Gesamtregression mit Format-0.2-Fixtures sind
nicht geschlossen. Daher C4/C5 nicht pauschal als abgeschlossen werten.
Die beauftragte Doppelberechnungs-/Initialisierungs- und Echtzeitoptimierung
ist umgesetzt und nachgewiesen. Alle Aenderungen uncommittiert;
kein Commit, Push, Tag oder Release.
