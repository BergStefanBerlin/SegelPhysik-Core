# Spezifikation CORE: Temporärer Testantrieb und Laufsteuerung

**Status: finalisiert und auf ausdrücklichen Auftrag technisch implementiert;
gezielte CORE-Tests bestanden, GUI-Übergabe erfolgt; Nutzerabnahme offen.**  
**Stand:** 27.10.2026 · **CORE-Zielversion:** C-0.5 (Nutzerzuordnung)  
**Entwurfspunkte:** [Temporärer Testantrieb](Entwurfspunkte/Temporaerer_Testantrieb.json)

**Umsetzungsplanung:** [einziger Plan C-0.5](../Umsetzungsplaene/Umsetzungsplan_C-0.5.md),
finalisiert mit Umsetzungsschritten; Implementierung am 27.10.2026 um
22:16 ausdrücklich beauftragt. GUI-API-Integration anschließend bestätigt.

Dieses Dokument ist die einzige CORE-Spezifikation des temporären
Bug-Testantriebs. Es konkretisiert den CORE-Lieferbedarf der finalisierten
[GUI-Spezifikation](../../GUI/Spezifikationen/Spezifikation_GUI_Testantrieb_und_Fahrtdarstellung.md),
insbesondere A-G-00038 bis A-G-00040 und A-G-00045.
Der [GUI-Plan G-0.5, Abschnitt 9](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.5.md)
ist ein Abnehmerplan, kein CORE-Umsetzungsplan und keine Versionsfreigabe
für CORE. GUI G-0.5 bedeutet nicht automatisch CORE C-0.5.

Die Nutzerantworten vom 27.10.2026 finalisieren die fachlichen Verträge.
Die Entscheidungshistorie bleibt in der verlinkten Liste erhalten.
Neue Anforderungen folgen projektweit auf A-C-00051; die Nummerierung
beginnt nicht mit C-0.5 neu. Finalisierung ist kein Implementierungs- oder Git-Auftrag.

| Kennung | Maßgeblicher Vertrag und Abnahmekriterium | Herkunft | Status |
|---|---|---|---|
| A-C-00052 | Parameter, temporärer Gesamtmodellumfang, inaktiver Altpfad und Statikablehnung; Abschnitte 1–2 und Fälle Parameter/Altpfad | IDEE-C-00027 | technisch implementiert/getestet |
| A-C-00053 | Physischer Bugzugpunkt, Deduplikation, Toleranzen und Quellenbezug; Abschnitt 3 und Fall Buggeometrie | IDEE-C-00028 | technisch implementiert/getestet |
| A-C-00054 | Horizontale Bugkraft und echtes Hebelarmmoment ohne Kursregelung; Abschnitt 3 und Fall Kraft/Moment | IDEE-C-00027, IDEE-C-00028 | technisch implementiert/getestet |
| A-C-00055 | Integratorauswertungen, XY-Begrenzer, Startzustand und separater Δp-/ΔE-Nachweis; Abschnitt 4 und Fälle Integratorparität/Begrenzer | IDEE-C-00029 | technisch implementiert/getestet |
| A-C-00056 | Schrittgrenzen, geordnete Ereignisse, Bestätigung und historische Kontexte; Abschnitt 5 und Fall Live-Ereignis | IDEE-C-00030 | CORE implementiert/getestet; GUI-Integration bestätigt, Belege separat |
| A-C-00057 | Reale Testbeiträge und Bilanz ohne Doppelzählung, Endzustand getrennt vom Übergang; Abschnitt 6 und Fälle Kraft/Moment/Integratorparität | IDEE-C-00031 | technisch implementiert/getestet |
| A-C-00058 | Ergebnisversion 3 für Testläufe, verpflichtender Kontext, alter Aufruf Version 2 und explizite Fehler; Abschnitt 6 und Fälle Validierung/Kompatibilität | IDEE-C-00031 | technisch implementiert/getestet |
| A-C-00059 | Bibliotheks-Batch-/Einzelschrittparität, GUI-Datenbezüge und kein neuer CLI-/Renderumfang; Abschnitte 5–7 und Fall Kompatibilität | IDEE-C-00030, IDEE-C-00032 | CORE implementiert/getestet; GUI-Übergabe erfolgt, Nutzerabnahme offen |
| A-C-00060 | Gezielte Abnahme mit verbindlichen Toleranzen, CORE-Lieferung und Freigabegrenze; Abschnitte 8–9 und Fall Integration | IDEE-C-00029, IDEE-C-00032 | technische Nachweise im Plan; Nutzerabnahme offen |

## 1. Ausgangsbefund und notwendige Änderungen

Geprüfte aktuelle Einstiegspunkte:

- [Laufoptionen und Ausgabe](../Code/include/segelphysik/simulation.hpp):
  keine Testantriebsparameter oder schrittbezogenen Steuerereignisse.
- [Dynamikschritt](../Code/src/hydrostatik.cpp): Kräfteauswertungen zu
  Start- und Mittellage verwenden Hydrostatik/Schwerkraft; kein zusätzlicher
  Bugschub und kein Geschwindigkeitsbegrenzer.
- [Zustandsserialisierung](../Code/src/simulation.cpp): Kräfte werden für
  den gelieferten Zustand aus der Hydrostatik neu ausgewertet. Ein nur im
  Integrator ergänzter Schub würde hier fehlen.
- [Physikmodell](../Code/include/segelphysik/modell.hpp): enthält Masse,
  Schwerpunkt, Trägheit, Volumenmeshes und Flutränder, aber nicht mehr alle
  eingelesenen Massennetzpunkte. Reine Darstellungsdateien sind bisher
  ausdrücklich keine Physikquelle.
- [GUI-Liveadapter](../../GUI/Code/src/core_adapter.cpp): ruft den
  CORE-Einzelschritt auf und serialisiert gespeicherte Zustände später
  erneut. Ein veränderliches globales Aktivflag wäre deshalb für Rückblick
  und Bilanz falsch.

Damit sind CORE-Änderungen an Kraftauswertung, Schrittintegration,
Laufkonfiguration und Ergebnis-/Steuerungskontext nötig. Die bestehende
[Kraft-/Momentenbilanz](Spezifikation_Kraft_und_Momentenbilanz.md) und
[Hydrostatikspezifikation](Spezifikation_Hydrostatikkern.md) werden gezielt
erweitert, nicht durch eine GUI-Parallelphysik ersetzt.
Ohne beauftragte Testfunktion bleiben deren bestehende Verträge unverändert.

Nicht im CORE: Schaum, Bugwellengrafik, Raster, technische Fahrspur,
Kameraführung, Setup-Dialog oder neue numerische Instrumente. Keine
Widerstands-/Segelantriebsphysik, keine neue Strömung und keine Kursregelung.

## 2. Bestätigte Laufparameter und Geltung

| Parameter | Default / gültiger Bereich | Verhalten |
|---|---|---|
| Aktivflag | aus | vor Laufstart und während Dynamik schaltbar |
| Kraftbetrag | 200 N; endlich, 0 bis 100000 N | während Lauf unveränderlich |
| Horizontale Wasserrelativ-Grenze | exakt 10 kn = 463/90 m/s; endlich, 0,01 bis 20 m/s | während Lauf unveränderlich |

Der Testantrieb gilt einmal für das simulierte Gesamtmodell, nicht einmal
je Bauteil. Keine dauerhafte Modelldateiänderung oder Modellspeicherung der
Testparameter. Die GUI startet nach Programmneustart mit Aktivflag aus.
Der CORE arbeitet ausschließlich mit expliziter Laufkonfiguration, nicht
mit Benutzerregistry oder Grafikprofilen.

Durchgehend inaktiver Testantrieb muss den bisherigen Physikpfad erhalten:
keine neue Kraft, kein Begrenzer, keine zusätzliche Modellablehnung wegen
fehlender Bugdaten. Ungültige ausdrücklich gelieferte Optionen werden
dagegen mit vorhandenen CORE-Fehlermechanismen zurückgewiesen.

Aktiven Testantrieb im Gleichgewichtsmodus ausdrücklich
ablehnen; keine still ignorierte Schubkonfiguration oder statische
Geschwindigkeitsregelung. Bestehende Statik ohne Testfunktion unverändert.

## 3. Angriffspunkt, Kraft und Moment

### Fachlich festgelegt

Der körperfeste Zugpunkt liegt an der am weitesten in Körper-+X
gelegenen tatsächlichen Modellgeometrie, unabhängig von Wasserlinie oder
Wasserkontakt. Bei mehreren gleich weit vorn gelegenen Geometriepunkten
wird ihre Mitte benutzt. Keine Auswahl nach Elementnamen und kein erfundener
Boundingbox-Eckpunkt. Der Antrieb bleibt auch ohne Wasserkontakt zuschaltbar.

Für Pose R (Modell nach Welt, Quaternion xyzw) ist
`b = R * (1,0,0)`, `h = (b.x,b.y,0)` und
`F_welt = Kraftbetrag * h / |h|`.
Der Zugpunkt wird als Punkt rotiert und verschoben, die Kraft nur rotiert.
Zur Beitragsausgabe ist `F_modell = R^T * F_welt`.
Das tatsächliche Moment um den Gesamtschwerpunkt S lautet
`M_S_modell = (Zugpunkt_modell - S_modell) × F_modell`.

Keine Unterdrückung dieses Moments, keine künstliche Kursstabilisierung
oder Abdriftkorrektur. „Gerade nach vorn“ bezeichnet die horizontale
Kraftrichtung, nicht eine garantierte gerade Fahrbahn.
Ist die horizontale Projektion undefiniert, keine erfundene Ersatzrichtung.

### Verbindliche CORE-Konkretisierung

Maßgeblich sind ausschließlich die physikalischen Massen- und Volumennetze
einschließlich Punktmassen. Beim Import die dafür erforderlichen tatsächlichen
Geometriepunkte beziehungsweise Extrema erhalten. Keine Darstellungs-OBJ
als Physikabhängigkeit öffnen. Eine abweichende sichtbare Bugspitze ändert
den physischen Zugpunkt nicht; kein automatischer Wechsel zwischen Quellen.
Geometrieherkunft und Modellfassung müssen nachvollziehbar sein.
Geometrisch deduplizierte Punkte der Max-X-Auswahl arithmetisch mitteln.
Fehlt bei aktivem positivem Schub geeignete physische Geometrie, den Lauf
mit Diagnose ablehnen, nicht auf Schwerpunkt oder Darstellung ausweichen.

Verbindliche Toleranzen: `L` als größte physische Modellabmessung;
X-Gleichheit und Punktdeduplikation mit
`max(1e-9 m, 1e-9 * L)`, horizontale Einheitsachsen-Projektion gültig für
`|h| > 1e-10`. Diese lokalen Toleranzen sind keine globale
Rundungsvorschrift oder Freigabe anderer Geometrieverfahren.
Bei positivem aktivem Schub und singulärer Richtung Schritt mit Diagnose
ablehnen, ohne einen halben Zustand zu veröffentlichen. Nullkraft darf als
bekannter Nullbeitrag ohne Richtungsnormalisierung ausgewertet werden.

## 4. Integration und künstliche Geschwindigkeitsgrenze

Die Testkraft ist in allen relevanten Kraftauswertungen des verwendeten
Integrators zu berücksichtigen, insbesondere an Start- und Mittellage.
Das Ereignisflag wird vor dem Schritt eingefroren und darf nicht zwischen
diesen Auswertungen wechseln. Der zugrunde liegende Integrator wird nicht
nebenbei durch ein anderes Verfahren ersetzt.

Nach dem regulären Physikschritt, aber vor Veröffentlichung des nächsten
Zustands, gilt ausschließlich bei aktivem Flag:

1. Horizontale Wasserrelativgeschwindigkeit `u = (v.x,v.y,0)` bestimmen;
   für diesen Umfang ruht das Wasser im Weltsystem.
2. Falls `|u| > v_max`, XY-Anteile mit `v_max/|u|` skalieren.
3. Vertikalgeschwindigkeit, Winkelgeschwindigkeit und Pose unverändert
   gegenüber dem regulären Ergebnis belassen.
4. Vorher-/Nachherwerte und Eingriff protokollieren.

Bei inaktivem Flag wirken weder Schub noch Grenze. Abschalten ist kein
Sofortstopp und setzt bereits erreichte Fahrt nicht zurück.
Ein aktives Flag bei Kraftbetrag 0 lässt die Grenze weiterhin aktiv.
Startzustand unverändert ausgeben, erste Begrenzung nach dem
ersten Schritt; keine rückwirkende Geschwindigkeitsänderung bei t=0.
Die Grenze gilt für veröffentlichte Schritt-Endgeschwindigkeiten, nicht als
nachträgliche Begrenzung der bereits integrierten Weglänge dieses Schritts.

Der Begrenzer ist ein künstlicher Testeingriff, keine Widerstandskraft.
Als Nachweis mindestens Geschwindigkeitsvektoren vor/nach Begrenzung,
linearer Korrekturimpuls `Δp = m * (v_nach - v_vor)` und Änderung
der translatorischen kinetischen Energie dokumentieren.
Keine Darstellung dieses Impulses als gewöhnliche Kraft in N ohne
explizites Zeitintervall und gesonderten fachlichen Vertrag.

## 5. Live-Flag und reproduzierbare Steuerereignisse

Die Bibliothek unterstützt eingefrorene Parameter und schrittbezogenen
Aktivzustand gleichermaßen im Batch-Lauf und bei direkten Einzelschritten.
Die genaue typsichere C++-API wird im ersten Umsetzungsschritt technisch
konkretisiert, ohne diesen fachlichen Vertrag zu ändern; kein neuer
Netzwerk-/WebSockettransport und kein Qt-/VTK-Zwang im CORE.

An der Grenze vor Beginn des nächsten noch nicht berechneten Schritts
werden verfügbare Flagänderungen atomar in eine deterministisch geordnete
Ereignisliste übernommen. Während der Berechnung ankommende Änderungen
wirken frühestens am folgenden Schritt. Ein Flag gilt für den ganzen Schritt.

Jedes akzeptierte Ereignis erhält Reihenfolgenkennung, neuen Flagwert,
wirksame Übergangskennung (von Schritt n nach n+1) und Simulationsstartzeit
dieses Übergangs. Eine Bestätigung nennt den tatsächlichen Wirksamkeits-
punkt, nicht nur den Zeitpunkt des GUI-Klicks. Mehrere Ereignisse an derselben
Grenze bleiben in Reihenfolge nachvollziehbar; das letzte bestimmt das Flag.
Unwirksam gebliebene Änderungen nach Laufende werden nicht als angewandt
bestätigt. Abbruch und Fehler lassen bereits veröffentlichte Zustände und
angewandte Ereignisse bestehen.

Der GUI-Worker kann den Transport/Synchronisationsbehälter besitzen,
aber nur eingefrorenen Kontext an den CORE übergeben. Kein veränderliches
globales Flag in der nachträglichen Serialisierung. Aufzeichnung und
Wiedergabe verwenden den historischen Kontext; aktueller Schalterstand
darf nie alte Kräfte oder Zustände umdeuten.

## 6. Beiträge, Bilanz und Ergebnisvertrag

Gemeinsame Kraftauswertung für Integration und Endzustandsausgabe
wiederverwenden. Der heutige alleinige Hydrostatik-Neuaufruf in
`zustand_json` genügt nicht mehr. Es müssen voneinander getrennt vorliegen:

- Am gelieferten Zustand ausgewertete Kräfte und Momente einschließlich
  Testantrieb mit dem dokumentierten historischen Auswertungskontext.
- Kontext des Übergangs, der den Zustand erzeugte: angewandtes Flag,
  Ereigniskennung, dt und tatsächlicher Begrenzungseingriff.

Endzustandskräfte sind weiterhin keine Behauptung über den exakten
Integrator-Mittelwert. Nachträgliche Serialisierung desselben gespeicherten
Kontexts muss dieselbe fachliche Aussage liefern.

Verbindliche Beitragsmetadaten: stabile Quelle und Gruppe
`testantrieb`, stabiler Kraftbeitrag `testantrieb.bug.kraft`,
Modellframe, realer Zugpunkt, Einzelbeitrag, Moment um S.
Aktiv und Betrag 0 liefert bekannten Nullbeitrag; deaktiviert wird eindeutig
als deaktiviert geführt, nicht als fehlender oder gültiger unbekannter Beitrag.
Nicht pauschal einem beliebigen Modellbauteil zurechnen.
Zulässige Darstellung als Gesamtmodellbeitrag, erwarteter Quellenumfang
und etwaige zusätzlich benötigte Momentenbeiträge müssen mit dem bestehenden
Bilanzschema abgestimmt werden; kein doppeltes Zählen des Hebelarmmoments.

Summen umfassen die tatsächlich berücksichtigten kontinuierlichen Beiträge.
Der Geschwindigkeits-Korrekturimpuls steht separat daneben. Ein
`bilanzstatus` darf Vollständigkeit des deklarierten Kraftumfangs ausdrücken,
aber nicht verschweigen, dass ein künstlicher Schrittimpuls wirkte.

Verpflichtender Kontext für Ergebnisversion 3:

| Inhalt | Einheit / Semantik |
|---|---|
| Konfiguration | temporärer Modus, Kraft N, Grenzwert m/s, Geometrieherkunft/-fassung |
| Historisches Flag | für Auswertung beziehungsweise erzeugenden Schritt wirksam |
| Zugpunkt und Schwerpunkt | Modellframe, m; gemeinsamer Pose-/Modellstand |
| Schub | Welt- und/oder eindeutig deklarierter Modellvektor, N |
| Übergang/Ereignisse | Schrittbezug, Simulationszeit s, deterministische Reihenfolge |
| Begrenzung | wirksam ja/nein; v vor/nach in Welt-m/s, Δp in N·s, ΔE in J |

Testläufe mit explizitem Testkontext liefern Ergebnisversion `"3"`, auch
in ihren zeitweise inaktiven Abschnitten. Alte Aufrufe ohne Testkontext
liefern weiterhin Version `"2"`. Bestehende Ergebnisfelder behalten ihre
Bedeutung; Version 3 ergänzt den verpflichtenden Testkontext.
Die GUI muss Version 3 ausdrücklich unterstützen und ältere Aufzeichnungen
weiter lesen. Ein versionierter optionaler Zusatzblock in Version 2 ist
nicht die gewählte Variante.

Konfiguration, historischer Auswertungszustand und Geometrieherkunft sind
für Testläufe Pflicht. Für den Startzustand existiert noch kein erzeugender
Schritt: Übergang und Begrenzung sind dort ausdrücklich nicht anwendbar.
Bei einem Schritt ohne Begrenzungseingriff sind Vor-/Nachgeschwindigkeit
gleich und Korrekturimpuls/Energieänderung null; dies ist kein fehlender
Messwert. Geometrisch nicht benötigte Angaben bei inaktivem Testantrieb
dürfen nicht durch erfundene Zugpunkte ersetzt werden.
Die genaue technische Feldbenennung, Nullabbildung nicht anwendbarer Daten
und bestehende Fehlerkatalogzuordnung werden vor Codeänderungen im ersten
Umsetzungsschritt dokumentiert und durch Vertragstests abgesichert.
Dies ist technische Ausarbeitung, keine offene fachliche Entscheidung.
Alte Aufrufe ohne Testkontext bleiben kompatibel; ein aktiver
Testlauf darf niemals über einen Altaufruf still als reine Hydrostatik
ausgegeben werden. Ein fehlender erforderlicher Kontext ist ein Fehler.
Batch-Lauf und direkter Einzelschritt verwenden dieselbe Bibliotheksphysik.
Keine neuen CLI-Argumente oder CLI-Livebedienung in C-0.5; die bestehende
CLI ohne Testkontext bleibt unverändert.

## 7. Datenbedarf der GUI-Fahrtdarstellung

Pose, Geschwindigkeit, Zeit und bekannte Modellmasse/-schwerpunkt sind
weitgehend vorhanden und sollen wiederverwendet werden. Bezug der
Geschwindigkeit ist ausdrücklich die Weltgeschwindigkeit des Gesamtschwerpunkts,
nicht die Ableitung des wandernden Konstruktionsursprungs.
Schwerpunkt und dessen Modellbezug müssen für den Abnehmer eindeutig
verfügbar sein; spätere Massenbezugswechsel dürfen nicht als Fahrt zählen.

Wasserlinien-Konturen liegen derzeit in der GUI-Geometrieauswertung;
ein neuer CORE-Konturexport ist für diesen Auftrag nicht automatisch
erforderlich. Die GUI darf die vorhandene Darstellungskontur für ihre
grafischen Anker weiterverwenden. CORE liefert keine Schaumvertices,
Markenabstände oder Spurhistorie.
Neue notwendige physikalische Kontaktdaten wären ausdrücklich als separater
Vertrag zu prüfen, nicht still aus Darstellungsgeometrie zu erfinden.

## 8. Gezielte Abnahme und Rückwärtskompatibilität

Nachweisfälle; tatsächliche Ausführung und Grenzen siehe C-0.5-Plan:

| Fall | Nachweis |
|---|---|
| Altpfad | Durchgehend aus: gleiche Physikzustände, Flutung und bisherige Ergebnisaussagen; kein neuer Bugdatenzwang |
| Buggeometrie | Unterschied zu Wasserlinienbug, mehrere Max-X-Punkte, Duplikate, rein punktförmige/fehlende geeignete Geometrie und Geometrieherkunft |
| Kraft/Moment | 200-N-Test, rotierte/gekippte Pose, korrekte horizontale Projektion, realer Hebelarm, Rotation nicht unterdrückt |
| Integratorparität | Schub an allen relevanten Auswertungsstellen; Ergebnisbeiträge zum gelieferten Zustand konsistent |
| Begrenzer | Unter/auf/über Grenze, 0 N bei aktivem Flag, Ausschalten, Start oberhalb Grenze; XY-Betrag, Z/omega unverändert gegenüber ungekürztem Ergebnis, korrekter Impuls/Energienachweis |
| Live-Ereignis | Wechsel vor/während Schritt, mehrere Änderungen, Abbruch/Laufende; deterministische Bestätigung und historische Serialisierung |
| Validierung | NaN/Inf, negative/zu große Kraft, unzulässige Grenze, singuläre Bugprojektion; eindeutige Diagnose und kein halber Zustand |
| Kompatibilität | Altaufrufe und vorhandene Ergebnisversionen, aktiver Kontext nicht verloren, Batch-/Einzelschrittparität |
| Integration | Kurzer echter 420er-Lauf und einfacher analytischer Körper; GUI-Rendering separat im GUI-Plan |

Verbindliche Toleranzen für numerische Vergleiche:
Geschwindigkeitsgrenze `max(1e-10 m/s, 1e-10 * v_max)`, Kraft-/Momenten-
Vergleiche `max(1e-8 N, 1e-9 * |F_Referenz|)` beziehungsweise
`max(1e-8 N·m, 1e-9 * |M_Referenz|)`. Keine globale Rundung.
Keine unnötige volle GUI-/DPI-Suite. Bestehende CORE-Testziele gezielt nutzen.
Keine neue Performancezusage ohne gemessenen Nachweis.

## 9. Freigabe und Lieferstatus

Diese Spezifikation und der zugehörige Plan wurden am 27.10.2026 auf
ausdrückliche Nutzerantwort finalisiert. Anschließend hat der Nutzer die
Implementierung beider Projekte am 27.10.2026 um 22:16 ausdrücklich
beauftragt, ohne Git-Aktionen.
API-/Schema-/Fehlerausarbeitung und technische Nachweise stehen im einen
Plan C-0.5 und in der Schnittstellenbeschreibung. Technische CORE-Tests
sind keine Nutzerabnahme. Die GUI-Koordination bestätigt inzwischen
erfolgreiche API-Integration, produktiven Release-Build, acht fokussierte
GUI-Tests und GPU-/MainWindow-Nachweise; diese GUI-Belege bleiben im
separaten GUI-Plan, die tatsächliche CORE-Übergabe steht im C-0.5-Plan.
Eine fachliche Nutzerabnahme wird damit nicht behauptet.
Keine Tag-/Veröffentlichungsfreigabe.
Die Umsetzung gehört ausschließlich in den einen Plan C-0.5,
U-C-000036 bis U-C-000044. GUI G-0.5 bleibt separat.
Kein bestehender Physik-/Modellvertrag wird durch dieses Dokument still
als bereits geändert markiert. Keine Codeänderung, kein Commit/Push/Tag/Release.
