# Core-Spezifikation: sailforce

**Stand:** 17.11.2026 · **Status:** FINAL auf ausdruecklichen Nutzerauftrag.
**Zielversion:** C-0.25. Umsetzung, Tests, Integration und Nutzerabnahme
sind offen. Dieser Auftrag finalisiert Dokumente, beauftragt aber keine
Code-, Modell-, Commit-, Push-, Tag- oder Release-Aktion.

**Einziger Versionsplan:** [C-0.25](../Umsetzungsplaene/Umsetzungsplan_C-0.25.md).
**Herkunft:** [Entwurfspunkte und Entscheidungen](Entwurfspunkte/sailforce.json).
Die dortigen Vorschlaege zu Eingaben, gekoppelten Lasten und Ergebnissen
sind mit diesem Finalisierungsauftrag in der nachfolgenden Fassung
angenommen. Historische Entwurfsentscheidungen bleiben nachvollziehbar.

## 1. Gegenstand, bestehende Vertraege und Grenzen

sailforce verarbeitet das bestehende
[Windmodell](Spezifikation_Windmodell.md) und vorbereitete
Einzel-Segelmodelle aus SegelMacher. Es ersetzt weder das Windmodell noch
die bestehende [Gesamtbilanz](Spezifikation_Kraft_und_Momentenbilanz.md).
Die Fremdkraft bleibt unveraendert und unabhaengig aktivierbar erhalten.
Einzelsegel werden intern gekoppelt berechnet; eine aerodynamische
Interferenz zwischen Fock und Grosssegel wird nicht modelliert.

Die bestehende [Segelmontage](Spezifikation_Segelkonfigurator.md) bleibt
massgeblich fuer Einbau, Stellwinkel und Originalseite. Unterschiede
zwischen deren Montageachsen und den Achsen eines vorbereiteten Modells
werden explizit aufgeloest, nicht aufgrund gleichlautender Achsennamen
gleichgesetzt. Keine neue Tuchmechanik, Trimmoptimierung, Stall-Hysterese,
automatische Schotfuehrung oder GPU-Pflicht.

Die einzige verbindliche Definition des aerodynamischen SDP steht unter
[A-C-00178](Spezifikation_Lateraldruckpunkt.md#6-c-025-aerodynamischer-seitenkraft-sdp).
Die bisherigen LDP- und Fremdkraft-Ersatz-SDP-Vertraege bleiben erhalten.
Die [GUI-Anforderungen](../../GUI/Spezifikationen/Spezifikation_GUI_Kraft_und_Momentendarstellung.md#3-sailforce-darstellung-und-bedienung-g-025)
und ihr [Plan](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.25.md) bleiben
getrennte GUI-Arbeitsprodukte.

## 2. Finale Anforderungen

Alle folgenden Anforderungen sind fachlich final, aber noch nicht umgesetzt
oder abgenommen. A-C-00178 wird hier nur als thematischer Verweis gefuehrt.

| ID | Gegenstand | Herkunft |
|---|---|---|
| A-C-00169 | Modellidentitaet sailforce | IDEE-C-00113 |
| A-C-00170 | Bestehendes Windfeld als Eingabe | IDEE-C-00114 |
| A-C-00171 | Gesamtbilanz und parallele Fremdkraft | IDEE-C-00115 |
| A-C-00172 | Lokale dreidimensionale Anstroemung | IDEE-C-00116 |
| A-C-00173 | Gekoppelte Streifenlasten und Summen | IDEE-C-00117 |
| A-C-00174 | Vollstaendige konsistente Ergebniszustaende | IDEE-C-00118 |
| A-C-00175 | Effizienz und Echtzeitfaehigkeit | IDEE-C-00119 |
| A-C-00176 | Wiederverwendbare Streifengrenzlinien | IDEE-C-00120 |
| A-C-00177 | Versionierter Ergebnis- und Schnittstellenvertrag | IDEE-C-00121 |
| A-C-00178 | Seitenkraft-SDP, siehe Druckpunkt-Spezifikation | IDEE-C-00122 |
| A-C-00179 | Dynamik und stationaeres Fahrtgleichgewicht | IDEE-C-00123 |
| A-C-00180 | Explizite Gueltigkeits- und Fehlerbehandlung | IDEE-C-00124 |
| A-C-00181 | Aktivierung je Segel | Finalisierungsentscheidung 17.11.2026 |
| A-C-00182 | Live-quasistatische Segelverstellung | Finalisierungsentscheidung 17.11.2026 |
| A-C-00183 | Explizite Lufteigenschaften je Lauf | Finalisierungsentscheidung 17.11.2026 |
| A-C-00184 | Datenzuordnung, Produzentengate und Formparitaet | Finalisierungsentscheidung und Vertragspruefung 17.11.2026 |

### A-C-00169: Modellidentitaet

Das physikalische Modell heisst `sailforce`. Quelle, Segelidentitaet und
Beitragsrolle bleiben von bestehenden Fremdkraft- und Windanzeigedaten
unterscheidbar. **Abnahme:** Ergebnisinventar und Diagnosen identifizieren
sailforce und das jeweilige Segel eindeutig.

### A-C-00170: Windfeld

Das bestehende wahre Windfeld einschliesslich Hoehenprofil, Weltkompass,
Richtungsmodi und zeitbezogener Eingaben bleibt unveraendert. sailforce
erweitert seine Auswertung um die benoetigten Streifenpunkte, nicht um ein
zweites Windmodell. **Abnahme:** Gleiche Position, Zeit und Einstellungen
liefern dieselben Windwerte wie die vorhandene Windfunktion; insbesondere
keine Welt-Z-Komponente des wahren Windes und keine neue Mindestgrenze
fuer den scheinbaren Wind.

### A-C-00171: Gesamtbilanz

Alle Streifenlasten gehen genau einmal in die bestehende Kraft- und
Momentenbilanz ein. Die Fremdkraft darf gleichzeitig wirken und bleibt
ein separat identifizierbarer Beitrag. Segel- und sailforce-Summen sind
Sichten auf Einzelbeitraege, keine zusaetzlichen Lasten.
**Abnahme:** Einzelbeitraege rekonstruieren die Solverbilanz desselben
Zustands, mit und ohne Fremdkraft; fuer algebraische Transformation und
Summation gelten `rtol=1e-10`, `atol=1e-9` in SI wie im Bilanzvertrag.

### A-C-00172: Lokale Anstroemung

Je Streifen-Abfragepunkt P gilt im Weltrahmen:

```text
v_P = v_S + omega_welt kreuz (P_welt - S_welt)
v_scheinbar(P) = v_wahr(h(P)) - v_P
```

Der scheinbare Stroemungsvektor wird anschliessend in das aktuelle
Segelmodell-System transformiert. Einbau, Stellwinkel, Bootspose und
gepruefte Formparitaet werden konsistent genau einmal angewendet.
Die Bibliothek erhaelt den vollstaendigen lokalen Vektor in m/s, nicht
einen Herkunftswinkel oder einen auf horizontal reduzierten Vektor.
Die quasistatische Stellungsannahme aus A-C-00182 erzeugt keinen weiteren
Stellgeschwindigkeitsanteil.
**Abnahme:** Analytische Translation, Rotation, Kraengung, geneigtes
Vorstag und Nullrelativwind; Vektoren und Punkte bleiben korrekt getrennt.

### A-C-00173: Gekoppelte Lasten

Alle Abfragewinde eines Segels werden gemeinsam an die vorbereitete
SegelMacher-Laufzeitbibliothek uebergeben. Keine unabhaengigen Streifen-
Lookups und kein alter uniformer Gesamtsegel-Lookup als Ersatz.
Normalbetrieb verwendet die freigegebene 48-Moden-Fassung.
Je Streifen werden die dimensionale Kraft und das vollstaendige Moment
um seinen Abfragepunkt P verarbeitet. Bei eigentlicher Rotation R:

```text
F_i,Modell = R * F_i,lokal
M_i,S = R * M_i,P + (P_i,Modell - S_Modell) kreuz F_i,Modell
```

Bei Formspiegelung gilt zusaetzlich die gepruefte Polar-/Axialbehandlung
aus A-C-00184. Alle Segel- und Gesamtsummen verwenden denselben aktuellen
Gesamtschwerpunkt S. **Abnahme:** Direkter Bibliotheksvergleich, reines
Moment, verschobener Bezug, mehrere Segel, Gegenkraefte und Summe ohne
Doppelzaehlung. Ein lokal windstiller Abfragepunkt wird nicht eigenmaechtig
auf Nullast gesetzt; die Bibliotheksinterpolation und Kopplung bleiben
massgeblich. Bei ueberall Nullwind sind alle aerodynamischen Lasten null.

### A-C-00174: Ergebniszustaende und asynchroner Abruf

Der Core rechnet ohne GUI-Abhaengigkeit und unabhaengig von deren Abruf-
oder Bildrate. Er stellt unveraenderliche, konsistente Ergebniszustaende
bereit. Die GUI ruft sie asynchron ab; ein langsamer oder fehlender
Darstellungsabruf darf weder Rechenschritte auslassen noch die Physik
takten. Vorhandene Aufzeichnungsanforderungen bleiben bestehen.

Je veroeffentlichtem Zustand sind mindestens gemeinsam zugeordnet:
Lauf-/Modellfassung, Zeit und Schritt, Pose, Schwerpunkt S, bestaetigte
Wind-, Luft-, Aktivierungs- und Stellungsfassung; je Segel/Streifen
stabile IDs, lokale Abfragepunkte mit expliziter Transformation,
Modell-Kraft und vollstaendiges Moment um S; Segel-/Gesamtsummen,
SDPs mit Status, LDP aus bestehendem Vertrag sowie die passende
Liniengeometriefassung. Die lokale Modellidentitaet/Manifestfassung ist
mitgefuehrt. Fuer Segel- und sailforce-Summenkraftpfeile liefert der Core
S explizit als Darstellungsbezug; das ist kein physischer Druckpunkt.
Kraefte an Darstellungsbezugspunkten werden als solche
bezeichnet, nicht als gemessene Druckangriffspunkte.

Ergebnisse gehoeren zum ausgegebenen Endzustand, nicht zu einem zufaelligen
internen Integratorzwischenzustand. Notwendige Endzustandsauswertungen
zaehlen zum Laufzeitbudget. Fehler und letzter gueltiger Zustand bleiben
unterscheidbar. **Abnahme:** Verzogerter/ausbleibender GUI-Abruf,
gleichzeitiger Leser, Pause, Modellwechsel, Abbruch, Liveaenderung und
Wiedergabe ohne gemischte Fassungen oder nachtraegliche Mutation.

### A-C-00175: Effizienz und Echtzeit

Algorithmen werden unter Einhaltung fachlicher und numerischer Vertraege
auf Rechengeschwindigkeit ausgewaehlt. Modelle einmal laden;
geometrieabhaengige Vorbereitung/Faktorisierung und unveraenderliche
Geometrie wiederverwenden. Kein Datei-I/O im laufenden Lastpfad und keine
vermeidbare Wiederaufbereitung pro Schritt. Interne Allokationen und
Serialisierung sind zu messen, nicht ungeprueft als abwesend auszugeben.
Die qualitative GPU-Abschaetzung fuer wenige Segel, 10-35 Streifen je
Segel und normalerweise 48 Moden faellt negativ aus: CPU-Bibliothek
verwenden. Bei spaeter positiv ausfallender technischer Abschaetzung
GPU nutzen; ein spezieller CPU/GPU-Vergleich ist nicht gefordert.

Verbindlicher Nachweis auf vorhandener **Ryzen 5 8600G / Radeon 760M
unter Windows**, nativer Release-Build:

- 420, Folkeboot und Optimist jeweils einzeln, mit allen im Referenzfall
  aktiven Physikmodellen und allen zugeordneten Segeln; keine isolierte
  sailforce-Zeit als Gesamtnachweis.
- Nach abgeschlossener Vorbereitung je 60 s gueltige Dynamik mit
  0,01 s Simulationsschritt: 6000 Schritte in hoechstens 60 s Wandzeit.
  Alle internen Integratorauswertungen, Endzustandsauswertungen und
  Ergebnisbereitstellung einschliessen.
- Fuer denselben integrierten GUI-Lauf gilt
  [A-G-00172](../../GUI/Spezifikationen/Spezifikation_GUI_Kraft_und_Momentendarstellung.md#a-g-00172-echtzeit-und-fluessige-darstellung).
  Core-Echtzeit muss auch dort erhalten bleiben.
- Modell-/Bibliotheksfassungen, Startzustand, Wind, Luftdaten,
  Segel-/Ruderstellungen, Fremdkraft und uebrige Optionen dokumentieren.
  Ein ungueltiger oder vorzeitig abgebrochener Lauf besteht nicht.
- Schrittzeiten mit Median, p95 und Maximum sowie Rueckstand zur
  simulierten Zeit berichten. Lade-/Vorbereitungszeit und Speicher
  separat messen. Messumgebung und Stoerungen offen ausweisen.

Das Echtzeitziel gilt fuer Dynamik. Stationaere Konvergenz und
Startvorbereitung werden separat gemessen, nicht als 10-ms-Garantie
ausgegeben. Tatsaechliche Leistungsnachweise und offene GUI-Gates stehen
im Versionsplan, nicht in dieser fachlichen Anforderung.

### A-C-00176: Streifengrenzlinien

Der Core liefert die tatsaechlichen 3D-Schnittlinien der vom geladenen
Segelmodell definierten Streifengrenzen auf der zugeordneten Segelflaeche.
Stabile Segel-/Grenz- und angrenzende Streifen-IDs, Koordinatenrahmen,
Geometriefassung und Transformation sind eindeutig. Keine neuen
gefuellten Teilmeshes und keine unabhaengige Einteilung in der GUI.
Reine Lageaenderungen transformieren vorhandene Linien; eine neue
Quellgeometrie invalidiert den Cache. Fehlende/inkonsistente Geometrie
ergibt eine Diagnose statt erfundener Linien.
**Abnahme:** Dreieckiges und viereckiges Segel, geneigter Einbau,
Stellwinkel und Bootspose; Linien liegen auf der passenden Oberflaeche,
Grenzen entsprechen dem Segelmodell und unveraenderte Schnitte werden
nicht je Zeitschritt neu berechnet. Keine Rueckwirkung auf Lasten.

### A-C-00177: Vertraege und Versionierung

Den bestehenden [Bilanzvertrag](Spezifikation_Kraft_und_Momentenbilanz.md),
die [Core-GUI-Schnittstelle](../Dokumentationen/Schnittstelle_Core_GUI.md)
und das [Register](../Dokumentationen/Schnittstellenregister_Core.md)
konsistent erweitern, keinen Parallelvertrag einfuehren.
Konkrete neue exportierte Datentypen/Funktionen erhalten vor ihrer
Implementierung einzelne CPPI-IDs oberhalb des dann aktuellen Hoechststands.
Im Vertragsgate die tatsaechliche Ergebnisversion, Featurekennung,
Pflichtfelder, Status, Ownership, Abruf-/Threadgrenzen und Migrationsmatrix
festlegen. Keine Versionsnummer ohne Pruefung bereits vorhandener Formate
wiederverwenden. Alte Ergebnisse bleiben ohne erfundene Segeldaten lesbar;
unbekannte/fehlerhafte neue Pflichtwerte werden explizit gemeldet.
**Abnahme:** Typisierter und serialisierter Pfad liefern denselben
Snapshot; Bestandsformate und Verbraucher sind gegen die dokumentierte
Kompatibilitaetsmatrix getestet, neue Interfaces einzeln registriert.

### A-C-00179: Rechenarten

Die vorhandenen Dynamik- und stationaeren Fahrtgleichgewichtspfade werden
erweitert, kein neuer Kraengungssolver eingefuehrt. Lasten werden fuer
jeden tatsaechlich ausgewerteten Integrator-/Solverzustand neu bestimmt.
Die stationaere Suche verwendet eingefrorene Laufparameter, aber
zustandsabhaengige scheinbare Anstroemung und die vollstaendige
Sechskomponentenbilanz. Eine quasistatische Bedienaenderung waehrend einer
stationaeren Suche gilt erst fuer einen neuen Rechenfall.
Reine Hydrostatik bleibt separat ohne aktive Segellast verfuegbar;
ein ausdruecklich angefordertes reines Hydrostatikproblem mit aktivem
sailforce wird als widerspruechliche Eingabe abgelehnt, nicht still
umgedeutet. **Abnahme:** Beide Fahrtpfade zeigen zustandsabhaengige
Lasten; Hydrostatikregression unveraendert, keine Kraft einfrieren.

### A-C-00180: Gueltigkeit und Fehler

Windmodellgrenzen werden nicht an Segelmodellgrenzen angepasst. Alle
benoetigten Segelauswertungen muessen innerhalb der manifestierten
Gueltigkeit liegen, derzeit lokaler Wind 0 bis einschliesslich 50 m/s
sowie die Modellbereiche fuer Luftdichte und Viskositaet. Interne
Interpolations-/Integrationsstellen des Produzenten unterliegen dessen
Pruefung. Nichtendliche Werte, ungueltige Windhoehen, Modell-/Integritaets-
und Numerikfehler liefern Kontext mit Segel-/Streifen-ID soweit vorhanden,
Wert und Grenze beziehungsweise Ursache.

Keine Begrenzung, Nullkraft, stille Deaktivierung oder Erfolgsmeldung.
Bei Dynamikfehler keine Fortschreibung ueber den ungueltigen Schritt;
letzten gueltigen Zustand mit erkennbarer Fehlerlage erhalten.
Ein ungueltiger stationaerer Suchzustand ist kein gueltiges Ergebnis;
nur regulaer zulaessige weitere Suchschritte duerfen fortsetzen.
Ohne gueltige konvergierte Loesung bleibt der Solverstatus entsprechend
offen/fehlgeschlagen. **Abnahme:** Grenzwerte selbst, knapp ausserhalb,
NaN/Inf, Nullwind, Korruption und Integratorzwischenzustandsfehler.
Ein unbestimmbarer SDP allein ist kein Lastberechnungsfehler.

### A-C-00181: Aktivierung

Jedes Segel ist separat aktivierbar. Ohne gespeicherte/bestaetigte Auswahl
ist sailforce fuer ein explizit zugeordnetes Segelmodell zunaechst an.
Vorhandene ausdrueckliche Auswahl hat Vorrang. Ein Segel ohne Zuordnung
hat keinen aktiven sailforce-Pfad; der fehlende Modellbezug wird sichtbar
ausgewiesen. Eine dennoch angeforderte Aktivierung wird abgelehnt.
Eine vorhandene, aber defekte Zuordnung wird nicht als fehlend
verschluckt. Aktive Modelle muessen vor Start gueltig geladen sein.
Hydrostatik verwendet eine ausdrueckliche Konfiguration ohne Segellast.
Die Fremdkraft wird durch Aktivierung weder entfernt noch automatisch
abgeschaltet. Aenderung der Aktivierung gilt fuer den naechsten Lauf;
Liveaenderungen sind in diesem Ausbau auf den Stellwinkel beschraenkt.
**Abnahme:** Kein/ein/zwei Segel, gespeicherte Auswahl, Neustart,
fehlendes/defektes Modell und parallele Fremdkraft.

### A-C-00182: Live-quasistatische Verstellung

Bestaetigte Stellwinkel fuer Grosssegel und Fock werden als vollstaendige
Konfigurationsfassung ab einer eindeutigen naechsten Schrittgrenze
uebernommen. Alle internen Auswertungen dieses Schritts verwenden diese
Fassung. Winkelkonvention und Drehachsen bleiben gemaess Segelmontage
erhalten. Lasten, Geometrie, Masse-/Schwerpunktbezuege soweit betroffen
und Ergebnisse verwenden konsistente Stellungen.

Die Aenderung ist eine quasistatische Neupositionierung, keine simulierte
Stellbewegung: keine aus UI-Spruengen abgeleitete Geschwindigkeit,
keine Stellkinetik oder Stellimpulse. Aerodynamische Lastspruenge durch
neue Anstroemung bleiben erhalten. Keine lineare Zwischenbewegung erfinden.
Alte Zustaende bleiben unveraendert; bestaetigte, ausstehende und
abgelehnte Eingaben sind unterscheidbar. Beim Laufende nicht mehr
angewandte Aenderungen ausdruecklich melden.
**Abnahme:** Beide Winkelvorzeichen, Nullstellung, schnelle Folge,
Schrittgrenze, Pause/Weiterlauf, Ablehnung und historische Wiedergabe.

### A-C-00183: Lufteigenschaften

Explizite Laufparameter mit Vorgaben `rho=1.225 kg/m^3` und
`nu=1.5e-5 m^2/s`. Beide muessen endlich, positiv und innerhalb der
Bereiche aller aktiven Segelmodelle liegen. Sie sind vor Laufstart
aenderbar und waehrend eines Laufs konstant; kein neues Wetter- oder
Temperaturmodell. Vorgaben, bestaetigte Werte, Einheiten und Fassung
werden mit dem Lauf gespeichert und an die Bibliothek uebergeben.
**Abnahme:** Vorgaben, geaenderte Werte, manifestierte Grenzen,
Bereichsschnitt mehrerer Segel, ungueltige Werte und Wiederanlauf.

### A-C-00184: Daten- und Produzentengate

Pro Segel sind Element-ID, vorbereiteter Modellordner, intrinsische
Quelle, Darstellungsgeometrie, lokale Bezuege, Originalseite und
Quellen-/Manifestfassung explizit zugeordnet. Neue Referenzen werden
nach dem aktuellen Modellformatvertrag vereinbart, nicht aus alten
Lookup-Pfaden oder Elementnamen geraten. Keine stillen Fallbacks.
Manifest, Tabellen, SHA-256, Schema-/Bibliotheksversion, SI-Einheiten,
Streifenidentitaeten und Form-/Montagekompatibilitaet beim Laden pruefen.
Die portable Laufzeit benoetigt eine gelieferte Bibliothek samt ihren
Abhaengigkeiten, nicht den SegelMacher-Quellarbeitsbereich.

Nutzerentscheidung 17.11.2026 zur Geometrieautoritaet: Bei bestaetigtem
Formkonflikt ist die ausdruecklich dem vorbereiteten Segelmodell
zugeordnete intrinsische aerodynamische Quellgeometrie massgeblich.
Die Darstellung wird daraus passend erzeugt, nicht das Kraftmodell
an ein abweichendes Darstellungsnetz skaliert. Vor Aenderungen die
aktuellen Dateien erneut vergleichen. Boots-/Riggmontage nicht still
aendern; passt die massgebliche Segelform nicht zum Rigg, den Konflikt
gesondert klaeren. Modelldateiaenderungen benoetigen weiterhin
separate Freigabe und vorherige gepruefte Sicherung. Die Quellenwahl
allein ist kein Nachweis bereits kompatibler Geometrien.

Nutzerentscheidung zur Montage: Vorhandenen Segelhals und gerichtete
Mast-/Vorstagachse als Bezuege erhalten und die massgebliche
aerodynamische Form daran starr platzieren. Abweichende Kopf- und
Schothornpositionen sind keine Freigabe zur Riggveraenderung.
Konkrete Konflikte mit Mast, Baum oder Spriet vor jeder Modellkorrektur
einzeln zeigen und klaeren; keine Streckung oder Skalierung.

Die typisierte Bibliothek liefert gekoppelte Streifenkraefte
und Momente. Fuer A-C-00178 werden raeumliche Lastmomente beziehungsweise
ausreichende Verteilungsdaten, um Q/X/Z und die Aufhebungsgroesse B
im tatsaechlichen Modellrahmen zu berechnen. Lokale skalare xFy/zFy
oder ein volles Giermoment allein genuegen bei beliebiger Rotation nicht.
Auch B darf nicht aus bereits kompensierten Streifensummen gebildet
werden. Ein transformierbarer Kraft-Erstmomenttensor kann X/Z liefern,
ersetzt aber nicht die vorzeichenabhaengige B-Auswertung der Einzellasten.
Der konkrete typisierte Liefervertrag ist inzwischen als
`Modell::berechnen_mit_sdp` mit `Transformation`, `SdpSummen` und
`Berechnung` geliefert. Q/B/X/Z werden aus transformierten Einzellasten
vor Streifenaggregation gebildet; lokale Streifenlasten bleiben
unveraendert. Die zusaetzliche Rotation ist eigentlich orthonormal;
Spiegelmatrizen sind in `Transformation` nicht erlaubt. Die Produzententests
`model_sdp_contract` und `model_consumer_contract` bestanden im
Release-Build (2/2). Dies schliesst nur die zusaetzliche SDP-Lieferluecke,
nicht allein die Core-Anbindung oder das folgende Formparitaetsgate.
Die spaetere Lieferung ergaenzt einen eigenen expliziten Parameter
`form_gespiegelt` fuer die nachgewiesene intrinsische Spiegelung;
der eigentliche Rotationsvertrag bleibt dadurch unveraendert.

Fuer einen Seitenwechsel muss die vorbereitete Aerodynamik zur
tatsaechlich dargestellten gespiegelten Form passen. Entweder ist eine
explizit zugeordnete passende Modellvariante vorhanden oder eine
physikalisch/numerisch nachgewiesene Spiegelabbildung ist vereinbart.
Nutzerentscheidung 17.11.2026 zur Reihenfolge: Bevorzugt prueft und
unterstuetzt SegelMacher die Formspiegelung in seiner Laufzeitbibliothek
auf Basis desselben vorbereiteten Datensatzes. Wind, Kraefte, axiale
Momente und SDP-Summen sind gemeinsam nachzuweisen. Nur falls die
Gleichwertigkeit nicht nachweisbar ist, werden passende vorbereitete
Modellvarianten fuer beide Seiten benoetigt. Keine ungepruefte
Spiegelung im Core; die Entscheidung allein schliesst das Nachweisgate
nicht und beauftragt keine Aenderung im SegelMacher-Projekt.
Dabei sind Orts-/Wind-/Kraftvektoren polar, Momente axial:
unter Spiegelung S gilt fuer Momente `det(S)*S*M`. Keine optische
Spiegelung mit unveraenderter unpassender Aerodynamik und keine
ungepruefte Symmetrieannahme. Fehlender Nachweis blockiert den
betroffenen Betriebsfall; nicht durch Beschraenken auf eine Seite
die vollstaendige Abnahme behaupten.

**Abnahme:** Reale Pakete fuer 420, Folkeboot und Optimist,
fehlende/defekte Referenzen, Modellwechsel, Fingerprints, geneigte Achsen,
beide Formseiten und gepruefter Zusatzdatenvertrag. Quellenkonflikte
zwischen aktuellen Darstellungsnetzen und intrinsischer Aerodynamik
explizit aufloesen. Die Anforderung autorisiert keine Aenderung an
SegelMacher oder dem getrennten Modellrepository.

## 3. Abnahme und tatsaechlicher Stand

Verbindliche Testfaelle, Abhaengigkeiten, Meilensteine und die
Anforderungsabdeckung stehen ausschliesslich im
[Versionsplan C-0.25](../Umsetzungsplaene/Umsetzungsplan_C-0.25.md).
Erzeuger-Vorpruefungen sind kein Core-Integrations- oder physikalischer
Validierungsnachweis. Fehlende reale Messreferenzen bleiben als
Modellgrenze sichtbar; sie werden nicht zu einer erfundenen
Laufzeitvoraussetzung.

Nach dem ausdruecklichen Core-/GUI-Umsetzungsauftrag und der erweiterten
Freigabe fuer notwendige Produzenten-/Modellkorrekturen sind reale
Segelphysik, Solverbilanz, Formparitaet, explizite Modellzuordnung,
Ergebnisversion 8 und GUI-Snapshots implementiert und gezielt getestet.
Der Headless-Nachweis fuer je 6000 gueltige Schritte aller drei Boote
ist bestanden. Die genaue Rueckverfolgung, Rohdaten, verbleibende
GUI-/Regressionsgates und die getrennte Nutzerabnahme stehen ausschliesslich
im Versionsplan. Implementierung und Teilnachweise behaupten keinen
pauschalen Gesamtabschluss.
