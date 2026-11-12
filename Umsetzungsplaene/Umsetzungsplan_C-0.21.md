# Core-Umsetzungsplan C-0.21: Modellformat 0.3 und Fingerprints

**Stand:** 12.11.2026. **Status:** Umsetzung und Core-/GUI-Postgates bestanden; keine Git-Veröffentlichung.
Einziger maßgeblicher Core-Plan für C-0.21.
Maßgeblich ist die [Core-Spezifikation](../Spezifikationen/Spezifikation_Modellbeschreibung.md#modellformat-03-fingerprints-und-modellpfade);
der C++-Vertrag für externe Werkzeuge steht in
[Schnittstelle Core → Modellwerkzeuge](../Dokumentationen/Schnittstelle_Core_Modellwerkzeuge.md).

**Projekttrennung:** Dieser Plan verantwortet ausschließlich Core-Code,
Core-Tests und den einmaligen Modellordner-Cutover. GUI-Implementierung und
GUI-Abnahme stehen im getrennten
[GUI-Plan G-0.21](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.21.md).
Der physische Cutover ist von dessen bestandenem Vorab-Gate abhängig.

## 1. Bestand, Umfang und Abhängigkeiten

Der neue Bestand enthält elf Modellordner im Verzeichnis `Modelle-NEW` mit
`modells_data.json` und `modells_validation.json`; die beiden Schema-Dateien
liegen an dessen Wurzel. Der aktive Core-Katalog wird auf dieses Format
umgestellt. Ein stiller Fallback auf `modells_information.json` ist für den
aktiven Katalog ausgeschlossen. Bestehende, getrennte Legacy-Konverter oder
Importverträge bleiben außerhalb dieses Katalogpfads erhalten. Aktive
0.3-Tests verwenden 0.3-Fixtures und temporäre Kopien; Legacy-0.1/0.2-Fixtures
bleiben ausschließlich in explizit als Kompatibilität markierten Tests.

Umfang:

- Draft-2020-12-Validierung und dateiübergreifende Ressourcen-/Identitätsprüfung
  vor jeder Verwendung.
- Fünf deterministische SHA-256-Bereichs-Fingerprints, Vergleich ohne
  Schreibzugriff und ausdrückliche, atomare Aktualisierung.
- Eine öffentliche C++-Funktion für Core, GUI und externe Modellwerkzeuge.
- Gemeinsame, editierbare Laufzeit-Defaults statt festcodierter aktiver
  Modellwurzeln.
- Vollständige C++-/CTest-Abnahme vor und nach dem Cutover.

Nicht enthalten sind die GUI-Implementierung, Änderungen der Modellphysik,
eine GUI-eigene Format- oder Hashauswertung sowie Commit, Tag oder Release.
Keine Ordner werden vor dem abschließenden Cutover-Schritt verschoben.

Projektweit vor Vergabe geprüft: vorher höchste A-C-00147 und U-C-000130;
dieser Plan vergibt A-C-00148 bis A-C-00153 und U-C-000131 bis U-C-000137.
Im CPPI-Register war CPPI-0142 die höchste Kennung; CPPI-0143 und CPPI-0144
werden einzeln für die Ergebnisdatentypen geführt. CPPI-0141 bleibt als
zurückgezogene, zu breit vergebene Entwurfskennung nachvollziehbar.

## 2. Geordnete Umsetzung

| U-ID | Umsetzungsschritt | Anforderungen | Abhängigkeiten | Nachweis/Abnahmekriterium |
|---|---|---|---|---|
| U-C-000134 | Laufzeitauflösung für `%APPDATA%\SegelPhysik\defaults.json` bereitstellen (`modelRoot`, `schemaRoot`, `defaults`), Priorität explizite Auswahl → Test-Override → Umgebungsvariable → Datei durchsetzen und aktive festcodierte Modellwurzeln in Code, CMake, Startprofilen und Tests beseitigen. | A-C-00152 | keine | Konfigurations-/Prioritätstests bestehen; Produktions-Build hat keinen festcodierten aktiven Modellwurzelpfad; Fehler führen zu Diagnose statt stiller Ersatzpfade. |
| U-C-000131 | Eine Draft-2020-12-fähige Validatorabhängigkeit auswählen, deklarieren und ausliefern; den 0.3-Import mit Schema-, Datei-, Identitäts-, Ressourcen- und Pfadprüfung vor Deserialisierung bereitstellen. Fehlerursachen mit Datei/Feld in den Core-Fehlerkatalog aufnehmen; keinen stillen Legacy-Fallback zulassen. | A-C-00148 | U-C-000134 | Validator ist im nativen Windows-Build und zur Laufzeit verfügbar; gültige Referenzmodelle werden angenommen, jeder spezifizierte Negativfall wird vor Nutzung abgelehnt. |
| U-C-000132 | Aktiven Katalog und Core-Import auf `modells_data.json`/`modells_validation.json` umstellen; Semantik der Gruppen, Elemente, Darstellung, Physik und Bewegung erhalten. Aktive 0.3-Testfixtures migrieren; Legacy-Fixtures ausschließlich für getrennte Alt-Konverter-/Import-Kompatibilitätstests erhalten und kennzeichnen. Öffentliche API-Exportintegration vorbereiten; API-Implementierung erfolgt in U-C-000133. | A-C-00148, A-C-00151 | U-C-000131 | Core liefert die elf Modelle über den neuen Vertrag; aktive 0.3-Tests verwenden den neuen Bestand; Legacy-Tests sind getrennt markiert; kein doppelter Formatleser oder Legacy-Fallback im aktiven Katalog. |
| U-C-000133 | Einen verfügbaren, plattformneutralen SHA-256-Provider auswählen oder eine vorhandene Providerabhängigkeit wiederverwenden. Fünf Bereiche mit deterministischer Pfad-/Byte-Kodierung implementieren; beim Katalog-/Ladevorgang ohne Schreiben vergleichen und explizite selektive Aktualisierung mit Schema-Prüfung, atomarem Replace und Konfliktschutz ermöglichen. Öffentliche Typen und Funktion gemäß CPPI-0140 und CPPI-0142 bis CPPI-0144 exportieren; Fehlerkennungen im Core-Fehlerkatalog registrieren. | A-C-00149, A-C-00150, A-C-00151 | U-C-000131, U-C-000134 | Hash-, Vergleichs-, Teilaktualisierungs-, Idempotenz-, Fehler- und Verbraucher-Buildtests bestehen; nicht angeforderte Bereiche bleiben unverändert. |
| U-C-000135 | Core-Integration mit allen elf Modellordnern und vollständiger nativer Regression auf `Modelle-NEW` ausführen; Ergebnisse, Revision und Modellinventar als Cutover-Vorbeleg dokumentieren. | A-C-00149, A-C-00150 | U-C-000131, U-C-000132, U-C-000133, U-C-000134 | Core-Build und CTest bestehen; alle elf Modelle sind schema- und fingerprintgeprüft; Quellinventar bleibt unverändert. |
| U-C-000136 | Nach bestandener GUI-Vorprüfung einen dataset-only Cutover innerhalb der bestehenden Git-Wurzel ausführen: nur die elf Legacy-Modellordner nach `Modelle_alt` verschieben; die elf Format-0.3-Modellordner und beide Schema-Dateien aus `Modelle-NEW` in die aktive Wurzel `Modelle` verschieben; Runtime-Defaults und den gespeicherten GUI-Modellordner aktualisieren. `.git`, README, Git-Attribute/-Ignore und die Git-Wurzel bleiben an Ort und Stelle. Vorher alle Zielkollisionen ausschließen; bei Konflikt ohne Änderungen abbrechen. | A-C-00148, A-C-00152, A-C-00153 | U-C-000135 und extern: G-0.21 / U-G-000160 bestanden | Nur der neue Bestand liegt aktiv unter `Modelle`; vorheriger Modellbestand ist in `Modelle_alt` erhalten; Schema-Dateien, Pfadinventare und Konfiguration sind nachvollziehbar. Kein Teilbestand und kein Überschreiben. |
| U-C-000137 | Core-Build und vollständige CTest-Regression nach dem Cutover mit dem endgültigen aktiven Pfad wiederholen; verbleibende hartcodierte Pfade und ungewollte Schreibzugriffe ausschließen. | A-C-00153 | U-C-000136 | Alle elf Modelle laden und bestehen Schema-/Fingerprint-Prüfung; Core-Build und CTest bestehen gegen `Modelle`; `Modelle_alt` bleibt unverändert. |

## 3. Testfälle

| Test | Prüfung | Erwartung |
|---|---|---|
| C21-01 | Elf echte Modelle und alle erforderlichen Dateipaare/Schemas | Alle werden mit ihrer bisherigen Modellsemantik eingelesen. |
| C21-02 | Fehlende Pflichtdatei, Legacy-Datei neben 0.3, fehlerhaftes/mehrdeutiges JSON, Schemafehler, fehlende/ungültige Schemadatei, falscher Draft und unbekannte Formatfassung | Eindeutige Core-Diagnose; Katalog-/Inspektionsfreigabe gesperrt; vorhandene Validierungsdatei bytegleich unverändert. |
| C21-03 | Uneinheitliche `schema`-/`identity`-Werte, Modellordner-ID-Konflikt, fehlende Ressource, Pfadflucht | Core lehnt das Modell vor Geometrie-/Physiknutzung ab; Katalogeintrag bleibt gesperrt oder wird nicht angeboten. |
| C21-04 | Jeder der fünf Bereiche einzeln verändert; Dateien hinzugefügt, entfernt oder umbenannt | Nur der betroffene Bereichshash ändert sich; gleiche Eingabe bleibt plattform-/laufunabhängig identisch. |
| C21-05 | Versteckte Dateien, leere Bereiche, Symlinks und Validierungs-/Schema-Dateiänderungen | Versteckte Dateien sind enthalten, leerer Bereich deterministisch, Symlinks abgelehnt; Metadaten außerhalb des Bereichs ändern keinen Hash. |
| C21-06 | Katalog-/Ladevergleich bei veraltetem Hash | Diagnose und Nutzungsstatus ändern sich; `modells_validation.json` bleibt bytegleich unverändert. |
| C21-07 | Explizite Einzel- und Mehrbereichsaktualisierung | Nur ausgewählte Hashfelder ändern sich; `modifiedAt` ändert sich nur bei neuem Hash; Ausgabe bleibt schema-valide. |
| C21-08 | Fehler bei Eingabe, Schema, Lesen, parallelem Schreiben, atomarem Replace oder Ausgabeprüfung | Fehler wird zurückgegeben/gemeldet; bestehende Validierungsdatei bleibt unverändert. |
| C21-09 | Separater C++-Verbraucher mit Modellordner als Argument | Segelmacher-/GUI-Aufruf nutzt dieselbe Core-Funktion; kein eigener Hash- oder JSON-Schreibpfad. |
| C21-10 | Konfiguration fehlt, ist ungültig/unlesbar oder enthält relative/absolute Pfade; jede Prioritätsstufe wird überschrieben; beliebige Defaults-Schlüssel werden ergänzt und zur Laufzeit neu gelesen | Pfadauflösung folgt dem Vertrag; Fehler bleiben sichtbar; unbekannte Schlüssel werden nicht abgelehnt; Tests sind unabhängig von der Host-Konfiguration. |
| C21-11 | Vorher-/Nachher-Cutoverinventar und alle elf Modelle am finalen Pfad | `Modelle_alt`-Altbestand erhalten, neuer Bestand vollständig aktiv; keine unerwarteten Änderungen. |

Automatisierte Schreibtests verwenden ausschließlich temporäre Modellkopien.
Die Originalmodelle werden vor/nach den Tests inventarisiert und ihre
Datei-Hashes verglichen.

## 4. Build- und Laufzeitabhängigkeiten

Bestandsprüfung und Umsetzungsstand der Builddateien am 11.11.2026:

| Bereich | Vorhanden | Konsequenz für C-0.21 |
|---|---|---|
| Core-JSON | `Code/CMakeLists.txt` verlangt `nlohmann_json`; die Core-CMake-Datei liest auch den projektierten Fehlerkatalog. | JSON-Parsing kann wiederverwendet werden; es ist keine Draft-2020-12-Validierung. Neue Fehlerkennungen gehören in den Core-Fehlerkatalog. |
| JSON-Schema | Core deklariert `jsoncons` v1.10.0 über CMake FetchContent; das gehashte Quellarchiv ist header-only und wird unter Boost Software License geführt. | Draft-2020-12-Validierung ist im nativen Windows-Build eingebunden. URL und SHA-256 des Archivs sind gepinnt; Offline-/Quelloverrides bleiben über die üblichen FetchContent-Optionen möglich. |
| SHA-256 | Core deklariert `PicoSHA2` v1.0.1 über CMake FetchContent; das gehashte Quellarchiv ist header-only und MIT-lizenziert. | Der plattformneutrale Provider ist eingebunden; URL und SHA-256 des Archivs sind gepinnt. Die öffentliche Hashsemantik bleibt unabhängig vom Provider. |
| GUI | `GUI/CMakeLists.txt` bindet Core direkt ein und deklariert Qt6/VTK; `GUI/vcpkg.json` enthält Qt/VTK, aber keine Schema-/Hash-Bibliothek. | GUI verwendet ausschließlich Core. Validator- und Hashabhängigkeiten werden im Core-Build-/Verteilungspfad geführt, nicht als GUI-Doppelimplementierung. |
| Modellpfade | Der Core verwendet die gemeinsame Laufzeit-Defaults-API; `SEGELPHYSIK_MODELLE_DIR` bleibt als expliziter Testdatenpfad für Legacy-Fixtures in Core-CMake bestehen. Die GUI hält Testpfade in ihrer CMake-Konfiguration. | Laufzeitauflösung und Testwurzeln sind getrennt: aktive Format-0.3-Tests verwenden `Modelle`, Legacy-Fixtures `Modelle_alt`. Buildzeit-Testpfade sind keine produktiven Fallbacks. |

Die Provider-Versionen, Lizenzen, Quell-URLs und Archivprüfsummen sind in
`Code/CMakeLists.txt` festgehalten. Beide Abhängigkeiten sind header-only;
eine dynamische Laufzeitbibliothek muss nicht zusätzlich ausgeliefert werden.

## 5. Meilensteine und Gates

| Meilenstein | Eintritt | Abnahmekriterium |
|---|---|---|
| C1 – Validierung und Katalog | U-C-000131 und U-C-000132 | Testfälle C21-01 bis C21-03 bestehen; alle elf Modelle werden ausschließlich über Format 0.3 importiert. |
| C2 – Fingerprints und API | U-C-000133 | C21-04 bis C21-09 bestehen; Schnittstellenregister und externe Verbraucher stimmen mit dem Header überein. |
| C3 – Laufzeitkonfiguration | U-C-000134 | C21-10 besteht; aktive Modell-/Schema-Pfade haben keine Buildzeit-Abhängigkeit. |
| C4 – Vorab-Gate | U-C-000135 und GUI U-G-000160 | Abgeschlossen: Core- und GUI-Vorab-Builds sowie die relevanten vollständigen Regressionen gegen `Modelle-NEW` wurden vor dem Cutover nachgewiesen. |
| C5 – Cutover und Nachtests | U-C-000136 und U-C-000137 | Abgeschlossen: dataset-only-Cutover ausgeführt; Core-Posttests 29/29 und GUI-Posttests 62/62 gegen die endgültigen Pfade bestanden. Der dokumentierte Rückfallpfad blieb unbenötigt. |

Der Cutover ist ein nicht automatischer, einmaliger Dateisystemschritt. Die
Modelle-Repository-Wurzel `Modelle` bleibt einschließlich `.git` unverändert
am selben Pfad. Vorher werden die elf Legacy-Ordner, die elf neuen
Format-0.3-Ordner, beide Schema-Dateien, alle Zielnamen, die aktive
Defaults-Datei und der gespeicherte GUI-Modellordner geprüft und gesichert.
`Modelle_alt` nimmt ausschließlich die alten Modellordner auf. Im
tatsächlichen Stand liegt dieser Archivordner als Geschwisterordner
außerhalb der Git-Wurzel neben `Modelle`; die Testpfade zeigen auf diesen
Bestand. Die neuen Modellordner und `schema_data.json` sowie
`schema_validation.json` werden aus `Modelle-NEW` in die bestehende
Repository-Wurzel verschoben; `README.md`, `.git`, `.gitattributes`,
`.gitignore` und sonstige Repository-Metadaten werden nicht verschoben oder
überschrieben. Die nicht aktiven Zusatzdateien in `Modelle-NEW` bleiben dort.
Vor jedem Move werden alle Quellen und Ziele einzeln auf Existenz geprüft;
bei einer Kollision beginnt der Cutover nicht.

Der Rückfallpfad verschiebt die elf aktiven Format-0.3-Ordner und beide
Schemas zurück nach `Modelle-NEW`, stellt danach die elf Legacy-Ordner aus
`Modelle_alt` an ihren ursprünglichen Pfad zurück und setzt Defaults-Datei
und gespeicherten GUI-Modellordner aus der Sicherung wieder ein. Es wird
nichts gelöscht oder überschrieben; bei
jedem unerwarteten Zustand wird angehalten und die Sicherung erhalten.

## 6. Verbindliche Core-Validierung

Für den nativen C++-Build und CTest:

```powershell
cmake -S Code -B build\c021 -DBUILD_TESTING=ON
cmake --build build\c021 --config Release
ctest --test-dir build\c021 -C Release --output-on-failure
```

Zusätzlich sind die gezielten Format-, Hash- und Konfigurationstests zu
benennen und auszuführen. Für die elf Modellprüfungen wird
`SEGELPHYSIK_MODELFORMAT03_TEST_ROOT` explizit auf den aktiven Pfad
`Modelle` gesetzt. Legacy-Kompatibilitätstests verwenden
`SEGELPHYSIK_MODELLE_DIR=Modelle_alt`.
Das getrennte Buildverzeichnis vermeidet den vorhandenen `build\standard`-
Cache mit Pfaden des früheren Checkouts. Ein fehlgeschlagener Test blockiert den
Cutover-Gate; Fehler werden nicht als bekannte Baseline angenommen, ohne
einen isolierten Vergleich gegen denselben unveränderten Stand.

## 7. Umsetzungsstatus

### Vorläufige Ladephasen-Entscheidung

Auf ausdrückliche Nutzerentscheidung wird der Format-0.3-Katalog vorläufig
ohne vollständige Aerodynamikprüfung und ohne Aerodynamik-Fingerprintprüfung
geladen. Die übrigen Bereiche bleiben unverändert geprüft. Dies ist eine
Umsetzungsabweichung; die finale Core-Spezifikation wird dadurch nicht
geändert oder als finalisiert behandelt. Windfelddaten bleiben außerhalb
dieses Arbeitsschritts.

| Schritt | Tatsächlicher Zwischenstand |
|---|---|
| U-C-000134 | Laufzeit-Defaults und Vorrangregeln implementiert und getestet. Aktive `modelRoot`-/`schemaRoot`-Werte zeigen auf `Modelle`; CMake trennt den aktiven Format-0.3-Testroot von expliziten Legacy-Fixtures in `Modelle_alt`. |
| U-C-000131 | Draft-2020-12-Provider und schema-first 0.3-Import implementiert. Die 22-fallige Fehler-/Sperrmatrix besteht: fehlende Dateien/Schemas, fehlerhafte Schema-Definition und Draft-Angabe, Fehler in beiden Modelldateien, Schema-/Versions-/Identitätsfehler, Legacy-Konflikt, Ressourcenfehler und Pfadflucht. Jeder Fall prüft Core-Diagnose, Katalog-/Inspektionssperre soweit ein Katalogeintrag besteht sowie unveränderte Validierungsdatei. Materialreferenzen bleiben auf ausdrücklichen Nutzerwunsch in C-0.21 abgelehnt; keines der elf aktuellen Modelle verwendet `physics.mass.material`. |
| U-C-000132 | Neuer aktiver Katalog, Normalisierung und Inspektionsimport verwenden Format 0.3. Aktive 0.3-Tests laufen gegen `Modelle` und temporäre Kopien; Legacy-0.1/0.2-Tests verwenden ausschließlich `Modelle_alt` und bleiben als CTest-Gruppe `legacy-compatibility` erhalten. |
| U-C-000133 | Öffentliche API, SHA-256-Provider, selektives Update, Sperre und atomarer Dateiaustausch implementiert. Hash-, Verbraucher-, Fehler- und Post-Cutover-Tests bestehen. Die einmalige Aktualisierung der elf Original-Fingerprints erfolgte ausdrücklich freigegeben über die API. |
| U-C-000135 | Technischer Vorab-Lauf am 12.11.2026 grün: nativer Release-Build und vollständige CTest-Suite bestehen mit 29/29; die 0.3-Schema-, Fingerprint-, Fehlermatrix-, Source-Integration- und Katalogtests bestehen. Das `Modelle-NEW`-Inventar umfasst elf Modellordner und 153 Dateien. |
| U-C-000136 | Dataset-only-Cutover ausgeführt: Git-Wurzel samt `.git` blieb unverändert. Elf Legacy-Ordner liegen in `Modelle_alt`; elf Format-0.3-Ordner und beide Schemas liegen aktiv in `Modelle`. Defaults-Datei und gespeicherter GUI-Modellordner zeigen auf die aktive Wurzel. Die selektiven Hashes der elf `modells_data.json`-Dateien und Visualisierungsdateien blieben unverändert. |
| U-C-000137 | Nach-Cutover-Release-Build und vollständige CTest-Regression bestanden: 29/29 Tests gegen `Modelle`/`Modelle_alt`. Elf aktive Modelle bestehen Schema-/Fingerprint-Prüfung; die elf Legacy-Fixtures bleiben erhalten. |

### Bisherige Nachweise

- Nativer Release-Build in `build\c021` erfolgreich. Das getrennte
 Buildverzeichnis vermeidet den vorhandenen `build\standard`-Cache mit
 Pfaden des früheren Checkouts.
- Zuletzt erneut ausgeführt am 12.11.2026: vollständiger Release-Build und
 `ctest --test-dir build\c021 -C Release --output-on-failure`:
 **29/29 bestanden**, einschließlich `hydrodynamik_traegheit`,
 `modell_geometrie_v2` und `segelkonfiguration_contract`. Getestete
 Basisrevision: `acfb61142ab44af59cde6ee52b5d2feb86ea5fed`; die
 Arbeitskopie enthält darüber hinaus nicht committete Änderungen.
- Ein früherer vollständiger Lauf mit der erweiterten 22-falligen Matrix
  bestand mit 28/28; `modellformat_03_fehlermatrix` bestand darin mit 1/1.
  Der abschließende Post-Cutover-Lauf bestand mit 29/29.
- Gezielt bestanden: `modellformat_03_fingerprints`,
  `modellformat_03_schemabestand`, `modellformat_03_fehlermatrix` und
  `modellkatalog_schnittstelle`. Enthalten sind der Fingerprintvertrag, die
  schreibfreie Prüfung aller elf Originalmodelle auf Schema, Identität und
  Ressourcen, die 22-fallige Fehlermatrix sowie der Katalogvertrag.
- Die Inspektionsregression bestand zusätzlich; unter gleichzeitigem
 VTK-Abhängigkeitsbau dauerte der letzte Lauf 469,67 Sekunden.
 Sie ist kein Importnachweis aller elf 0.3-Modelle.
- Die zuvor offenen Fehlerbilder in `hydrodynamik_traegheit` und
 `modell_geometrie_v2` traten im aktuellen vollständigen Lauf nicht mehr
 auf. Die v2-Geometrieregression besteht jetzt ebenfalls nach dem Fix.
- Die direkte Schema-3-Migration eines 0.2-Modells wurde im GUI-Pretest
 durch ein verbliebenes Legacy-Metadatenobjekt `modell` blockiert.
 `konvertiere_modell_v3` normalisiert nun das 0.2-Dokument vor der
 Schema-3-Validierung; ein Core-Regressionstest prüft Ablehnung ohne
 Opt-in, unveränderte Quelle und erfolgreiche explizite Migration.
 `gui_model_catalog` besteht damit.
- Der neue Test verwendet temporäre Kopien und
 `SEGELPHYSIK_MODELFORMAT03_TEST_ROOT`; er prüft selektive/alle Bereiche,
 Idempotenz, versteckte Dateien, leeren Bereich, Schreibsperre,
 unveränderte Validierungsdatei bei Fehlern, ungültiges/mehrdeutiges JSON,
 Schema-/Identitäts-/Ressourcenfehler, ausgeblendete Elemente ohne
 Volumenersatz, Ansicht trotz veraltetem Physikfingerprint sowie
 Laufzeit-Defaults.
- `modellformat_03_fehlermatrix` prüft zusätzlich 22 isolierte Fehlerfälle
 durchgängig über Core-Diagnose, Katalogfreigabe und Inspektionssperre.
 CTest kennzeichnet die drei 0.3-Tests mit `model-format-0.3`; alte
 Katalog-, Geometrie-, Inspektions- und Bilanztests tragen
 `legacy-compatibility`, damit ihre weiterhin benötigten Alt-Fixtures nicht
 mit aktiven 0.3-Testdaten verwechselt werden.
- Schreibkonflikt, Schreib-/Austauschfehler und veraltete Fingerprints
 sind als `CORE-FPR-0001` bis `CORE-FPR-0003` im Fehlerkatalog dokumentiert.
 Dessen Erweiterung überschritt MSVCs Einzelliteralgrenze; die
 CMake-Einbettung verwendet deshalb byteerhaltend benachbarte
 zeilenweise Literale. Der erneute native Build bestand.
- Der Nutzer hat die einmalige Aktualisierung der elf
 `modells_validation.json` ausdrücklich freigegeben. Die Aktualisierung
 erfolgte über die Core-API. Die elf vorherigen Dateien sind im
 Session-Backup gesichert und wurden anhand `manifest.csv` auf SHA-256 und
 Dateigröße verifiziert. Andere Modellinhalte wurden nicht geändert; die
 Nutzer-Defaults und der gespeicherte GUI-Modellordner wurden beim Cutover
 auf die neue aktive Modellwurzel aktualisiert.
- Der erneute manuelle Allmodelllauf bestand nach Integration der 0.3-
 `placement`-Pose in die Segelinspektion: alle elf Modelle meldeten
 aktuellen Katalog-/Fingerprintstatus sowie verwendbare Physikimporte und
 Inspektionen. Die ursprüngliche 420-Inspektion scheiterte zuvor an den
 fest codierten Legacy-Rig-IDs; der 0.3-Pfad verwendet nun die deklarierte
 Pose, ohne den Legacy-Import zu verändern. Persistente Segelaktionen für
 0.3 bleiben mangels Formatvertrag ausdrücklich gesperrt.
- `materialien.json` ist laut Migrationslog kein freigegebener
 Laufzeitkatalog. Schema-valide `physics.mass.material`-Einträge werden
 deshalb weiterhin explizit abgelehnt. Der Nutzer hat am 11.11.2026 die
 Materialkatalog-Erweiterung für C-0.21 zurückgestellt. Eine Prüfung der elf
 `Modelle-NEW/*/modells_data.json` ergab keine `physics.mass.material`-
 Einträge; der Entwurfskatalog enthält ausdrücklich unkalibrierte
 Beispielwerte und bleibt nicht freigegeben. Damit wird kein aktuelles
 Referenzmodell blockiert. Der Format-0.3-Katalog verwendet keinen
 Legacy-Fallback; Legacy-Importe bleiben getrennte, explizite
 Kompatibilitätstests.
- Der GUI-Pretest verglich `Modelle-NEW` vor dem Cutover:
 **153 Dateien, SHA-256-Inventar unverändert**. Die CMake-Testroots waren
 getrennt: Format 0.3 aus `Modelle-NEW`, Legacy aus `Modelle`.
- Der Post-Cutover-Corelauf in `build\c021` bestand mit **29/29**; die
 GUI-Release-Regressionssuite in `build\g021` bestand mit **62/62**.
 Aktive Tests zeigen auf `Modelle`, Legacy-Kompatibilitätstests auf
 `Modelle_alt`.
- Vor und nach den Post-Cutover-Tests blieben SHA-256-Werte für genau 45
 Dateien unverändert: die elf `modells_data.json`-Dateien und Dateien in
 den Visualisierungsordnern. Andere Modellbereiche wurden nicht gehasht.
- Die ursprüngliche Modelle-Git-Wurzel und `.git` blieben am bisherigen
 Pfad. Die nicht aktiven `materialien.json`- und `MIGRATIONSLOG.md`-Dateien
 blieben in `Modelle-NEW`. Keine Git-Veröffentlichung erfolgte.
