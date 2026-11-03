# Spezifikation Core: Allgemeine Fremdkraft im dynamischen Lauf

**Status:** Teilfinalisiert fuer den Core-Mindestvertrag in Abschnitt 8.
Priorisierte Ursachenanalyse und Messlaeufe sind nach Abschluss der
GUI-Messungen und ausdruecklicher Freigabe des Core-Messfensters am
03.11.2026 abgeschlossen; Ergebnisse siehe C-0.9-Umsetzungsplan,
Abschnitt 4.3. Weitergehende Matrix, GUI-Abnahme und Implementierung von
Optimierungen bleiben offen.
**Stand:** 03.11.2026, Mindestumfang ausdruecklich beauftragt ab 18:43 Uhr.
**Arbeitszielversion:** C-0.9; keine Tag- oder Release-Freigabe.

Einzige Spezifikation dieses neuen Themas. Entwurfspunkte:
[Dynamische_Fremdkraft.json](Entwurfspunkte/Dynamische_Fremdkraft.json).
GUI-Gegenstueck:
[gemeinsame Rechenbedienung, Abschnitt 13](../../GUI/Spezifikationen/Spezifikation_GUI_Statisches_Modell.md#13-entwurf-identische-rechenbedienung-fuer-statik-und-dynamik).
Der bestehende
[stationaere Vertrag](Spezifikation_Stationaeres_Fahrtgleichgewicht.md)
und der
[temporaere Testantrieb](Spezifikation_Temporaerer_Testantrieb.md)
werden weder umbenannt noch als Ersatz fuer diesen Vertrag verwendet.

Die Abschnitte 1 bis 7 halten Bestand, Quellen und spaetere Pruefideen fest.
Verbindlich und zur Umsetzung freigegeben ist ausschliesslich der in
Abschnitt 8 abgegrenzte Core-Mindestvertrag. Ursachenanalyse und
Core-Messlaeufe sind nach Abschnitt 9 abgeschlossen und im C-0.9-Plan
nachgewiesen. Optimierungen bleiben ohne separaten Auftrag gesperrt.

## 1. Ziel und gepruefter Bestand

Die aus der statischen GUI bekannte allgemeine Fremdkraft soll auch in
einer zeitabhaengigen dynamischen Simulation wirken. Die GUI gibt Eingaben
vor; der Core berechnet Kraefte, Momente, Translation und Rotation.
Kein Gleichgewicht suchen und keine stationaere Geschwindigkeit vorgeben.
Windphysik, Segelmodell und Autopilot sind nicht Bestandteil.

Gepruefter Ausgangsstand:

- [hydrostatik.hpp](../Code/include/segelphysik/hydrostatik.hpp):
  stationaere Fremdkraftkonfiguration vorhanden; dynamischer Testantrieb
  ist dagegen ein spezieller Bugzug mit eigenem Begrenzer. Dynamischer
  Zustand und Kraftauswertung besitzen noch keinen allgemeinen
  Fremdkraftkontext fuer diesen Auftrag.
- [simulation.hpp](../Code/include/segelphysik/simulation.hpp):
  Dauer, Schrittweite, dynamischer Startzustand und Bewegungsereignisse
  sind bereits vorhanden. Dafuer ist kein neues Integrationsverfahren
  erforderlich.
- Der GUI-Livepfad startet den Core schrittweise. Nur Batch-Unterstuetzung
  waere deshalb keine vollstaendige Integration.

## 2. Vorgeschlagener Eingangsvertrag

Ein Rechenfall enthaelt:

| Eingabe | Bedeutung |
|---|---|
| aktiv | Fremdkraft fuer diesen Lauf ein/aus |
| Kraft X/Y/Z | endliche, vorzeichenbehaftete Komponenten in N |
| Ansatzpunkt X/Y/Z | endliche Modellkoordinaten in m |
| Elementstellungen | gueltige IDs und Werte nach vorhandenen Core-Bewegungsmetadaten |
| Dauer/Schrittweite | bestehende Laufparameter und bestehende Validierung |

**Vorschlag, noch fachlich zu bestaetigen:** Kraftvektor modellfest wie
im aktuellen statischen Modell, Ansatzpunkt modellfest gemaess Nutzerbild.
Keine stillschweigende Interpretation eines Weltvektors als Modellvektor.
Die GUI-Praezisierung vom 03.11.2026, 18:22 Uhr verlangt nahezu
identische statische/dynamische Eingaben und eine erhaltene
Koerpermodell-/Weltmodell-Auswahl. Deren Bedeutung ist noch zu klaeren:
Die bisherige GUI-Implementierung steuert die Kamera. Daraus folgt
kein freigegebener weltfester Kraftvertrag und keine neue
Core-Physikoption. Falls stattdessen Kraftbezug gemeint ist, diesen
Entwurf vor Finalisierung um beide Frames und deren Abnahme erweitern.
Die endgueltigen API-Namen und Typen werden erst im freigegebenen Plan
festgelegt; dieser Entwurf legt keine konkrete exportierte Schnittstelle
oder CPPI-Kennung vorzeitig fest.

Konfiguration beim Laufstart einfrieren. Keine Live-Aenderungsereignisse
fuer Fremdkraft oder Stellungen als Teil dieses neuen Umfangs.
Bestehende anderweitige Live-Schnittstellen bleiben kompatibel.
Jeder Zustand beziehungsweise seine rekonstruierbaren Laufdaten muss
den tatsaechlich wirksamen Rechenfall eindeutig zuordnen koennen.
Alte Laeufe duerfen nicht von spaeteren GUI-Eingaben abhaengen.

Fehlender/neuer Kontext ist fuer bisherige Aufrufer neutral: ohne
angeforderte Fremdkraft bleiben bestehende dynamische Ergebnisse erhalten.
Nichtendliche Komponenten, unbekannte Element-IDs und Werte ausserhalb
der Core-Grenzen werden mit konkreter Diagnose abgewiesen; keine
stillen Nullwerte, kein Clamping und kein Ignorieren angeforderter Daten.
Aktive Nullkraft ist gueltig und von deaktiviert unterscheidbar.

## 3. Dynamische Wirkung und Moment

Fuer den vorgeschlagenen modellfesten Vertrag gilt zu jedem ausgewerteten
Zustand mit Core-Orientierung R und Modellursprung x:

```text
F_welt = R * F_modell
p_welt = x + R * p_modell
M_S_modell = (p_modell - S_modell) x F_modell
```

S stammt aus dem vorhandenen Core-Massen-/Momentenbezug; kein angenommener
Drehpunkt und kein Drehmoment um den falschen Modellursprung.
Die Kraft muss zusammen mit ihrem Moment im bestehenden dynamischen
Kraftpfad und Integrator wirksam werden, nicht erst nachtraeglich im JSON.
R an den vom bestehenden Verfahren ausgewerteten Posen verwenden.
Keine zusaetzliche diskrete Geschwindigkeitskorrektur.

Deaktiviert bedeutet keine Fremdkraft und kein Fremdkraftmoment;
bereits vorhandene Geschwindigkeit wird dadurch nicht geloescht.
Die allgemeine Fremdkraft besitzt weder die horizontale Projektion des
Testantriebs noch dessen Geschwindigkeitsgrenze oder Bug-Zugpunkt.
Keine versteckte Aktivierung des Testantriebs. Dessen separate bisherige
Vertraege bleiben fuer andere Aufrufer erhalten. Fuer eine gleichzeitig
explizit angeforderte Kombination ist vor Umsetzung die Zulassungsregel
zu finalisieren; der neue GUI-Rechenfall fordert nur die Fremdkraft an.

## 4. Feste Ruder-/Schwertstellungen

Vorhandene Core-Bewegungsmechanik verwenden. Stellungen wirken bereits
im ersten ausgegebenen Zustand und jeder relevanten Kraftauswertung;
nicht erst am ersten spaeteren Live-Stellereignis.
Alle danach ausgegebenen Zustaende zeigen dieselben vorgegebenen
Stellungen, solange kein unabhaengig beauftragter anderer Vertrag wirkt.
Keine neue Stellungsoptimierung und keine GUI-Werte als Core-Defaults
hartkodieren. Existierende Grenzen, Einheiten und Transformationsvertraege
bleiben massgeblich.

## 5. Beitraege, Bilanz und Ausgabevertrag

Der aktive Fremdkraftbeitrag ist eindeutig identifizierbar und traegt
Quelle, Kraftvektor, Frame, Ansatzpunkt und Moment um S nach dem
bestehenden Ergebnisvertrag. Inaktiv/null/gueltig bleiben nachvollziehbar.
Genau einmal in die passenden Gruppen- und Gesamtsummen aufnehmen.
Keine zusaetzliche Anzeigezeile nochmals in die Bilanz addieren.
SDP-/Ersatzdruckpunktdarstellung nur gemaess bestehendem Vertrag
ableiten; keinen neuen physikalischen Segeldruckpunkt behaupten.

Batchlauf, schrittweiser GUI-Livepfad und historische Zustandsausgabe
muessen denselben Rechenfall und dieselben Beitraege liefern.
Ausgabeversionierung und Kompatibilitaet explizit festlegen, bevor neue
Pflichtfelder eingefuehrt werden; alte Daten nicht nachtraeglich als
mit Fremdkraft berechnet ausgeben.

## 6. Geplante Abnahme

### 6.1 Nicht freigegebener Entwurf einer weitergehenden Matrix

Nachpruefung auf Nutzerauftrag vom 03.11.2026, 18:28 Uhr, vor den
Performance-Messlaeufen. Vollstaendig meint die fachlichen
Physik-/Datenvertraege, nicht die Uebernahme eines stationaeren Solvers
als dynamischen Integrator. Die folgende Matrix ist eine
spaetere Pruefidee und kein freigegebener Umsetzungsschritt. Die
Befundspalte dokumentiert ausschliesslich den Stand vor der hier
freigegebenen Mindestumsetzung.

| Erkenntnis aus stationaerer Umsetzung | Dynamisch angepasster Vertrag | Befund vor Mindestumsetzung |
|---|---|---|
| Eine physische Geometrie fuer feste Stellungen | Masse, Schwerpunkt, Traegheit, Benetzung, Auftrieb, Angriffspunkte und Visualisierung verwenden dieselben Stellungen ab erstem Zustand | `kraefte` erzeugt ein bewegtes physisches Modell; `dynamikschritt` verwendet fuer I und CG-Fortschreibung noch das Grundmodell. Konsistenz ist nicht vollstaendig nachgewiesen und muss vor Abnahme geschlossen werden. |
| Rotation veraendert lokale Anstroemung | An jedem Angriffspunkt Schwerpunktgeschwindigkeit plus omega x Hebelarm minus Fluidgeschwindigkeit verwenden | Gemeinsamer Hydrodynamikpfad vorhanden; Referenztests mit gedrehter/gekraengter Pose und mehreren festen Stellungen erforderlich. |
| Schwerpunkt ist kein angenommener Drehpunkt | Welt-/Modellachsen, Ursprungs-/Schwerpunktgeschwindigkeit und Momente um S konsistent halten | Dynamik integriert CG und rekonstruiert Modellursprung; Bezug auf bewegte Massenverteilung ist noch zu pruefen. |
| Zentripetal-/Kreisel-/Zusatzmassenterme | Volle zeitabhaengige Translation/Rotation, effektive Traegheit und Kirchhoff-Terme, keine stationaere Nullbeschleunigungsannahme | Kirchhoff-/Kreiselpfad vorhanden; Paritaet zum stationaeren relativen Gleichgewicht noch nachzuweisen. |
| Sechs Bilanzkomponenten statt nur Fz/Mx/My | Alle drei Kraft- und Momentenkomponenten bestimmen Beschleunigungen; aeussere Summe darf dynamisch ungleich null sein | Dynamischer Integrationspfad vorhanden; neue Fremdkraft darin noch nicht implementiert. Keine statische Konvergenzanzeige auf dynamische Summen anwenden. |
| Lasten unveraenderlich und frameklar | Eingaben einfrieren, Kraft an jeder ausgewerteten Pose transformieren; keine stationaere Invarianzbedingung fuer beliebige zeitabhaengige Bahn verlangen | Allgemeine dynamische Fremdkraft fehlt. |
| Reale Geometrie, Flutung und Datenfehler ernst nehmen | Vorhandene Flutungs-/Diagnosevertraege erhalten, keine Ersatzwerte fuer fehlende Daten, keine Eingabeveraenderung fuer ein schoenes Ergebnis | Flutung und Fehlerpfade vorhanden; stellungsabhaengige Konsistenz und neue Konfiguration gezielt pruefen. |
| V6-Ausgabe und ehrliche Resultierende | Vollstaendige Einzelbeitraege, Summen, S-/Framebezug und signierte Welt-Kursdrehrate; Null, ungueltig, deaktiviert unterscheiden | V6 und Kursdrehrate vorhanden; neuer Fremdkraftbeitrag sowie historischer Kontext fehlen. |
| Schwierige Ruderfaelle brauchen physikalisch richtige Bilanz | Stationaere 420er-Referenzen mit 0/+-12/+-24/+-36 Grad als Vergleichsfamilie nutzen, ohne dynamische Stabilitaet vorauszusetzen | Stationaere Suchinitialisierung ist keine Dynamikverbesserung. Fortentwicklung eines Referenzzustands mit Schrittweitenverfeinerung und begrenztem Zeitfenster nachweisen. |
| Solver-Toleranzen sind nicht Integrationsfehler | Zeitintegration ueber dt, dt/2, dt/4 gegen analytische/hochaufgeloeste Referenz pruefen; Fehlergrenzen vorher festlegen | Eigener Konvergenznachweis fuer den neuen Vertrag offen. Kein Anspruch auf Einhaltung stationaerer Residuentoleranzen bei beliebiger Dynamik. |

Die stationaere Hydrostatik-Startsuche, Gierwurzelabtastung,
Trust-Region-/Newton-Initialisierung und deren Iterationsbudget nicht
in jeden dynamischen Schritt kopieren. Sie sind Loesersuche fuer
Gleichgewicht, keine zeitabhaengige Physik. Stationaere Konvergenz
beweist keine dynamische Stabilitaet. Der neue GUI-Lauf startet weiterhin
nicht heimlich aus dem Ergebnis einer solchen Gleichgewichtssuche.

### 6.2 Weitergehende, noch nicht freigegebene Prueffaelle

Die hier gelisteten erweiterten Matrix-/Konvergenzfaelle sind nicht Teil
der Mindestfreigabe. Die tatsaechlich ausgefuehrten Mindesttests stehen
in Abschnitt 8 und im C-0.9-Umsetzungsplan. Diese weitergehenden Tests
bleiben bis zu einer erneuten Nutzerfreigabe offen:

- Null/inaktiv: neutral gegen den bisherigen dynamischen Lauf.
- Kraft am Schwerpunkt: Fremdkraftmoment exakt null.
- Seitlicher Ansatzpunkt: bekanntes Kreuzprodukt mit richtigem
  Vorzeichen; Beispiel Hebel (0, 1, 0) m, Kraft (10, 0, 0) N ergibt
  Moment (0, 0, -10) N m. Bezug S beruecksichtigen.
- Gedrehte/gekraengte Pose: Weltkraft und Ansatzpunkt aus derselben
  Orientierung; keine Verwechslung von Modell- und Weltachsen.
- Isolierter kontrollierter Dynamikfall: vorgegebene Kraft und Moment
  veraendern tatsaechlich Translation/Rotation, nicht nur Anzeige.
- Feste Ruder-/Schwertstellungen vor erstem Zustand und Kraftschritt;
  ungueltige IDs/Grenzwerte werden diagnostiziert.
- Gleicher Start, Konfiguration und Schrittfolge: Batch und Live
  numerisch konsistent innerhalb vorab festgelegter Toleranzen;
  Rueckblick unveraendert nach Aenderung spaeterer GUI-Eingaben.
- Gesamtsummen enthalten den Beitrag genau einmal; kein Begrenzer aus
  dem Testantrieb; bisherige Statik und Laeufe ohne neuen Kontext erhalten.
- Physische Stellungen: verschobene Masse mit nichttrivialem Schwerpunkt
  und Traegheit, nicht nur optisch gedrehtes Ruder testen. Alle
  Integratorstufen, Ergebnis-Kraftauswertung und GUI-Pose muessen
  denselben Bezug verwenden.
- Stationaeres relatives Gleichgewicht als dynamischen Referenzstart
  verwenden und nur die lokal erwartete Fortentwicklung pruefen;
  keine dauerhafte Stabilitaet oder automatische Konvergenz behaupten.
- Referenzfamilie schwieriger Ruderwinkel mit Schrittweitenverfeinerung
  statt Uebernahme von Suchheuristiken; Eingaben bleiben unveraendert.

Offene Entscheidungen ausschliesslich in der Entwurfspunkteliste.
Der anschliessend ausdruecklich beauftragte
[Core-Planentwurf](../Umsetzungsplaene/Umsetzungsplan_C-0.9.md)
ordnet die Arbeitspakete und Nachweise; Zielversion C-0.9 ist ein
noch zu bestaetigender Vorschlag. Keine Release-Aktion beauftragt.

## 7. Nachtrag: Versionsvergleich nach Mindestintegration

Nutzerauftrag vom 03.11.2026, 18:37 Uhr, danach konkretisierter
Planauftrag: zuerst GUI und minimale korrekte Core-Anpassungen fuer
den neuen Rechenfall, dann gemeinsame Tests und historischer
Performancevergleich. Das Go vom 03.11.2026, 19:23 Uhr hob den Stopp fuer
die Ursachenanalyse auf; nach Abschluss der GUI-Messungen wurde das
Core-Messfenster ausdruecklich freigegeben. Der Vergleich ist in
Abschnitt 4.3 des C-0.9-Umsetzungsplans dokumentiert.
Dieser Auftrag startet keine vorgezogene Codeoptimierung und hebt offene
fachliche Entscheidungen nicht auf.

Die aktuelle Dynamik gegen kompatible aeltere Staende bis etwa
27.10.2026 vergleichen. Gleiche Modellgeometrie, Anfangszustaende,
Schritte und gemeinsam verfuegbare Physikfunktionen verwenden.
Core-Schrittzeit, Laden und Ausgabe getrennt messen; keine
Echtzeit-Wartezeit als Rechenzeit ausweisen. Reproduzierbare Rohdaten,
vollstaendige Commit-Hashes, numerische Ergebnisse und Streuung
festhalten. Neue Physikfeatures von Regressionen bei gleichem Umfang
unterscheiden. Nicht vergleichbare Schemas/APIs kennzeichnen,
keine neue Fremdkraft in alte Versionen zurueckportieren, um
scheinbare Vergleichbarkeit herzustellen.

Historische Quellen getrennt bauen, aktuelle Arbeitsstaende und Modelle
nicht ueberschreiben. Ursachen anhand der geaenderten dynamischen
Aufrufkette belegen. Ein jetziger Einansicht-/Vieransichtvergleich
ist kein historischer Core-Performancevergleich.

## 8. Freigegebener Core-Mindestvertrag (03.11.2026, 18:43 Uhr)

Nur die folgenden Anforderungen sind fuer C-0.9 finalisiert und zur
Core-Umsetzung freigegeben. Kraft und Ansatzpunkt sind modellfest; eine
GUI-Kameraauswahl aendert diesen Core-Vertrag nicht. Neue Konfigurationen
werden beim Start eingefroren. Fuer diese erste Stufe ist eine
gleichzeitig aktive Fremdkraft und ein gleichzeitig aktiver Testantrieb
ungueltig und wird mit Diagnose abgewiesen. Die API und CPPI-IDs sind vor
der Implementierung im Schnittstellenregister eingetragen.

### A-C-00126: Dynamischer Eingangs- und Kontextvertrag

Ein optionaler Laufkontext enthaelt ein Aktivflag, einen endlichen
modellfesten Kraftvektor in N und einen endlichen modellfesten Ansatzpunkt
in m. Seine Konfiguration ist unveraenderlich und wird am Laufstart an
Modellfassung und ausgegebenen Zustand gebunden. Aktive Nullkraft ist
gueltig und von deaktivierter Kraft unterscheidbar. Ungueltige Werte,
verlorene Pflichtkontexte und gleichzeitig aktive Testantrieb-/Fremdkraft-
Kontexte werden mit konkreter Diagnose abgewiesen. Fehlt der neue Kontext,
bleiben bisherige Aufrufer und Ergebnisse unveraendert. Vorgesehene feste
Bewegungsstellungen gelten bereits vor dem ersten Zustand und bleiben
unveraendert, solange kein bestehendes unabhaengiges Bewegungsereignis
wirkt.

### A-C-00127: Dynamische Kraft-, Momenten- und Beitragswirkung

Die konfigurierte Kraft wird an jedem vom bestehenden Integrator
ausgewerteten Zustand mit dessen Pose vom Modell- in den Weltrahmen
transformiert. Ihr Moment wird aus modellfestem Ansatzpunkt und Kraft
bezogen auf den tatsaechlichen Schwerpunkt des physischen Modells
berechnet. Kraft und Moment wirken in allen Kraftauswertungen des
Integrators, nicht erst in der Ergebnisprojektion. Ergebnis und Summen
enthalten genau einen identifizierbaren Beitrag mit Modellvektor,
Ansatzpunkt, Frame und Moment; deaktiviert, aktive Nullkraft und aktive
Kraft bleiben unterscheidbar. Der Beitrag wird nicht doppelt bilanziert.
Zustaende mit diesem Kontext verwenden Ergebnisvertragsversion `"7"` und
geben den eingefrorenen Kontext samt Modellfassung, Auswertungsschritt und
-zeit aus. Zustaende ohne neuen Kontext behalten ihre bisherige
Vertragsversion und Ausgabeform.

### A-C-00128: Konsistenter Bezug bewegter physischer Modelle

Bei festen Elementstellungen verwenden Kraftauswertung, Integrator,
Translation des Modellursprungs, Gesamt-Schwerpunkt, Traegheit und
Ergebnisbezug dieselbe bewegte physische Modellfassung. Verschobene
Massen und ihre Traegheitsanteile sind damit ab dem Start und an allen
Integratorstufen konsistent. Die bestehende Bewegungsmechanik wird
wiederverwendet; kein neuer Integrator und keine Stellungsoptimierung.

Weitere Entwurfspunkte und geplante Matrixfaelle sind nicht Teil dieser
Freigabe. Die GUI-Integration und gemeinsame GUI-Pruefung bleiben offen.
Die priorisierte Ursachenanalyse der historischen Laufzeit ist seit
03.11.2026, 19:23 Uhr ausdruecklich freigegeben; Core-Messungen wurden
nach Abschluss der GUI-Messungen und ausdruecklicher Freigabe des
Messfensters durchgefuehrt. Analyse und Messungen duerfen offene
Physiknachweise weder schliessen noch als Abnahme ausweisen.
Implementierung, Produktionscode-Aenderungen und Optimierungen bleiben
bis zu einem separaten ausdruecklichen Auftrag blockiert.

## 9. Analysefreigabe (03.11.2026, 19:23 Uhr)

Freigegeben ist ausschliesslich die Ursachenanalyse des Laufzeitverlaufs
von `C-0.7` (`ebca994770b216a749e18917d2ae387de1612dff`) ueber `C-0.8`
(`d6d57c7dc5084dc3599f7208b3c63c35446b3345`) bis `C-0.9`
(`3fa197135a0858ee525d3e4bc8273242e0ee97c2`): Quellpfad-Inventar,
Modell-/Element-/Meshkomplexitaet, getrennte statische und dynamische
Aufrufpfade, JSON-/Inspektionsaufwand sowie qualitative, physikalisch
bewertete Ursachenhypothesen. Keine Code- oder Modelldaten-Aenderung,
kein Zurueckportieren von Physik und kein Performance-Fix ist damit
autorisiert.

Die Core-Messungen mit identischen 420er- und Folkeboot-Eingaben,
`dt = 0,01 s`, festem Simulationshorizont und ungebremstem Lauf erfolgten
nach dem GUI-Messfenster. Je Build: gleicher Release-Compiler und
gleiche Einstellungen, ein Warmlauf und fuenf Messlaeufe; Median und
Spannweite sind berichtet. Start/Laden, reine Integrationsschritte,
Einzelzustand-JSON und CLI-Gesamtausgabe sind getrennt ausgewiesen.
Die erneute GUI-Messung mit Scratch-Harness (Matrix `gui-perf-final3`)
enthaelt 1 Warmup + 5 x 6 Snapshots, alle fuenf einfachen Flags ein/aus,
Single/Four/Hidden und beide Workspace-Pfade. Sie ist kein Core-
Laufzeitnachweis; ihre Ergebnisse stehen im G-0.10-Plan, Abschnitte
13.7-13.8. Die Core-Messungen stehen separat im C-0.9-Plan, Abschnitt
4.3; sie wurden nach GUI-Abschluss und ausdruecklicher Slot-Freigabe
seriell durchgefuehrt.
Historische Modelldaten wurden nicht veraendert. Das aktuelle Schema 3
ist fuer C-0.7 nicht ladbar; ein getrennt ausgewiesener Dreiversionslauf
mit archivierten `0.1-entwurf`-Eingaben ist im C-0.9-Plan dokumentiert.

Der Vergleich prueft gemeinsame Physikpfade, behauptet aber keine
vollstaendige Statik-Dynamik-Paritaet. Hydrostatik, reale Geometrie,
Masse/Schwerpunkt und passende Kraftbeitraege sind relevante
Gemeinsamkeiten. Zeitintegration, zeitabhaengige Pose und Geschwindigkeit,
hydrodynamische Daempfung/Zusatzmasse, mehrere Kraftauswertungen je Schritt
sowie Ausgabe pro Zeitschritt sind legitime dynamische Mehrarbeit.
Statische Gleichgewichts-/Kurvenfahrtsuche ist dagegen ein separater
iterativer Solver und kein dynamischer Zeitschritt. Die offenen
Rotations-, Bewegungs-, Flutungs-, Frame- und Integrationskonvergenz-
Nachweise aus Abschnitt 6 bleiben unveraendert offen.
