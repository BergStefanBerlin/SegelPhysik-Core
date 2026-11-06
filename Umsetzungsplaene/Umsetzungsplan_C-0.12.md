# Core-Umsetzungsplan C-0.12: persistente Segel-Einbaulage

**Stand:** 06.11.2026. **Status:** Core-Feature einschliesslich aus
Rigg-OBJ-Geometrie abgeleiteter Anschlaege/Achsen implementiert. Debug- und
Release-Feature-CTest bestanden; die gezielten Nachtests fuer atomare
Schreibfehler, Quellkonflikt, endliche GUI-Eingaben und Formspiegelung
bestanden. Alle sechs nativen Vollsuite-Fehler sind
durch einen isolierten `git archive`-Lauf auf HEAD als bereits dort
vorhanden nachgewiesen; derselbe Sechser-Selektor trifft nach den Core-
Aenderungen exakt dieselben sechs Fehler. GUI-Integration und
Nutzerabnahme offen. Keine Git-Veroeffentlichung.
Einziger Plan fuer den Arbeitsversionsschritt C-0.12.
**Aktuelle autorisierte Revision: einheitliche Montage aller Objekte,
06.11.2026 nach erneuter Quellenrekonstruktion.**
Bestehende U-C-000122/123/124/126/128 werden im selben fachlichen
Referenz-/Montagevertrag erweitert, ohne neue Objektkennungen oder
Boots-ID-Sonderfaelle. Massgeblich ist die aktuelle Revision in
[Spezifikation, Abschnitt 2a](../Spezifikationen/Spezifikation_Segelkonfigurator.md).
`MontageReferenz` (CPPI-0139) gilt fuer alle aktuellen Objektgeometrien.
Der kanonische Objektbaum referenziert ausschliesslich die eine aktuelle
Darstellung; physische Quellen bleiben erhalten und wirken unveraendert
in der Physik. Die drei expliziten Besatzungs-Punktmassen werden als Punkte
an ihren deklarierten Positionen dargestellt, nicht als erfundene Boxen.

Die alten Folk-/Opti-Montageindices passen nicht mehr zu den jetzt
rekonstruierten Segel-OBJs. Der Core prueft die OBJ-Quelldeklaration und
semantischen Konturenden; alte ungepruefte Indices werden nur mit
nichtentarteter erhaltener Legacy-Massengeometrie starr adaptiert.
Gespeicherte Lagen werden zusammengesetzt, nicht auf Null zurueckgesetzt.
Neue Quellenchecks entstehen nur bei bestaetigten Aktionen unter
`setting.montage_referenz`; Basisfassung und gemeinsamer Snapshot bleiben
stabil. Nach dieser Bindung werden geaenderte Quellen strikt abgewiesen.
Unterschiedliche historische Segel-/Sprietabmessungen bleiben ein
sichtbarer Quellenkonflikt; die unabhaengige Sprietgeometrie wird weder
passend skaliert noch unbemerkt umpositioniert.

Der gesamte aktuelle Originalbestand (146 Dateien) wurde vor/nach den
Kopietests mit SHA256 verglichen: **0 geaenderte Dateien**.
SHA256 des nach Pfad sortierten Quelleninventars:
`2a739634af9d2421b4ab36caf27290dfe5ef46ceb32087a99ffdbb2d511500e7`.
Keine Originaldatei geschrieben, keine Git-Aktion ausgefuehrt.
Alle Builds isoliert unter `CORE\build\unified-mount`; Shared-GUI-Build
und GUI-Abnahme bleiben beim Parent.

Finale native Nachweise auf dem fertigen Core-Quellenstand:
`segelkonfiguration_contract` bestanden (209,65 s),
`segel_mittschiffs_basis` bestanden (73,03 s),
`montage_alle_objekte` bestanden (233,86 s), **3/3, 516,61 s gesamt**.
`modellinspektion_c08` bleibt mit exakt `missing tree type hydrodynamik`
fehlerhaft (34,34 s), identisch zum unten dokumentierten HEAD-Baselinefall.
Der neue Selector `montage_alle_objekte` prueft elf Modelle/38 bestehende
Objekte, alle drei Boote/fuenf Segel, Auto/Translation/Rotation/absoluten
Fockversatz, Persistenz und frischen Inspektions-Reload auf eigenen Kopien.
Die vorherige direkte Semantikpruefung vor Ergaenzung der drei
Punktdarstellungen bestand fuer 35 Meshobjekte und alle 22 gespeicherten
Segelaktionen: maximale Halsabweichung 0 m, gerichteter normierter
Vorliekfehler 1,11022e-16. Der finale 38-Objektlauf bestaetigt alle 22
gespeicherten Aktionen, Hals-/gerichtete Vorliektoleranz <1e-6,
neutrale semantische Eckebene <1e-7 m, starre Formabstaende,
exakte angeforderte Translation und Eckrotation mit festem Halspivot
innerhalb 1e-9 m, absoluten Fockversatz +10 cm und idempotente Automatik.
Neustartgeometrie entspricht punktweise `getMesh("darstellung")`;
Masse/Schwerpunkt/Traegheit bleiben identisch. Gefaelschte bestaetigte
OBJ-Pruefsumme wird mit `CORE-SEG-0031` abgewiesen.
Testkopien werden automatisch bereinigt; Original-SHA256 erneut identisch.

```powershell
Set-Location 'C:\_DATEN\Code\Cpp\Segelphysik\CORE'
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build build\unified-mount --config Debug --target montage_tests segelkonfigurator_tests -j 4
& (Join-Path (Split-Path $cmake) 'ctest.exe') --test-dir build\unified-mount -C Debug -R '^(montage_alle_objekte|segel_mittschiffs_basis|segelkonfiguration_contract)$' --output-on-failure
```

**Autorisierte Ablageaenderung vom 06.11.2026:** U-C-000122/126/128 werden
im bestehenden Umfang auf `bootsmodell.json` mit generischem
`setting` direkt an jedem bestehenden Element, einmaligen
`Segel.quelle_datei`-/`lookup_datei`-Referenzen, reinen gemeinsamen
`konfiguration`-Metadaten und `bootsmodell.lock` angepasst.
Keine neuen Kennungen, kein Betriebs-Trimm und keine neuen Quellen.
Historische Testergebnisse unten betreffen die damalige Sidecar-Ablage;
aktuelle Nachweise der neuen Ablage stehen im folgenden Migrationsabschnitt.
Neue Tests pruefen Teilkonfiguration, Erhalt fremder Felder/Nicht-Segel,
stabile gemeinsame Modellfassung, Legacy-Konflikt und stale Modell-/Quellstand.
Weitere autorisierte Revision nach Wiederaufnahme: keine zentrale
Elementmap/neuen IDs und keine redundant persistierten cm-/Quell-IDs.
Generische Nicht-Segel-Montageposen wirken in der Darstellung; bestehende
Physik bleibt unveraendert. Der gemeinsame Basisfingerprint blendet nur
echte Element-Settings und Top-Level-Metadaten aus; referenzierte Dateien
statt vollstaendiger Lookup-Verzeichnisse. Frischer Katalog-/Physikimport
und Neustart mit Revision12 sind explizite Abnahmetests.

**Migrationsvalidierung 06.11.2026, aktueller Arbeitsstand:** Der Parent hat
alle elf realen Modelle auf `bootsmodell.json` mit Settings an den 38
bestehenden Elementen migriert; keine neuen Objekt-IDs. Frischer nativer
Katalog/Inspektionslauf meldet alle elf Modelle verwendbar und die
420er-Konfiguration bereit mit unveraenderter Revision12. Bestaetigte
Segellagen, Original-Buglagen und Quellfingerprints blieben erhalten;
Basisfingerprints wurden nach dem finalen Ausblendungsvertrag neu ermittelt.
Diese reale Migration wurde vom Parent ausgefuehrt, nicht vom Core-Test.
Parent-Nachweise: `segelkonfiguration_contract` bestanden (19,85 s),
`modell_geometrie_v2` bestanden (66,97 s), `gui_sail_configurator_420`
bestanden (165,68 s; vorheriger Menue-/Aktionstimeout danach nicht
reproduziert). Read-only-Snapshot und Automatik/Save auf einer eigenen
sauberen GUI-Fixturekopie wurden nativ erfolgreich geprueft.

Nach Korrektur der neuen Setting-Diagnosefundstellen und typenvertraeglicher
Kontextbildung bei numerischer Element-ID wurde ausschliesslich
`modell_tests` mit seiner Core-Library neu gebaut: `modell_import`
bestanden (0,29 s; CTest 1/1). Der bestehende Importvertrag fuer fehlerhafte
IDs bleibt erhalten; kein nlohmann-Typeerror statt Core-Diagnose.
Nach diesem letzten Library-Rebuild sind GUI und weitere statisch
gelinkte Testprogramme vom Parent erneut zu linken; deren oben genannte
Laeufe stammen vom vorherigen, funktional identischen Setting-Stand vor
der reinen Diagnosekorrektur.

Nicht alle Tests bestanden: `modellkatalog_schnittstelle` bleibt mit
`Referenzgeometrie fehlt` auf bekanntem Baseline-Stand; `gui_core_integration`
erreicht den bekannten stationaeren Ruderfehler; `gui_model_catalog`
meldet weiterhin fehlende Bewegungsdefinitionen. Diese Bestandsluecken
sind keine erfolgreiche Gesamtabnahme und wurden nicht im Montageauftrag
behoben. Nutzerabnahme und abschliessender Parent-Relink bleiben getrennt
von den genannten technischen Feature-Nachweisen.

**Autorisierte 0-Grad-Grundstellung, 06.11.2026 nach Wiederaufnahme:**
Der Nutzer hat belegte OBJ-Montagereferenzen fuer alle drei Boote und
sichtbare Dokumentation der intrinsischen Quellenkonflikte freigegeben.
Bestehende U-C-000123/124/126/128/129 werden entsprechend ergaenzt;
keine neuen Kennungen oder UI-Steuerungen. Automatik und Betriebswinkel
delta=0 beziehen sich auf die neutrale Mittschiffsbasis, nicht auf
historisch geoeffnete Rohbaeume. Nichttriviale Quaternionen erhalten
Vorliek-/Vorstagneigung; Opti-Spriet folgt der starren Segelpose, Mast fest.
Belegte Vertex-/Ringreferenzen und Quellenkonflikte stehen ausschliesslich
in [Spezifikation, Abschnitt 2a](../Spezifikationen/Spezifikation_Segelkonfigurator.md).

Vor Datenaenderung wurden alle elf Modelle in
`files/zero-base-backup/Modelle` im Sitzungsordner gesichert und 126
Dateien mit Groesse/SHA-256 inventarisiert. Dry-run auf eigener Kopie,
danach Migration der realen Modelle mit dem explizit aufgerufenen
nativen `--normalize-base`-Werkzeug. Nur drei Dateien geaendert:
`Modelle/420/bootsmodell.json`, `folkeboot/bootsmodell.json`,
`optimist/bootsmodell.json`. Saemtliche OBJ-/intrinsischen Quellen und
sonstigen 123 Dateien byteidentisch; alle bestehenden Element-IDs und
420-Originalseiten (`Großsegel: unbekannt`, `Fock: backbord`) erhalten.
Revisionen danach 420 **15** (vorher 12), Folkeboot **2**, Optimist **2**;
Revision12 bleibt im Backup historisch nachvollziehbar, wurde nicht
zurueckgesetzt. Quellenkonflikte bleiben als `CORE-SEG-0030` sichtbar;
der widerspruechliche verschachtelte Opti-Alt-Sidecar bleibt unveraendert
und wird nicht als Datenvertrag genutzt.

Isolierter nativer Build unter `CORE/build/zero-base-native`, kein
Shared-Build: finale komplette Segel-Vertragssuite bestanden.
Auch `modell_tests` mit denselben finalen Core-Quellen isoliert gebaut
und auf dem realen Quader ausgefuehrt: alle Modellimport-Pruefungen bestanden.
`--zero-base-contract` auf den elf realen Modellen bestanden:
alle elf Katalogeintraege verwendbar, drei neutrale Bootsbasis-Snapshots,
fuenf Segel-Referenzebenen mit Oeffnung innerhalb 1e-7 m von Y=0;
Tatsaechlich ausgegebene Oeffnungswinkel: 420 Großsegel
-5,99716e-34 Grad / Fock 0 Grad, Folk Großsegel -4,19794e-15 Grad /
Fock 0 Grad, Opti Großsegel -1,59867e-15 Grad (Gleitkomma-Rundung).
Großbaumachsen neutral bei erhaltenen Endpunkthoehen. Semantische
Halshoehen: 420 Großsegel 1,35 m / Fock 0,616428 m,
Folk Großsegel 0,78 m / Fock 0,680273 m, Opti Großsegel 0,679582 m.
Gemessene vorhandene Großsegel/Baum-Anschlagabstaende:
420 0,0584617 m, Folk 0,0497342 m, Opti 0,0378334 m;
keine Skalierung oder kuenstliche Nullsetzung dieser konstruktiven
Abstaende. Opti-Spriet/Peak-Abstand erhalten und kleiner 0,08 m.
Test prueft ausserdem Vorliekneigung/Formabstaende, richtige Vorzeichen
von +/-10 Grad, delta=0 identisch zur Montage, generische
Folk-/Opti-Inspektionsdarstellung ohne Doppeltransformation,
Automatik reproduziert neutrale Basis sowie stabile Basisfingerprints.
Neuer CTest-Selektor `segel_mittschiffs_basis` verwendet dieselbe native
Testdatei. Shared-Relink/GUI-Nachtests dieses erweiterten Stands stehen
beim Parent noch aus; vorherige PASS-Zeiten weiter oben belegen nur den
damaligen Setting-Stand. Baseline-Luecken bleiben offen.

**Autorisierte Quellenpfad-Nachfuehrung, 06.11.2026, 16:16 Uhr:**
Nach obigem Nachweis wurden extern Folk-/Opti-Quellen in Unterordner
verschoben. Die drei vorhandenen `Segel.quelle_datei`-Referenzen wurden
auf `main/main.json` beziehungsweise `fock/fock.json` nachgefuehrt.
Verschobene Quellenbytes sind identisch zum Zero-Base-Backup.
Frische Bootsmodelle vor der Korrektur unter
`CORE/build/zero-base-native/relocated-source-backup` gesichert;
Korrektur unter exklusiver Windows-Sperre `bootsmodell.lock` und
atomarem Dateiersatz, ohne andere Modellinhalte/Posen/Originalseiten
zu aendern. Revision Folk/Opti jeweils **2 → 3**, 420 bleibt **15**.
Aktuelle Basisfingerprints: Folk `fnv1a64:95a04deb114fe8c4`,
Opti `fnv1a64:bbb667f1c52497b1`; durch frisch gebautes natives
`--fingerprint` berechnet. Modell-/Segelquellenfingerprints aktualisiert,
`CORE-SEG-0030` bleibt als nichtfataler Quellenkopplungskonflikt erhalten.
Finaler Shared-Release-Test direkt auf dem aktuellen Bestand:
`--zero-base-contract` und komplette Segel-Vertragssuite bestanden;
alle elf Modelle verwendbar und alle drei neutralen Snapshots bereit.
Finaler CTest `segel_mittschiffs_basis` nach ergaenzter expliziter
`CORE-SEG-0030`-Assertion: **PASS, 4,54 s**.
Die zuvor gemeldeten Import-PASS und GUI-Testergebnisse sind getrennte
Nachweise; GUI-Wiederholung und bekannte Baseline-Luecken bleiben offen.

Massgeblich: [Core-Spezifikation](../Spezifikationen/Spezifikation_Segelkonfigurator.md).
Abnehmer: [getrennter GUI-Plan G-0.13](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.13.md).

Kennungen vor Vergabe projektweit geprueft: bisher A-C-00135,
U-C-000121 und CPPI-0119. Neu A-C-00136 bis A-C-00147,
U-C-000122 bis U-C-000130 sowie CPPI-0120 bis CPPI-0138.
Ergaenzung auf Nutzerfreigabe vom 06.11.2026, 10:11 Uhr:
A-C-00146 und U-C-000129 fuer den einheitlichen Winkelvertrag.
Weitere Nutzerfreigabe vom 06.11.2026: A-C-00147 und U-C-000130
fuer Original-Buglage und separate Formparitaet.

## 1. Gepruefter Bestand und technische Voraussetzungen

- `Code/include/segelphysik/modell.hpp`: Modellbeschreibung mit erhaltenem
  JSON-Dokument, Element-IDs und Quellmeshes.
- `Code/include/segelphysik/inspektion.hpp`: unveraenderlicher
  ModelInspection-Snapshot, Mesh-/Baum-/Bewegungsvorschau.
  Diese Lesevertraege nicht heimlich in mutierbare Objekte verwandeln.
- `Dokumentationen/Schnittstellenregister_Core.md`: projektlokales
  Register fuer jeden neuen exportierten Typ und jede Funktion/Methode.
- Reales 420er-Modell: Element-IDs `Großsegel`, `Fock`, `Mast`,
  `Großbaum`, `Vorstag`; Darstellungsquellen unter `modells/gui`.
- Grosssegel-Quelle `modells/aerodynamik/main/main.json`:
  ID `420_grosssegel`, Hals [0,0,0], Unterliek Richtung -X,
  Vorliek Richtung +Z, vorderer und hinterer Kopfpunkt sowie mehrere
  Achterliek-Zwischenpunkte. Nicht pauschal als Dreieck behandeln.
- Beide neuen Darstellungs-OBJ nennen ihre jeweilige intrinsische JSON,
  Halsursprung, Meter und lokale Achsen; diese Kommentare sind
  Bestandsbelege, kein ausreichender maschinenlesbarer Zuordnungsvertrag.
- `modells/aerodynamik/main/LookUp` enthaelt lookup.json, summen.csv,
  kraefte.csv, profile.csv, pruefbericht.json und auftrag_ausgefuehrt.json.
- SegelMacher wurde nur lesend untersucht. Dessen Lookup-Spezifikation
  Abschnitt 8 verlangt lokale Kraft-/Momentenbezuege und Pruefsummen;
  Einbaulagen gehoeren ins Gesamtmodell, nicht in intrinsische Geometrien.

Core-seitiges Referenzmanifest ist umgesetzt. Es ordnet `Großsegel` und
`Fock` explizit ihren intrinsischen JSON-IDs, Darstellungs-OBJ-IDs und
Lookups zu und liest semantische Eckpunkte aus den benannten IDs der
Grundkontur. Rigg-OBJ-Pfade werden anhand der semantischen Modellelemente
`Mast`, `Großbaum`, `Vorstag` aus deren `Darstellung.datei` in `bootsmodell.json`
aufgeloest und gegen die expliziten 420-Dateipfade geprueft.
Rigganschlaege/-richtungen werden aus den Original-OBJs der Modelldaten
ermittelt, nicht aus parallel definierten Koordinatenkonstanten:
der Mastanschlag ist die Mast-Mittellinie auf Hoehe der Grossbaum-Endkappen,
der Baumvektor verlaeuft zwischen deren Zentren, der Fockanschlag ist das
Zentrum des ersten Vorstagquerschnitts und die Vorstagachse wird aus den
ersten zwei Querschnittszentren gebildet. Die aus dem 420er-Datensatz
reproduzierten Referenzwerte sind Mast/Baumanschlag
`[0.85070675, 0, 1.35] m`, Baumrichtung zum Schothorn
`[-2.338488, 0.539883, 0] m`, Fockdeck `[2.00570675, 0, 0.616428] m`
und Vorstagrichtung `[-0.1475, 0, 0.510447] m`. Das sind Geometriebezüge
des nicht simulationsfreigegebenen 420er-Entwurfs, keine
Messgenauigkeitsbehauptung.
Originale werden zur Laufzeit nur gelesen. Die Original-Buglage ist in
den Quellen nicht explizit deklariert und bleibt daher initial
`Unbekannt`; keine Ableitung aus Wölbung oder Meshseite.

## 2. Geordnete Umsetzung

Die Implementierung ist in Core vorhanden. Technischer Nachweis und
GUI-Integration sind in der Statusspalte getrennt ausgewiesen.

| Schritt | Arbeit und Ergebnis | Anforderungen | Voraussetzung | Status/Nachweis |
|---|---|---|---|
| U-C-000122 | Typisierte API, versioniertes Konfigurationsschema, Diagnosen und CPPI-Vertrag umgesetzt. | A-C-00136, A-C-00142, A-C-00143, A-C-00144 | keine | umgesetzt; CPPI-0120–0138 und GUI-Vertragsmeldung vorhanden |
| U-C-000123 | Explizites 420er-Mapping, semantische Ecken, Lookup-IDs, SHA-256-Quellfingerprints und aus Rig-OBJ abgeleitete Anschlaege/Achsen. | A-C-00136, A-C-00145 | U-C-000122 | umgesetzt; Kopietest liest 420er-Quellen/Lookups und prueft Rigreferenzen unabhaengig gegen Mast-/Baum-/Vorstag-OBJ |
| U-C-000124 | Starre Transformation und deterministische Großsegel-/Fockautomatik mit Fehlerdiagnosen. | A-C-00137, A-C-00138, A-C-00142 | U-C-000123 | umgesetzt; Halsposition und Liek-Rigachsen treffen die aus OBJ abgeleiteten Referenzen innerhalb 1e-6 m/rad |
| U-C-000125 | Translation, semantische Pivotrotation und absoluter Fock-cm-Versatz. | A-C-00139, A-C-00140, A-C-00143 | U-C-000124 | umgesetzt; XY/XZ/YZ-, Drei-/Viereck-, Pivot- und +/-/0-cm-Tests bestanden |
| U-C-000126 | Modelldateinahe Ablage, atomare Ersetzung, OS-Schreibsperre und Revisions-/Quellenprüfung. | A-C-00141, A-C-00143 | U-C-000125 | umgesetzt; Neustart, konkurrierende Prozesse und Windows-Fehler-Injektionen vor Temp-Schreiben und vor atomarer Ersetzung bestanden |
| U-C-000127 | Read-only-Geometrie der bestätigten Lage; keine Änderung des Inspektionssnapshots oder der Quell-/Lookupdateien. | A-C-00142, A-C-00143, A-C-00144 | U-C-000126 | umgesetzt; Kopietest prüft Wiederherstellung und unveränderte Inspektionsmeshes |
| U-C-000128 | Core-Vertrags-/Regressionssuite und dokumentierte GUI-Lieferung. | A-C-00136 bis A-C-00145 | U-C-000127 | Debug/Release-Feature-CTest bestanden; alle sechs Vollsuite-Fehler sind auf HEAD reproduziert und gelten damit als baseline. Aktueller Sechser-Selektor reproduziert dieselben Fehler. GUI-Build/Integration separat |
| U-C-000129 | Lokaler Segelbezug, Achse Kopf-zum-Hals und delta-Konvention, ohne Stellereignisse/Physik. | A-C-00146 | U-C-000122, U-C-000123 | umgesetzt; synthetische +90/−90-Rotation um lokales -Z bestanden |
| U-C-000130 | Persistente Original-Buglage und getrennte lokale Formparität inkl. Dreiecksorientierung. | A-C-00147 | U-C-000126, U-C-000127, U-C-000129 | umgesetzt; beide Seiten, 0/unbekannt, alte Konfiguration, gewoelbte Punktgeometrie, Face-Normalen aus Dreiecksorientierung und Idempotenz getestet |

## 3. Meilensteine und GUI-Uebergabe

| Meilenstein | Eintritt | Abnahmekriterium / Lieferung | Status |
|---|---|---|---|
| C1: Daten und Vertrag | U-C-000122/123 | CPPI, Schema, Diagnosen und 420er-Referenzen; GUI-Vertrag kommuniziert. | technisch erreicht |
| C2: Geometrische Operationen | C1; U-C-000124/125/129 | Automatik, XY/XZ/YZ, Drei-/Viereckpivots, cm-Formel und delta-Konvention. | Feature-Tests bestanden |
| C3: Dauerhafte Transaktion | C2; U-C-000126/127/130 | Neustart, atomare Datei, Prozesssperre/Revision, Diagnose, unveraenderliche Inspektion. | gezielte Fault-Injection vor Temp-Schreiben und vor Ersetzung bestanden; Quellkonflikt verwirft Commit ohne Dateiaenderung |
| C4: Core-Abschluss | C3; U-C-000128 | Anforderungen technisch belegt; Quellen/Lookups unverändert; Bestandsregression und GUI-Lieferung. | sechs native Fehler auf HEAD als baseline bestaetigt; aktueller Sechser-Selektor ohne neue Abweichung; GUI-Integration offen |

Core-technischer Abschluss und GUI-Integration sind getrennte Status.
Die persoenliche 420er-Bedienabnahme erfolgt nach GUI-Meilenstein G4,
nicht allein durch erfolgreiche Core-Tests.

## 4. Rueckverfolgbarkeit und Tests

Anforderungstexte stehen ausschliesslich in der Core-Spezifikation,
Abschnitt 7. Tatsächlicher Umsetzungs- und Nachweisstatus:

| Anforderung | Umsetzung | Geplanter Nachweis |
|---|---|---|
| A-C-00136 | U-C-000122/123, U-C-000128 | 420er-Mapping, semantische Ecken, explizite Referenzen, Fehl-/Mehrdeutigkeitsfaelle |
| A-C-00137 | U-C-000124, U-C-000128 | Hals-/Liek-Toleranzen, Idempotenz, inkompatible Winkel |
| A-C-00138 | U-C-000124, U-C-000128 | Deckbezug, geneigtes Vorstag, achterliche Ausrichtung und Versatz 0 |
| A-C-00139 | U-C-000125, U-C-000128 | Numerische Distanz-/Pivot-/Tiefenpruefung fuer Drei-/Viereck in drei Ebenen |
| A-C-00140 | U-C-000125, U-C-000128 | Formeltests 0/+10/-10, wiederholte Eingabe und Erhalt manueller Korrektur |
| A-C-00141 | U-C-000126, U-C-000128 | Getrennte Prozesse, Schreibverweigerung, unterbrochener Write und konkurrierende Revision |
| A-C-00142 | U-C-000122, U-C-000124, U-C-000127/128 | SHA-256 aller Quellen/Lookups vor/nach jeder Aktionsart; lokale/Modell-Punkt-/Kraft-/Momentenbezuege synthetisch |
| A-C-00143 | U-C-000122, U-C-000125 bis 128 | Typisierte Antworten, Vorschau/Bestaetigung, Fremd-ID, NaN/Inf, kaputte Daten, veraltete Quellen/Revision |
| A-C-00144 | U-C-000122, U-C-000127/128 | Native Bestandsregression; keine Qt/VTK-Abhaengigkeit; 420er-Freigabestatus erhalten |
| A-C-00145 | U-C-000123, U-C-000128 | Reale 420er-Referenz, reproduzierbare Befehle, Quellen-/Buildbindung und Uebergabeprotokoll |
| A-C-00146 | U-C-000129 | Rotation um lokale -Z: 0 mittschiffs, +90 Backbord, -90 Steuerbord; Hals fest, geneigte Fockachse und getrennte Einbaulage/Stellung pruefen |
| A-C-00147 | U-C-000130 | Beide Originalseiten gegen delta positiv/negativ/0/unbekannt; alte Konfiguration ohne Angabe, Neustart/Konflikt, S getrennt von R, Kontur/Quellbytes/Normalen und idempotente Darstellung |

| Anforderung | Tatsächlicher Status/Nachweis |
|---|---|
| A-C-00136 | Kopietest lädt IDs und Quell-/Lookup-Mapping; Mehrdeutigkeitsdiagnose implementiert |
| A-C-00137 | Hals-/Mast/Baumrichtung, Wiederholung und inkompatibles Fixture getestet |
| A-C-00138 | Kopietest prüft Fock-Vorliek, achterliche Schothornrichtung und 0 cm |
| A-C-00139 | Synthetische Dreieck-/Viereckfixtures, alle Ebenen, beide Pivotregeln und Punktabstände bestanden |
| A-C-00140 | Test 0/+10/-10/+12.5 cm, wiederholte Eingabe und orthogonale manuelle Verschiebung bestanden |
| A-C-00141 | Separate Writer-Prozesse mit gleicher Ausgangsrevision: genau ein Commit, ein stale-revision-Fehler. Lock-Verweigerung bewahrt letzte Revision. Windows-Fehler-Injektion blockiert einmal das temporaere Schreiben und einmal die Ersetzung eines vorhandenen Ziels; in beiden Faellen bleiben alte Config-Bytes/Revision identisch und keine Tempdatei bleibt zurueck. Kein Stromausfall-/Dateisystem-Crash-Test |
| A-C-00142 | SHA-256-Fingerprints binden Segel-JSON/OBJ, Lookup-JSON und Rigg-OBJ; unveränderter Fingerprint nach Aktionen sowie geänderter Lookup-Fingerprint getestet. Kein Vorher/Nachher-Hash der mehreren GB großen Lookup-CSV-Produkte; Punkt-/Vektor-/Moment-Physik bleibt außerhalb C-0.12 |
| A-C-00143 | Status/Revision/Diagnosen fuer Vorschau/Commit; Fremd-ID, kaputtes JSON und stale Revision getestet. NaN sowie +Inf/-Inf fuer Translation/Rotation/Fock-cm/Stellwinkel werden abgewiesen; unendlicher Commit veraendert Revision nicht. Commit mit nach Vorschau geaendertem Quellenfingerprint liefert CORE-SEG-0016 und laesst Konfigurationsbytes unveraendert |
| A-C-00144 | Vollsuite 17/23; sechs Fehler alle auf HEAD reproduziert und Arbeitsstand-Selektor identisch; keine Qt/VTK/Core-Physik-Änderung. 420 bleibt `simulationsfreigabe=false` |
| A-C-00145 | Integrationstest nutzt kopierte reale Modell-/Segel-/Lookup-JSONs; GUI-Vertrag an Agent 2eb46ce7-b973-4ed7-b9b9-3ee5a1c67005 gemeldet; GUI-Prüfung offen |
| A-C-00146 | Synthetischer +90 Backbord/-90 Steuerbord und 420-Fockachse getestet |
| A-C-00147 | Beide Seiten, delta=0/unbekannt, alte Konfiguration und Dreiecksorientierung getestet. Neuer nichtplanarer Camber-Fixturetest prueft punktweise S-vor-R, Orientierung und daraus berechnete Face-Normale sowie Wiederholungs-Idempotenz. `InspectionMesh` traegt keine expliziten Normalen; deren Downstream-Renderer-Neuberechnung bleibt GUI-Integration. Aerodynamische Profil-/Lookup-Tabellen werden absichtlich nicht gespiegelt oder physikalisch umgerechnet und bleiben ausserhalb C-0.12 |

### Gezielte Nachpruefung der offenen Vertragsbelege

Nach dem vorherigen Featurelauf wurden ausschliesslich die zuvor fehlenden
Vertragspunkte ergaenzt; kein Vollsuite-Lauf:

1. **Schreibfehler vor Temp-Schreiben:** separater Windows-Testprozess,
   temporaeren Zielpfad `segelkonfiguration.json.tmp.0` als Verzeichnis
   blockiert; erwarteter Fehler, identische alte Config-Bytes/Revision und
   keine verbliebene Tempdatei.
2. **Abbruch vor atomarer Ersetzung:** bestehende Config im Windows-Test
   ohne `FILE_SHARE_DELETE` geoeffnet; Tempdaten werden geschrieben,
   `MoveFileExW` muss scheitern; alte Bytes/Revision bleiben identisch,
   Tempdatei wird bereinigt. Dies belegt Prozessfehler vor Commit, nicht
   Stromausfall-/Datentraeger-Dauerhaftigkeit.
3. **Geometry-/Quellenkonflikt:** nach Snapshoterstellung wird die
   kopierte Grosssegel-Darstellungs-OBJ semantisch unveraendert um einen
   Kommentar erweitert, wodurch ihr Quellfingerprint wechselt. Commit
   liefert `CORE-SEG-0016` und schreibt weder Revision noch Configinhalt um.
4. **Endliche GUI-Eingaben:** Translation (NaN/+Inf), Rotation,
   Fock-cm und Stellwinkel (je NaN/+Inf/-Inf) werden abgewiesen;
   nichtendlicher Commit veraendert die persistierte Revision nicht.
5. **Formparitaet:** synthetisches nichtplanar gewoelbtes Segel prueft die
   gespiegelten Punkte und Reihenfolge S vor Stellrotation, invertierte
   Triangle-Winding, daraus unabhaengig berechnete Face-Normale,
   unveraenderte Quellpunkte und idempotente Wiederholung.

Der Prozess-Revisionskonflikt war bereits belegt: unter Windows schreiben
zwei getrennte Writer-Prozesse mit derselben Snapshotrevision; genau einer
speichert, der zweite wird als stale abgewiesen. Die OS-Sperre hat einen
eigenen Verweigerungstest.

Der finale begrenzte Lauf baute nur `segelkonfigurator_tests` in
`CORE\build\standard` (Debug) und fuehrte nur
`ctest -R '^segelkonfiguration_contract$'` aus: **1/1 CTest bestanden,
110,22 s**. Darin liegen die fuenf oben einzeln ausgewiesenen neuen
Nachweisgruppen; der bereits bestehende separate Prozess-Revisionskonflikt
und OS-Locktest wurden ebenfalls erneut ausgefuehrt. Es wurde keine
Vollsuite gestartet. Genaues Kommando:

```powershell
Set-Location 'C:\_DATEN\Code\Python\Organisierter Code\Segelphysik\CORE'
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build build\standard --config Debug --target segelkonfigurator_tests -j 4
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
& $ctest --test-dir build\standard -C Debug -R '^segelkonfiguration_contract$' --output-on-failure
```

Anschliessend wurde derselbe endgueltige Teststand in Release gebaut und
ausgefuehrt (ebenfalls nur dieses Target/dieser Test):

```powershell
& $cmake --build build\standard --config Release --target segelkonfigurator_tests -j 4
& $ctest --test-dir build\standard -C Release -R '^segelkonfiguration_contract$' --output-on-failure
```

Release-Build erfolgreich; `segelkonfiguration_contract` **1/1 bestanden,
11,11 s**. Damit bestehen die ergaenzten Nachtests in Debug und Release.

**Technischer Umfang A-C:** A-C-00136 bis A-C-00144 sowie A-C-00146 sind
Core-seitig implementiert und durch die im Statusnachweis genannten Tests
belegt; bei A-C-00142 bleiben Hashes der mehrgigabytegrossen CSVs und jede
Physik-/Lookup-Auswertung explizit ausserhalb des Core-Nachweises. A-C-00145
hat den Core-420-Kopietest und dokumentierten GUI-Vertrag, aber keine
abgeschlossene GUI-Integration oder Nutzerabnahme. A-C-00147 belegt
Core-seitige lokale Camber-Geometriespiegelung samt winding-konsistenter,
aus Facetten ableitbarer Normalen; `InspectionMesh` hat kein Feld fuer
explizite Vertexnormalen, daher ist deren Anzeige-/Neuberechnung im GUI
noch zu integrieren. Lookup-Profile werden nicht gespiegelt: deren
physikalische Paritaet ist kein C-0.12-Feature. Die 420er-Originalseite
bleibt mangels Quelldeklaration `Unbekannt`; keine nichtnullige Spiegelung
ist fuer die Originaldaten ohne ausdruecklich gesetzte Buglage moeglich.

C4 setzt zusaetzlich U-C-000129 voraus. Der Winkelvertrag ist in Header,
API und synthetischen Tests umgesetzt; neue Stellereignisse, GUI-Regler und
Wind-/Lookup-Auswertung sind kein Teil dieses Arbeitsschritts.
C4 setzt ausserdem U-C-000130 voraus. Die Tests fuer A-C-00142 pruefen
weiterhin eigentliche starre R-Transformationen; die ausdrueckliche
Formspiegelung hat ihren eigenen Nachweis unter A-C-00147.
Keine Aerodynamik-Symmetrie oder Gueltigkeit gespiegelter Lookups behaupten.
Die Riggreferenzen werden in `Code/src/segelkonfigurator.cpp` anhand der
`Darstellung.datei`-Referenzen der Modellelemente `Mast`, `Großbaum`,
`Vorstag` aus den Originaldateien
`Modelle/420/modells/gui/{mast,grossbaum,vorstag}.obj` ermittelt; die Pfade
werden gegen die expliziten 420-Dateipfade geprueft. Deren
Koordinatenkonvention lautet Meter, +X Bug, +Y Backbord, +Z oben. Alle
drei Meshes sind Teil des bestehenden Quellenfingerprints.
Der Kopietest liest sie unabhaengig, berechnet Mast-/Vorstagquerschnitte
und Grossbaum-Endkappen direkt aus den OBJ-Vertices und prueft
Anschlagabweichung <= 1e-6 m sowie Winkelabweichung der automatisch
transformierten Liek-/Rigachsen <= 1e-6 rad. Grossbaum-Endkappen werden
zusaetzlich gegen ihre separat berechneten Querschnittszentren validiert.
Die verwendeten Vertexbereiche sind explizite Geometriemetadaten dieses
Modells: Mast-Ringe 1–24/25–48, Baumringe 1–24/25–48 mit Endkappen
49/50, Vorstag-Anschlagringe 1–8/9–16.

Tests nutzen ausschließlich eine Kopie des 420er-Modells unter
`build\standard` sowie synthetische Referenzen. Die API schreibt nur
`segelkonfiguration.json`; Quell-/Lookup-Pfade sind ausschließlich
Lesezugriffe. Der Test bindet Segel-JSON/OBJ, Lookup-JSON und Rigg-OBJ per
SHA-256-Fingerprint und prüft Erkennung veralteter Quellen. Die mehreren
GB großen Lookup-CSV-Produkte wurden nicht vollständig gehasht; es gibt
keinen behaupteten Vorher/Nachher-Prüfsummenbeleg für diese Dateien.
`Modelle\420\segelkonfiguration.json` wurde nicht erzeugt. Lookupdaten
wurden nicht aerodynamisch neu validiert oder interpoliert.

### Aktuelle Core-Featuretests

Nach Ableitung der Riggreferenzen aus den OBJs wurden die betroffenen Debug-
Targets gebaut und der CTest-Selektor fuer die sechs alten Fehler plus
Featuretest ausgefuehrt. Danach wurde der Featuretarget in Release gebaut
und getestet:

```powershell
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
& $cmake --build build\standard --config Debug --target aufbereitung_tests hydrodynamik_tests simulation_tests stationaere_kurvenfahrt_tests katalog_tests inspektion_tests segelkonfigurator_tests -j 4
& $ctest --test-dir build\standard -C Debug -R '^(modellaufbereitung|hydrodynamik_referenzmodelle|simulation_gesamtlauf|stationaere_kurvenfahrt|modellkatalog_schnittstelle|modellinspektion_c08|segelkonfiguration_contract)$' --output-on-failure
& $cmake --build build\standard --target segelkonfigurator_tests --config Release -j 4
& $ctest --test-dir build\standard -C Release -R '^segelkonfiguration_contract$' --output-on-failure
```

Der Debug-Build aller sieben Selektor-Targets war erfolgreich. Der Debug-
CTest lief nach dem abschliessenden Modelldatei-Pfadcheck 286,37 s:
Featuretest 1/1 bestanden; die sechs uebrigen Faelle schlugen wie auf HEAD
fehl. Der Release-Build des Featuretargets war erfolgreich;
`segelkonfiguration_contract` bestand 1/1 in 7,69 s. Der letzte
vollstaendige vorherige Debuglauf hatte 17/23 Tests bestanden. Die HEAD-
Baseline und der Arbeitsstand vergleichen genau die sechs fehlgeschlagenen
Testnamen; der aktuelle Featuretest besteht.

### Gezielte HEAD-Baseline

Unveraenderter `HEAD` war `27b0d806459f8ebd642d9240a8b4f8264230e6d3`.
Dieser Commit wurde mit `git archive` (ohne Branchwechsel, Worktree oder
Schreibzugriff auf Git-Referenzen) nach
`C:\Users\Katarina\.copilot\session-state\459eb8b3-37e1-45a7-b60e-2ee92980a121\files\baseline-core-head-27b0d80-20261106-1122\source`
entpackt. Die Baseline wurde separat konfiguriert/gebaut; `SEGELPHYSIK_MODELLE_DIR`
zeigte auf dieselben Originaldaten unter
`C:\_DATEN\Code\Python\Organisierter Code\Segelphysik\Modelle`, und
`nlohmann_json` wurde aus derselben bestehenden GUI-vcpkg-Installation
bezogen. Eigener Buildordner:
`...\baseline-core-head-27b0d80-20261106-1122\build`.

Der abgegrenzte Lauf:

```powershell
$core = 'C:\_DATEN\Code\Python\Organisierter Code\Segelphysik\CORE'
$base = 'C:\Users\Katarina\.copilot\session-state\459eb8b3-37e1-45a7-b60e-2ee92980a121\files\baseline-core-head-27b0d80-20261106-1122'
$src = Join-Path $base 'source'
New-Item -ItemType Directory -Path $src -Force | Out-Null
git -C $core archive --format=tar --output="$(Join-Path $base 'head.tar')" HEAD
tar -xf (Join-Path $base 'head.tar') -C $src
$cmake = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
$models = 'C:\_DATEN\Code\Python\Organisierter Code\Segelphysik\Modelle'
$json = 'C:\_DATEN\Code\Python\Organisierter Code\Segelphysik\GUI\vcpkg_installed\x64-windows\share\nlohmann_json'
& $cmake -S "$base\source\Code" -B "$base\build" -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON "-DSEGELPHYSIK_MODELLE_DIR=$models" "-Dnlohmann_json_DIR=$json"
& $cmake --build "$base\build" --config Debug --target aufbereitung_tests hydrodynamik_tests simulation_tests stationaere_kurvenfahrt_tests katalog_tests inspektion_tests -j 4
& $ctest --test-dir "$base\build" -C Debug -R '^(modellaufbereitung|hydrodynamik_referenzmodelle|simulation_gesamtlauf|stationaere_kurvenfahrt|modellkatalog_schnittstelle|modellinspektion_c08)$' --output-on-failure
```

Baseline-Configure und gezielter Sechs-Target-Build erfolgreich. CTest:
**0/6 bestanden**, Gesamtdauer 152,52 s. Jeder der sechs Fehler wurde auf
unveraendertem HEAD mit denselben Original-Modelldaten reproduziert. Der
nachfolgende aktuelle Arbeitsstand-Selektor reproduziert dieselben sechs
Fehlermeldungen, waehrend `segelkonfiguration_contract` besteht. Damit ist
der Status dieser sechs Fehler beweisbar **Baseline auf HEAD**, nicht eine
durch C-0.12 neu eingefuehrte Regression. Diese Aussage bezieht sich genau
auf den archivierten Commit und die angegebene Modellauswahl/Abhaengigkeit.

| Test | HEAD-Baseline-Fehler, identisch nach C-0.12 |
|---|---|
| `modellaufbereitung` | `CORE-HYD-0001: Modell enthaelt kein Hydrodynamik-Element.` |
| `hydrodynamik_referenzmodelle` | `Optimist-Schwert-Hubweg weicht von der Meshreferenz ab` |
| `simulation_gesamtlauf` | `Stationaerer Solver hat mehrere feste Stellungen nicht uebernommen`; vorherige analytische, exzentrische Kraft- und Galilei-Referenzen bestanden. |
| `stationaere_kurvenfahrt` | 420: 0°, ±1° konvergieren; bei 12° Trust-Region-/Liniensuche stagniert nach 82 Iterationen (`CORE-SIM-0004`), Kraftresiduum `[21.5984,-1.70235,17.0968] N`, Momentresiduum `[-0.87604,-2.5321,-2.91858] Nm`. |
| `modellkatalog_schnittstelle` | `Referenzgeometrie fehlt`; Legacy-Fixture erwartet `q\boot.obj`, waehrend die Modellstruktur `modells\gui\rumpf.obj` referenziert. |
| `modellinspektion_c08` | `missing tree type hydrodynamik` im kopierten 420er-Snapshot. |

Die Fehler umfassen den Aufbereitungspfad ohne hydrodynamisches Element,
eine Optimist-Bewegungsreferenz, Solver-Stellungsuebernahme, einen 420er-
Kurvenfahrt-Solverfall sowie Quader-Asset- und 420-Baumtyp-Assertions.
Keiner ruft den Segelkonfigurator auf. Keine Korrektur an diesen
unabhaengigen Altpfaden wurde vorgenommen; die sechs sind nun als Baseline
belegt statt bloss vermutet.

### Quellhash-Nachweis fuer den letzten Feature-CTest

Im frueheren Feature-CTest wurden die folgenden neun Originaldateien unter
`..\Modelle\420` vor und nach dem Lauf mit PowerShell
`Get-FileHash -Algorithm SHA256` gehasht; alle Vorher-/Nachher-Werte waren
gleich. Nach der neuen riggeometrischen Implementierung wurden Mast-,
Grossbaum- und Vorstag-OBJ erneut gehasht und stimmten jeweils bytegenau
mit diesen vorherigen SHA-256-Werten ueberein. Die Tests schreiben keine
Originaldateien; die 420er-Konfiguration wurde nicht erzeugt. Dies ist kein
Hashnachweis fuer die gesamte Vollsuite oder die mehrgigabytegrossen
Lookup-CSV-Dateien.

| Originaldatei | SHA-256 |
|---|---|
| `modells\aerodynamik\main\main.json` | `E8C47FFCA62459C08AE525320794B19884A72544A2857C77606473CCEA862EB2` |
| `modells\gui\grosssegel.obj` | `080D5DBABA0D28C7F8389FFCFD2C6DA4041096DAF697430F45A1A2A321B7648B` |
| `modells\aerodynamik\main\LookUp\lookup.json` | `9C8B9F62CD7E974FA90C6B6449900C3746BE9F19E24BB2BDF4002823F2A9FACF` |
| `modells\aerodynamik\fock\fock.json` | `C06862FF7016F37933E32A8ADFD8DCDB7F8B67F41967767BCDE7D21A0ECA0646` |
| `modells\gui\fock.obj` | `F16EDF3B8E35B56E70B1D7C52A56473A3F39D550268280A1FEE0C0E0A347AE77` |
| `modells\aerodynamik\fock\LookUp\lookup.json` | `56D13A1592480E8AE9C1CFA62D63A73BAEFD4F2922084E891E774C1BEF2EB787` |
| `modells\gui\mast.obj` | `138595E2C2448B421FB99D933AFBAFC2690184935BC47B41F603D40C19294BFC` |
| `modells\gui\grossbaum.obj` | `D62F5909D6124F4AB1A3E307B1E3DEEB6FBFB4119208CE93A8323D3648D38716` |
| `modells\gui\vorstag.obj` | `5FC837408DF9094C3F87A5F94A62774FB2B91E6A23994CBB3C9D8AFEA4AC4091` |

Der Nutzer bestaetigt zusaetzlich eine eigene Vorher-/Nachher-Pruefung
aller 40 Original-Quelldateien einschliesslich Lookup-CSV-Produkten gegen
die archivierte Baseline; diese externe Pruefung wurde fuer den gezielten
Nachtest nicht erneut ausgefuehrt. Die hier aufgefuehrten neun SHA-256-
Werte und bestehenden Kopietests bleiben der Core-eigene Nachweis.

## 5. Abschlussstand

Core-Code, Schnittstellenregister und Core-GUI-Dokumentation umgesetzt.
Debug- und Release-Feature-CTest bestanden; finaler ergaenzter Nachtest
in Debug (110,22 s) und Release (11,11 s) je 1/1 bestanden. Alle sechs nativen
Vollsuite-Fehler sind auf archiviertem HEAD reproduziert und damit
nachweislich Baseline; der aktuelle Sechser-Selektor zeigt keine neue
Abweichung. Getrennte GUI-Integration und Nutzerabnahme bleiben offen.
Die Original-Buglage der 420er-Quellen bleibt mangels expliziter
Quelldeklaration `Unbekannt` und muss vor einer nichtnulligen Stellwinkel-
Spiegelung ausdrücklich festgelegt werden. Keine Nutzerabnahme behauptet.
Keine neue Aerodynamik/Lookup-Auswertung, Simulationsintegration oder
Segelverformung. Keine Branch-/Worktree-/Commit-/Push-Aktion ausgefuehrt;
fuer den angeforderten Baselinebeleg wurde nur `git archive HEAD` gelesen.
Die eindeutig benannte Session-Baseline-Extraktion und ihr separater
Buildordner bleiben erhalten, weil sie fuer den dokumentierten
HEAD-Fehlerbeleg weiterhin benoetigt werden; es wurden keine Dateien daraus
geloescht.
