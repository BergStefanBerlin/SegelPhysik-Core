# Schnittstellenbeschreibung / Ergebnisvertrag Core → GUI

## Status und Geltungsbereich

Stand: 26.10.2026. Technische Dokumentation zu A-C-00016, A-C-00028,
A-C-00031 und A-C-00032 der
[Hydrostatikspezifikation](../Spezifikationen/Spezifikation_Hydrostatikkern.md).
Keine weitere fachliche Spezifikation.

**Tatsächlich verfügbar:** Bibliotheks-/CLI-Abfrage des Fehlerkatalogs,
CLI-Buildinformation und CLI-Fehler mit eindeutigen IDs sowie
Pose-Transformationen, Modellimport/-prüfung, statischer Solver,
Starrkörperdynamik und versionierte Ergebnisaufzeichnungen.
**Noch nicht verfügbar:** GUI-Verbindung oder Live-Transport.
Technischer Sicherungsstand: **C-0.1**, kein GitHub-Release und
keine vorbehaltlose fachliche Gesamtabnahme.

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

Es gibt aktuell 18 Katalogeinträge: zwei CLI-, drei Pose-, acht
Modell-, vier Simulations- und eine Ausgabediagnose.
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
| `vertragsversion` | String `"1"` |
| `modell_id`, `modell_fassung` | Strings; Fassung ist ein FNV-1a-Inhaltsfingerprint des Imports, keine Sicherheitsprüfung |
| `status`, `diagnose` | Strings; leere Diagnose bei regulärem Erfolg |
| `zeit_s`, `schritt` | Simulationszeit in s, nicht negative laufende Ganzzahl |
| `position_m` | Drei Zahlen: fester Modellursprung in der Welt |
| `orientierung_xyzw` | Vier Zahlen: normiertes Quaternion Modell nach Welt |
| `geschwindigkeit_welt_m_s` | Drei Zahlen: Geschwindigkeit des Gesamtschwerpunkts im Weltframe |
| `omega_modell_rad_s` | Drei Zahlen: Winkelgeschwindigkeit im Modellframe |
| `volumen_m3` | Gesamte eingetauchte Verdrängung |
| `vektoren` | Array der nachfolgend beschriebenen Beiträge |
| `elementvolumina` | Array mit Element-ID, `volumen_m3`, `auftriebspunkt_modell_m` |
| `moment_modell_nm` | Gesamtes Moment im Modellframe |
| `moment_bezug_modell_m` | Gesamtschwerpunkt im Modellframe |

Alle Felder sind vorhanden. Ein nicht existierender Auftriebspunkt wird
als JSON-`null` ausgegeben, nicht als scheinbar gültiger Ursprung.
Auch ein Element ohne Volumen wird mit exakt null Verdrängung geführt.
Einheiten sind SI; Zeit ist nicht die Rechenlaufzeit.

Vektorbeiträge enthalten `id`, `element_id`, `art`, `einheit`,
`frame: "modell"`, `punkt_m` und `vektor`.
`art: "kraft"` verwendet N; `punkt_m` ist der Angriffspunkt.
`art: "moment"` verwendet N·m; `punkt_m` ist der Momentenbezug.
`gesamt:moment` verwendet die leere Element-ID als ausdrücklich
reservierten Gesamtmodellbezug. IDs `<Element-ID>:gewicht` und
`<Element-ID>:auftrieb` identifizieren Elementkräfte.
Richtung und Länge werden aus den drei Vektorkomponenten gewonnen;
die GUI wählt den grafischen Maßstab. Nullvektoren haben keine Richtung.

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

## 4. Tests und Beispiele

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

Der aktuelle Katalog selbst ist eine gültige vollständige Referenzantwort.
Die Dateien unter [Beispiele_Core_GUI](Beispiele_Core_GUI) enthalten
echte statische und dynamische CLI-Aufzeichnungen als GUI-Eingangsdaten.
Sie sind Beispiele, keine zusätzliche Spezifikation.

## 5. Abgrenzung zur GUI-Planung

Die Desktop-/Web-Architektur wurde auf Nutzerauftrag **in GUI übernommen**.
Die [GUI-Übergabe](GUI_Uebergabe.md) enthält die Entscheidungen zu
Qt Widgets/VTK, lokaler Anbindung und späterem Web-Adapter samt
Umsetzungshinweisen. Sie sind keine aktiven Core-Anforderungen.
Dieser Ergebnisvertrag bleibt als Beschreibung der tatsächlich
verfügbaren Core-Schnittstellen bestehen; GUI-/Web-Adapter sind
weiterhin nicht implementiert.
