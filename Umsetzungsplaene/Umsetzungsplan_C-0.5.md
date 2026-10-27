# Umsetzungsplan C-0.5: Temporärer Testantrieb

## 1. Status, Quellen und Geltung

**Stand: 27.10.2026. Status: nach ausdrücklichem Implementierungsauftrag
technisch umgesetzt und gezielt getestet; fachliche Nutzerabnahme offen.**
C-0.5 ist der vom Nutzer benannte aktuelle
CORE-Arbeitsversionsschritt. Die GitHub-Prüfung am 27.10.2026 ergab
veröffentlichte Versionstags bis C-0.4 und noch keinen Plan C-0.5.
Ein Arbeitsversionsschritt setzt keinen bereits veröffentlichten Tag voraus.

Dieser Plan ist der einzige maßgebliche Umsetzungsplan für C-0.5.
Ältere Pläne und deren Abnahmestände bleiben unverändert.
Der hier geplante Lieferumfang ist ausschließlich der temporäre Testantrieb.
Weitere Themen werden dadurch weder aufgenommen noch als erledigt erklärt.

Maßgebliche fachliche Quelle ist der
[finalisierte CORE-Spezifikation](../Spezifikationen/Spezifikation_Temporaerer_Testantrieb.md)
mit seiner
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Temporaerer_Testantrieb.json).
Abnehmer sind die separat finalisierte
[GUI-Spezifikation](../../GUI/Spezifikationen/Spezifikation_GUI_Testantrieb_und_Fahrtdarstellung.md)
und der [GUI-Plan G-0.5, Abschnitt 9](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.5.md).
GUI- und CORE-Versionen, Freigaben und Nachweise bleiben getrennt.

Die ursprünglichen Nutzerantworten vom 27.10.2026 finalisierten Spezifikation
und Plan; der nachfolgende Auftrag autorisiert nun die Implementierung
beider Projekte, ausdrücklich ohne Git-Aktionen. A-C-00052 bis A-C-00060 und
U-C-000036 bis U-C-000044 folgen projektweit auf die bisher höchsten
Kennungen A-C-00051 und U-C-000035. Keine historischen Kennungen ändern.
U-C-000036 bis U-C-000043 sind technisch umgesetzt und gezielt nachgewiesen.
U-C-000044: technische Matrix vorhanden, Nutzerabnahme ausstehend.

## 2. Voraussetzungen und Freigabegrenze

Vor Implementierungsbeginn geprüft (ursprüngliche Leitplanken):

- Tatsächlichen CORE-Arbeitsbaum, benannten Branch, vorhandene Änderungen,
  verwendeten Build und passende Testziele dokumentieren. Fremde Änderungen
  weder übernehmen noch verwerfen.
- Finalisierte Buggeometriequelle ausschließlich aus physikalischen
  Massen-/Volumennetzen und Punktmassen sowie die Toleranzen der Spezifikation
  verwenden. Darstellungsdaten bleiben unabhängig.
- In U-C-000036 den fachlich festgelegten Lauf-/Ereignisvertrag und
  Ergebnisversion 3 technisch in API, Feldnamen, Nullabbildung und
  Fehlerkatalogzuordnung ausarbeiten; keine neue fachliche Auswahl.
  Aktive Statik ablehnen; Startzustand unverändert, erste Grenze nach
  erstem Schritt. Keine neuen CLI-Argumente.
- Die IDEE-zu-A-Zuordnung und Rückverfolgbarkeit dieses finalisierten
  Plans erhalten. Interface-Register nur pflegen, falls eingerichtet
  und für konkret geplante Schnittstellen einschlägig.
- Gesonderte Implementierungsfreigabe durch den aktuellen Nutzerauftrag
  vorhanden; weiterhin keine Freigabe für Git-Aktionen.

Defaultwerte 200 N und exakt 463/90 m/s sowie die bereits bestätigten
GUI-Verhaltensregeln werden nicht erneut zur Disposition gestellt.
Alle abgefragten fachlichen CORE-Festlegungen sind entschieden.
Technische Ausarbeitung und Nachweise siehe Abschnitt 9.

## 3. Abhängige Arbeitspakete

Die folgende Tabelle erhält den freigegebenen Umfang und seine
Abhängigkeiten. Tatsächlicher Status je Schritt steht in Abschnitt 9;
fachliche Nutzerabnahme wird nicht vorweggenommen.

| Kennung | Arbeitspaket | Anforderungen | Schrittvoraussetzungen | Geplantes Ergebnis und Prüfung |
|---|---|---|---|---|
| U-C-000036 | Technische Vertragskonkretisierung | A-C-00052, A-C-00053, A-C-00056, A-C-00057, A-C-00058, A-C-00059, A-C-00060 | keine | Typsichere API, V3-Schema, Fehlerzuordnung und Testfälle dokumentiert; fachliche Nutzerentscheidungen unverändert; Meilenstein Vertrag |
| U-C-000037 | Geometrie und Laufparameter | A-C-00052, A-C-00053 | U-C-000036 | Validierte unveränderliche Parameter, physischer körperfester Zugpunkt samt Quellenbezug und dokumentierter deterministischer Deduplikation; Randfälle getestet |
| U-C-000038 | Gemeinsame Kraftauswertung | A-C-00054, A-C-00057 | U-C-000037 | Horizontale Bugkraft, reales Moment um S, stabile Beitragsmetadaten und Summen ohne Doppelzählung; Kraft-/Momentenreferenzen |
| U-C-000039 | Dynamik und Begrenzer | A-C-00054, A-C-00055 | U-C-000038 | Schub in allen relevanten Integratorauswertungen; richtungstreuer XY-Begrenzer nach Schritt, getrennte Impuls-/Energiediagnose; Meilenstein Physik |
| U-C-000040 | Schrittbezogene Steuerung | A-C-00056, A-C-00059 | U-C-000036 | Unveränderlicher Schritt-/Auswertungskontext, geordnete Ereignisübernahme, Wirksamkeitsbestätigung, Abbruch-/Laufendeverhalten; deterministische Grenztests |
| U-C-000041 | Ergebnisse und Wiederholbarkeit | A-C-00055, A-C-00056, A-C-00057, A-C-00058, A-C-00059 | U-C-000038, U-C-000039, U-C-000040 | Historischer Kontext, Endzustandsbeiträge, separate Korrekturdiagnose und V3-Schema; alter Aufruf V2; keine Umdeutung alter Zustände; Meilenstein Vertragstreue |
| U-C-000042 | Laufwege und Kompatibilität | A-C-00052, A-C-00056, A-C-00058, A-C-00059 | U-C-000041 | Batch-/Einzelschrittparität, alter inaktiver Pfad, Statikdiagnose und unveränderte CLI; Vertragsregressionen |
| U-C-000043 | CORE-Nachweis und GUI-Übergabe | A-C-00052 bis A-C-00060 | U-C-000042 | Gezielte Tests, reproduzierbare Referenzaufzeichnung, implementierte Schnittstellendokumentation und Übergabe an GUI; Meilenstein Lieferung |
| U-C-000044 | Fachliche Abnahme | A-C-00060 | U-C-000043 | Anforderungs-/Schritt-/Nachweismatrix vollständig, bekannte Abweichungen sichtbar; Nutzerabnahme separat bestätigen |

Die Schrittsteuerung kann nach Vertragsfreigabe unabhängig von der
Physikimplementierung entstehen. Ihre Integration erfolgt erst mit
Ergebnis-/Wiederholbarkeitsnachweisen. Es gibt keine zyklischen Abhängigkeiten.

## 4. Technische Leitplanken

- Keine GUI-Parallelphysik: Erzeugung und Begrenzung von Bewegung in CORE.
  GUI synchronisiert Bedienereignisse und zeichnet Darstellung.
- Ein Testantrieb pro simuliertem Gesamtmodell, nicht pro Element.
  Kein neues Mehrkörpersystem.
- Keine heimliche Kursstabilisierung. Echtes Hebelarmmoment, Abdrift und
  Rotation bleiben erhalten. Kraftprojektion an der jeweiligen Auswertungspose.
- Der eingefrorene Aktivzustand gilt für den gesamten Physikschritt.
  Laufende Flagänderungen greifen erst an einer noch nicht berechneten
  Schrittgrenze; keine Änderung veröffentlichter Historie.
- Aktivflag aus: weder Zusatzkraft noch Begrenzer. Abschalten ist kein
  Stopp. Aktiv bei 0 N: Begrenzer bleibt aktiv.
- Begrenzung ändert ausschließlich die horizontalen Endgeschwindigkeits-
  komponenten. Kein nachträgliches Umpositionieren, keine Änderung von
  Vertikal- oder Winkelgeschwindigkeit durch den Begrenzer.
- Ergebnisbeiträge zum gelieferten Zustand nicht mit zeitlich gemittelten
  Integratorwerten gleichsetzen. Künstlichen Korrekturimpuls separat zeigen,
  nicht als versteckte Widerstandskraft in den Kraftsummen.
- Keine globalen Laufparameter bei nachträglicher Serialisierung.
  Gespeicherter Zustand und historischer Auswertungskontext bleiben gekoppelt.
- Ungültige Eingaben und undefinierte Richtungen mit expliziter Diagnose;
  keine stillen Defaults, Ersatzrichtungen oder halben Ergebniszustände.
- Ohne Testfunktion bisherige Physik und Modellvalidierung erhalten.
  Keine neue Darstellungsdatei-Pflicht für bisher gültige Modelle.
- Keine neue Netzwerkschicht, keine Qt-/VTK-Abhängigkeit, keine
  Wasserwiderstandsphysik und kein CORE-Rendering.

## 5. Meilensteine und tatsächlicher Status

| Meilenstein | Eintritt | Überprüfbare Abnahme | Aktueller Status |
|---|---|---|---|
| Vertrag | Finalisierte Spezifikation und Plan vorhanden; Implementierung separat freigegeben | U-C-000036 technisch konkretisiert, API-/V3-Schema-/Fehler-/Testzuordnung dokumentiert | technisch umgesetzt; Schnittstellenbeschreibung aktualisiert |
| Physik | Vertrag abgenommen; Geometrie und Kräfte integriert | Kraft-/Hebelarmreferenzen, Integratorauswertungen, aktive/inaktive Grenze und Eingriffsnachweis bestehen | technische CORE-Tests bestanden |
| Vertragstreue | Physik und Schrittsteuerung verfügbar | Live-Ereignisse an tatsächlicher Grenze bestätigt; historische Ausgabe unverändert; vereinbartes Ergebnisformat und Fehlerfälle bestehen | CORE-Vertragstests bestanden; GUI-Threadintegration separat |
| Lieferung | Vertragstreue sowie Laufweg-/Kompatibilitätsnachweise bestanden | CORE-Referenzlauf, technische Dokumentation und reproduzierbare Abnehmerdaten übergeben | technisch geliefert; GUI-Koordination bestätigt API-Integration und Abnehmernachweise |
| Fachliche Abnahme | Lieferung nachgewiesen | Aktive finale Anforderungen vollständig belegt; Nutzerentscheidung und Einschränkungen dokumentiert | offen; keine Abnahme |

Ein erfolgreicher Build ersetzt keinen Physiknachweis, bestandene Tests
keine GUI-Integration und eine technische Lieferung keine Nutzerabnahme.
GUI-Integration wird im GUI-Plan nachgewiesen, nicht hier vorweggenommen.

## 6. Gezielter Test- und Nachweisplan

Vorhandene CORE-CTest-/CMake-Toolchain mit gezieltem zusätzlichem
Testziel `temporaerer_testantrieb`; keine neue Testtoolchain, keine
GUI-/DPI-Suite. Runner/Selektoren und Ergebnisse siehe Abschnitt 9.

| Nachweisbereich | Fälle und messbare Aussage |
|---|---|
| Parameter/Altpfad | Default 200 N und exakt 463/90 m/s; Grenzen 0/100000 N und 0,01/20 m/s; NaN/Inf und Werte außerhalb abweisen; durchgehend inaktiver Altpfad unverändert |
| Zugpunkt | Vorderste tatsächliche Geometrie unabhängig von Wasserlinie; mehrere gleich weit vorn gelegene Punkte; Duplikate; Quellen-/Fassungsbezug; fehlende geeignete Geometrie nach finalem Vertrag |
| Kraft und Moment | Gedrehter/gekippter einfacher Körper, horizontale Richtung und realer Hebelarm; kein künstlicher Kurszwang; positive/Nullkraft und singuläre Richtung |
| Integration | Zusatzkraft an Start-/Mittelauswertungen; analytischer einfacher Referenzkörper; unveränderte Flutungsbehandlung |
| Begrenzer | Unter/auf/über Grenze, anfängliche Überfahrt und Abschalten; XY-Norm höchstens Grenzwert plus freigegebener Toleranz; Z und omega gegenüber ungekürztem Ergebnis unverändert; korrekter Δp-/ΔE-Nachweis |
| Ereignisse | Änderungen vor/während Schritt, mehrfach an derselben Grenze, bei Abbruch und nach Laufende; bestätigter Übergang und Reihenfolge stimmen mit tatsächlich angewandtem Flag überein |
| Historie und Bilanz | Spätere Flagänderung verändert keine gespeicherten Zustände oder deren serialisierte Beiträge; Endzustandskräfte und Übergangskorrektur getrennt; Summen zählen Hebelarmmoment nicht doppelt |
| Laufwege | Batch und direktes Einzelschreiten bei identischen Ereignissen gleichwertig innerhalb freigegebener Toleranzen; alte Aufrufe und Ergebnisformate weiter lesbar; fehlender aktiver Kontext expliziter Fehler |
| Abnehmerlieferung | Kurzer echter 420er-Lauf mit Ein/Aus-Ereignissen und Begrenzung; Zustand, Konfiguration, Ereignisse und Diagnosen vollständig übergeben; keine Behauptung über gerenderte Kielwasserqualität |

Absolute/relative Toleranzen der finalen Spezifikation sind verbindlich:
Geometrie `max(1e-9 m, 1e-9 * L)`, horizontale Einheitsachsen-Projektion
`> 1e-10`, Geschwindigkeitsgrenze `max(1e-10 m/s, 1e-10 * v_max)`,
Kraft-/Momentenvergleich `max(1e-8 N bzw. N·m, 1e-9 * Referenzbetrag)`.
Diese Werte müssen in Test und Nachweis verwendet werden.
Keine Proxy-Abnahme nur durch erfolgreichen Lauf.
Gezielte bestehende Import-, Hydrostatik-, Bilanz- und Simulationstests
decken die jeweils berührten Pfade ab. Erst bei unerklärten Regressionen
auf breitere CORE-Tests eskalieren. Laufzeitänderungen bei Bedarf messen;
keine unbelegte Performancefreigabe und keine Übernahme älterer Messwerte
als Nachweis dieses neuen Umfangs.

## 7. Rückverfolgbarkeit und Nachweise

| Aktive Anforderung | Spezifikationsabschnitt | Umsetzungsschritte | Maßgeblicher Nachweis | Stand |
|---|---|---|---|---|
| A-C-00052 | 1–2 | U-C-000036, U-C-000037, U-C-000042, U-C-000043 | Default-/Altpfad-, Validierungs- und Statikfälle | technisch bestanden; Abschnitt 9 |
| A-C-00053 | 3 | U-C-000036, U-C-000037, U-C-000043 | Physische Zugpunktquelle, Deduplikation, Max-X-Mittel und Toleranzen | technisch bestanden; Abschnitt 9 |
| A-C-00054 | 3 | U-C-000038, U-C-000039, U-C-000043 | Kraft-/Hebelarm-/Singularitätsreferenzen und Integratorparität | technisch bestanden; Abschnitt 9 |
| A-C-00055 | 4 | U-C-000039, U-C-000041, U-C-000043 | Endgeschwindigkeitsgrenze, unveränderte Z/omega/Pose durch Begrenzer, Startzustand und Δp/ΔE | technisch bestanden; Abschnitt 9 |
| A-C-00056 | 5 | U-C-000036, U-C-000040, U-C-000041, U-C-000042, U-C-000043 | Ereignisgrenzen, Bestätigung, Abbruch und historische Ausgabe | CORE bestanden; GUI-Worker separat |
| A-C-00057 | 6 | U-C-000036, U-C-000038, U-C-000041, U-C-000043 | Endzustandsbeiträge, Summen, Quellenstatus und getrennte Übergangskorrektur | technisch bestanden; Abschnitt 9 |
| A-C-00058 | 6 | U-C-000036, U-C-000041, U-C-000042, U-C-000043 | V3-Pflichtkontext, alter Aufruf V2, explizite Fehler statt Kontextverlust | technisch bestanden; Abschnitt 9 |
| A-C-00059 | 5–7 | U-C-000036, U-C-000040, U-C-000041, U-C-000042, U-C-000043 | Batch-/Einzelschrittparität, Schwerpunktbezug und unveränderte CLI | CORE bestanden; GUI separat |
| A-C-00060 | 8–9 | U-C-000036, U-C-000043, U-C-000044 | Gezielte Tests mit finalen Toleranzen, 420er-Lieferung, vollständige Matrix und separate Nutzerabnahme | technische Lieferung; Nutzerabnahme offen |

Die IDEE-Herkunft und Nutzerantworten bleiben in der Spezifikation und
Entscheidungsliste dokumentiert. Die nachstehende tatsächliche Matrix
in Abschnitt 9 ergänzt die ursprüngliche Zuordnung um Belege und Grenzen.
Ein leerer Nachweis wird nicht als bestanden interpretiert.

## 8. Dokumentation, Übergabe und Abschluss

Während der Umsetzung den Status dieses einen Plans fortschreiben.
Tatsächliche Implementierung, ausgewählte Testbefehle, Ergebnisse,
Referenzdateien und bekannte Abweichungen je Arbeitspaket erfassen.
Unterstützende Belege sind keine zweite Spezifikation oder Nebenplanung.

Nach erfolgreicher technischer Umsetzung:

- [CORE-GUI-Schnittstellenbeschreibung](../Dokumentationen/Schnittstelle_Core_GUI.md)
  um das tatsächlich implementierte Schema und Bibliotheksverhalten ergänzen;
  noch geplante Felder weiterhin klar trennen.
- [Code-Dokumentation](../Code/README.md) für den freigegebenen Aufrufumfang,
  Fehlerdiagnosen und Einschränkungen aktualisieren.
- GUI erhält den nachgewiesenen CORE-Stand, Vertrags-/Fassungsangaben und
  einen kurzen reproduzierbaren Testlauf einschließlich Ereigniskontext.
  Numerische HUDs und grafische Fahrthistorien sind keine CORE-Lieferung.
- Fachliche Abnahme separat einholen; offene Abweichungen nicht als
  erledigt markieren oder eigenmächtig ins Backlog verschieben.

Technische Umsetzung und Tests sind erfolgt; kein Commit, Push, Tag oder
Release. Vorhandene fremde Änderungen bleiben erhalten.

## 9. Tatsächliche Umsetzung und technische Lieferung

Arbeitsbaum zu Beginn: benannter Branch `main`; bereits dirty waren
`Code/README.md`, `modell.hpp`, `modell.cpp`, `katalog_tests.cpp`,
`Schnittstelle_Core_GUI.md`, Modellbeschreibung und C-0.2-Plan sowie
unversionierte Lizenzdokumente, Testantrieb-Spezifikation/-Entwurfsliste
und dieser Plan. Material-/Lizenzänderungen wurden nicht verworfen oder
committet. Ausschließlich CORE bearbeitet, keine GUI-Datei.

| Schritt | Tatsächlicher technischer Stand und Beleg |
|---|---|
| U-C-000036 | Additive API in `hydrostatik.hpp`, V3-Felder/Nullabbildung und Zuordnung zu `CORE-SIM-0001` in `Schnittstelle_Core_GUI.md`; kein eingerichtetes CPPI-Register gefunden, keine neue Nummernreihe erfunden |
| U-C-000037 | Import erhält physische Massenpunkte einschließlich Punktmassen; Max-X-Auswahl aus Massen-/Volumennetzen, deterministische Deduplikation mit finaler Toleranz; Konfiguration validiert; Randfälle in `testantrieb_tests.cpp` |
| U-C-000038 | Gemeinsames `kraefte`, horizontaler 200-N-Schub, echtes Schwerpunktmoment, ein Gesamtmodellbeitrag, erwarteter Bilanzumfang; Referenzen für Gier/Rollen/Stampfen und keine doppelte Momentsumme |
| U-C-000039 | Unveränderter Integrator nutzt Schub an Start/Mitte, Endzustandsvalidierung; aktiver XY-Begrenzer nach Integration mit separatem Δp/ΔE; Nullkraft, unter/auf/über Grenze, unveränderte Pose/Z/omega und Start geprüft |
| U-C-000040 | `shared_ptr<const TestantriebKontext>` pro Zustand; übergebene Ereignisliste vor Berechnung kopiert, streng steigende Kennungen, letzte Änderung bestimmt ganzen Schritt; Erfolgszustand bestätigt tatsächliche Grenze; GUI besitzt Mutex/Transport |
| U-C-000041 | Historische V3-Serialisierung; Auswertung und erzeugender Übergang getrennt; Startübergang `null`; deaktiviert versus bekannte Nullkraft; fehlender/falscher Kontext explizit abgelehnt; Historienvergleich |
| U-C-000042 | Batch nutzt gleichen Schrittpfad und geordnete Grenzereignisse; Bytevergleich Batch/Einzelschritt bei identischem Zeitraster; V2 unverändert, aktive Statik abgewiesen, keine CLI-Erweiterung |
| U-C-000043 | Strenger MSVC-Releasebuild und 11 gezielte CTests bestanden; echte 420er-Referenz mit Begrenzung/Ein/Aus; API-Dokumentation übergeben und GUI-Integration durch GUI-Koordination bestätigt, Details im Übergabenachtrag |
| U-C-000044 | Technische Anforderungs-/Nachweismatrix vollständig; **fachliche Nutzerabnahme offen**, bestätigte GUI-Integrationsbelege im separaten GUI-Plan |

### Ausgeführte Befehle und Ergebnisse

Compilerumgebung: VS 2022 BuildTools `vcvars64.bat`, MSVC 19.44;
CMake aus `Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`,
Ninja, C++17, Release, `/W4 /WX /permissive- /fp:precise /utf-8`.
Neuer auftragsbezogener Build `build\c05`; vorhandene Buildordner unberührt.

```powershell
cmake -S Code -B build\c05 -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build\c05
ctest --test-dir build\c05 --output-on-failure -R "temporaerer_testantrieb|hydrostatik_referenzen|kraft_momentenbilanz|modell_import"
ctest --test-dir build\c05 --output-on-failure -R "simulation_gesamtlauf|cli_"
```

Finaler Build erfolgreich; erste Testgruppe **4/4**, zweite **7/7** bestanden.
Für bestehende Tests mit Scratch-Ausgaben `TEMP`/`TMP` auf den projektlokalen
Ordner `build\c05` gesetzt. Keine GUI-Suite.
Zwischenstände: erster Paritätsvergleich verwendete für direkten Restschritt
fälschlich konstant 0,01 statt identischem Batch-Zeitraster; Test korrigiert.
Eine zunächst unvollständige Volumen-Testfixture wurde durch echten
importierten Quader ersetzt. Danach beide Gruppen vollständig grün.

Reproduzierbare Aufzeichnung: `build\c05\testantrieb-420-referenz.jsonl`,
vier V3-Zustände mit physischem Modellfingerprint, Start 6 m/s unverändert,
danach aktive Grenze, Ereignis 1 aus, Ereignis 2 ein.
Erzeugung: CTest `temporaerer_testantrieb` erneut ausführen.
Zusätzlich kurzer 420er-Lauf über zehn Schritte à 0,001 s mit 200 N.
Keine Performancezusage.

### Tatsächliche Anforderungs-/Nachweismatrix und Grenzen

Die ursprüngliche Zuordnung aus Abschnitt 7 gilt unverändert; ihr damaliger
Planstatus wird durch folgende tatsächliche Nachweise ersetzt:

| Anforderung | Tatsächlicher Beleg | Status |
|---|---|---|
| A-C-00052 | Parametergrenzen/NaN/Inf, Nullkraft, inaktiver Altpfad und Statikfälle im neuen Test; bestehende Simulations-/CLI-Tests | technisch bestanden |
| A-C-00053 | Max-X-Mittel, Duplikate/nahe Punkte, physische Volumengeometrie fern der Wasserlinie, Punktmasse/fehlende Geometrie; `modell_import` | technisch bestanden |
| A-C-00054 | 200-N-Kraft und Hebelarmreferenzen bei gedrehten/gekippten Posen, singuläre Projektion, dynamische Rotation | technisch bestanden |
| A-C-00055 | Analytische Schubintegration, Grenze unter/auf/über, aktiv 0 N, inaktiv, unveränderter Start, Δp/ΔE und Z/omega/Pose-Parität | technisch bestanden |
| A-C-00056 | Mehrere geordnete Ereignisse pro Grenze, Folgeschritt, historische Ausgabe, Abbruch und unbestätigtes Ereignis nach Laufende | CORE technisch bestanden; während Schritt eintreffende GUI-Ereignisse über dokumentierten Worker-Mutex separat zu integrieren |
| A-C-00057 | Reale Beiträge/Quelle/Gruppe, Bilanz ohne Doppelzählung, Endzustandsausgabe getrennt vom Korrekturimpuls; `kraft_momentenbilanz` | technisch bestanden |
| A-C-00058 | V2/V3, Startnullabbildung, Pflichtkontext, Kontextverlust/-vertauschung und Kennungsfehler | technisch bestanden |
| A-C-00059 | Bytegleiche Batch-/Einzelschritt-Ausgabe bei identischem Zeitraster, Schwerpunktframe dokumentiert, alle sechs CLI-Vertragstests | technisch bestanden; GUI-Darstellung separat |
| A-C-00060 | 11 gezielte CORE-Tests, strenger Build, einfacher analytischer Körper und echter 420er, API-/Nachweisdokumentation | technische Lieferung; Nutzerabnahme offen |

Keine fachlichen Abweichungen eingeführt. CORE übernimmt ausschließlich
bereits atomar eingefrorene Listen; keine GUI-Synchronisation oder
Netzwerkschicht im CORE. Technische Tests ersetzen weder Prüfung tatsächlicher
GUI-Klicks während eines laufenden Workers noch Nutzerabnahme oder Rendering.

### Enger Nachweisnachtrag (27.10.2026)

Nur `testantrieb_tests.cpp` gezielt ergänzt, keine erneute Gesamtprüfung:
explizite Default-inaktive Batchkonfiguration liefert drei V3-Zustände;
schrittübergreifende doppelte Ereigniskennung wird vor Dateierstellung
abgewiesen; neue Kennung nach vorherigem Ereignisschritt wird an der
tatsächlichen Folgegrenze bestätigt; aktiver Nullkraftbatch begrenzt erst
den ersten Endzustand auf 5 m/s, nicht den Start mit 6 m/s.
Analytisch konstruierte Rotation erreicht bei positiver Kraft erst am
Schrittende die singuläre Bugprojektion: Schritt wird abgelehnt,
vorherige Zustandsserialisierung bleibt bytegleich. Geänderter
Modellfingerprint mit gespeichertem Kontext wird von Serialisierung,
Einzelschritt und gemeinsamer Kraftauswertung abgewiesen.

Ausgeführt: `cmake --build build\c05 --target testantrieb_tests` mit
MSVC `/W4 /WX`, danach
`ctest --test-dir build\c05 --output-on-failure -R "^temporaerer_testantrieb$"`.
**1/1 bestanden (1,23 s).** Referenzaufzeichnung erneut erzeugt,
Flags der vier 420er-Zustände: `true,true,false,true`; Grenzen:
Start nicht anwendbar, dann `wirksam=true,false,true`.
GUI-Integrationsbelege bleiben separat; nachfolgende Übergabebestätigung
siehe unten. Fachliche Nutzerabnahme weiterhin offen.

### Tatsächliche GUI-Übergabe zu U-C-000043

Implementierungsfreigabe: ausdrücklicher Nutzerauftrag vom 27.10.2026
um 22:16 für beide Projekte, ohne Git-Aktionen.
Die GUI-Koordination meldet anschließend die erfolgreiche Integration
der gelieferten CORE-API und des V3-Kontexts; damit ist die technische
GUI-Übergabe **erfolgt**, nicht nur vorbereitet.

Gemeldete Abnehmerbelege (durch GUI-Koordination ausgeführt, nicht als
erneute CORE-Testausführung dargestellt):

- Produktive Release-EXE gebaut.
- Acht fokussierte GUI-Tests bestanden, zusammen 22,39 s:
  `water_motion`, `graphics_settings`, `graphics_dialog`,
  `core_integration`, `force_display`, `navigation`,
  `navigation_camera`, `water_motion_render`.
- GPU-Nachweise mit 420 und Folkeboot, jeweils 101 echte CORE-Zustände
  bei 200 N und Geschwindigkeitsgrenze 0,1 m/s.
- Historische V3-Ereignisse, Live-Aktivflag und während des Laufs gesperrte
  Zahlenparameter im MainWindow geprüft.

Maßgebliche GUI-Details verbleiben im GUI-Plan G-0.5. Weitere GUI-Tests
für einfachen Konturkörper und Fehlerisolation sind laut Koordination
noch in Ergänzung und werden hier nicht vorweggenommen.
U-C-000044 bleibt hinsichtlich **fachlicher Nutzerabnahme offen**.
Keine Git-Aktionen oder zusätzlichen Codeänderungen bei diesem Nachtrag.
