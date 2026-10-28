# Umsetzungsplan C-0.8: Bewegliche Ruder und Schwerter

**Status:** Umsetzung teilweise abgeschlossen; technische und fachliche Abnahmen offen.  
**Stand:** 28.10.2026 · **Zielversion:** C-0.8 (Folgeumfang nach C-0.7)

Maßgebliche fachliche Grundlage ist Abschnitt 13 der
[Core-Hydrodynamik-Spezifikation](../Spezifikationen/Spezifikation_Hydrodynamik.md#13-dynamische-verstellung-von-ruder-und-schwert).
Die Herkunftszuordnung und offenen Datenpunkte stehen in der
[Entwurfspunkteliste Hydrodynamik](../Spezifikationen/Entwurfspunkte/Hydrodynamik.json).
Dieser Plan ist der einzige Umsetzungsplan für den Bewegungsumfang C-0.8.
Er enthält ausschließlich Core-Arbeiten; die GUI erhält einen getrennten
[GUI-Plan G-0.7](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.7_Ruder_Schwertsteuerung.md).
GUI-Code ist kein Lieferbestandteil dieses Plans.

Die Versionszuordnung C-0.8 autorisiert keinen Tag oder Release.
Die Implementierung wurde beauftragt; Commit und Push sind nicht beauftragt.

## 1. Umfang und Rückverfolgbarkeit

Der Plan setzt die Core-Bewegungsdefinition, Stellzustände und deren
hydrodynamische Verwendung um. Die vollständige sichtbare Darstellung
wird von der GUI verantwortet. Bewegte Elemente ändern weder
Massenverteilung noch Schwerpunkt, Trägheit, Auftrieb oder Hydrostatik.
Hub-Kiele, Aktuatordynamik, Bauteilinterferenz, Wasserzutritt durch
Rumpfdurchbrüche und Strömungsnachläufe sind ausgeschlossen.

| IDEE-ID | Core-Anforderung | Gegenstand |
|---|---|---|
| IDEE-C-00050 | A-C-00079 | JSON-Bewegungsvertrag und Importvalidierung |
| IDEE-C-00050 | A-C-00080 | Modellachsen, Drehpunkte und projektweite Stellkonvention |
| IDEE-C-00050 | A-C-00082 | Zeitgeordnete Eingaben und Hydrodynamikpose |
| IDEE-C-00050 | A-C-00084 | Invariante Masse, Schwerpunkt, Trägheit und Hydrostatik |
| IDEE-C-00050 | A-C-00085 | Core-Bewegungstests, Integrations- und Regressionstests |

A-C-00081 und A-C-00083 sind GUI-seitige Liefergegenstände und werden im
separaten G-0.7-Plan nachgewiesen. Core liefert dafür die validierten
Bewegungsmetadaten und bestätigten Posen, die gemäß A-C-00079/A-C-00082
erforderlich sind.

Die nächsten freien Core-Umsetzungsschrittkennungen nach C-0.7 sind
U-C-000060 bis U-C-000066:

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit |
|---|---|---|---|
| U-C-000060 | A-C-00080, A-C-00085 | Bestehende Modelle, Meshachsen, Ruderstock-/Schwertlager und relevante Dreh-/Hubräume inventarisieren; Pivot-Startwerte und Vorzeichen gegen Modellframe prüfen, Abweichungen dokumentieren | Keine |
| U-C-000061 | A-C-00079, A-C-00080 | Rückwärtskompatiblen JSON-Bewegungsvertrag, Einheiten, gültige Bewegungsarten, Vektor-/Pivotvalidierung und robuste Hubwegableitung in die bestehende Modellbeschreibung integrieren | U-C-000060 |
| U-C-000062 | A-C-00079, A-C-00080, A-C-00085 | Modellfeste Pose-/Transformationslogik für Drehen und Heben umsetzen; projektweite Ruderwinkel- und Schwertstellungsabbildung samt Endstellungen isoliert testen | U-C-000061 |
| U-C-000063 | A-C-00082, A-C-00085 | Typisierte Stellereignisse, Simulationszeitbezug, stabile Ereignisfolge, Bestätigung und reproduzierbare Zustands-/Ergebnisbindung implementieren | U-C-000061, U-C-000062 |
| U-C-000064 | A-C-00082, A-C-00084 | Core-Bewegungszustand in allen Integratorstufen verwenden; benetzte bewegte Simulationsgeometrie und Hydrodynamikbeiträge aktualisieren, Massen- und Hydrostatikwerte invariabel halten | U-C-000062, U-C-000063 |
| U-C-000065 | A-C-00079 bis A-C-00085 | Referenzmodelle, Fehlerszenarien, dynamische Ereignisfolgen und Live-/Batch-Parität integrieren; Ergebnis-/Diagnoseverträge regressionsprüfen | U-C-000064 |
| U-C-000066 | A-C-00079 bis A-C-00085 | Rückverfolgbarkeitsmatrix, technische Nachweise, Modell-/Pivotgrenzen und offene Validierungslücken vervollständigen | U-C-000065 |

Die Umsetzungsschritte werden nur nach tatsächlicher Bearbeitung als
abgeschlossen markiert. Geometrieinventar, Import, Transformation,
Ereignisfolge, Dynamikintegration und Abschlussnachweise sind getrennte
Prüfgegenstände; ein erfolgreicher Build ersetzt keinen dieser Nachweise.

## 2. Voraussetzungen und Grenzen

- Den tatsächlich integrierten und freigegebenen C-0.7-Core-Stand,
  seine APIs, Ergebnisfelder und den Testbaseline zuerst feststellen.
  Der vorhandene Arbeitsbaum enthält bereits Änderungen; sie sind fremd
  beziehungsweise nicht Bestandteil dieses Plans und dürfen nicht
  überschrieben, zurückgesetzt oder pauschal gestagt werden.
- Vor der Bewegungsimplementierung Modell-JSON- und Meshfassungen für
  Optimist und 420er mit dem getrennten Modelldepot abstimmen. Die
  in A-C-00080 aufgeführten Pivotkoordinaten sind meshbasierte
  Startnäherungen, keine vermessenen Lagerdaten. Sie müssen mit
  Darstellungsmesh und Skizzen plausibilisiert werden; eine erforderliche
  Anpassung bleibt in den entsprechenden Modell-JSONs und benötigt deren
  getrennte Änderungserlaubnis.
- Achsen werden im modellfesten Frame ausgedrückt: +X zum Bug,
  +Y nach Backbord, +Z nach oben in der aufrechten Referenzlage.
  Optimist-Schwert: Heben +Z. 420er-Schwert: Drehung +Y, wobei die
  0-%-Stellung 90° nach achtern gegenüber der 100-%-Meshlage ist.
  Ruder: Drehung um +Z, projektweite Winkelgrenzen -90° bis +90° und
  Backbord-/Steuerbordsemantik.
- Der Import muss alte Modelle ohne Bewegungsblock unverändert
  akzeptieren und darf ihnen keine Bewegung aus Namen oder Meshs
  erraten. Ein ungültiger deklarierter Bewegungsblock muss ausdrücklich
  diagnostiziert werden; er darf nicht als stilles „unbeweglich“ oder
  als gültiger Nullwert fehlinterpretiert werden.
- Die Stellposition ist für sämtliche Berechnungen eines
  Integratorzeitschritts konsistent. Die Ereignisreihenfolge bei
  identischem Simulationszeitpunkt muss stabil und die Folge mit
  Modellfassung und Laufparametern reproduzierbar sein.
- Verstellungen verändern keine Massen-, Schwerpunkt-, Trägheits- oder
  Hydrostatikdaten. Die Bewegung muss sich allein auf Darstellungspose
  beziehungsweise auf Hydrodynamikgeometrie und -kräfte auswirken.
- Ein hydrodynamisches Simulationsmesh darf von der vollen
  Darstellung getrennt und auf erforderliche benetzte Geometrie
  beschränkt werden. Rumpfdurchbrüche dürfen nur im
  Hydrodynamik-Simulationskörper geschlossen werden; weder
  Darstellungsmesh noch Hydrostatikvolumen werden dadurch verändert.
- Hydrodynamische Nachläufe, gegenseitige Abschirmung, Kollisionen,
  Wasseraufprall/Flutung und Betätigungsdynamik sind keine stillen
  Ersatzmodelle dieses Umfangs. Fehlende externe Bootsversuche und
  vermessene Lagerdaten begrenzen die fachliche Genauigkeit, blockieren
  aber technische Tests nicht.
- API-Erweiterung, Ergebnisserialisierung, Live-/Batch-Parität und GUI-
  Adapter sind vor Änderung auf Kompatibilität zu prüfen. Inkompatible
  Pflichtfeldänderungen sind nicht Teil einer stillen Erweiterung.

## 3. Abhängigkeiten und Integrationsreihenfolge

| Stufe | Eingang | Ausgang / Freigabekriterium |
|---|---|---|
| A. Baseline und Geometrie | C-0.7-Build/API, freigegebene Referenzmodelle | Verifizierte Achsen, Meshfassungen, plausible Drehpunkte und Hubendstellungen; Datenherkunft dokumentiert |
| B. Datenvertrag | Stufe A | Importiert Bewegungsarten und Bewegungsdaten strikt und rückwärtskompatibel; Hubweg reproduzierbar |
| C. Kinematik | Stufe B | Testbare posebasierte Dreh-/Hubtransformation ohne Starrkörper-, Masse- oder Hydrostatikmutation |
| D. Eingabe und Laufzustand | Stufe C | Stellereignisse deterministisch angenommen, bestätigt und reproduzierbar |
| E. Hydrodynamikkopplung | Stufe D | In jedem Integratorsample konsistente bewegte Geometrie und gültige additiv bilanzierte Kraft-/Momentenbeiträge |
| F. Integration und Übergabe | Stufe E | Core-Regressionen, Live-/Batch-Parität und dokumentierte GUI-Schnittstelle bestehen; danach kann GUI G-0.7 integriert werden |

Der GUI-Plan G-0.7 hängt von den Ergebnissen der Stufen B bis F ab.
Core- und GUI-Änderungen werden in den jeweiligen Repositories getrennt
implementiert, getestet und abgenommen; ein Core-Test ersetzt keinen
GUI-Integrationstest.

## 4. Meilensteine

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| M1 – Geometrie und Baseline | Planstart; U-C-000060 begonnen | Aktuelle Baseline und Modellfassungen bekannt; alle Pivots/Achsen als plausibel, zu korrigieren oder offen markiert |
| M2 – Modellvertrag und Kinematik | M1 bestanden | Strikte JSON-Validierung; bestehende Modelle laden; Dreh-/Hubtransformationen und Stellendpunkte analytisch getestet |
| M3 – Dynamische Stellereignisse | M2 bestanden | Ereignisannahme, Zeitbezug, Tie-break-Reihenfolge und Bestätigung sind reproduzierbar; Reset und Laufende löschen keinen historischen Laufzustand |
| M4 – Hydrodynamik und Invarianten | M3 bestanden | Dynamische Pose ändert erwartete Hydrodynamik; Masse, Schwerpunkt, Trägheit und Hydrostatik bleiben unverändert |
| M5 – Integration und Übergabe | M4 bestanden | Core-Build, gezielte und vollständige CTest-Suite sowie Live-/Batch-Parität bestanden; GUI-Vertrag, Grenzen und Diagnosepfad dokumentiert |
| M6 – C-0.8 Abschluss | M5 bestanden | A-/U-Rückverfolgbarkeit vollständig, technische und fachliche Abnahme getrennt berichtet, offene Daten-/Validierungslücken ausgewiesen |

## 5. Prüfstrategie

1. **Import/Vertrag:** gültige Dreh- und Hubdefinition, fehlende Pflichtfelder,
   Null-/nicht endliche Vektoren, nicht unterstützte Bewegungsart,
   ungültiger Pivot, unbekannte Baugruppen-ID, nicht ableitbarer Hubweg
   und Altmodell ohne Bewegung.
2. **Kinematik:** -90°, 0°, +90° am Ruder; 0 %, Zwischenstellung und
   100 % am Schwert; Drehung um Pivot statt Modellursprung; Hubrichtung
   und Rumpfunterkantenbedingung; Körperrotation und Frame-Transformation.
3. **Ereignisfolge:** mehrere Änderungen im selben und in verschiedenen
   Simulationszeitpunkten; identische Zeitstempel; Reset/Abbruch; gleiche
   Folge mit identischer Modellfassung und Parametern reproduzieren.
4. **Dynamik/Hydrodynamik:** Rechenzustand nach jedem Ereignis in
   sämtlichen Integratorstufen, benetzte Geometrie, Kraftangriffspunkt,
   Moment und Bilanz; bei fehlenden Beiträgen explizite Diagnose statt
   Erfolg-als-Null.
5. **Invarianten/Regression:** Masse, Schwerpunkt, Trägheitstensor,
   Auftrieb, Wasserlinie und hydrostatische Beiträge vor/nach
   Stelländerung; Altmodelle, ältere Simulationen und Ergebnisverträge.
6. **Build/Integration:** nativer Release-Build und CTest im Core;
   gezielte neue Tests werden separat ausgewiesen. GUI-/Core-Integration
   wird im GUI-Plan geprüft. Quantitative Bootsgenauigkeit wird nur mit
   unabhängigen Messdaten und festgelegten Toleranzen behauptet.

## 6. Rückverfolgbarkeit und tatsächlicher Stand

| A-ID | Core-Arbeitspaket(e) | Nachweis | Status |
|---|---|---|---|
| A-C-00079 | U-C-000061–000063, U-C-000065–000066 | JSON-Import, Referenzmodelle und gezielte Validierungstests; weitere Fehler-/Altmodellfälle offen | teilweise |
| A-C-00080 | U-C-000060–000062, U-C-000065–000066 | Achsen, Transformationen und Hubweg technisch umgesetzt; Pivot-Näherungen noch nicht visuell/fachlich abgenommen | teilweise |
| A-C-00082 | U-C-000063–000065 | Ereignisannahme und V4-Zustand vorhanden; Ereignisreihenfolge, Replay und Live-/Batch-Parität noch nicht vollständig geprüft | teilweise |
| A-C-00084 | U-C-000064–000066 | Bewegungsabhängige Hydrodynamik; Ruderseitenkraft und Giermoment im lokalen Profilrahmen mit synthetischem Ruder und 420er-Rudermesh auf Vorzeichen geprüft. Umfassende Hydrostatik-/Integratorregression und quantitative Validierung offen | teilweise |
| A-C-00085 | U-C-000060–000066 | Core-Release-Build und Core-Tests bestanden; integrierter GUI/Core-CTest-Lauf: 29/29 bestanden. Der GUI-Anwendungstarget wurde nach der Ruderkorrektur erfolgreich neu gelinkt; vollständiger integrierter Lauf einschließlich Render-Tests: 29/29 bestanden. Fachliche Pivot-/Modellabnahme und vollständige Rückverfolgbarkeitsmatrix offen | teilweise |

Die Anzeige-/Bedienanforderungen A-C-00081/A-C-00083 sind als
Core/GUI-Grenze in diesem Plan dokumentiert; ihre konkrete Umsetzung und
Abnahme werden unter A-G-00046 bis A-G-00051 im GUI-Plan geführt.

| U-ID | Tatsächlicher Stand | Nachweis / offener Punkt |
|---|---|---|
| U-C-000060 | teilweise umgesetzt | Achsen/Pivots in Optimist und 420 JSON ergänzt; Pivotlagen sind Näherungen und warten auf Sicht-/Geometrieprüfung |
| U-C-000061 | technisch umgesetzt | Bewegungsdefinitionen, Importvalidierung und Hubwegableitung; Core-Build und Referenzmodelltests bestanden |
| U-C-000062 | teilweise umgesetzt | Dreh- und Hubtransformationen einschließlich Ruder-/Schwertrotation und Hubtranslation getestet; vollständige Endlagen-/Pivotabnahme offen |
| U-C-000063 | teilweise umgesetzt | Stellereignisse, Bestätigung und Ergebnisvertrag V4 umgesetzt; zeitgleiche Reihenfolge, Wiederholung und Live-/Batch-Parität nicht vollständig nachgewiesen |
| U-C-000064 | teilweise umgesetzt | Bewegte Geometrie wirkt auf Hydrodynamik; Ruderseitenkraft nutzt den mitbewegten lokalen Profilrahmen. Korrektur 28.10.2026: Profilabmessungen werden am unverstellten Mesh bestimmt (ein stark ausgelenktes Ruder galt zuvor als Nicht-Tragfläche und lieferte keine Seitenkraft), Bezugsfläche folgt der lokalen Profilnormalen, oberhalb des Ablösewinkels geht die Kraft in ein Flachplatten-Normalkraftmodell über (cn = (1,1 + 0,018·AR)·sin α, geschätzt, nicht kalibriert). Vorzeichen von Seitenkraft und Giermoment für ±20°, ±60° sowie ±85° (Moment nur bis 60°) am 420er-Ruder getestet; Core-Szenario 420er (100 N bis 1,9 m/s, Kraft weg, Ruder −20°/0°/+20°) zeigt Drehung des Bugs in die erwartete Richtung. Erste Masse-/Hydrostatik-Invariante getestet; Integratorstufen und quantitative Kraft-/Momentenabnahme offen |
| U-C-000065 | teilweise umgesetzt | Referenzmodelle, Import-/Hydrodynamiktests und Regressionen bestanden; integrierte GUI/Core-CTest-Suite 29/29 bestanden (nach Ruderkorrektur inkl. Render-Tests und neu gelinkter GUI-EXE); fachliche Pivotabnahme und vollständige Live-/Batch-Nachweise offen |
| U-C-000066 | teilweise umgesetzt | Core-/Schnittstellendokumentation ergänzt; Core-CTest und integrierter GUI/Core-CTest-Lauf bestanden (29/29). Vollständige Rückverfolgbarkeit, Pivotvalidierung, quantitative Bootsvalidierung und offene Datenlücken noch abzuschließen |

Statusänderungen erfolgen erst nach tatsächlichem Ergebnis. Build-/Test-
Erfolg, technische Integration, externe Validierung und Nutzerabnahme werden
getrennt ausgewiesen. Modelländerungen sind Bestandteil des beauftragten
Umfangs. GUI-Arbeiten folgen dem getrennten GUI-Plan; Commit, Push, Tag und
Release bleiben nicht beauftragt.
