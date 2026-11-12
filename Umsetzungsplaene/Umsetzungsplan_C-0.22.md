# Core-Umsetzungsplan C-0.22: Gruppeneigene Bewegungs-Vorschau

**Status:** Finaler Umsetzungsplan; Umsetzung und Core-Regression bestanden,
fachliche Nutzerabnahme offen.
**Spezifikation:** [Modellbeschreibung, A-C-00154 bis A-C-00155](../Spezifikationen/Spezifikation_Modellbeschreibung.md#c-022-ergänzung-gruppeneigentümerschaft-und-elementlokale-segelseite)
und [Modellkatalog-Schnittstelle, A-C-00156 bis A-C-00157](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#5-c-022-gruppenbewegungs-vorschau-aus-dem-inspektionssnapshot).
**Projektgrenze:** Dieser Plan betrifft ausschließlich SegelPhysik-Core.
Die GUI-Umsetzung und ihr Fortschritt sind im getrennten
[GUI-Plan G-0.22](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.22.md)
dokumentiert.

## 1. Umfang und Grenzen

C-0.22 erhält die Format-0.3-Bewegung als Gruppendatum, stellt sie in der
typisierten Core-Modellstruktur an der Gruppe bereit und ergänzt eine
read-only Gruppenbewegungs-Vorschau über eine Gruppen-ID. Eine Pose enthält
die vollständigen Core-transformierten Darstellungsmeshes aller
Gruppenmitglieder. Für Segelgruppen werden Backbord- und
Steuerbordvarianten desselben Ausschlags geliefert; die Konturspiegelung
wird ausschließlich für das jeweilige Segel anhand von
`placement.originalTack` bestimmt.

Das aktive `schema_data.json` unter `C:\_DATEN\Code\Cpp\Segelphysik\Modelle`
ist die maßgebliche Felddefinition. Es wird nicht geändert. Ebenso bleiben
`modells_data.json`, `modells_validation.json`, Modellressourcen und
Fingerprints unangetastet. Es werden keine neuen Steuerfelder oder
Bewegungsgrenzen in das Schema eingeführt. Bestehende elementeigene
Legacy-Bewegungen bleiben kompatibel.

Die Vorschau verändert weder Simulationszustand noch Modell- oder
Quelldateien. Das bestehende Solververhalten wird nicht fachlich erweitert;
seine Regression muss unverändert bestehen. Nicht enthalten sind
GUI-Implementierung, persistente Benutzerplatzierungen, neue
Steuer-/Grenzdefinitionen, Schemaänderungen sowie Commit, Push, Tag oder
Release.

Vor Vergabe geprüft: höchste A-C-Kennung A-C-00153, höchste U-C-Kennung
U-C-000137, höchste IDEE-C-Kennung IDEE-C-00104 und höchste
Schnittstellenkennung CPPI-0147. C-0.22 verwendet A-C-00154 bis A-C-00157,
U-C-000138 bis U-C-000140 sowie CPPI-0148 bis CPPI-0150. IDEE-C-00004
bleibt als Herkunft erhalten und wird teilweise finalisiert.

## 2. Geordnete Umsetzung

| U-ID | Umsetzungsschritt | Anforderungen | Abhängigkeiten | Nachweis/Abnahmekriterium |
|---|---|---|---|---|
| U-C-000138 | Den Format-0.3-Import und die typisierte Modellstruktur so erweitern, dass `groups.<id>.movement` gruppeneigen bleibt und nicht als Bewegungsdefinition eines Controller-Elements ausgegeben wird. Alle und nur die Gruppenmitglieder bleiben der Bewegung zugeordnet; Elementplatzierungen bleiben unverändert. `placement.originalTack` bleibt am einzelnen Segel-Element und wird nicht auf Gruppenmitglieder übertragen. Legacy-Importpfade bleiben erhalten. | A-C-00154, A-C-00155 | keine | Import-/Strukturtests bestätigen Gruppeninhaberschaft, Mitgliederabdeckung, statische Gruppen ohne Bewegung, erhaltene `placement`-Werte und elementlokale Originalbuglage. Die aktiven Modelle bestehen unverändert das vorhandene Schema. |
| U-C-000139 | `InspectionGroupMovementPreview`, `getGroupMovementPreview(snapshot, group_id)` und `groupMovementPreviewJson(snapshot, group_id)` gemäß CPPI-0148 bis CPPI-0150 bereitstellen. Core berechnet Neutral- und verfügbare Grenzposen für die ganze Gruppe. Für Segelgruppen beide gleich großen, entgegengesetzt signierten Schotvarianten mit den Labels `backbord`/`steuerbord` liefern. Die Konturspiegelung verwendet die vom Nutzer festgelegten drei Punkte aus Visualisierungsmesh und Drehachse, ohne Aerodynamikdateien. Fehlende Vorschaugeometrien und unbekannte Originalbuglagen ausdrücklich diagnostizieren; keine Ersatzpose erfinden. | A-C-00156, A-C-00157 | U-C-000138 | Gruppen- und Element-IDs, Achsen, Drehpunkte, Posen und Meshes stimmen in C++ und JSON überein. Alle Gruppenmitglieder werden transformiert; nur das betroffene Segelmesh wird gespiegelt. Die Abfrage ist read-only. |
| U-C-000140 | Bestehende Modellformat-, Inspektions-, Bewegungs- und Solverregression mit allen aktiven Format-0.3-Modellen ausführen; Schnittstellenregister und Core→GUI-Vertrag vervollständigen. Quellinventar und Fingerprints vor/nach der Prüfung vergleichen. | A-C-00154 bis A-C-00157 | U-C-000138, U-C-000139 | Nativer Core-Build und vollständige CTest-Suite bestehen. Kein Modelldaten-/Schema-/Fingerprint-Diff; bisherige Solverergebnisse bleiben innerhalb ihrer bestehenden Abnahmetoleranzen unverändert. GUI-Arbeit kann anschließend gegen CPPI-0148 bis CPPI-0150 fortgesetzt werden. |

## 3. Gezielte Abnahmetests

| Test | Prüfung | Erwartung |
|---|---|---|
| C22-01 | Aktives `schema_data.json` und vorhandene 0.3-Modelle | Schema bleibt unverändert; zulässige Gruppenbewegung und sail-spezifisches `originalTack` werden exakt wie definiert verarbeitet. |
| C22-02 | 420er-Gruppen `mainsail` und `jib` | Bewegung ist jeweils an der Gruppe sichtbar; `mainsail` umfasst `Mainsail` und `Boom`; der Boom ist kein eigener Bewegungsinhaber. |
| C22-03 | Gruppe ohne `movement`, fremde Gruppe und individuelle `placement`-Werte | Statische Gruppe und nicht zugehörige Elemente bleiben unverändert; Platzierungswerte werden nicht überschrieben. |
| C22-04 | Segelgruppe mit bekanntem `originalTack` und mindestens einem Nicht-Segelmitglied | Backbord-/Steuerbordpose haben gleichen Betrag und entgegengesetztes Vorzeichen; nur die Segelkontur wird gegebenenfalls gespiegelt. |
| C22-05 | Segel mit `originalTack: "unknown"` und nichtneutraler Zielseite | Core liefert eine konkrete Diagnose und gibt die betroffene Seitenpose nicht aus; neutrale Pose sowie Vorschauen anderer Gruppen bleiben verfügbar. |
| C22-06 | C++-Vorschau gegenüber JSON-Projektion | Gruppenkennung, Elementkennungen, Achsen, Grenzwerte, Posen, Meshpunkte und Diagnosen sind semantisch identisch. |
| C22-07 | Vorschau vor/nach Aufruf sowie Modelldaten- und Solverregression | Keine Datei- oder Simulationsmutation; Modellinventar/Fingerprints bleiben identisch und bestehende Solvertests bestehen. |

Fixtures mit absichtlich geänderten `originalTack`-Werten werden ausschließlich
in temporären Testkopien verwendet. Das separate Repository `Modelle` wird
weder beschrieben noch umgeordnet.

## 4. Meilensteine und Gates

| Meilenstein | Eintritt | Abnahmekriterium |
|---|---|---|
| C1 – Schema- und Importsemantik | U-C-000138 | C22-01 bis C22-03 bestehen; die Core-Struktur exponiert Bewegungen ausschließlich auf der Gruppe und bewahrt das lokale Segelattribut. |
| C2 – Gruppen- und Segelvorschau | U-C-000139 | C22-04 bis C22-06 bestehen; CPPI-0148 bis CPPI-0150 sind im Header, JSON-Vertrag und Schnittstellenregister konsistent. |
| C3 – Core-Regression und Übergabe | U-C-000140 | C22-07 sowie nativer Build und vollständige CTest-Suite bestehen; keine Modelldatenänderung. Erst danach wird der GUI-Blocker für die Implementierung gegen die neue Core-API aufgehoben. |

Build und CTest folgen dem
[Core-README](../README.md#einstieg), vom Core-Repository-Stamm:

```powershell
cmake -S Code -B build\standard -DBUILD_TESTING=ON
cmake --build build\standard --config Release
ctest --test-dir build\standard -C Release --output-on-failure
```

## 5. Rückverfolgbarkeit

| Anforderung | Umsetzungsschritte | Nachweis |
|---|---|---|
| A-C-00154 | U-C-000138, U-C-000139, U-C-000140 | C22-01 bis C22-03, vollständige Core-Regression |
| A-C-00155 | U-C-000138, U-C-000139, U-C-000140 | C22-01, C22-04, C22-05, C22-07 |
| A-C-00156 | U-C-000139, U-C-000140 | C22-02, C22-03, C22-06 |
| A-C-00157 | U-C-000139, U-C-000140 | C22-04 bis C22-07 |

## 6. Tatsächlicher Umsetzungsstand

**U-C-000138 bis U-C-000140 – umgesetzt und Core-Abnahmetests bestanden
(12.11.2026):** Format-0.3-Bewegungen werden in der öffentlichen
Beschreibung und Modellstruktur gruppeneigen exponiert; der interne
Controller-Adapter bleibt ausschließlich für die bestehende Solver-
Kompatibilität. `getMovementPreview` gibt für solche Controller keine
Elementvorschau zurück. Originalbuglage bleibt am Segel-Element.

Die gruppeneigene Core-Vorschau liefert vollständige, nur auf
Visualisierungsmeshes gestützte Gruppenposen, beide vorzeichenbehafteten
Segelseiten und elementbezogene Diagnosen. Unbekannte Buglage, entartete
Spiegelebene und fehlende/verborgene Darstellungsmeshes führen nicht zu
geratenen oder unvollständigen Posen. Auch Volumenersatz- und
Punktmassen-Referenzgeometrien werden nicht als Visualisierungsersatz
verwendet. C++- und JSON-Projektion sowie die geneigte Fockachse werden
durch Tests geprüft. ±90° bleibt für Gruppen ohne Schema-Grenzen eine
Vorschau-Referenz, keine Modell- oder Simulationsgrenze.

**Nachweise:** Nativer Core-Release-Build bestanden; vollständige
Core-CTest-Suite **29/29 bestanden**, einschließlich 4/4
Modellformat-0.3-Tests und 420-Source-Integration. Die Regression verwendet
auch unveränderte Legacy-Fixtures. Ein isoliertes 420-Testfixture entfernt
Aerodynamikreferenzen und kopiert keine Aerodynamikdateien. Das
Modelle-Repository wurde durch diese Umsetzung nicht beschrieben. Die
Source-Integration bestand. Rund 50 Sekunden nach Abschluss der GUI-
Vollregression wurde im parallel genutzten Modelle-Repository eine
uncommittete Änderung an `420/modells_validation.json` (nur der
Aerodynamik-Fingerprint) sowie aktualisierte Aerodynamik-Lookup-Dateien
beobachtet. Diese fremde Änderung wurde weder ausgelöst noch angefasst;
Fingerprint-Gleichheit nach diesem parallelen Vorgang wird nicht behauptet.

**Meilensteine C1, C2 und C3:** technische Kriterien erfüllt. CPPI-0148 bis
CPPI-0152 sind implementiert und getestet. GUI-Übergabe erfolgt gegen diese
API; GUI-Abnahme und persönliche fachliche Nutzerabnahme sind keine
Core-Abnahmetests und bleiben separat.
