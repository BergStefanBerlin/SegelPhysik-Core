# Schnittstellenbeschreibung / Ergebnisvertrag Core → GUI

## C-0.8: Modellinspektion – technisch in GUI integriert, Nutzerabnahme offen

Der finale [Inspektionsvertrag, Abschnitt 3.7](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#37-normativer-daten--und-api-vertrag)
definiert A-C-00098 bis A-C-00104 und CPPI-0032 bis CPPI-0059. Die am
02.11.2026 finalisierte Erweiterung in
[Abschnitt 4](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#4-c-08-erweiterung-objektzentrierte-gesamtinspektion)
definiert A-C-00112/00113 und CPPI-0074 bis CPPI-0078.
Er ist die maßgebliche Quelle für Aufrufe, Schema und Fehler; hier wird
kein paralleler Vertrag geführt.

`segelphysik/inspektion.hpp` stellt `readModelInspection` und die
Baum-, Property-, Mesh- und JSON-Abfragen bereit. Der Snapshot ist
unveränderlich; Detailabfragen lesen keine Dateien erneut und bleiben nach
Dateiänderungen gültig. `CORE-INSP-0001` bis `CORE-INSP-0004` sind im
Runtime-Fehlerkatalog dokumentiert. JSON-Antworten verwenden
`inspektionsversion: "1"`; Katalog- und Simulationsergebnisversionen sind
unverändert.

Für den GUI-Ladeworker wird `readModelInspection` einmal aufgerufen.
Darstellung und kompatible Altprojektionen kommen danach aus genau diesem
Snapshot:

```cpp
const auto inspection = readModelInspection(catalogue, model_id, model_version);
const auto structure = modellstruktur(*inspection);
const auto display = darstellungsgeometrie(*inspection);
```

Diese Überladungen lesen keine Quelldateien. Gruppen stehen als
`kind == "gruppe"` im Baum `modell`; ihre `element_ids` verweisen auf
Elemente. Die Gesamtmasse ist die Property `@gesamtmasse_kg` (kg), die
Darstellungshülle `@huellquader_m` (Objekt `min`/`max`, m), jeweils an der
Wurzel `""`. Bewegungsdetails einschließlich abgeleitetem `@hubweg_m`
stehen im Baum `bewegung` am Knoten
`/gruppen/<RFC6901-Gruppe>/elemente/<RFC6901-Element>/Bewegung`.
Katalog-/Versionsprüfung, Import, Meshleser und kompatible
Katalog-/Struktur-/Geometrieeinstiege verwenden gemeinsame Core-Bausteine;
die alten Signaturen, Antwortformen und Lebenszyklen bleiben erhalten.
Die technische GUI-Integration verwendet diese Snapshotprojektionen im
Ladeworker. Verifiziert wurden GUI-Release-Builds in
`C:\spdeps\gui-workspace-tabs` und `C:\spdeps\guibuild\Release` sowie
35/35 integrierte CTest-Fälle mit Ausschluss `^gui_graphics_render_`
(19 Core- und 16 GUI-Tests), einschließlich Inspektions- und Workspace-
DPI-Tests bei 100, 150 und 200 Prozent. Der ausgeschlossene,
bekanntermaßen bereits zuvor fehlschlagende `gui_graphics_render_100`-
Test meldet weiterhin, dass bei Modell 420 das Deaktivieren der Kurslinie
das exakte Bild nicht wiederherstellt. Die persönliche fachliche
Nutzerabnahme bleibt offen.

## C-0.8-Erweiterung 02.11.2026: Objektbaum und Bewegungs-Vorschau

Für die gemeinsame Objektansicht nutzt die GUI den kanonischen Baumtyp
`objekte`. Er enthält nur Modell-, Baugruppen- und Elementknoten; Metadaten,
`geometrie`-Quellen und Fachbereiche bleiben Eigenschaften, keine
Pseudoknoten. Alle Properties kommen aus demselben Snapshot und besitzen
das additive Feld `area` (`Model`, `Geometrie`, `Darstellung`, `Masse`,
`Material`, `Hydrostatik`, `Hydrodynamik` oder `Bewegung`).

```cpp
const auto objectTree = getObjectTree(*inspection, "objekte");
const auto properties = getObjectProperties(*inspection, "objekte", selectedIds, true);
```

`modellstruktur(*inspection).gruppen[].elemente[].bewegung` bleibt die
kompatible typisierte Bewegungsdefinition. Für eine Vorschau ruft die GUI
`getMovementPreview(*inspection, controllerElementId)` auf. Der Rückgabewert
enthält modellfeste Achs-/Bezugswerte, Referenzstellung und die
Core-transformierten Meshes der Normal- sowie verfügbaren Grenzstellungen.
Werte in Prozent verwenden die bestehenden 0–100-Steuerwerte; eine
Winkeldrehung mit `ruderwinkel` besitzt mangels Modellgrenzen keine
erfundenen Minimum-/Maximum-Posen. Fehlende Vorschaugeometrie wird als
Fehler gemeldet, nicht durch eine zweite GUI-Bewegungsimplementierung
ersetzt.

## C-0.5: Temporärer Testantrieb, Ergebnisversion 3

Technisch implementiert am 27.10.2026 nach ausdrücklichem Auftrag, keine
Nutzerabnahme oder Veröffentlichung. Maßgeblich sind A-C-00052 bis A-C-00060.
Die bisherigen V2-Aufrufe bleiben unverändert. Kein neuer CLI-Schalter.

```cpp
TestantriebKonfiguration c; // kraft_n=200.; grenze_m_s=463./90.
Zustand s = testantrieb_start(modell, start, c, false);
s = dynamikschritt(modell, s, dt,
    std::vector<TestantriebEreignis>{{1, true}, {2, false}, {3, true}});
auto json = zustand_json(modell, s, "laufend"); // vertragsversion "3"
```

Alle Typen/Funktionen stehen in `segelphysik/hydrostatik.hpp`, die Ausgabe
in `simulation.hpp`. Der bestehende Dynamikaufruf ohne Ereignisliste übernimmt
den im Zustand gespeicherten Testkontext ebenfalls. Ein Zustand ohne
Testkontext liefert weiterhin V2; `testantrieb_start` liefert auch bei
inaktivem Flag V3. Konfiguration wird für einen Lauf festgehalten und nicht
im Modell gespeichert. `Laufoptionen::testantrieb` ist optional;
`testantrieb_aktiv` setzt das Startflag. `testantrieb_ereignisse` enthält
Paare `(Quellschritt, TestantriebEreignis)`, geordnet nach Grenze und Kennung.
Batch und Einzelschritt verwenden denselben Integrator.

`Zustand::testantrieb` ist `shared_ptr<const TestantriebKontext>`.
Zustandskopien behalten die unveränderliche Historie. `testantrieb_erforderlich`
markiert Testzustände: Verlust des erforderlichen Zeigers ist ein Fehler.
Das Feld nicht manuell löschen und Kontexte nicht zwischen Zuständen oder
Modellfassungen transplantieren. `auswertung_schritt`/`auswertung_zeit_s`
werden geprüft. Der CORE besitzt keine globale Livevariable.

Der GUI-Worker übernimmt vor dem nächsten Schritt atomar die komplette
ausstehende Ereignisliste unter seinem Mutex, gibt den Mutex frei und
übergibt die Liste als festen Wert. Währenddessen ankommende Ereignisse
bleiben bis zur nächsten Grenze im Worker. `TestantriebEreignis` hat
`size_t kennung` (streng aufsteigend ab 1) und `bool aktiv`; letztes Ereignis
bestimmt das Flag für sämtliche Auswertungen des Schritts. Erst der
erfolgreich zurückgegebene Zustand bestätigt Anwendung unter
`testantrieb->uebergang->ereignisse`. Übergang: `von_schritt`,
`nach_schritt`, `startzeit_s`, `dt_s`. Bei Fehler/Abbruch/Laufende keine
unberechneten Ereignisse bestätigen; alte Zustände bleiben gültig.
Der CORE implementiert keinen Transport und keine GUI-Threadsteuerung.

### Zusätzlicher JSON-Pflichtblock `testantrieb`

| Feld | Inhalt |
|---|---|
| `modus` | `temporaerer_bugtest` |
| `konfiguration` | `kraft_n`, `grenze_m_s` in SI |
| `aktiv` | historisches Auswertungsflag; auch Flag des erzeugenden Übergangs |
| `geometriequelle`, `modell_fassung` | physische Massen-/Volumennetze einschließlich Punktmassen; Inhaltsfingerprint des Imports |
| `zugpunkt_modell_m` | deduplizierter Max-X-Mittelpunkt; `null`, wenn nicht vorhanden/nicht benötigt |
| `schwerpunkt_modell_m` | Schwerpunkt des Gesamtmodells in m |
| `schub_welt_n` | horizontale reale Kraft, inaktiv/Nullkraft `[0,0,0]` |
| `auswertung_schritt`, `auswertung_zeit_s`, `letzte_ereigniskennung` | gespeicherter Zustands-/Ereignisbezug |
| `uebergang` | am Start `null`, sonst Objekt mit Grenze, Zeit, `aktiv`, geordneten `ereignisse` und `begrenzung` |

`begrenzung` enthält `wirksam`, `vorher_welt_m_s`, `nachher_welt_m_s`,
`korrekturimpuls_ns` und `energieaenderung_j`. Ohne Eingriff sind v vor/nach
identisch und Δp/ΔE null. Dieser künstliche Impuls gehört nicht zur
kontinuierlichen Kraftsumme. Er verändert weder Pose noch Z- oder
Winkelgeschwindigkeit des regulären Schritts; aktiv bei 0 N begrenzt weiter.
Am Start gibt es weder erzeugenden Übergang noch Begrenzung.

Beitrag `testantrieb.bug.kraft`, Quelle/Gruppe `testantrieb`, Rolle
`einzelbeitrag`, Frame `modell`, leere `element_id` für das Gesamtmodell:
reale Kraft am Zugpunkt, `moment_s_nm` enthält genau einmal den Hebelarm.
`gesamt:moment` bleibt eine nicht nochmals summierte Gesamtsumme.
Inaktiv: `zustand: deaktiviert`; aktiv mit 0 N: bekannter `null`-Beitrag.
Fehlt bei Nullkraft die Geometrie, ist auch dessen `punkt_m: null`;
kein erfundener Schwerpunkt-Ersatz. Der erwartete Bilanzumfang umfasst den
Testbeitrag, nicht den Korrekturimpuls. `bilanzstatus` erklärt ausschließlich
die kontinuierlichen Beiträge, nicht Impulsfreiheit.

Eingabe-/Kontext-/Richtungsfehler und aktive Statik verwenden den bestehenden
Katalogeintrag `CORE-SIM-0001` mit konkreter Diagnose, Dynamikträgheit weiter
`CORE-SIM-0002`. Fehler veröffentlichen keinen halben Schritt.
Geometrie: tatsächliche Importpunkte aus `Element::massenpunkte_m` und
Volumennetzen; lexikografisch sortierte Max-X-Kandidaten, euklidische
Deduplikation mit `max(1e-9 m,1e-9*L)`, arithmetisches Mittel.
Keine Darstellungs-OBJ-Abhängigkeit, kein Wasserlinienkriterium.

Nachweis: CTest `temporaerer_testantrieb`, bestehende Import-/Hydrostatik-/
Bilanz-/Simulations-/CLI-Tests; reproduzierbare vierzeilige 420er-Aufzeichnung
`build\c05\testantrieb-420-referenz.jsonl` entsteht bei Ausführung des
Testziels. Sie enthält unveränderten Start mit 6 m/s, aktive Begrenzung,
Ausschalten und erneutes Einschalten. Keine grafische Abnahme.

## C-0.7: Dynamische Ruder-/Schwertbewegung, Ergebnisversion 4

Eine Elementdefinition kann optional `Bewegung` enthalten:
`art: "drehen"` mit `achse_modell` und `drehpunkt_modell_m` oder
`art: "heben"` mit `richtung_modell`. `mitbewegte_elemente` nennt
zusätzliche Darstellungs-Elemente. Achsen werden beim Import normiert;
Hubwege werden aus dem bewegten Netz und der unteren Rumpfreferenz
abgeleitet. Unbewegliche Bestandsmodelle bleiben unverändert gültig.

Für Modelle mit mindestens einer Bewegungsdefinition liefert
`zustand_json` Ergebnisversion `"4"` und ergänzt:

| Feld | Inhalt |
|---|---|
| `bewegungen` | Bestätigter Winkel in Grad für Ruderlagerung bzw. Stellwert in Prozent für Schwerter |
| `letzte_bewegungsereigniskennung` | Zuletzt übernommene, streng aufsteigende Stellereigniskennung |
| `bewegungsuebergang` | `null` am Laufstart, sonst Schritt, Zeit, `dt_s` und übernommene Stellereignisse |
| `elementtransformationen_modell` | Pro Modellelement eine zeilenweise 4×4-Matrix, die lokale Modellpunkte für die Darstellung transformiert |

Stellereignisse werden vor sämtlichen Kraftauswertungen des nächsten
Zeitschritts übernommen. Der Core wendet sie auf alle Integratorstufen
an. Die verstellten Volumennetze wirken ausschließlich auf
Hydrodynamikbeiträge; Hydrostatik, Masse, Schwerpunkt und Trägheit
verwenden unveränderte Modelldaten. Live- und Batchpfad teilen denselben
Integrator und dasselbe Ereignismodell. Bei gleichzeitiger Nutzung des
Testantriebs bleibt dessen Kontext optional in Version 4 enthalten.

Version 4 ist für das Core- und GUI-Bewegungspaket C-0.7/G-0.7 bestimmt;
das unterstützt weder Hub-Kiele noch Stellgeschwindigkeit,
Betätigungskräfte oder Kollisionen.

## Status und Geltungsbereich

### Exakte V3-Form des Zusatzblocks (Schrittzustand)

Die folgenden Zahlen illustrieren die Feldtypen, keine zusätzliche
Referenzsimulation. Sämtliche genannten Felder werden ausgegeben:

```json
{
  "testantrieb": {
    "modus": "temporaerer_bugtest",
    "konfiguration": {"kraft_n": 200, "grenze_m_s": 5.144444444444445},
    "aktiv": true,
    "geometriequelle": "physische_massen_volumennetze_punktmassen",
    "modell_fassung": "Importfingerprint",
    "zugpunkt_modell_m": [2, 0, 1],
    "schwerpunkt_modell_m": [0, 0, 0],
    "schub_welt_n": [200, 0, 0],
    "auswertung_schritt": 1,
    "auswertung_zeit_s": 0.01,
    "letzte_ereigniskennung": 1,
    "uebergang": {
      "von_schritt": 0,
      "nach_schritt": 1,
      "startzeit_s": 0,
      "dt_s": 0.01,
      "aktiv": true,
      "ereignisse": [{"kennung": 1, "aktiv": true}],
      "begrenzung": {
        "wirksam": false,
        "vorher_welt_m_s": [0.2, 0, -0.0981],
        "nachher_welt_m_s": [0.2, 0, -0.0981],
        "korrekturimpuls_ns": [0, 0, 0],
        "energieaenderung_j": 0
      }
    }
  }
}
```

Am Start ist `uebergang: null`; es gibt dort keinen separaten
`begrenzung`-Block. `zugpunkt_modell_m` darf bei nicht benötigter,
fehlender Geometrie `null` sein. Ohne Ereignisse: `ereignisse: []`.
Die Ereigniskennung bleibt über Schritte streng monoton.
`letzte_ereigniskennung: 0` bedeutet noch kein angewandtes Ereignis.
V3 bleibt auch mit `aktiv: false` erhalten.

Stand: C-0.2 in Arbeit. Technische Dokumentation zu A-C-00016, A-C-00028,
A-C-00031 und A-C-00032 der
[Hydrostatikspezifikation](../Spezifikationen/Spezifikation_Hydrostatikkern.md)
sowie zu den Umsetzungen der
[Kraft- und Momentenbilanz](../Spezifikationen/Spezifikation_Kraft_und_Momentenbilanz.md)
(Ergebnisvertrag Version 2, Abschnitt 3) und der
[Modellkatalog-Schnittstelle](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md)
(Abschnitt 4). Keine weitere fachliche Spezifikation.

**Tatsächlich verfügbar:** Bibliotheks-/CLI-Abfrage des Fehlerkatalogs,
CLI-Buildinformation und CLI-Fehler mit eindeutigen IDs sowie
Pose-Transformationen, Modellimport/-prüfung, statischer Solver,
Starrkörperdynamik, versionierte Ergebnisaufzeichnungen mit
Beitragsklassifikation, Summen und Bilanzstatus (Version 2) sowie
der Modellkatalog als Bibliotheksschnittstelle.
**Noch nicht verfügbar:** Live-Transport (WebSocket o. Ä.) und
CLI-Befehle für den Modellkatalog.
Technischer Sicherungsstand: **C-0.1** plus uncommittete C-0.2-Umsetzung,
kein GitHub-Release und keine vorbehaltlose fachliche Gesamtabnahme.

## 1. Anfrage: „gib mir Fehlerdoku“

| Eigenschaft | Vertrag des implementierten Abfragewegs |
|---|---|
| Fachlicher Befehlsname | `gib mir Fehlerdoku` |
| CLI-Aufruf | `segelphysik_cli.exe --fehlerdoku` |
| Bibliotheksfunktion | `segelphysik::fehlerdokumentation_json()` |
| Antwort | Vollständiges UTF-8-JSON-Dokument des Fehlerkatalogs |
| Modell erforderlich | Nein |
| Erfolg | CLI-Exitcode 0, JSON auf stdout, stderr leer |
| Live-Nachrichtenschema | Noch nicht festgelegt oder implementiert |

Der fachliche Befehlsname ist kein aktuell akzeptiertes wörtliches
CLI-Argument und noch kein WebSocket-Nachrichtentyp. Der dokumentierte
CLI-Schalter bildet diese Anfrage bis zur späteren Transportintegration ab.
Eine GUI kann derzeit einen CLI-Unterprozess aufrufen und dessen stdout
als UTF-8-JSON lesen. Die Core-Bibliothek liefert eine unveränderliche
Zeichenansicht mit statischer Lebensdauer.

Der [maßgebliche JSON-Katalog](../Code/ressourcen/fehlerkatalog.json)
wird beim Build validiert und eingebettet. Die Abfrage liefert exakt
diesen Inhalt für den jeweiligen Build; am Installationsort wird keine
separate Datei und kein Arbeitsverzeichnis vorausgesetzt.
Nach einer Katalogänderung muss der Core neu gebaut werden.
Die GUI soll den Katalog der tatsächlich verwendeten Core-Instanz
abfragen und keine unabhängig gepflegte Kopie verwenden.

### Antwortfelder, Vertragsversion 1

| Feld | Typ | Bedeutung |
|---|---|---|
| `vertragsversion` | String, Pflicht | Aktuell `"1"` |
| `umfang` | String, Pflicht | Geltungsbereich und Implementierungshinweis |
| `fehler` | Array von Objekten, Pflicht | Bekannte Laufzeitdiagnosen dieses Core-Stands |
| `fehler[].id` | String, Pflicht | Eindeutige dauerhafte Fehlerkennung |
| `fehler[].kurztext` | String, Pflicht | Menschenverständliche Zusammenfassung |
| `fehler[].pass_kriterium` | String, Pflicht | Beschreibt die erfüllte Prüfbedingung |
| `fehler[].fail_kriterium` | String, Pflicht | Beschreibt die Fehlerbedingung |
| `fehler[].handlungsanweisung` | String, Pflicht in Version 1 | Korrekturhinweis; falls nicht möglich, ausdrücklich begründen |
| `fehler[].pruefstatus` | String, Pflicht | `"implementiert"` oder `"geplant"` |

Es gibt aktuell 25 Katalogeinträge: zwei CLI-, drei Pose-, acht
Modell-, vier Simulations-, eine Ausgabe-, eine Bilanz- (`CORE-BIL-0001`)
und sechs Modellkatalogdiagnosen (`CORE-KAT-0001` bis `-0006`).
Prüfstatus beschreibt die Verfügbarkeit der Prüfung, nicht das
Prüfergebnis eines konkreten Modells. Alle aktuellen Einträge
bezeichnen implementierte Prüfungen innerhalb der unten dokumentierten
Grenzen, keinen allgemeinen Nachweis physikalischer Modelleignung.

Die GUI soll Fehler über `id` zuordnen und den Text nicht als Schlüssel
verwenden. Unbekannte IDs müssen mit der mitgelieferten konkreten
Diagnose angezeigt werden können; sie dürfen nicht still verschwinden.
IDs werden nicht umgewidmet. Neue Einträge sind additiv möglich.
Inkompatible Änderungen an Pflichtfeldern oder ihrer Bedeutung
benötigen eine neue Vertragsversion. Konkrete geometrische Prüfkriterien
und zugehörige Detaildiagnosen werden mit der Implementierung nachgeführt.

## 2. Aktuell implementierte CLI-Fehler

| Fall | Exitcode | Fehler-ID |
|---|---:|---|
| Für den jeweiligen Befehl falsche Argumentzahl | 2 | `CORE-CLI-0001` |
| Unbekanntes einzelnes Argument | 2 | `CORE-CLI-0002` |

Fehler erscheinen derzeit als menschenlesbarer Text auf stderr,
beginnend mit `Fehler: <ID>:`. stdout bleibt leer.
Bei Simulationsläufen steht zusätzlich der Laufbericht auf stdout.
Ein fehlgeschlagener Lauf hat Exitcode 1 und eine Diagnose auf stderr;
eine eventuell vorhandene Teilausgabe darf nicht als erfolgreicher
Lauf behandelt werden. Ctrl+C fordert einen kontrollierten Abbruch an.

### Modellprüfung

`segelphysik_cli.exe --pruefen MODELL` gibt ein UTF-8-JSON-Dokument aus.
Exitcode 0 bedeutet strukturell/numerisch verwendbar nach den aktuellen
Prüfregeln; Exitcode 1 bedeutet abgelehnt. Eine physikalische
Modellfreigabe wird dadurch nicht erteilt.

Erfolg: `vertragsversion`, `gueltig: true`, `modell_id`,
`modell_fassung` und leeres `fehler`-Array.
Fehler: `vertragsversion`, `gueltig: false` und ein `fehler`-Array
mit `id`, `datei` und `details`. `details` enthält den tatsächlich
verfügbaren Datei-/Feld-/Elementkontext des Importers.
Es wird derzeit der erste Fehler zurückgegeben, nicht eine behauptete
vollständige Fehlerliste. Die GUI kann den Kurztext, Kriterien und
Korrekturhinweis über die ID aus dem Katalog beziehen.

Unterstützt wird `schema_version: "0.1-entwurf"` mit dem bestätigten
Beispielaufbau und OBJ-Dreiecken. Polygonflächen werden ausdrücklich
abgelehnt, nicht ungeprüft trianguliert. Punktmassen, homogene
Vollkörper und dünne Materialflächen werden unterstützt.
Schalen verwenden Dichte × Wanddicke × Materialfläche; es gibt weder
automatische Doppelwandkorrektur noch eine Dicken-Trägheitsintegration.
Entwurfsmetadaten werden nicht als physikalische Freigabe interpretiert.

Geschlossene Volumennetze werden auf Indizes, Entartung, Kanten,
Orientierung je Komponente und geometrische Selbstschnitte geprüft.
Toleranzbasierte Prädikate sind kein exakter Boolean-Kern.
Positive Innenüberdeckung der AABBs zweier Verdrängungsnetze wird
konservativ abgelehnt; achsenparalleler Randkontakt ist erlaubt.
Damit können auch disjunkte Netze bzw. schräge Randkontakte abgelehnt
werden. `CORE-MOD-0007` behauptet ausdrücklich keinen bewiesenen
tatsächlichen Volumenschnitt. Eine Vereinigung ist nicht implementiert.

## 3. Implementierte Simulationsergebnisse

Der vereinbarte fachliche Ergebnisumfang:

- Modellidentität und -fassung, Vertragsversion und Berechnungsstatus.
- Position des festen Modellursprungs in Weltkoordinaten, in Metern.
- Orientierung vom Modell- ins Weltsystem als Quaternion.
- Zusammengehörige Vektorliste im Modellkoordinatensystem:
  stabile Beitrags-ID, Element-/Gesamtmodellbezug, Größenart/Einheit,
  Ansatz-/Bezugspunkt und drei physikalische Vektorkomponenten.
- Simulationszeit, bei Dynamik zusätzlich Schrittnummer und
  dokumentierte Geschwindigkeiten.
- Eindeutige Fehlerausgaben mit Katalog-IDs statt Erfolgsergebnissen.

Ansatzpunkte werden rotiert und verschoben, Vektoren nur rotiert.
Physikalische Größen verwenden SI-Einheiten; die GUI bestimmt
Darstellungsmaßstäbe. Die Quaternion-Komponentenreihenfolge ist **xyzw**,
Identität `[0, 0, 0, 1]`, Rotation vom Modell- ins Weltsystem.
Die folgenden Felder beschreiben den implementierten Vertrag.

| Feld | Typ / Einheit / Bedeutung |
|---|---|
| `vertragsversion` | String `"2"` (seit C-0.2), `"3"` mit Testantrieb oder `"4"` mit Bewegungsdaten |
| `modell_id`, `modell_fassung` | Strings; Fassung ist ein FNV-1a-Inhaltsfingerprint des Imports, keine Sicherheitsprüfung |
| `status`, `diagnose` | Strings; leere Diagnose bei regulärem Erfolg |
| `zeit_s`, `schritt` | Simulationszeit in s, nicht negative laufende Ganzzahl |
| `position_m` | Drei Zahlen: fester Modellursprung in der Welt |
| `orientierung_xyzw` | Vier Zahlen: normiertes Quaternion Modell nach Welt |
| `geschwindigkeit_welt_m_s` | Drei Zahlen: Geschwindigkeit des Gesamtschwerpunkts im Weltframe |
| `omega_modell_rad_s` | Drei Zahlen: Winkelgeschwindigkeit im Modellframe |
| `volumen_m3` | Gesamte eingetauchte Verdrängung |
| `geflutete_elemente` | Array der Element-IDs, die dauerhaft vollgelaufen sind (A-C-00047, seit C-0.2 additiv); leer, wenn nichts geflutet ist |
| `vektoren` | Array der nachfolgend beschriebenen Beiträge |
| `elementvolumina` | Array mit Element-ID, `volumen_m3`, `geflutet` (bool, A-C-00047), `auftriebspunkt_modell_m` |
| `moment_modell_nm` | Gesamtes Moment im Modellframe |
| `moment_bezug_modell_m` | Gesamtschwerpunkt im Modellframe |

### Ergebnisvertragsversion 5: stationäres Fahrtgleichgewicht

Ein stationärer Lauf wird über `Laufoptionen::stationaer` aktiviert, also
durch Belegung des optionalen Konfigurationsfelds. `aktiv` schaltet nur die
konfigurierte Fremdkraft ein oder aus; `aktiv: false` ist ein gültiger
stationärer Nullastfall. Die Konfiguration enthält `modellfest`,
`kraft_welt_n` (Komponenten im gewählten Bezugsrahmen),
`angriffspunkt_modell_m` und feste `stellungen` als
`Bewegungsereignis`-Werte (`element_id`, `wert`). `modellfest: true`
interpretiert den Kraftvektor im Modellrahmen; andernfalls im Weltrahmen.
Der Angriffspunkt bleibt in beiden Fällen modellfest.
Prozentstellungen sind auf 0–100 begrenzt. Für Ruderrichtungen enthält das
Modell derzeit keine deklarierte physische Winkelgrenze; stationäre
Konfigurationen akzeptieren daher endliche Werte bis zur technischen Grenze
±1e6 Grad, ohne diese als physikalisch zulässigen Bereich auszugeben.

Der übliche Zustandsdatensatz behält Pose, Geschwindigkeit, Beiträge,
Summen und Modellfassung. Ergänzend enthält `stationaer`:

| Feld | Bedeutung |
|---|---|
| `status`, `diagnose`, `konvergiert` | Expliziter Solverstatus; nur `konvergiert` besagt, dass alle sechs Residuen innerhalb der Toleranzen liegen |
| `modellfest`, `kraft_n`, `kraft_bezugssystem`, `angriffspunkt_modell_m`, `stellungen` | Eingabesnapshot; ungültige nichtendliche Kraft-/Punktwerte werden als `null` ausgegeben |
| `iterationen`, `flutungswiederholungen` | Solveriterationen und automatische Neuberechnungen nach neu erkannter Flutung |
| `residuum_kraft_welt_n`, `residuum_moment_modell_nm` | Drei ungerundete Kraft- und Momentkomponenten |
| `krafttoleranz_n`, `momenttoleranz_nm` | Angewandte Komponenten-Toleranzen, höchstens 0,1 N bzw. 0,1 Nm |
| `referenzkurs_verwendet` | Wahr, wenn eine numerisch erkannte yaw-Invarianz den Startkurs als Referenz übernimmt |
| `geschwindigkeit_relativ_wasser_m_s`, `geschwindigkeit_relativ_wasser_betrag_m_s`, `abdriftwinkel_rad` | Relativgeschwindigkeit und daraus berechnete Abdrift |
| `nullen` | Explizit festgehaltene `vz_m_s: 0` und `omega_modell_rad_s: [0,0,0]` |

Die Ergebnisvertragsversion ist `"5"` ausschließlich bei diesem Laufpfad;
Versionen 1–4 und ihre Felder bleiben für bestehende Läufe unverändert.
Das Feld `stationaer` bleibt auch bei Abbruch und Solverfehler vorhanden.
Mögliche Solverstatus sind `konvergiert`, `nicht_konvergiert`,
`rangdefizit`, `suchgrenze`, `abgebrochen` und `ungueltig`. Erkannte
Flutungen werden in `geflutet` und `flutungswiederholungen` festgehalten;
`flutung` ist kein Konvergenzstatus.
Hydrostatik, stellungsabhängige Masse/Schwerpunkt/Benetzung und die
vorhandenen stationär wirksamen Hydrodynamikbeiträge werden gemeinsam
ausgewertet; es wird kein Zeitintegrator und kein Testantriebslimit benutzt.
Eine erfolgreiche Lösung ist weder ein Stabilitäts- noch ein
Eindeutigkeitsnachweis.

Der gemeinsam wiederverwendbare analytische GUI-/Core-Testfall liegt unter
`Code/tests/testdata/stationaere_referenz/modell.json` (Mesh `fin.obj` im
selben Ordner). Mit 1500 N Weltkraft in +X am Modellpunkt `[0,0,-0.4]`,
ruhigem Wasser mit `rho=1000 kg/m³`, `nu=1e-6 m²/s`, `cd=1`, `cl=0` und
einer benetzten Seitenfläche von 2,4 m² gilt unabhängig
`c=0,5*rho*cd*A=1200 N/(m/s)²` sowie
`v=sqrt(F/c)=1,118033989 m/s`. Der Test erwartet höchstens 0,1 m/s
Abweichung und prüft separat alle sechs Core-Residuen gegen 0,05 N/Nm.
Erzeugung und Nachweis: `Code/tests/simulation_tests.cpp`, Test
`simulation_gesamtlauf`.

Alle Felder sind vorhanden. Ein nicht existierender Auftriebspunkt wird
als JSON-`null` ausgegeben, nicht als scheinbar gültiger Ursprung.
Auch ein Element ohne Volumen wird mit exakt null Verdrängung geführt.
Einheiten sind SI; Zeit ist nicht die Rechenlaufzeit.

**Volllaufen (A-C-00047):** Ein Element mit offenem Schalen-Massennetz
und Verdrängungsvolumen läuft voll, sobald ein Punkt seines Bordrands
(Kanten mit nur einer Fläche) unter der Wasserebene liegt. Es bleibt bis
Laufende geflutet, behält sein Gewicht und liefert Auftrieb null
(`zustand: null`, Bezugspunkt). Die Statik wiederholt die Suche nach
neuer Flutung und nennt geflutete Elemente in der Diagnose bei
`unloesbar`. Ein Lauf kann über `Laufoptionen::start` (Bibliothek) von
einem vorhandenen Zustand inklusive Flutung starten; die Statik verwendet
davon nur Pose und Flutung als Startschätzung.

Vektorbeiträge enthalten `id`, `element_id`, `art`, `einheit`,
`frame: "modell"`, `punkt_m` und `vektor`.
`art: "kraft"` verwendet N; `punkt_m` ist der Angriffspunkt.
`art: "moment"` verwendet N·m; `punkt_m` ist der Momentenbezug.
`gesamt:moment` verwendet die leere Element-ID als ausdrücklich
reservierten Gesamtmodellbezug. IDs `<Element-ID>:gewicht` und
`<Element-ID>:auftrieb` identifizieren Elementkräfte.
Richtung und Länge werden aus den drei Vektorkomponenten gewonnen;
die GUI wählt den grafischen Maßstab. Nullvektoren haben keine Richtung.

### Erweiterungen der Vertragsversion 2

Version 2 ist additiv: Alle Felder der Version 1 bleiben mit gleicher
Bedeutung erhalten. Ein Leser der Version 1 kann unbekannte Felder
ignorieren. Zusätzlich gilt:

| Feld je Vektor | Bedeutung |
|---|---|
| `quelle` | `schwerkraft`, `hydrostatik`, `summe` oder `nicht_zugeordnet` |
| `gruppe` | Darstellungs-/Summengruppe, derzeit `gewicht_auftrieb` bzw. `gesamt` |
| `rolle` | `einzelbeitrag`, `gruppensumme` oder `gesamtsumme` |
| `punktart` | `angriffspunkt` oder `bezugspunkt` |
| `zustand` | `gueltig`, `null`, `deaktiviert`, `ungueltig`, `nicht_berechnet` |
| `fehler_id` | `null` oder Fehler-ID (z. B. `CORE-BIL-0001`) |
| `moment_s_nm` | Moment des Beitrags um den Gesamtschwerpunkt im Modellframe |

Bei `ungueltig` und `nicht_berechnet` sind `punkt_m`, `vektor` und
`moment_s_nm` JSON-`null`. Ein Element mit Volumen, das trocken ist,
liefert einen `:auftrieb`-Beitrag mit `zustand: "null"`, Nullvektor und
dem Elementschwerpunkt als `bezugspunkt` – fehlende Einträge sind damit
nicht mehr mit „kein Auftrieb“ verwechselbar.

| Feld oberste Ebene | Bedeutung |
|---|---|
| `beitraege_auswertung` | `"gelieferter_zustand"`: Beiträge gelten für Pose/Zeit des Zustands |
| `summen.gesamt` | `gruppe`, `frame`, `kraft_modell_n`, `moment_s_nm`, `quellen[]` (summierte Beitrags-IDs) |
| `summen.gruppen[]` | Gleiche Felder je Gruppe |
| `bilanzstatus` | `vollstaendig`, `teilsumme` oder `ungeklaert` |
| `erwartete_beitraege`, `fehlende_beitraege` | Beitrags-IDs je Element (`:gewicht`, `:auftrieb` bei Volumen) |
| `gleichgewicht` | Bei Statik Objekt mit `krafttoleranz_n`, `momenttoleranz_nm`, `residuum_kraft_n`, `residuum_moment_nm`, `freiheitsgrade[]`, `iterationen`; sonst `null` |

Summiert werden nur Einzelbeiträge mit `gueltig` oder `null`.
Residuen sind die tatsächlich an der gelieferten Pose berechneten
Weltgrößen `|F_z|` bzw. `max(|M_x|, |M_y|)`.
Die Prüfantwort (`--pruefen`) bleibt bei Vertragsversion 1.
Bibliotheksfunktionen: `segelphysik::bilanz(modell, beitraege)` und
`segelphysik::moment_um(...)` ([Header](../Code/include/segelphysik/hydrostatik.hpp)).

Statische Statuswerte sind `konvergiert`, `nicht_konvergiert`,
`unloesbar`, `abgebrochen`. Dynamische Aufzeichnungen enthalten
`laufend` und abschließend `abgeschlossen` oder `abgebrochen`.
Ein gesamter Lauf kann `fehlgeschlagen` sein, auch wenn bereits
Teilzustände geschrieben wurden. Der CLI-Laufbericht entscheidet über
den erfolgreichen Abschluss, nicht bloß die Existenz einer Datei.
Statische Geschwindigkeiten stammen aus dem Startzustand (CLI: null);
sie sind keine aus der Gleichgewichtssuche abgeleiteten Bewegungen.

Als Bibliotheksbaustein ist `segelphysik::Pose` verfügbar
([Header](../Code/include/segelphysik/pose.hpp)).
Der Konstruktor erwartet Position in Metern und ein endliches Quaternion
in xyzw-Reihenfolge ungleich null; dieses wird normiert.
Die Methoden `modellpunkt_nach_welt` und `weltpunkt_nach_modell`
transformieren Punkte, `modellvektor_nach_welt` und
`weltvektor_nach_modell` ausschließlich Vektoren.
Ungültige Positionen/Quaternionen erzeugen `std::invalid_argument`
mit `CORE-POSE-0001` bzw. `CORE-POSE-0002`.
Ungültige Punkt-/Vektoreingaben erzeugen `std::invalid_argument`,
numerische Überläufe `std::overflow_error`, jeweils mit `CORE-POSE-0003`.

Dieser Baustein verwendet vorläufig `double`. Das ist noch keine
nachgewiesene optimale Zahlenformatwahl für die vollständige Physik;
deren Vergleich und Faktor-10-Nachweis stehen aus.

### Aufruf und Aufzeichnungsform

```powershell
segelphysik_cli.exe --statisch MODELL AUSGABE
segelphysik_cli.exe --dynamisch MODELL AUSGABE DAUER_S DT_S
```

Eine Ausgabe mit Endung `.jsonl` enthält einen vollständigen Zustand pro
Zeile; andere Endungen erhalten ein JSON-Array vollständiger Zustände.
Beides wird inkrementell geschrieben, ohne alle Zustände im Speicher
zu sammeln. Bestehende Ausgabedateien werden ausdrücklich nicht überschrieben.
Die Bibliotheksoption `jsonl` ist standardmäßig aktiv.
Ein dynamischer Zeitraum von null Sekunden liefert genau einen
abgeschlossenen Zustand. Ein positiver Zeitraum enthält Anfangszustand,
ausgewählte Schritte und Endzustand; ein Restschritt endet exakt am
angeforderten Zeitpunkt. Nicht endliche oder bei der Startzeit nicht
darstellbare Zeitbereiche werden ausdrücklich abgelehnt.

Im gemessenen Gesamtlauf mit 1000 Schritten lag JSONL bei rund
33,8 ms, das JSON-Array bei 34,7 ms; die Streuungen überlappen.
JSONL ist damit die vorläufige Arbeitswahl, kein universelles
Geschwindigkeitsversprechen. Bei großen Einzelzuständen kann die
Array-Zusammenstellung günstiger sein. Belege:
[Performancevergleich](PerformanceOptimiert.json).
Ein späterer Live-Transport bleibt separat.

Nach der C-0.2-Erweiterung (Version 2) liegt der Kernpfad im kleinen
Benchmarkfall bei 0,243 ms Median je 500 Auswertungen (Vergleich:
0,326 ms); die Serialisierung von 1000 Zuständen dauert wegen der
zusätzlichen Felder rund 0,21–0,24 ms statt 0,013 ms. Zusammengesetzter
und großer Benchmarkfall wurden nicht erneut gemessen.

## 4. Modellkatalog (Katalogversion 1)

Bibliotheksschnittstelle
([Header](../Code/include/segelphysik/katalog.hpp)), kein globaler
Zustand. Die GUI wählt und überwacht den Ordner; der Core liest ihn.

| Funktion | Antwort |
|---|---|
| `modellkatalog(ordner)` | Katalog über `ordner/bootsmodell.json` und `ordner/*/bootsmodell.json`, sortiert nach Ordnername |
| `katalog_json(kat)` | `katalogversion`, `modelle[]` mit `modell_id`, `name`, `beschreibung`, `modell_fassung`, `status` (`verwendbar`/`fehlerhaft`), `fehleranzahl` |
| `modellstruktur(kat, id, fassung)` | Gruppen und Elemente (`darstellung`, `hydrostatik_volumen`, optional `bewegung`), `gesamtmasse_kg`, `huellquader_m{min,max}` |
| `darstellungsgeometrie(kat, id, fassung[, element])` | `teile[]` mit `element_id`, `gruppe`, `herkunft` (`darstellung`/`volumen_ersatz`), `farbe_rgb`, `oberflaeche`, `punkte_m`, `dreiecke`; dazu `diagnosen` |
| `modelldiagnose(kat, id, fassung)` | Diagnosen mit `fehler_id`, `text`, `fundstelle` |
| `katalogmodell(kat, id, fassung)` | Geladenes `Modell` für Prüfung/Simulation |
| `katalogfehler_json(text)` | Fehlerantwort mit `fehler_id` und `text` |

Antworten enthalten keine Dateipfade; Fundstellen sind relativ zum
Modellordner. Detailabrufe verlangen die im Katalog gemeldete Fassung:
`CORE-KAT-0003` unbekannte ID, `-0004` veraltete Fassung (neu einlesen),
`-0002` doppelte ID, `-0006` unbekanntes Element; ein nicht lesbares
Darstellungsnetz (`-0005`) erscheint als Diagnose, die übrigen Teile
bleiben verfügbar. Ohne Darstellungsnetz wird das Hydrostatikvolumen
als `volumen_ersatz` geliefert. OBJ-Polygone werden für die Darstellung
fächertrianguliert.

`farbe_rgb` (Werte 0..1) stammt aus dem optionalen Modellfeld
`Darstellung.farbe` (`"#rrggbb"`, A-C-00048); ohne Angabe ist es `null`.
`oberflaeche` (`"matt"`, `"glaenzend"`, `"metall"`, `"chrom"`, `"lack"`,
`"glas"`, `"leuchtend"`, `"schillernd"`, `"facettiert"`, `"drahtgitter"`,
`"perlmutt"`, `"unsichtbar"`, `"stoff"`, `"holz"`, `"metall_gebuerstet"`) stammt aus dem
optionalen Modellfeld `Darstellung.oberflaeche` (A-C-00049); ohne Angabe
ist es `null`.

Die stabilen Namen `stoff`, `holz` und `metall_gebuerstet` ergänzen die
bisherigen Werte rückwärtskompatibel für die explizite Materialwahl.
Beschreibung (`ElementBeschreibung.oberflaeche`), Katalog
(`Darstellungsteil.oberflaeche`) und JSON transportieren denselben String
unverändert. Es gibt keine neuen Felder, Datentypen, Funktionen oder
ABI-Änderungen. Oberflächen sind reine Darstellung; physikalische Materialien
und Ergebnisse bleiben unverändert. Eine Änderung der JSON-Metadaten ändert
wie bisher die Modellfassung. UV-Koordinaten und Texturpfade sind nicht
erforderlich; die GUI darf die vorhandenen lokalen Modellkoordinaten zur
prozeduralen Projektion nutzen. Unbekannte Namen bleiben CORE-MOD-0002
mit Fundstelle `Darstellung.oberflaeche`; die tolerante Darstellungsabfrage
liefert dafür `null`, nicht ein still ersetztes Material.

Einschränkungen: Das Modellformat 0.1-entwurf kennt keine Elementnamen
oder Beschreibungen; `name` ist daher die ID, `beschreibung` ist `null`. Die Modellfassung enthält bewusst keine
Darstellungsdateien; die GUI leert ihren Geometriepuffer daher bei
Dateiänderungen selbst. `fehleranzahl` beruht auf dem ersten
Importfehler (plus ggf. Duplikat). CLI-Befehle sind nicht implementiert.

## 5. Tests und Beispiele

Die CTest-Fälle `cli_error_catalog`, `cli_no_arguments`,
`cli_invalid_argument` und `cli_extra_argument` prüfen Katalogabfrage
und vorhandene CLI-Fehler. Der Abfragetest vergleicht die Ausgabe exakt
mit der maßgeblichen JSON-Datei; Fehlerpfade prüfen ID und Katalogstatus.
Beim Textvergleich werden ausschließlich Windows-Zeilenenden normalisiert;
die Ausgabe wird ausdrücklich als UTF-8 verarbeitet.
Der Build validiert Pflichtfelder, IDs und Katalogversion.
`pose_transformations` prüft Punkte/Vektoren, inverse Transformationen,
Achsenrotationen, Nullvektoren, Quaternionnormierung und Fehlerfälle.

`modell_import` prüft unter anderem alle Massenvarianten, Steiner,
Unicode, JSON-Duplikate, Nichtendlichkeit, OBJ-Indices und fehlerhafte
Volumennetze. `hydrostatik_referenzen` prüft Schnitt, Statik, freien Fall,
Rotation und Schrittweitenkonvergenz. `simulation_gesamtlauf` prüft
Aufzeichnung, Modellfehler, Abbruch, Gesamttimer und Überschreibschutz.
`kraft_momentenbilanz` prüft Referenzfälle, Wrench/Frame-Wechsel,
ID-Stabilität, Datenzustände, Summen, Gleichgewichtsresiduen und die
Felder der Version 2. `modellkatalog_schnittstelle` prüft Katalog,
Fassung, Detailabrufe, Fehler-IDs und Pfadfreiheit an den Beispielmodellen.

Der aktuelle Katalog selbst ist eine gültige vollständige Referenzantwort.
Die Dateien unter [Beispiele_Core_GUI](Beispiele_Core_GUI) enthalten
echte statische und dynamische CLI-Aufzeichnungen (Version 2, Quader;
dynamisch 0,1 s mit dt 0,01 s) als GUI-Eingangsdaten.
Sie sind Beispiele, keine zusätzliche Spezifikation.

## 6. Abgrenzung zur GUI-Planung

Die Desktop-/Web-Architektur wurde auf Nutzerauftrag **in GUI übernommen**.
Die [GUI-Übergabe](GUI_Uebergabe.md) enthält die Entscheidungen zu
Qt Widgets/VTK, lokaler Anbindung und späterem Web-Adapter samt
Umsetzungshinweisen. Sie sind keine aktiven Core-Anforderungen.
Dieser Ergebnisvertrag bleibt als Beschreibung der tatsächlich
verfügbaren Core-Schnittstellen bestehen; GUI-/Web-Adapter sind
weiterhin nicht implementiert.

## 7. Hydrodynamische Kraft- und Momentenbeiträge

**Status: ITTC-Rumpfreibung und dynamischer Core-Vertrag implementiert für
C-0.6; Restwiderstand und Kielpolaren nicht implementiert/nicht abgenommen.**
Maßgeblich:
[CORE-Spezifikation Hydrodynamik](../Spezifikationen/Spezifikation_Hydrodynamik.md).
Entscheidungsherkunft und technische Restpunkte:
[Entwurfspunkte Hydrodynamik](../Spezifikationen/Entwurfspunkte/Hydrodynamik.json).
Der dazugehörige
[Umsetzungsplan C-0.6](../Umsetzungsplaene/Umsetzungsplan_Hydrodynamik.md)
dokumentiert Schrittstatus und Nachweise. Die neuen Einzelbeiträge erweitern
Ergebnisversion 2 additiv; bestehende Pflichtfelder und deren Bedeutung
bleiben unverändert.
Die dauerhaften C++-Kennungen sind im
[Schnittstellenregister CORE](Schnittstellenregister_Core.md) festgehalten.

### 7.1 Core-Aufruf und Bewegungsintegration

Die öffentliche C++-Schnittstelle liegt in `segelphysik/hydrostatik.hpp`;
die Laufoption in `segelphysik/simulation.hpp`. `Laufoptionen::hydrodynamik`
kann `HydrodynamikKonfiguration` mit `rho_kg_m3`, `nu_m2_s` und
`fluidgeschwindigkeit_welt_m_s` übergeben. Ohne explizite Konfiguration
aktiviert ein dynamischer Lauf bei vorhandener Rumpfrolle das dokumentierte
Standardwasserprofil (1000 kg/m³, 1e-6 m²/s, ruhendes Fluid); die tatsächlich
verwendeten Werte und die Modellfassung werden im Ergebnis serialisiert.
Ungültige explizite Parameter werden nicht ersetzt.

Für direkte Zustandsläufe bindet `hydrodynamik_start` die typisierte
Konfiguration an die Modellfassung. Der unveränderliche Kontext bleibt über
Zustandskopien und Integratorstufen erhalten. `kraefte` liefert die
Einzelbeiträge; vorhandene Aufrufe ohne Hydrodynamik bleiben kompatibel.
Der GUI-Core-Adapter initialisiert diesen Kontext für Live-Läufe bei
Modellen mit Rumpfrolle ebenfalls mit dem Standardwasserprofil. So stimmen
Live- und Batch-Läufe im Hydrodynamik-Kontext überein; GUI-Darstellung und
Kraftanzeige bleiben unverändert.

Der Integrator muss den Kraftbaustein für jede erforderliche Integratorstufe
mit der Pose und Geschwindigkeit dieser Stufe aufrufen. Er darf die
hydrodynamische Kraft nicht nachträglich nur auf die ausgegebene Pose
addieren. Der Core führt die Beiträge genau einmal mit den vorhandenen
physikalischen Kräften zusammen. Die GUI verändert weder Kräfte noch
Geschwindigkeiten.

Der erste vorgesehene Integrationsumfang ist die dynamische
Starrkörperbewegung. Die erste Relativgeschwindigkeit nutzt die Translation
am Gesamtschwerpunkt; lokale Rotationsanteile sind eine spätere Optimierung.
Die bestehende hydrostatische Statik und der Testantrieb-Vertrag V3 bleiben
davon getrennt. Dauerhafte Hydrodynamik- und Profilangaben liegen im
bestehenden `bootsmodell.json`-Vertrag und werden in der vorhandenen
Modellbeschreibungsspezifikation geführt. Laufbezogene Fluiddaten werden in
einem typisierten Core-Kontext transportiert. Der bestehende Simulationsstart
bleibt zunächst unverändert; die GUI benötigt dafür keine neuen
Hydrodynamik-Eingaben.

### 7.2 Einzelbeitrag

Die Hydrodynamik verwendet den bestehenden Beitragsvertrag aus
Ergebnisversion 2; bestehende Pflichtfelder und deren Bedeutung bleiben
unverändert. Für jeden berechneten Effekt ist folgender additive Beitrag
vorgesehen:

| Feld | Hydrodynamik-Sollinhalt |
|---|---|
| `id` | deterministische, stabile ID, z. B. `Rumpf:hydrodynamik:reibung` |
| `element_id` | zugeordnetes physikalisches Modellelement |
| `art`, `einheit` | `kraft`, `N`; eigenständige Momentenbeiträge `moment`, `N·m` |
| `quelle` | `hydrodynamik` |
| `gruppe` | `hydrodynamik`; differenzierte Untergruppen sind späterer GUI-Ausbau |
| `rolle` | `einzelbeitrag`; Summen bleiben in den vorhandenen Summenfeldern |
| `punktart` | `angriffspunkt` für reale Kraftangriffsstelle |
| `frame` | `modell` |
| `punkt_m` | Angriffspunkt in Metern im Modellframe |
| `vektor` | tatsächliche Kraft in Newton im Modellframe, ohne Darstellungsskalierung |
| `moment_s_nm` | Beitrag um den aktuellen Gesamtschwerpunkt, genau einmal übertragen |
| `zustand`, `fehler_id` | bestehende Status- und Fehlerregeln des Ergebnisvertrags |

Für einen Kraftbeitrag mit Angriffspunkt A und Kraft F gilt derselbe
Bezugswechsel wie im aktuellen Core-Vertrag:

```text
M_i,S = (A - S) × F + M_i,A
```

Kraft, Angriffspunkt und Moment bilden einen konsistenten Wrench. Der
Integratorsumme und der Ergebnisbeitrag müssen aus derselben Auswertung
stammen. Einzelbeiträge werden in Summen nur einmal berücksichtigt;
Gruppen- oder Gesamtsummen sind keine weiteren Kraftsummanden.

### 7.3 Datenzustände und Bilanz

Eine berechnete Kraft von exakt null ist ein bekannter Nullbeitrag und wird
nicht weggelassen. Nicht konfigurierte Physik ist nicht dasselbe wie ein
berechneter Nullbeitrag. Ein erwarteter, aber fehlender oder ungültiger
hydrodynamischer Beitrag darf nicht still als null behandelt werden. Der
Bilanzumfang muss aus dem tatsächlich aktivierten Rechenumfang hervorgehen;
Teilsummen bleiben als solche markiert.

Die Quelle `hydrodynamik` und Gruppe `hydrodynamik` sind implementiert. Sie
werden additiv in die bestehenden Kraft-/Momentensummen aufgenommen und
ändern die Bedeutung vorhandener Beitrags-IDs nicht. Neue Quellen-/
Gruppenwerte allein erfordern keine neue Ergebnisvertragsversion; eine neue Version ist nur bei
inkompatibler Änderung an Pflichtfeldern oder deren Bedeutung notwendig.
V3 bleibt ausschließlich an den bereits spezifizierten
Testantriebskontext gebunden.

### 7.4 Schemaform eines Kraftbeitrags (Zahlenillustration)

Das folgende Objekt zeigt die tatsächlich verwendete Feldform; die Zahlen
sind keine Messwerte und nicht die Ausgabe eines Referenzmodells:

```json
{
  "id": "Rumpf:hydrodynamik:reibung",
  "element_id": "Rumpf",
  "art": "kraft",
  "einheit": "N",
  "frame": "modell",
  "quelle": "hydrodynamik",
  "gruppe": "hydrodynamik",
  "rolle": "einzelbeitrag",
  "punktart": "angriffspunkt",
  "zustand": "gueltig",
  "fehler_id": null,
  "punkt_m": [0.0, 0.0, 2.0],
  "vektor": [-120.0, 0.0, 0.0],
  "moment_s_nm": [0.0, -240.0, 0.0]
}
```

Die Zahlen dienen ausschließlich der Illustration des Schemas und nehmen
einen Schwerpunkt am Modellursprung an. Angriffspunkt, Richtung, Betrag und
Moment müssen aus der tatsächlichen Core-Berechnung stammen. Die Punktwerte
sind keine Vorgabe für einen Referenzrumpf.

### 7.5 GUI-Kompatibilität und spätere Darstellung

Die bestehende Bewegungsanzeige kann weiterhin Pose und Geschwindigkeit aus
dem üblichen dynamischen Core-Ergebnis verwenden; die neue Kraftphysik bleibt
im Core. Die aktuelle GUI-Kraftanzeige liest generische Vektoreinträge und
ordnet unbekannte Quellen der Kategorie „weitere Quelle“ zu. Neue
hydrodynamische Beiträge können daher ohne spezielle GUI-Änderung generisch
angezeigt werden. Neue Farben, Filter, Gruppenbedienung und
Hydrodynamik-spezifische Beschriftung sind nicht Teil der ersten Core-Stufe.
Dieses vorhandene generische Anzeigeverhalten bleibt unverändert und ist
keine fachliche Abnahme der Hydrodynamik.

Die spätere GUI kann dieselben Core-Felder `quelle`, `gruppe`, `id`,
`element_id`, `punkt_m`, `vektor`, `moment_s_nm`, `frame` und `zustand`
nutzen, um eine eigene Darstellung aufzubauen; sie führt keine eigene
hydrodynamische Berechnung ein. Eine spätere Änderung, welche die generische
Anzeige unterdrückt oder speziell steuert, gehört in eine separate
GUI-Spezifikation.

Der Core-Test prüft die dynamische JSON-Ausgabe und die generische
Beitragsform. GUI-Dateien wurden nicht geändert; die generische GUI-Integration
ist damit vertragsseitig vorbereitet, aber nicht als GUI-Build oder
End-to-End-Integration getestet.

## Schema 2: ein OBJ, explizite Verwendungen und Quellherkunft (C-0.8)

Maßgeblich sind A-C-00105 bis A-C-00109 in der vorhandenen
`Spezifikation_Modellbeschreibung.md`; umgesetzt im bestehenden Plan
`Umsetzungsplan_Modellinspektion.md`, U-C-000086 bis U-C-000089.
Keine zweite Themenspezifikation, keine Schreib-/Editorfreigabe der GUI.

Beispiel für das neue Importformat (Dichte und Farbe sind lediglich
Beispielwerte, keine zusätzliche physikalische Modellfreigabe):

```json
{
  "schema_version": "2",
  "id": "boot",
  "geometrie": {"datei": "boot.obj", "laengeneinheit": "m"},
  "gruppen": {
    "koerper": {
      "elemente": [{
        "id": "bauteil-17",
        "Darstellung": {"mesh": "huelle-a", "farbe": "#aabbcc", "oberflaeche": "matt"},
        "Hydrostatik": {
          "masse": {"art": "schale", "dichte_kg_pro_m3": 600,
                    "wanddicke_m": 0.01, "mesh": "schale-a"},
          "volumen": {"mesh": "volumen-a"}
        },
        "Hydrodynamik": {"rolle": "rumpf"}
      }]
    }
  }
}
```

`boot.obj` enthält hier `o huelle-a`, `o schale-a` und `o volumen-a`.
Die Identität ist jeweils der explizite `o`-Name, nicht der Elementname,
die Rolle oder ein `g`-Name. Ein Quellnetz darf für mehrere Verwendungen
referenziert werden. Bestehende unterschiedliche Schalen-/Volumennetze
bleiben echte Varianten in einer einzigen Datei, damit Masse, Trägheit,
Verdrängung und offene Flutränder exakt erhalten bleiben. Core schließt
keine Schale automatisch und erfindet keine fehlenden Varianten.
Im Schema 2 gibt es keine Rolle aus Namen; ohne `Hydrodynamik`-Block
ist keine hydrodynamische Verwendung aktiviert. Ein vorhandener Block
benötigt `rolle`: `rumpf`, `kiel` oder `ruder`.
Das Altformat und seine historische Referenzkörperregel bleiben lesbar.

### GUI-Abfrage ohne Schemaauswertung

Die GUI fragt weiterhin ausschließlich den besitzenden, unveränderlichen
`ModelInspection` ab. `getAvailableMeshTypes` kann zusätzlich `quelle`
anbieten; `getMesh(snapshot, "quelle")` liefert jedes Quellnetz einmal,
einschließlich nicht referenzierter Varianten. Bestehende Fachtypen
`darstellung`, `hydrostatik_volumen`, `hydrodynamik`, `massen_mesh`,
`massenpunkte`, `flutrand` bleiben bestehen.

Zusätzliche Felder von `InspectionMesh` und den JSON-Projektionen:

| Feld | Bedeutung |
|---|---|
| `source_id` | Schema 2: exakte stabile OBJ-`o`-ID; Altformat: Core-Quellkennung pro normalisierter Dateireferenz; leer bei expliziten JSON-Punktmassen |
| `provenance` | `quelle` für unveränderte Quell-/Fachnetze; `abgeleitet` für Punktprojektionen oder berechnete offene Randknoten |
| `derivation` | Lesbare, von Core gelieferte Unterscheidung zwischen Quellkopie, extrahierten Vertices, Punktmassenposition und berechnetem Schalenrand |
| `element_ids` | Alle verwendenden Elemente; leer bei unbenutzter Quelle |
| `element_id` | Kompatibel erste Verwendung oder leer; nicht die vollständige Beziehung |
| `origin` | Bei Quellnetzen `OBJ.o:<ID>` (Schema 2) bzw. logische OBJ-Dateiherkunft im Altformat; keine absoluten Pfade |

`InspectionNode.mesh_ids` verbindet vorhandene Elementknoten mit
Mesh-Antworten. Die GUI kann daraus Quelle-/Berechnet-Unterknoten und
überlagerbare, unabhängig anwählbare Geometrien aufbauen. Zur
Deduplizierung unveränderter Varianten eignen sich `(Element-ID,
source_id)`; die unterschiedlichen Topologien/Ableitungen nicht nur
anhand der gemeinsamen Quell-ID zusammenwerfen. Massenpunkte sind
extrahierte Vertices, keine neu integrierte Punktmassenverteilung.
Flutrandpunkte sind tatsächlich über Kanteninzidenz berechnet.

Schema 3 verschärft den Vertrag auf genau eine Quelle je geometrischem
Element. Nach ausdrücklicher Nutzerfreigabe können `Darstellung`,
`Hydrostatik.masse` und `Hydrostatik.volumen` eine `ableitung` angeben:
`ohne_deckflaeche` selektiert die vorhandene Oberfläche ohne horizontale
Deckfläche, `z_schnitt` verwendet den geschlossenen Quellteil unterhalb
`z = z_m + steigung_x*x + steigung_y*y` einschließlich deklarierter
Schnittfläche. Das ist eine explizite Teilgeometrie, kein weiterer
editierbarer OBJ-Körper und keine implizite Reparatur.
Diese Fachnetze haben `provenance = abgeleitet`, dieselbe `source_id`
und die tatsächliche Definition in `derivation`. Flutrand/Massenvertices
verweisen entsprechend auf die ausgewählte Massengeometrie. Die GUI
ordnet sie unter dem Element den abgeleiteten Daten zu, ohne selbst
eine Ableitung auszuwerten. Snapshot-, Bewegungs- und bisherige
Simulationsendpunkte verwenden dieselbe Core-Geometrie.
Konverter: `--schema3` bleibt strikt; `--schema3-ableitungen` erlaubt die
geometrisch geprüften Ableitungen. Originale bleiben unverändert.

Elementfilter von `getMesh` berücksichtigen alle `element_ids`.
Alle Mesh-IDs und Quell-IDs sind relativ zum Modell/Snapshot; sie werden
nicht über verschiedene Modelle hinweg als globale Identitäten behandelt.
Die JSON-Hüllversion bleibt `"1"`: alle Ergänzungen sind additive
Metadaten. Die Fassungskennung des neuen Dateiformats ändert sich
erwartungsgemäß; sie hasht das gesamte einzelne OBJ und das JSON.

### Explizite Migration und demonstrierbare Nutzung

```powershell
.\build\standard\Release\segelphysik_konvertieren.exe `
  "..\Modelle\420\bootsmodell.json" ".\build\standard\mein-420-v2"
```

Alternativ CPPI-0065 `konvertiere_modell_v2(quelle, zielordner)`.
Bei Schema-3-Quellen werden vorhandene Geometrieableitungen exakt als
eigene OBJ-Quellnetze materialisiert; Schema 2 enthaelt keine
`ableitung`-Felder. Physik und effektive Darstellungsgeometrie bleiben erhalten.
Der Zielordner darf nicht existieren; sein Elternordner muss existieren.
Originale werden nur gelesen, nie verändert. Ergebnis sind genau
`bootsmodell.json` und `boot.obj`; der normale Katalog-/Inspektionsimport kann
deren gemeinsamen Elternordner anschließend wie bei Altmodellen lesen.
Schreibfehler melden gegebenenfalls einen unvollständigen neuen Ordner,
der nicht stillschweigend entfernt wird.
Der Regressionstest erzeugt zusätzliche demonstrierbare Referenzkopien
unter `build\standard\v2-modelle-test`. Das sind Testprodukte, keine
Änderungen an `..\Modelle` und keine automatische Migration beim Laden.

## C-0.12: Persistente Segel-Einbaulage

Maßgeblich sind [Spezifikation Segelkonfigurator, Abschnitte 1–9](../Spezifikationen/Spezifikation_Segelkonfigurator.md)
und [CPPI-0120 bis CPPI-0136](Schnittstellenregister_Core.md). Die neue
Schnittstelle liegt in `segelphysik/segelkonfigurator.hpp`; sie ist
transport-, Qt- und VTK-unabhängig. `ModelInspection` bleibt ein
unveränderlicher Ausgangssnapshot. Die Segel-API liefert neue Meshkopien,
die die GUI anstelle der unveränderten Segelmeshes darstellen kann.

`leseSegelKonfiguration(katalog, inspection)` gibt direkt einen
`SegelKonfigurationSnapshot` zurück. Status `NichtKonfiguriert` bedeutet,
dass der Block `konfiguration` in `bootsmodell.json` fehlt; die Standardlage ist dann die
Identität. `Bereit` bezeichnet eine gültige geladene Fassung. Bei
beschädigter, fremder oder quellenveralteter Datei ist der Status `Fehler`
mit `diagnosen`; das Lesen wirft solche Formatfehler nicht nach außen.
Der Snapshot enthält Modell-ID/-Fassung, Quellfingerprint, Revisionsstring,
explizite Referenzen, Lagen und die unveränderten Ausgangsmeshes der
konfigurierbaren Segel. Fehlende Original-Buglage in einem älteren Eintrag
wird als `Unbekannt` geladen, ohne seine Einbaulage zu ändern.

Für den 420er ist das Elementmapping ausdrücklich
seit der Ordnerumstellung vom 06.11.2026 unter `modells/` abgelegt.
Der bisherige Datenordner `meshes/` wurde in allen elf Referenzmodellen
umbenannt; die Dateipfade in `bootsmodell.json` wurden entsprechend angepasst.
Die API-Felder `meshes` behalten dagegen ihren Namen und ihre Bedeutung.
Geometrie- und Lookup-Dateien bleiben byteidentisch. Da Modellfassung und
Segelquellen-Fingerprints auch Pfade berücksichtigen, wurden die
Fingerprints der vorhandenen 420er-Konfiguration kontrolliert migriert;
Revision, Einbaulagen und Original-Buglagen bleiben erhalten.

Das Elementmapping lautet
`Großsegel` → `420_grosssegel` / `modells/gui/grosssegel.obj` /
Lookup `420_grosssegel` und `Fock` → `420_fock` /
`modells/gui/fock.obj` / Lookup `420_fock`. Hals, Schothorn und
Kopfecken kommen aus den IDs der jeweiligen intrinsischen `grundkontur`,
nicht aus Punktreihenfolge. Die Riggreferenzen werden in
`Code/src/segelkonfigurator.cpp` aus den `Darstellung.datei`-Referenzen der
Modellelemente `Mast`, `Großbaum`, `Vorstag` aufgeloest und gegen die
expliziten 420-Dateipfade geprueft: Mast-/Baumanschlag ist die
Mast-Mittellinie auf Hoehe der Baum-Endkappen, die Grossbaumachse zwischen
deren Zentren, der Fockdeckanschlag das erste Vorstagquerschnittszentrum
und die Vorstagachse aus den ersten zwei Querschnittszentren. Die reproduzierbaren
420er-Geometriewerte sind Mast-/Baumanschlag `[0.85070675, 0, 1.35] m`,
Baumvektor zum Schothorn `[-2.338488, 0.539883, 0] m`, Fockdeck
`[2.00570675, 0, 0.616428] m` und Vorstagrichtung
`[-0.1475, 0, 0.510447] m`. Diese Werte stammen aus den OBJ-Vertices
(`modells/gui/mast.obj`, `grossbaum.obj`, `vorstag.obj`), deren Kommentare
Meter und +X Bug, +Y Backbord, +Z oben festlegen. Der Core-Kopietest
berechnet dieselben Anschlaege/Achsen unabhaengig aus den Quelldateien und
prueft den Winkel zur automatisch platzierten Segelkontur mit <= 1e-6 rad.
Es gibt keine AABB-/Vertex-Reihenfolge-Heuristik zur Wahl der
Segelkonturecken. Die Rigg-Querschnittsstruktur ist fuer diesen
rekonstruierten 420er explizit: `mast.obj` enthaelt 24-Vertex-Ringe
(erste zwei Ringe: Vertices 1–24 und 25–48); die Mastachse wird zwischen
den Ringzentren auf die z-Hoehe der Baum-Endkappen interpoliert.
`grossbaum.obj` enthaelt zwei 24-Vertex-Ringe (1–24, 25–48) und die
Endkappenzentren als Vertices 49/50; die Kappenzentren muessen innerhalb
1e-6 m mit den jeweiligen Ringzentren uebereinstimmen. `vorstag.obj`
enthaelt den Deckquerschnitt in Vertices 1–8 und den folgenden
Querschnitt in Vertices 9–16; ihre Differenz definiert die lokale
Vorstagachse. Der unabhaengige CTest liest diese OBJ-Vertices selbst und
prueft Anschlaege und resultierende Liekachsen gegen sie.
Die Geometriewerte beschreiben den nicht simulationsfreigegebenen
420er-Entwurf, keine Messgenauigkeit.

**Autorisierte neutrale Basis fuer alle drei Boote:** Automatik folgt
nicht mehr einem ausgeschwenkten Rohbaum, sondern stellt die
Mittschiffsbasis her. 420, Folkeboot und Optimist speichern und zeigen
Großsegel/Fock/Großbaum in 0-Grad-Oeffnung; Mast-/Vorliekneigung bleibt
erhalten, Quaternionen sind deshalb nicht generell Identitaet.
Opti-Spriet folgt konsistent der starren Segelpose. Keine neue UI.
Folk/Opti verwenden explizite `Montage`-OBJ-Vertex-/Ringreferenzen
direkt am bestehenden Element; genaue Indices, Semantik und gemessene
Quellenkonflikte stehen in
[Core-Spezifikation Abschnitt 2a](../Spezifikationen/Spezifikation_Segelkonfigurator.md).
Ihre intrinsische Quelle steht einmalig unter `Segel.quelle_datei`,
ein nicht vorhandener Lookup wird weder erfunden noch als notwendig
fuer reine OBJ-Montage ausgegeben. `CORE-SEG-0030` im nutzbaren
Snapshot kennzeichnet unvereinbare intrinsische Masse/OBJ und fehlende
Physikfreigabe. Die API raet keine unbekannten Modellzuordnungen.

**Aktueller datengetriebener Vertrag (06.11.2026):** Die obigen
Folk-/Opti-Indices sind historische Referenzen. Neu rekonstruierte
GUI-OBJs werden anhand ihrer Quelldeklaration und semantischen Kontur
aufgeloest; ungepruefte alte Vertexindices werden nicht auf neue
Meshes angewandt. `CORE-SEG-0031` benennt die veraltete Referenz und
den belegten read-only Legacy-Fallback, `CORE-SEG-0032` die starre
Umrechnung einer bestehenden Pose ohne Ruecksetzung. Quellenchecks
bereits bestaetigter Referenzen werden strikt validiert.
Die Referenzbildung verwendet keine Boots-ID-Sonderzweige.

`getMesh(inspection, "darstellung")` liefert genau eine aktuelle
Geometrie je vorhandener Objekt-ID, einschliesslich bestaetigter oder
belegt adaptierter Segellage. Andere Mesh-Typen bleiben physische Quellen;
`getObjectTree(..., "objekte")` referenziert nur die aktuelle Darstellung.
Mehrdeutige Darstellung wird diagnostiziert, nicht durch Auswahl des
ersten Meshes kaschiert. Das optionale Feld `InspectionMesh.montage`
traegt `MontageReferenz` (CPPI-0139) fuer alle dargestellten Objekte.
`inspectionMeshJson` exportiert denselben Block unter `montage`;
bestehende JSON-/C++-Geometriefelder bleiben kompatibel.
Segellose Modelle liefern einen gueltigen leeren Konfigurationssnapshot.
Die erste explizite Bestaetigung speichert Quellenchecks ausschliesslich
in `setting.montage_referenz`; Basisfassung und immutable Snapshot
bleiben stabil. Originalmodelle werden in diesem Auftrag nur gelesen.

Eine Aktionsvorschau wird mit `pruefeSegelAktion(snapshot, request)`
berechnet und schreibt nichts. Der Request benennt mit `element_id`
exakt `Großsegel` oder `Fock` und eine Aktion:

- `Automatisch` stellt die neutrale Mittschiffsbasis wieder her:
  420 an Mast/neutralem Baum bzw. Vorstag, Folk/Opti anhand expliziter
  belegter OBJ-Referenzen. Automatik ersetzt vorherige manuelle Lage,
  aber keine Original-Buglage.
- `Translation` interpretiert `ebene` als XY/XZ/YZ in Modellachsen.
  `translation_m` darf nur in den beiden Ebenenachsen Anteile haben;
  die Normal-Komponente muss null sein.
- `Rotation` interpretiert `winkel_grad` mathematisch um die positive
  Normale der genannten Modellebene. `bewegte_ecke` benennt den Griff:
  jede Ecke außer `Hals` rotiert um den Hals; beim Griff `Hals` rotiert
  um `KopfVorliek`. Es wird kein freier Weltpivot angenommen.
- `FockVersatzCm` ist ein absoluter vorzeichenbehafteter Zentimeterwert.
  Die feste Referenz ist der Deckanschlag und die normierte Richtung zeigt
  vom Deck zum Mast. Wiederholte Eingabe addiert nichts. Die resultierende
  `lage.translation_m`/Geometrie enthaelt den cm-Versatz bereits; die GUI
  darf ihn nach Uebernahme der Core-Lage nicht nochmals addieren.
- `OriginalBuglage` erfordert `original_buglage` (`Unbekannt`,
  `Backbord` oder `Steuerbord`) und wird wie die Lage transaktional
  gespeichert.

Vorschau- und Speicherergebnis haben Status `Vorschau`, `Gespeichert`
oder `Fehler`; sie tragen den Revisionsstring, optional die vollständige
resultierende Lage des betroffenen Segels, Meshkopien und strukturierte
Diagnosen. Normale Aktionen geben nur alle Meshes des betroffenen Segels
zurück. `leseKonfigurierteSegelgeometrie(snapshot)` liefert read-only
alle konfigurierten Segelmeshes für den initialen Modellwechsel; übrige
Modellgeometrie bezieht die GUI weiter aus `ModelInspection`. Damit wird
ein bestätigter Zustand dargestellt, ohne den Inspektionssnapshot zu
mutieren.

`bestaetigeSegelAktion(...)` verlangt die Revision der Vorschau. Unter
einer prozessübergreifenden Betriebssystemsperre werden Fassung,
Quellfingerprint und aktuelle Revision erneut geprüft; konkurrierende
oder veraltete Anforderungen scheitern mit Diagnose. `modells_information.lock`
schuetzt das gezielte Aktualisieren von `setting` direkt am bestehenden
Element in `bootsmodell.json` und der gemeinsamen `konfiguration`-Metadaten.
Keine zentrale Elementmap und keine neuen IDs. Andere Eintraege, unbekannte
Felder und sonstiger Modellinhalt bleiben erhalten. Eine temporäre Datei
im selben Modellordner wird erst nach
vollständigem Schreiben atomar ersetzt. Vorherige bestätigte Daten bleiben
bei Fehlern erhalten. Die Datei enthält Modell-/Quellidentitäten,
Revision, Quaternion `rotation_xyzw`, Translation in Metern,
Original-Buglage und Quellfingerprint je Segel. Der Fockversatz wird aus
Pose und Vorstagreferenz abgeleitet, nicht gespeichert. Intrinsische
Segel-JSON, Darstellungs-OBJ, Riggmeshes und Lookup-Dateien werden nie
geschrieben.

Persistenzschema (Montagepose, kein Betriebstrimm):

```json
{
  "konfiguration": {
    "schema_version": 1,
    "revision": 12,
    "modell_id": "420",
    "modell_fassung": "fnv1a64:...",
    "quellen_fingerprint": "..."
  },
  "gruppen": {
    "rigg": {
      "elemente": [{
        "id": "Fock",
        "Darstellung": {"datei": "modells/gui/fock.obj"},
        "Segel": {
          "quelle_datei": "modells/aerodynamik/fock/fock.json",
          "lookup_datei": "modells/aerodynamik/fock/LookUp/lookup.json"
        },
        "setting": {
          "translation_m": [0.0, 0.0, 0.0],
          "rotation_xyzw": [0.0, 0.0, 0.0, 1.0],
          "konfiguriert": true,
          "original_buglage": "unbekannt",
          "quellen_fingerprint": "..."
        }
      }]
    }
  }
}
```

Die Zahlen sind nur Schemaillustration, keine validierte Focklage.
Teilkonfigurationen sind erlaubt; fehlende Segel bleiben initial.
Der initiale leere Block darf ausschliesslich `schema_version:1`,
`revision:0` ohne Modell-/Quellenidentitaet enthalten;
der erste bestaetigte Speichervorgang ergaenzt diese Identitaet.
Alle vorhandenen Elemente verwenden dieselben gemeinsamen Posefelder in
`setting`; Nicht-Segel haben keine Buglage oder Segel-Quellfingerprints.
Bestaetigte Nicht-Segel-Montageposen werden genau einmal bei
Darstellungsaufbereitung angewendet. Die lokale 420-Segelquelle wird
nur von der Segel-API montiert. Folk-/Opti-OBJ-Segel erscheinen bereits
neutral in der generischen Inspektion; ihre Segel-API liest eigene
unveraenderte Rohmeshkopien und montiert genau einmal.
Physische Netze, Massenschwerpunkt und Traegheit bleiben unveraendert;
Montagedarstellung ist in dieser Stufe keine physikalische Verstellung.
Ein Legacy-Sidecar oder die alte zentrale Elementmap wird explizit abgelehnt.
Bei Migration Revision, Pose, bestaetigten Zustand und Original-Buglage
unveraendert uebernehmen; anhand bestehender IDs die vorhandenen Elemente
finden und ihre `setting`-Felder gezielt ergaenzen. Keine IDs hinzufuegen.
Redundante IDs und cm-Werte entfallen. Intrinsische Quelle/Lookup einmalig
am jeweiligen Element referenzieren.
Modellfassung nach Umbenennung neu ermitteln, SHA-256-Quellfingerprints
nur bei veraenderten Pfaden/Bytes neu ermitteln.

Modellfingerprint: FNV-1a64 mit Offset `14695981039346656037` und Faktor
`1099511628211`, modulo 2^64. Jede Teilzeichenfolge wird mit ihrer
UTF-8-Byte-Laenge als 8 little-endian Bytes vorangestellt gehasht:
zuerst das mit nlohmann geparste Modelldokument ohne Top-Level-`konfiguration`
und ohne `setting` direkt an `gruppen.*.elemente[]` mit String-ID
kanonisch per `dump()` serialisieren.
Dann alle rekursiv gefundenen Stringfelder `datei`, `quelle_datei`,
`lookup_datei`,
eindeutig und lexikographisch sortiert, jeweils relativer Pfad und
Dateiinhalt. Keine rekursive Inventarisierung aller Lookup-Produkte.
Alle anderen Modellinhalte inklusive gleichnamiger `setting`-Felder
ausserhalb der Elementobjekte sowie referenzierte Quellen bleiben erfasst.
Eine fehlende
referenzierte Datei liefert den expliziten Marker `[fehlende Quelldatei]`;
Lese-/JSON-Fehler werden nicht verschluckt. Ausgabe `fnv1a64:` plus 16
kleine Hexziffern. `lade_modell` und `modell_fassung` verwenden dieselbe
Berechnung; Konfigurationsspeicherung aendert die Modellfassung nicht.
Der native Test bietet `segelkonfigurator_tests --fingerprint <bootsmodell.json>`
zur exakten migrationsseitigen Berechnung ohne eigene JSON-Kanonisierung.

Der Winkelvertrag ist von Einbaulage getrennt. `stelleSegelStellwinkelVorschauDar`
wendet Grad-Winkel um die gerichtete Kopf-zum-Hals-Achse durch den
semantischen Hals vor der Einbaulage an (lokale 420-Quelle `(0,0,-1)`,
Folk/Opti explizite OBJ-Vorlieksachse). Positiv bedeutet Backbord,
negativ Steuerbord.
Bei bekannter, abweichender Original-Buglage spiegelt der Core lokal mit
`diag(1,-1,1)` und kehrt Dreiecksorientierungen um; bei gleicher Seite
bleibt die Form ungespiegelt. Bei `delta=0` bleibt die Originalform in
der gespeicherten neutralen Basis erhalten, nicht in der historisch
ausgeschwenkten Rohstellung. Nichtnullwinkel mit unbekannter Originalseite liefern Diagnose,
statt eine Seite aus dem Wölbungsvorzeichen abzuleiten. Der Winkel ist
in C-0.12 eine transiente Darstellungsvorschau, keine gespeicherte
Stellereignisfolge oder Physikauswertung.

Die native Vertragstest-Suite kopiert das 420er-Modell in das Core-Build-
Verzeichnis; die Originalmodelle und Lookup-Produkte sind keine
Testschreibziele.

## C-0.9: Dynamische modellfeste Fremdkraft – Core-Mindestvertrag

Maßgeblich sind [A-C-00126 bis A-C-00128](../Spezifikationen/Spezifikation_Dynamische_Fremdkraft.md)
und [CPPI-0105 bis CPPI-0112](Schnittstellenregister_Core.md). Dieser
Abschnitt dokumentiert nur die Core-Schnittstelle; GUI-Kamera,
Bedienungs- und Nutzerabnahme bleiben im GUI-Vertrag.

Für Batch-Aufrufe kann `Laufoptionen::dynamische_fremdkraft` mit
`DynamischeFremdkraftKonfiguration` gesetzt werden. Der direkte Livepfad
ruft nach Vorbereitung der festen `Zustand::bewegungsstellungen`
`dynamische_fremdkraft_start(modell, zustand, konfiguration)` auf. Der
zurückgegebene Zustand hält einen unveränderlichen
`DynamischeFremdkraftKontext`; `dynamikschritt` übernimmt diesen Kontext
ohne zusätzliche Kraftargumente. Die Konfiguration enthält `aktiv`,
`kraft_modellfest_n` und `angriffspunkt_modell_m`. Die Core-Schnittstelle
hat keinen Frame-Schalter: beide Eingaben sind immer modellfest. Die GUI
muss jede feste Elementstellung vor der Startfunktion in
`Zustand::bewegungsstellungen` setzen. Batch nutzt dieselbe Liste in
`Laufoptionen::start.bewegungsstellungen`. Gleichzeitige aktive Kontexte
von `dynamische_fremdkraft` und dem alten Testantrieb liefern
`CORE-SIM-0001`.

Ein Zustand mit Fremdkraftkontext verwendet Ergebnisvertrag `"7"`.
Er enthält den Konfigurationssnapshot:

```json
{
  "dynamische_fremdkraft": {
    "aktiv": true,
    "modellfest": true,
    "kraft_modellfest_n": [10, 0, 0],
    "angriffspunkt_modell_m": [0, 1, 0],
    "modell_fassung": "...",
    "auswertung_schritt": 0,
    "auswertung_zeit_s": 0
  }
}
```

Der zugehörige Einzelbeitrag erscheint genau einmal in `vektoren` mit
`id: "dynamisch.fremdkraft"`, `quelle: "externe_kraft"`,
`gruppe: "dynamische_fremdkraft"`, `frame: "modell"`,
`punkt_m`, `vektor` und `moment_s_nm`. Sein `zustand` ist `gueltig`,
`null` (aktiv mit Nullkraft) oder `deaktiviert`; fehlender Kontext erzeugt
keinen Beitrag und behält den bisherigen Ergebnisvertrag. Summen werden
aus Einzelbeiträgen abgeleitet und enthalten die Fremdkraft nicht doppelt.
Der Fremdkraftmomentbezug ist der physische Gesamtschwerpunkt derselben
Elementstellungen; dieser Schwerpunkt steht in
`moment_bezug_modell_m`. Die Kraft wirkt in den Kraftauswertungen des
Integrators, nicht nur in dieser Projektion.

Das Bilanzinventar nennt dieselbe Einzelbeitrags-ID
`dynamisch.fremdkraft`, nicht `stationaer.fremdkraft`. Ein gueltiger,
deaktivierter oder aktiver Nullbeitrag macht die ansonsten vollstaendige
Bilanz nicht unvollstaendig.

Auch dynamische Fremdkraft-Snapshots liefern
`druckpunkte.ersatz_segel` mit `quelle: "fremdkraft"`. Bei aktiver,
von null verschiedener Kraft gilt `status: "gueltig"` und
`position_modell_m` ist exakt der konfigurierte modellfeste
`angriffspunkt_modell_m`. Bei deaktivierter Kraft gilt `inaktiv`,
bei aktiver Nullkraft `kraft_null`; beide enthalten keine Position.
Dies ist der Ersatz-Ansatzpunkt fuer die Darstellung, kein durch
Windphysik berechneter Segeldruckpunkt. Historische Snapshots verwenden
ihren eigenen eingefrorenen Kraftkontext.

Nach Nutzerfreigabe vom 04.11.2026 kann ein laufender Aufrufer mit
`dynamische_fremdkraft_aendern(modell, zustand, konfiguration)` (CPPI-0119)
einen neuen unveraenderlichen Kontext fuer den naechsten Integrationsschritt
erzeugen. Die Funktion aendert weder Pose/Zeit noch den uebergebenen Zustand.
Der Aufrufer ersetzt damit nur seinen noch nicht veroeffentlichten
Schrittstart; gespeicherte Zustaende bleiben unveraendert. Ein vorhandener
passender Fremdkraftkontext ist erforderlich. Nichtendliche Werte und ein
gleichzeitig aktiver Testantrieb bleiben Fehler. Keine neue Batch-Zeitplanung
oder Aenderung des JSON-Formats. Ruder/Schwert verwenden weiterhin
`Bewegungsereignis`; deren bestehende Geometrie-/Traegheitsinvalidierung
bleibt massgeblich. Der GUI-Adapter puffert bis zur naechsten Schrittgrenze
den letzten vollstaendigen Kraft-Eingabestand und meldet nicht mehr
angewandte Aenderungen bei Laufende.

U-C-000112 bis U-C-000114 und technische Core-Tests sind abgeschlossen.
Der GUI-Adapter muss Vertragsversion 7 akzeptieren und `externe_kraft`
als Quelle der dynamischen Fremdkraft darstellen. GUI-Integration,
persönliche Prüfung sowie Performancevergleich sind getrennte,
noch nicht abgeschlossene Gates; der Performancevergleich ist bis zu
erneuter ausdrücklicher Nutzerfreigabe blockiert.
# C-0.10: explizite Aufbereitung fuer Live und Batch

Additiver C++-Vertrag CPPI-0113..0118, Header
`segelphysik/aufbereitung.hpp`: `Modellaufbereitung` besitzt eine
unveraenderliche Momentaufnahme des geladenen Modells und lebt genau so
lange wie der zugehoerige Lauf/Worker. Keine implizite Wiederverwendung
anhand von Dateiname oder Modell-ID. Beim Austausch von Modellinhalt,
Topologie oder Ableitungsdefinition einen neuen Kontext anlegen.

Die Startfunktionen weiterhin mit `kontext.modell()` aufrufen.
Im Livepfad `kontext.schritt(...)` statt `dynamikschritt(modell,...)`
und `kontext.json(...)` statt `zustand_json(modell,...)` verwenden.
Ereignisargumente, rho/g und JSON-Format behalten ihre bisherigen
Defaults und Bedeutung. `kontext.auswertung(zustand,rho,g)` liefert eine
vollstaendige `Kraefte`-Kopie; Aenderungen daran beeinflussen den Cache nicht.
Auswertung und JSON pruefen die historischen Pflichtkontexte identisch.
Alte Zustaende werden anhand ihrer eigenen Stellungen aufbereitet,
nicht anhand der zuletzt vom Worker verwendeten Geometrie.

Eine feste Stellung wird einmal vorbereitet. Effektive Stellungswechsel
kopieren/transformieren nur den Eigentuemer und seine mitbewegten
Elemente; Schwerpunkt und Traegheit werden konsistent neu aggregiert.
Die bestehende Zusatztraegheitsaktualisierung bei Ereignissen bleibt
erhalten, auch wenn ein Ereignis denselben Stellwert bestaetigt.
Gleiche Stellungen bedeuten keine gleichen Posen: Nasspanels, Schnitte
und Flutung werden weiterhin pro wirksamem Zustand bestimmt.
Der Kontext haelt hoechstens eine physische Stellungsfassung, lokale
Bounds der Basis, einen letzten Weltpunktestand je eigenem Mesh und
eine vollstaendige letzte Kraftauswertung. Keine Verlaufscaches.
Eigene Kontexte fuer verschiedene Worker/Laeufe verwenden.

`simulieren` verwendet den Kontext intern; seine JSON-Struktur,
Zustandsfolge und Ausgabedichte bleiben unveraendert. Bestehende freie
APIs sind weiter verfuegbar. Die statischen Solveralgorithmen,
Integratorgleichungen, Physikmeshes und fachlichen Toleranzen aendern
sich nicht.
