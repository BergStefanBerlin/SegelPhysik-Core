# Spezifikation Modellkatalog-Schnittstelle Core → GUI

**Status: FINAL – freigegeben am 26.10.2026, umgesetzt und per CTest nachgewiesen in C-0.2 (uncommittet); Nutzerabnahme offen.**
**Anforderungen:** A-C-00042 bis A-C-00046
**Umsetzungsplan:** [Umsetzungsplan C-0.2](../Umsetzungsplaene/Umsetzungsplan_C-0.2.md)
**Entwurfspunkte:** [Modellkatalog-Schnittstelle](Entwurfspunkte/Modellkatalog_Schnittstelle.json) (IDEE-C-00020 bis IDEE-C-00026, entschieden)

**C-0.8-Erweiterungen, fachlich finalisiert am 01.11. und 02.11.2026:**
Abschnitt 3 enthält den ursprünglichen Inspektionsvertrag; Abschnitt 4
ergänzt den kanonischen Objektbaum und typisierte Bewegungs-Vorschauen.
Die Herkunftszuordnung steht unter
[Entwurfspunkte Objektinspektion](Entwurfspunkte/Modellinspektion.json);
maßgeblich ist der
[Umsetzungsplan C-0.8](../Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md).

## 1. Zweck und Abgrenzung

Die GUI liest Modelle nicht mehr selbst aus Dateien. Der Core liest den
Modellordner, prüft die Modelle und reicht über eine einfache Schnittstelle
ausschließlich GUI-relevante Informationen weiter. Damit gibt es nur einen
Leser des Modellformats, und eine spätere Web-GUI kann dieselbe
Schnittstelle über eine dünne Transportschicht nutzen.

Die Schnittstelle ist zweistufig:

1. **Katalog:** der gesamte Objektkatalog des Ordners, nur Hauptobjekte
   (Modelle) mit wenigen Kopfangaben.
2. **Details auf Abruf:** Struktur, Darstellungsgeometrie und Diagnosen
   eines Modells erst bei Anforderung durch die GUI.

**Bei der GUI bleiben:** Ordnerwahl, Ordnerüberwachung, Anzeige, Auswahl
und Zwischenspeicher. Der Core öffnet keine Dialoge.

**Nicht Teil:** Netzwerktransport, Modellbearbeitung, Simulationsergebnisse
([Schnittstelle Core → GUI](../Dokumentationen/Schnittstelle_Core_GUI.md))
und Kraftdaten ([Kraft- und Momentenbilanz](Spezifikation_Kraft_und_Momentenbilanz.md)).

**C-0.21-Ergänzung:** Modellformat 0.3, seine Schema-Prüfung und die
gemeinsamen Fingerprints sind in
[Modellbeschreibung, A-C-00148 bis A-C-00153](Spezifikation_Modellbeschreibung.md#modellformat-03-fingerprints-und-modellpfade)
fachlich finalisiert. Der Katalog ergänzt die Felder `schema_valide`,
`visualisierung_hash_aktuell` und `fingerprints_aktuell`, damit Konsumenten
Ansichts- und Simulationsfreigaben getrennt behandeln können. Die 0.3-Daten
werden über die vorhandene Katalog-/Detailoberfläche bereitgestellt.

Bezug: [Modellbeschreibung](Spezifikation_Modellbeschreibung.md)
(A-C-00001, A-C-00003, A-C-00030), [Hydrostatikkern](Spezifikation_Hydrostatikkern.md)
(A-C-00017, A-C-00028, A-C-00031), IDEE-C-00006.

## 2. Anforderungen

| ID | Kurztitel | Version | Status |
|---|---|---|---|
| A-C-00042 | Nur GUI-relevante Informationen | C-0.2 | umgesetzt |
| A-C-00043 | Objektkatalog mit Hauptobjekten | C-0.2 | umgesetzt |
| A-C-00044 | Details auf Abruf | C-0.2 | umgesetzt |
| A-C-00045 | Bezug zwischen Katalog und Simulation | C-0.2 | umgesetzt |
| A-C-00046 | Transportunabhängige, versionierte Schnittstelle | C-0.2 | umgesetzt |

### A-C-00042: Nur GUI-relevante Informationen

**Version:** C-0.2 · **Status:** umgesetzt

Über die Schnittstelle gehen ausschließlich Angaben, die die GUI für
Auswahl, Anzeige und Bedienung braucht. Volumennetze für die Hydrostatik,
Trägheitstensoren, Schnittpuffer und andere interne Rechendaten werden nicht
übergeben. Dateipfade sind kein Bestandteil der Antworten; Modelle werden
über ihre Modell-ID angesprochen.

**Abnahme:** Vertragstests stellen fest, dass Katalog- und Detailantworten
nur die dokumentierten Felder enthalten.

### A-C-00043: Objektkatalog mit Hauptobjekten

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** IDEE-C-00020, -00021, -00022, -00025

Aufruf `modellkatalog(ordner)` mit dem von der GUI gewählten Ordner. Ein
Hauptobjekt ist ein Modell, also eine `bootsmodell.json` direkt im Ordner oder in
einem unmittelbaren Unterordner. Das Ergebnis ist ein Katalogobjekt, das die
GUI hält; der Core führt keinen globalen Zustand. Je Hauptobjekt:

| Feld | Bedeutung |
|---|---|
| `modell_id` | ID aus der Modellbeschreibung |
| `name` | Anzeigename aus der Modellbeschreibung |
| `beschreibung` | Optionaler Kurztext, sofern im Modell vorhanden |
| `modell_fassung` | Inhaltsfingerprint wie im Ergebnisvertrag |
| `status` | `verwendbar` oder `fehlerhaft` |
| `fehleranzahl` | Anzahl der Diagnosen; Details auf Abruf |
| `schema_valide` | Schema-, Identitäts- und Ressourcenprüfung bestanden |
| `visualisierung_hash_aktuell` | Visualisierungsbereich ist geprüft und aktuell |
| `fingerprints_aktuell` | Alle fünf Fingerprint-Bereiche sind geprüft und aktuell |

Die Reihenfolge ist stabil (nach Ordnername). Fehlerhafte Modelle erscheinen
im Katalog als `fehlerhaft` (A-C-00031). Tragen mehrere Modelle dieselbe
`modell_id`, ist das ein Fehler mit Fehler-ID; alle betroffenen Modelle
werden als `fehlerhaft` geführt. Ein nicht lesbarer Ordner liefert eine
Fehlerantwort mit Fehler-ID, keine leere Liste.

**Abnahme:** Der Beispielordner liefert alle Modelle in stabiler
Reihenfolge; ein fehlerhaftes Modell und ein ID-Duplikat erscheinen als
`fehlerhaft` mit `fehleranzahl > 0`.

### A-C-00044: Details auf Abruf

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** IDEE-C-00023, -00024

| Aufruf | Liefert |
|---|---|
| `modellstruktur(modell_id)` | Gruppen und Elemente mit IDs und Namen, optionalen Bewegungsdefinitionen, Gesamtmasse, Hüllquader im Modellbezug |
| `darstellungsgeometrie(modell_id[, element_id])` | Je Element Eckpunkte und Dreiecke im Modellbezug in Metern, Farbe sofern im Modell angegeben, Herkunft `darstellung` oder `volumen_ersatz` |
| `modelldiagnose(modell_id)` | Diagnosen mit Fehler-ID, Text und Fundstelle |

Fehlt ein Darstellungsnetz, liefert der Core das Volumennetz und kennzeichnet
dies als `volumen_ersatz`. Für fehlerhafte Modelle liefert der Core die
Geometrie, soweit sie lesbar ist („nur Ansicht“ in der GUI). Die
Darstellungsgeometrie hat keinen Einfluss auf die Physik (A-C-00017).
Die Daten stehen als C++-Strukturen für die Desktop-GUI und als
JSON-Serialisierung für Web und Tests bereit; eine Binärform folgt erst bei
gemessenem Bedarf.

Für Modelle mit `Bewegung`-Definition liefert jedes Element den optionalen
Katalogblock `bewegung` mit Bewegungsart und Bediensemantik. Drehungen
liefern Achse und Drehpunkt; Hubdefinitionen liefern Richtung und den beim
Core-Import abgeleiteten Hubweg. Die Modellstruktur bleibt damit die
GUI-Quelle für passende Stellregler; sie enthält keine Darstellungspfade.

**Abnahme:** Die Geometrie der Referenzmodelle stimmt mit der heutigen
GUI-Darstellung überein (gleiche Eckpunkt- und Dreieckszahl, gleiche Lage).

### A-C-00045: Bezug zwischen Katalog und Simulation

**Version:** C-0.2 · **Status:** umgesetzt

Prüfung und Simulation lassen sich über das Katalogobjekt und die Modell-ID
starten. Jede Detail- oder Laufanfrage nennt die erwartete `modell_fassung`.
Hat sich die Modelldatei inzwischen geändert, weist der Core die Anfrage
mit eindeutiger Fehler-ID zurück.

**Abnahme:** Nach Änderung einer Modelldatei liefert eine Anfrage mit der
alten Fassung die Fehler-ID; ein neuer Katalogabruf liefert die neue Fassung.

### A-C-00046: Transportunabhängige, versionierte Schnittstelle

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** IDEE-C-00024, -00026

Die Schnittstelle ist eine Bibliotheksfunktion ohne Netzwerkanteil. Die
JSON-Antworten tragen das Feld `katalogversion` (zunächst `"1"`), unabhängig
von der Vertragsversion der Simulationsergebnisse. Eine spätere
Web-Anbindung reicht dieselben Antworten ohne eigene Fachlogik weiter.
Die Beschreibung steht in `Dokumentationen/Schnittstelle_Core_GUI.md`
(A-C-00028); neue Fehler-IDs stehen im Fehlerkatalog.

**Abnahme:** Vertragstests prüfen die JSON-Antworten ohne GUI; Doku,
Beispiele und Fehlerkatalog stimmen mit der Implementierung überein.

## 3. C-0.8: Objektbäume, Eigenschaften und Mesh-Abfragen

**Status:** FINAL; Core-Implementierung und CTest am 01.11.2026 bestanden,
GUI-Integration und Nutzerabnahme offen.

**Zielversion:** C-0.8.

**Fachliche Entscheidungen vom 01.11.2026:** Alle im Core-Modell vorhandenen
Objektbaumtypen und fachlichen Eigenschaften sollen inspizierbar werden.
Auch bisher ausgeschlossene Physiknetze und fachliche Eigenschaften sind
für die Bootsauswahl als lesbare Inspektionsdaten vorgesehen. Dies ist die
ausdrückliche fachliche Zustimmung zur entsprechenden Vertragserweiterung;
der genaue Datenvertrag wird noch ausgearbeitet, nicht rückwirkend in V1
geändert. Reparaturen und Ummodellierungen sind eine spätere Perspektive,
keine Schreib- oder Reparaturschnittstelle dieses Umfangs.
**Anlass:** Bootsauswahl und Objektinspektion der GUI
([GUI-Spezifikation](../../GUI/Spezifikationen/Spezifikation_GUI_Bootsauswahl.md)).

Diese Erweiterung wird in der vorhandenen maßgeblichen
Modellkatalog-Spezifikation geführt, nicht als paralleler Katalogvertrag.
Die Anforderungen in Abschnitt 2 bleiben unverändert gültig.
Die Anforderungen A-C-00098 bis A-C-00104 gelten für C-0.8.
Die Core-Implementierung wurde ausdrücklich beauftragt. Git-Aktionen sind
nicht beauftragt.

### 3.1 Ziel und Zuständigkeit

Der Core liefert die fachlich definierten Sichten auf ein Modell, deren
Hierarchie, Eigenschaften und freigegebene Anzeigegeometrie auf Abruf.
Die GUI liest keine Modell- oder Meshdateien und rekonstruiert keine
Physikstrukturen aus Namen oder Geometrie.

Baumsicht, Eigenschaft und Mesh-Typ sind verschiedene Begriffe:
Eine Hydrodynamiksicht kann Knoten ohne eigenes Mesh enthalten. Derselbe
Modellbestandteil kann in mehreren Sichten vorkommen. Identität und
Zuordnung zu Elementen müssen deshalb ausdrücklich übertragen werden.

Auswahlbedienung, Glasdarstellung nicht ausgewählter Teile, Farben,
Wasser/Himmel, Kamerabedienung und Tabellenlayout bleiben GUI-Aufgaben.
Es werden keine neuen physikalischen Berechnungen oder Modellbearbeitung
beauftragt.

### 3.2 Fachliche Aufrufe

Die Bezeichnungen werden durch den konkreten C++-Vertrag in Abschnitt 3.7
festgelegt und im Core-Schnittstellenregister mit CPPI-Kennungen geführt.

Nur das erstmalige Einlesen verwendet Katalog, Modell-ID und erwartete
Modellfassung. Alle nachfolgenden Abfragen verwenden den festen Snapshot;
die Tabelle nennt die zusätzlichen fachlichen Argumente.

| Aufruf | Zusätzliche Argumente | Ergebnis |
|---|---|---|
| `getAvailableObjectTreeTypes` | keine | Tatsächlich unterstützte Sichten mit stabiler Typkennung und lesbarem Namen. |
| `getObjectTree` | Baumtyp | Hierarchie mit stabilen Knoten-IDs, Eltern-/Kindbeziehungen, Namen, Knotentyp und zugeordneten Modell-Element-IDs. |
| `getObjectProperties` | Baumtyp, Knoten-IDs, `includeChildren=true` | Eigenschaften der Auswahl einschließlich Nachkommen, eindeutig dem Objekt/Pfad zugeordnet. |
| `getAvailableMeshTypes` | keine | Tatsächlich abrufbare Inspektionsgeometriearten mit Typkennung, Name und beitragenden Element-IDs; die Herkunft steht am jeweiligen Mesh. |
| `getMesh` | Mesh-Typ, optionale Element-ID-Liste | Geometrie im Modellbezug in Metern, Dreiecke und eindeutige Elementzuordnung; vorhandene Darstellungsmetadaten erhalten. |

Ein nicht gesetzter Elementfilter bedeutet alle Elemente dieses Mesh-Typs;
eine ausdrücklich leere Liste bedeutet keine Auswahl. Unbekannte Typen
oder IDs sind Fehler, keine leeren Erfolgsergebnisse.

### 3.3 Bäume und Werte

Vorgesehen sind alle im Core-Modell vorhandenen fachlichen Baumsichten
und Eigenschaften, nicht nur Modell-/Darstellung und Hydrodynamik.
Typenliste, Hierarchie und Eigenschaftenmenge sind in Abschnitt 3.7
festgelegt; der Umfang ist nicht auf die bisherige Strukturschnittstelle
beschränkt.
Nicht vorhandene Merkmale werden nicht als vorhanden ausgegeben.

Knotenidentität ist mindestens innerhalb von Modellfassung und Baumtyp
eindeutig. Baumknoten und Modell-Element-ID werden nicht gleichgesetzt.
Gruppen können mehrere Elemente zuordnen. Mehrfachauswahl und überlappende
Gruppen werden über stabile Identitäten dedupliziert, nicht über
Anzeigenamen.

Eigenschaften erhalten fachliche Kennung, lesbaren Namen, typisierten
Wert, Einheit soweit anwendbar, Objektzuordnung und einen dokumentierten
Verfügbarkeitsstatus. Fehlend/nicht anwendbar ist nicht numerisch null.
Vektoren und Matrizen bleiben strukturierte Daten; die GUI formatiert sie.
Keine automatische Summierung ausgewählter Eigenschaften.

Die GUI benötigt Werte einschließlich Unterobjekten. Ein optionaler
allgemeiner API-Filter ist keine Verpflichtung zu einem zusätzlichen
GUI-Umschalter.

### 3.4 Vertragsgrenzen der Mesh-Abfrage

Die Erweiterung verwendet den vorhandenen Core-Import und die bestehende
Darstellungsgeometrie einschließlich `volumen_ersatz`. Zusätzlich werden
die vorhandenen physikalischen Mesh-Arten als Inspektionskopien erschlossen.
Sie dupliziert keinen Meshleser.

**Bewusste Erweiterung gegenüber A-C-00042:** Der Nutzer hat die Ausgabe
vorhandener Physiknetze für die Bootsauswahl fachlich bestätigt.
Die neue Inspektionsschnittstelle erhält dafür einen expliziten Vertrag
mit vollständig dokumentierten Mesh-Arten. Bestehende V1-Antworten werden
nicht stillschweigend erweitert oder umgedeutet.

Auch bisher ausgeschlossene fachliche Eigenschaften, etwa
Trägheitstensoren, gehören nach dieser Entscheidung zur Inspektion.
Alle vorhandenen fachlichen Eigenschaften sind im Dateninventar abzudecken;
interne Implementierungsdetails und temporäre Rechenpuffer sind keine
Objekteigenschaften.
Keine Rohpuffer, Dateipfade oder veränderbaren Referenzen auf Core-Interna.

### 3.5 Versionierung, Fehler und Konsistenz

Bestehende Katalogaufrufe und Antworten bleiben unverändert. Neue
Antwortschemas, Typkennungen, Einheiten, Statuswerte und Kompatibilitätsregeln
sind in Abschnitt 3.7 versioniert dokumentiert. Die Ergebnisversion
der Simulation wird nicht für Inspektionsdaten umgewidmet.

**Nutzerentscheidung vom 01.11.2026:** Der Core liest den Inspektionsbestand
einmal ein und liefert einen festen, unveränderlichen Stand. Objektbäume,
Eigenschaften und Mesh-Abfragen beziehen sich anschließend ausschließlich
auf diesen eingelesenen Bestand, nicht auf erneut gelesene Dateien.
Spätere Dateiänderungen verändern oder entwerten diesen Stand nicht.
Ein neuer Stand entsteht nur durch ausdrücklich angefordertes neues Einlesen.
Auch auf Abruf gelieferte Details müssen aus dem festen Bestand stammen.

Für den aktuellen Umfang gibt es keine automatische Aktualisierung,
Dateiüberwachung zur Ersetzung des Inspektionsstands oder erneute
Dateifassungsprüfung bei dessen Detailabfragen. Prüfung und Diagnose beim
anfänglichen Einlesen bleiben notwendig. Die bisherige Simulationsfassung
und bestehende Katalogaufrufe werden nicht neu definiert.
Mesh-Updates, Mesh-Modifikationen und die weitergehende Bedienung eines
Neuladens werden später besprochen und gehören nicht zu diesem Umfang.

Ungültige Anfrage, unbekannte Identität, veraltete Fassung, nicht
freigegebener Mesh-Typ und nicht lesbare Daten erhalten dokumentierte
Core-Diagnosen. Ein physikalisch fehlerhaftes Modell kann weiterhin
lesbare Ansichtsdaten liefern, aber nur mit ausdrücklichem Status und
Diagnosen; daraus folgt keine Simulationsfreigabe.

C++ und JSON bilden dieselben fachlichen Daten ab. Kein Netzwerktransport
und kein globaler veränderlicher Auswahlzustand im Core.

### 3.6 Abnahme und Nachweisstand

Vertragstests prüfen Typauflistung, echte Hierarchien, referenzielle
Integrität, Gruppen-/Mehrfachauswahl einschließlich Nachkommen,
Eigenschaftstypen und Einheiten, Deduplikation, Geometriezuordnung,
Prüfung beim Einlesen, Beständigkeit nach Dateiänderungen,
C++-/JSON-Parität und Fehler.
Bestehende Katalog-, Import- und Simulationsverträge müssen unverändert
regressionsgeprüft werden.

Die vollständigen Anforderungen, Typen und Prüfgrenzen stehen nachfolgend.
Core-Build und CTest sind bestanden; GUI-Integration und sichtbare
Nutzerabnahme sind keine Core-Tests und bleiben offen. Nachweise stehen im
[C-0.8-Umsetzungsplan](../Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md).

### 3.7 Normativer Daten- und API-Vertrag

#### A-C-00098: Vollständiges fachliches Dateninventar und Baumsichten

Die folgenden Baumsichten sind in dieser stabilen Reihenfolge unterstützt.
Eine Sicht wird für ein Modell angeboten, wenn mindestens ein zugehöriger
Fachblock oder ein zugehöriges Element vorhanden ist. `modell` ist immer
vorhanden, sofern das Modell strukturell lesbar ist.

| Typkennung | Anzeigename | Inhalt |
|---|---|---|
| `modell` | Modell | Alle Gruppen, Elemente, Metadaten und Fachblöcke; vollständiger Inspektionsbestand. |
| `darstellung` | GUI | Elemente mit Darstellung einschließlich `unsichtbar` oder dokumentiertem Volumenersatz; Farbe, Oberfläche und Meshbezug. |
| `hydrostatik` | Hydrostatik | Physikalische Elemente mit Masse, Schwerpunkt, Trägheit, Volumen, Massenart und Flutrand. |
| `hydrodynamik` | Hydrodynamik | Elemente mit Hydrodynamikrolle/-daten und modellweiter hydrodynamischer Trägheitsdefinition. |
| `bewegung` | Bewegung | Bewegungsdefinitionen und zugeordnete mitbewegte Elemente. |

Die GUI erfindet keine zusätzlichen Fachbäume. Innerhalb jeder Sicht:
Modellwurzel → vorhandene Modellgruppen → zugehörige Elemente →
Fachblock-/Unterobjektknoten. Modellweite Fachblöcke hängen unmittelbar
unter der Wurzel. Bewegungsreferenzen sind Eigenschaften/Elementzuordnungen,
keine zyklischen Kindverknüpfungen.

Vollständiges Ausgangsinventar nach `modell.hpp` und Modellimport:

| Bestand | Zu liefernde Eigenschaften/Unterobjekte |
|---|---|
| Modell/Metadaten | Modell-ID, Anzeigename, Beschreibung soweit vorhanden, Schema-/Metadatenstatus, vollständige `entwurf`-Metadaten ohne Dateipfade, Modellfassung und Prüfstatus; Gruppen-/Elementidentitäten. |
| Gesamte Physik | Gesamtmasse, Schwerpunkt, zeilenweise 3×3-Trägheit; vorhandene Hüllgrenzen. |
| Element | ID, Gruppe, Fachblockverfügbarkeit, Masse, Schwerpunkt, 3×3-Trägheit und Zuordnung aller Mesh-Arten. |
| Massendefinition | `punktmasse`: Masse/Position; `vollkoerper`: Dichte; `schale`: Dichte/Wanddicke; jeweils Geometriebezug statt Dateipfad. |
| Darstellung | deklarierte Farbe, Oberfläche einschließlich unsichtbar, tatsächliche Herkunft `darstellung`/`volumen_ersatz`, Geometriebezug. |
| Hydrostatik | vorhandenes Verdrängungsvolumen, Flutrandpunkte, Massenpunkte; Netzanzahlen und Referenzen. |
| Hydrodynamik | Rolle, Materialreferenz, alle Kraftmodelle mit Methode, Parametern, Einheit, Status, Quelle, Gültigkeit und Unsicherheit; alle Kennfeldpunkte mit Winkel, Reynolds-Zahl, cl/cd und Herkunftsangaben; Widerstandsanteile. |
| Bewegung | Art, Ruderbediensemantik, Achse, Drehpunkt, Richtung, Hubweg, mitbewegte Element-IDs. |
| Hydrodynamische Trägheit | sämtliche explizit definierten Zusatzmassen- und Dämpfungsparameter mit ihren dokumentierten Einheiten; nicht vorhandene Definitionen werden nicht als berechnete Laufdaten ausgegeben. |

Alle im aktuellen unterstützten Modellformat enthaltenen fachlichen
Schlüssel werden abgedeckt, auch dynamische Schlüssel in Parameter-,
Kennfeld- und Metadatenobjekten. Fachliche Struktur aus dem Originalformat
bleibt als Unterknoten erhalten; keine bloße reduzierte `Modell`-Projektion.
Unbekannte Physikfelder bleiben nach bestehendem Importvertrag ungültig.
Dateifelder werden durch logische Geometriereferenzen ersetzt, nicht
durch absolute oder relative Pfade. Quellen-/Referenztexte sind keine
Anweisung, weitere Dateien in der GUI zu öffnen.

Bei Punktmassen gibt es kein erfundenes Dreiecksmesh. Ohne physikalischen
Fachblock sind Masse/Trägheit nicht als frei erfundene Nullwerte auszugeben.
Bereits im Core berechnete echte Nullwerte bleiben dagegen echte Nullwerte.
Laufabhängige Kräfte, Zustände oder Trägheitsberechnungen werden nicht
allein für die Inspektion gestartet.

**Abnahme:** Vertragstests vergleichen das Inventar gegen alle genannten
importierten Typen und Fachblöcke an gültigen Fixtures einschließlich
reiner Darstellung, Punktmasse, Schale, Hydrodynamikkennfeld und Bewegung.
Jedes vorhandene Fachfeld ist in Baum/Eigenschaften oder Meshbezug auffindbar.

#### A-C-00099: Unveränderlicher, einmal eingelesener Snapshot

`readModelInspection` erzeugt einen besessenen, unveränderlichen Snapshot
aller lesbaren Inspektionsdaten einschließlich aller später abrufbaren
Meshes. Kein späteres Detail holt Daten erneut aus Dateien.
Kein globaler Snapshot, keine Prüfung der inzwischen geänderten Dateien
bei Detailabfragen. Der Aufrufer hält den Snapshot bis zur Freigabe.

Beim Einlesen wird die erwartete Katalogfassung nach bestehendem Vertrag
geprüft. Alle Quelldaten werden in einen lokalen Bestand übernommen; die
Physikprüfung und Geometrieaufbereitung verwenden genau diesen Bestand.
Während dieses Einlesevorgangs erkannte Änderungen führen zu einer
eindeutigen Ablehnung, nicht zu einem gemischten Bestand. Nach erfolgreicher
Rückgabe ist der Stand unabhängig vom Dateisystem. Die alte
Simulationsfassung bleibt unverändert; eine separate opaque `snapshot_id`
kennzeichnet den Inspektionsbestand, nicht eine neue Simulationsrevision.

Fehlerhafte, aber strukturell lesbare Modelle liefern einen Snapshot mit
Status `nur_ansicht`, Diagnosen und ausschließlich tatsächlich lesbaren
Daten; kein erfolgreicher Physikstatus. Nicht lesbare Gesamtstruktur
verursacht einen Fehler. Fehlende Eigenschaften sind explizit gekennzeichnet.

**Abnahme:** Nach Einlesen sämtliche Quell-/Darstellungsdateien ändern,
entfernen oder unlesbar machen; alle Snapshotabfragen liefern identische
Daten und gültige Meshes. Detailabfragen führen null Quelldateizugriffe aus.
Fehler-/Änderungsfälle beim Einlesen prüfen. Kein Einfluss auf Simulation.

#### A-C-00100: Typisierte Eigenschaften und Auswahlidentität

Stabile Knoten-ID: innerhalb eines Snapshots und Baumtyps eindeutig;
als JSON-Pointer nach RFC 6901 aus den logischen Strukturpfaden abgeleitet
(`~` und `/` korrekt maskieren). Modell-Element-IDs bleiben eigenständige
Referenzen. Reihenfolge: Gruppen/Elemente wie im Katalog, Fachobjektschlüssel
lexikografisch, Listen nach Fachreihenfolge.
Die Wurzel hat ID `""` und `parent_id: ""`; alle übrigen Eltern-IDs
verweisen auf genau einen Knoten derselben Sicht. `kind` ist
`modell`, `gruppe`, `element`, `objekt` oder `liste`.
Eigenschafts-IDs entsprechen ihren RFC-6901-maskierten relativen
Feldpfaden im Knoten; synthetische Fachwerte erhalten einen dokumentierten
`@`-Präfix, damit sie nicht mit Quellfeldern kollidieren.

Werte haben die Typen `null`, `bool`, `int`, `real`, `text`, `vector`,
`matrix`, `list`, `object`; Matrix mit expliziter Zeilen-/Spaltenzahl und
zeilenweisen Zahlen. Status: `vorhanden`, `nicht_vorhanden`,
`nicht_anwendbar`, `fehlerhaft`; nur `vorhanden` trägt einen echten Wert.
SI-Einheiten gemäß Core-Fachvertrag, dimensionslose Werte Einheit `1`,
nicht numerische Werte Einheit leer. Keine Schätzung einer unbekannten Einheit.

Eigenschaftszeile: Knoten-ID, lesbarer Objektpfad, stabile Eigenschafts-ID,
Anzeigename, typisierter Wert, Einheit, Status. `includeChildren=true`
vereinigt Auswahl und sämtliche Nachkommen. Deduplikation anhand
`(Baumtyp, Knoten-ID, Eigenschafts-ID)`, keine automatische Aggregation.
Leere Knotenliste ergibt eine leere Auswahl; unbekannte Knoten sind Fehler.

**Abnahme:** Überlappende Gruppen-/Mehrfachauswahl, Sonderzeichen,
echte Nullwerte, fehlende Werte und nichtskalare Werte prüfen.

#### A-C-00101: Vollständige lesbare Geometrieinspektion

| Mesh-Typ | Datenquelle und Semantik |
|---|---|
| `darstellung` | Bestehende Anzeigegeometrie mit Farbe/Oberfläche und markiertem `volumen_ersatz`. |
| `hydrostatik_volumen` | Eingelesenes physikalisches Verdrängungsnetz. |
| `hydrodynamik` | Das vom Core für die jeweilige hydrodynamische Rolle verwendete Netz; aktuell dasselbe physische Volumennetz, ausdrücklich als gemeinsame Quelle ausgewiesen. Kein erfundenes separates Netz. |
| `massen_mesh` | Tatsächliche Dreiecksfläche der Vollkörper-/Schalen-Massendefinition; dafür beim Einlesen vollständig erhalten. |
| `massenpunkte` | Eingelesene Massenpunkte einschließlich Punktmassen, Topologie `punkte`. |
| `flutrand` | Vorhandene Flutrandpunkte, Topologie `punkte`; keine erfundene Kantenreihenfolge. |

Nur Typen mit tatsächlich lesbarer Geometrie werden als verfügbar gelistet.
Jeder Typ beschreibt die tatsächlich beitragenden Element-IDs.
Ergebnisse: Teil-ID, Element-ID, Gruppe, Mesh-Typ, Herkunft, Topologie,
Modellbezug, Punkte in Metern und Dreiecke mit nullbasierten Indizes.
Bei Punktdaten bleibt die Dreiecksliste leer. Originalkoordinaten und
Topologie bleiben erhalten; kein reparierender oder vereinfachender Export.

`nullopt` als Elementfilter bedeutet alle beitragenden Elemente; eine
explizit leere Liste liefert keine Teile. Bekannte Elemente ohne Geometrie
dieses Typs liefern keine Teile, keine Ersetzung durch ein anderes Mesh.
Dieser erwartete Verfügbarkeitsfall ist über die vollständigen
`element_ids` der verfügbaren Mesh-Typen ausdrücklich erkennbar: Der
Aufrufer bildet die Differenz zur gültigen Auswahl und zeigt
„Keine Geometrie dieses Typs“ an. Dafür gibt es kein zusätzliches
Statusfeld in `InspectionMesh` und keinen Scheinteil mit leerer Geometrie.
Unbekannte Typen/Elemente sind Fehler; fehlgeschlagene Aufbereitung
erhält dagegen CORE-INSP-0004 und Snapshotdiagnosen.

**Abnahme:** Geometrien gegen tatsächliche Importdaten prüfen, auch
Massenmesh abweichend vom Volumennetz, unsichtbare Teile, Punktmasse,
gemeinsames Hydrostatik-/Hydrodynamiknetz und Volumenersatz.

#### A-C-00102: Konkrete API, Versionierung und Fehler

Neue Headerdatei bei Implementierung: `segelphysik/inspektion.hpp`,
Namensraum `segelphysik`. Die folgenden Werttypen sind öffentlich:

```cpp
struct InspectionMatrix {
    std::size_t rows, columns;
    std::vector<double> values;
};
using InspectionValue = std::variant<
    std::monostate, bool, std::int64_t, double, std::string,
    std::vector<double>, InspectionMatrix, nlohmann::json>;
struct InspectionType {
    std::string id, name;
    std::vector<std::string> element_ids;
};
struct InspectionNode {
    std::string id, parent_id, name, kind;
    std::vector<std::string> element_ids;
};
struct InspectionProperty {
    std::string node_id, object_path, id, name, value_type, unit, status;
    InspectionValue value;
};
struct InspectionMesh {
    std::string id, element_id, group, type, origin, topology;
    std::vector<Vektor3> points_m;
    std::vector<std::array<std::size_t, 3>> triangles;
    std::optional<std::array<double, 3>> color_rgb;
    std::optional<std::string> surface;
};
struct InspectionInfo {
    std::string model_id, model_version, snapshot_id, status;
    std::vector<KatalogDiagnose> diagnostics;
};
class ModelInspection; // opaque, nur unveränderlich öffentlich zugänglich
```

`nlohmann::json` dient nur objekt-/listenwertigen Fachmetadaten, nicht als
untypisierter Ersatz für sämtliche Ergebnisse. Nichtendliche Zahlen sind
kein zulässiger JSON-Wert; betroffene Eigenschaft wird `fehlerhaft` plus Diagnose.

```cpp
std::shared_ptr<const ModelInspection> readModelInspection(
    const Modellkatalog&, const std::string& model_id,
    const std::string& expected_model_version);
std::vector<InspectionType> getAvailableObjectTreeTypes(const ModelInspection&);
std::vector<InspectionNode> getObjectTree(const ModelInspection&, const std::string& tree_type);
std::vector<InspectionProperty> getObjectProperties(
    const ModelInspection&, const std::string& tree_type,
    const std::vector<std::string>& node_ids, bool includeChildren = true);
std::vector<InspectionType> getAvailableMeshTypes(const ModelInspection&);
std::vector<InspectionMesh> getMesh(
    const ModelInspection&, const std::string& mesh_type,
    const std::optional<std::vector<std::string>>& element_ids = std::nullopt);
std::string inspectionInfoJson(const ModelInspection&);
std::string inspectionTreeTypesJson(const ModelInspection&);
std::string inspectionTreeJson(const ModelInspection&, const std::string& tree_type);
std::string inspectionPropertiesJson(const ModelInspection&, const std::string& tree_type,
    const std::vector<std::string>& node_ids, bool includeChildren = true);
std::string inspectionMeshTypesJson(const ModelInspection&);
std::string inspectionMeshJson(const ModelInspection&, const std::string& mesh_type,
    const std::optional<std::vector<std::string>>& element_ids = std::nullopt);
```

Die kompatiblen Projektionen sind zusätzlich als Snapshot-Überladungen
verfügbar. Sie liefern die vorhandene V1-Struktur-/Darstellungsform aus
dem bereits eingelesenen Bestand und führen keine erneute Dateiprüfung
oder -lesung aus:

```cpp
Modellstruktur modellstruktur(const ModelInspection&);
Darstellungsgeometrie darstellungsgeometrie(
    const ModelInspection&, const std::optional<std::string>& element_id = std::nullopt);
```

Diese beiden neuen Überladungen sind CPPI-0053 und CPPI-0054. Für
Importerhaltungen der Massenmesh- und Massenartdaten sowie des bereits
durch den Bestandsparser ausgewerteten, pfadbereinigten Quelldokuments
gelten CPPI-0055 bis CPPI-0059. Die bisherigen Katalog-/Versionssignaturen
bleiben bestehen; sie nehmen den aktuellen Snapshot auf und projizieren
anschließend durch dieselben Funktionen.

Info hat C++-Entsprechung `const InspectionInfo& ModelInspection::info() const`.
`InspectionInfo::status` ist `verwendbar` oder `nur_ansicht`.
Keine öffentliche Mutationsmethode.
Nur die Fabrik kann den Snapshot vollständig aufbauen. Parallele Leseabfragen
sind sicher; Kopien der Antworten ändern den Snapshot nicht.

Alle erfolgreichen JSON-Antworten: `inspektionsversion: "1"`, `model_id`, `model_version`,
`snapshot_id`, `status`, `diagnostics` und genau eine Nutzlast:
`info` (Identitäten/Status wie oben), `tree_types`, `nodes`, `properties`,
`mesh_types` oder `meshes`. Arrays enthalten die gleichnamigen C++-Felder.
Properties verwenden `value_type`, `value` und die oben definierten Statuswerte;
Matrixwert ist `{rows,columns,values}`, Listen/Objekte sind JSON-Strukturen.
Meshantworten benennen zusätzlich `frame: "modell"` und `length_unit: "m"`.
Keine Änderung von `katalogversion: "1"` oder Simulationsergebnisversionen.

Die neue Inspektionsschnittstelle ist die ausdrückliche C-0.8-Ausnahme
zum Datenumfang von A-C-00042; die bisherigen Endpunkte bleiben beschränkt.
Keine zusätzlichen Exporte im alten V1-JSON und kein globaler Auswahlzustand.

Reservierte neue Fehler, bei Implementierung in den Runtime-Fehlerkatalog
aufzunehmen, bis dahin **nicht implementiert**:

| Fehler-ID | Bedingung | Handlung |
|---|---|---|
| `CORE-INSP-0001` | Unbekannter/nicht verfügbarer Baum- oder Mesh-Typ | Verfügbare Typen dieses Snapshots abrufen. |
| `CORE-INSP-0002` | Unbekannte Knoten-/Element-ID | Auswahl aus demselben Snapshot und Baum neu beziehen. |
| `CORE-INSP-0003` | Snapshot nicht konsistent einlesbar, unlesbare Gesamtstruktur oder Änderung während Einlesen | Diagnose anzeigen; neues Einlesen später ausdrücklich anfordern. |
| `CORE-INSP-0004` | Geometrie-/Wertaufbereitung für lesbare Teildaten fehlgeschlagen | Teilstatus/Diagnosen anzeigen; nicht als gültige Physik ausgeben. |

Bestehende unbekannte Modell-ID/veraltete Katalogfassung verwenden weiter
`CORE-KAT-0003`/`CORE-KAT-0004` beim Einlesen. Keine Entwertung bestehender
Snapshots mit diesen Fehlern bei nachträglichen Dateiänderungen.

Fehleraufbereitung folgt dem bestehenden Core-Muster: Bibliotheksfehler
mit `std::runtime_error` und Fehler-ID statt Erfolgsschema. Teilfehler
lesbarer Modelle erscheinen unter `diagnostics` und `status: "nur_ansicht"`;
ungültige Aufrufparameter sind keine bloßen Teildiagnosen.
Alle Diagnosen der neuen Inspektion enthalten ausschließlich logische
Objekt-/Feldfundstellen statt Quelldateipfade. Eine künftig notwendige
HTTP-Fehlerhülle ist nicht Bestandteil dieses Bibliotheksvertrags.

**Abnahme:** C++-/JSON-Parität und Schemaformen, Fehlermatrix,
Thread-Lesesicherheit und unveränderte Altantworten prüfen.

#### A-C-00103: Nachweise und Lieferung

Neue CTest-Fälle müssen alle Kriterien aus A-C-00098–00102 abdecken.
Fixtures enthalten mindestens Punktmasse, Schale, Vollkörper, reine
Darstellung, fehlerhaftes Modell, Kennfelder und bewegte Baugruppe.
Kein Snapshot oder Property darf einen Quelldateipfad ausgeben.
Während der Implementierung wird eine Feldabdeckungsmatrix zum Inventar
geführt; kein vorhandenes fachliches Feld wird stillschweigend ausgelassen.
Dokumentation, CPPI-Register und Fehlerkatalog müssen mit der gelieferten
Implementierung übereinstimmen. GUI-Integration und persönliche Sichtabnahme
sind getrennte Schritte, kein Core-Vertragstest ersetzt sie.

#### A-C-00104: Gemeinsamer Importpfad und kompatible Bestandsfunktionen

Nutzerergänzung vom 01.11.2026: Die neue Inspektion darf keine parallele
Implementierung der bisherigen Modell-, Struktur- oder Geometriefunktionen
einführen. Bestehende Funktionen müssen die neue gemeinsame
Datenaufbereitung tatsächlich mitverwenden; bloß gleiche Ergebnisse aus
zwei unabhängigen Importwegen erfüllen diese Anforderung nicht.

Es gibt eine gemeinsame Core-Pipeline für Quellaufnahme, Modellbeschreibung,
Meshlesen und fachliche Validierung. Vorhandene Parser und Leser werden
wiederverwendet beziehungsweise dorthin zusammengeführt, nicht kopiert.
Snapshotabfragen und kompatible Altprojektionen greifen auf dieselben
aufbereiteten Daten und Abfragebausteine zu. Interne Bausteine benötigen
keine zusätzlichen öffentlichen Ersatzinterfaces.

| Bestehender Einstieg | Verbindliche Wiederverwendung |
|---|---|
| `modellkatalog`, `modelldiagnose`, `katalog_datei` | Gemeinsame Beschreibung, Prüf- und Versionsbausteine; Ordnersuche und bisherige Fassungsprüfung bleiben ihre Aufgaben. Kein vollständiger Inspektionssnapshot für jedes Listenmodell allein zur Vereinheitlichung erforderlich. |
| `modellstruktur` | Kompatible Projektion der gemeinsamen Struktur-/Eigenschaftsdaten; keine eigene zweite Hierarchie- oder Massenaufbereitung. |
| `darstellungsgeometrie` | Kompatible Projektion derselben Darstellungsmesh-Abfrage, die `getMesh(..., "darstellung", ...)` versorgt; kein separater OBJ-Leser. |
| `katalogmodell`, `lade_modell` | Derselbe Modellimport-/Validierungskern wie bei der Snapshotaufnahme; Simulation erhält weiterhin ein eigenes simulationsfähiges Modell, keine Rekonstruktion aus Inspektions-JSON. |
| Bestehende JSON-Ausgaben | Serializer der kompatiblen Projektionen, keine zweite fachliche Datenaufbereitung und kein JSON-Roundtrip zwischen internen Aufrufen. |

Die öffentlichen Alt-Signaturen, CPPI-Kennungen, V1-Ausgabeformen,
Reihenfolgen, Filter, Diagnose-/Fehler-IDs und zulässigen Teilantworten
bleiben erhalten. Die gemeinsame Aufbereitung muss bedarfsgerecht bleiben:
Ein Fehler in ausschließlich neuen Inspektionsdaten darf eine bisher
erfolgreiche Alt-Geometrieabfrage nicht scheitern lassen.
Alte Ausschlüsse für Physikdaten bleiben in Altantworten wirksam.
Kompatibilitätsadapter übersetzen neue interne Diagnosen ausdrücklich in
den bisherigen Vertrag; kein pauschales Abfangen oder stiller Ersatz.

Gemeinsamer Code bedeutet nicht gemeinsamer Lebenszyklus: Altaufrufe und
Simulation behalten ihre bisherigen Datei-/Fassungsprüfungen. Nur explizite
Inspektionssnapshots bleiben nach Dateiänderungen unverändert.
Kein globaler Cache, keine automatische Umstellung der Simulation auf
einen möglicherweise älteren Inspektionsstand und keine neue
Nutzerbedienung durch diese technische Zusammenführung.

**Abnahme:** Vor dem Refactoring Charakterisierungstests der Altaufrufe
anlegen, anschließend Alt-/Neu-Projektionen desselben aufgenommenen
Bestands vergleichen. Gültige, physikalisch ungültige und nur teilweise
lesbare Modelle, unsichtbare Teile, Volumenersatz, unbekannte IDs sowie
Dateiänderungen zwischen getrennten Altaufrufen abdecken.
Aufrufpfade und instrumentierte Leser nachweisen: Innerhalb einer
Snapshotaufnahme wird dieselbe Quelldatei nicht für Struktur und Geometrie
erneut geparst; spätere Detailabfragen lesen nichts nach.
Bestehende eigenständige Aufrufe dürfen ihre vertraglich erforderliche
neue Aufnahme ausführen. Codeprüfung belegt, dass keine alternative
Parser-/Meshleserimplementierung für die Inspektion bestehen bleibt.

## 4. C-0.8-Erweiterung: objektzentrierte Gesamtinspektion

**Status: fachlich finalisiert am 02.11.2026; Umsetzung gemäß dem
maßgeblichen C-0.8-Plan beauftragt.** Die Herkunftszuordnung steht in
[Modellinspektion.json](Entwurfspunkte/Modellinspektion.json). Der
Schema-3-Ein-Quellmesh-Vertrag ist in der
[Modellbeschreibung](Spezifikation_Modellbeschreibung.md) festgelegt.

### A-C-00112: Kanonischer Objektbaum und bereichszugeordnete Eigenschaften

`getObjectTree(snapshot, "objekte")` liefert ausschließlich die Hierarchie
Modell → Baugruppe → Element. Jedes Element kommt genau einmal vor.
Modellmetadaten und Fachbereiche wie Geometrie, Darstellung, Masse,
Hydrostatik, Hydrodynamik und Bewegung erzeugen keine Hierarchieknoten,
sondern bleiben Eigenschaften der Modell- beziehungsweise Elementknoten.
Andere vorhandene Baumsichten und Fachabfragen bleiben kompatibel.

`getObjectProperties(snapshot, "objekte", ids, true)` gibt über denselben
unveränderlichen Snapshot alle vorhandenen Quell- und abgeleiteten
Facheigenschaften des ausgewählten Knotens und seiner Nachkommen zurück.
Jede Eigenschaft erhält das additive, exakt benannte Feld `area` mit einem
der stabilen Bereichswerte `Model`, `Geometrie`, `Darstellung`, `Masse`,
`Material`, `Hydrostatik`, `Hydrodynamik` oder `Bewegung`. Typen, Einheiten,
Verfügbarkeitsstatus, stabile Objekt-/Property-IDs und Herkunftsinformation
bleiben erhalten. Mehrfachauswahl mit überlappenden Nachkommen dedupliziert
eine logische Eigenschaft; Werte werden nicht automatisch summiert.

**Abnahme:** Der kanonische Baum enthält keine `entwurf`-, `geometrie`-,
Property- oder Fachbereichs-Pseudoknoten. Feldabdeckung umfasst alle bislang
freigegebenen Modellfelder und abgeleiteten Eigenschaften, inklusive
nichtskalarer Werte. C++- und JSON-Antworten stimmen überein; Mutation der
Quelldateien nach Aufnahme ändert Baum und Eigenschaften nicht.

### A-C-00113: Typisierte Bewegungs-Vorschau aus dem Snapshot

`modellstruktur(snapshot).gruppen[].elemente[].bewegung` bleibt die
kompatible Quelle der typisierten `Bewegungsdefinition`: Art, Achse,
Drehpunkt, Hubrichtung, mitbewegte Elemente und vorhandener Hubweg. Die
Achsen und Punkte sind modellfest in Metern. `getMovementPreview(snapshot,
element_id)` liefert eine optionale `InspectionMovementPreview` mit
Referenzstellung, vorhandener Achse, optionalem Drehpunkt, tatsächlichen
Steuerwertgrenzen und Snapshot-`InspectionMesh`-Geometrien für die
Normalstellung sowie verfügbare Grenzstellungen. Die Geometrien werden mit
der bestehenden Core-Bewegungstransformation aus dem eingelesenen Snapshot
abgeleitet; die GUI implementiert keine eigene Bewegungsphysik.

Für bestehende Prozentsteuerungen gelten die vom Core verwendeten Werte
0–100 mit Normalstellung 100. Eine als `ruderwinkel` deklarierte Drehung
verwendet Grad und Normalstellung 0; da ihr Modellvertrag keine
Winkelgrenzen enthält, werden keine Minimum-/Maximumstellungen behauptet
oder erfunden. Translation hat eine Richtungsachse, aber keinen künstlichen
Drehpunkt. Fehlt eine Bewegungsdefinition, ist das Ergebnis ausdrücklich
optional; fehlende Preview-Geometrie oder ungültige Physik liefert eine
Diagnose/Fehler statt einer plausibel wirkenden Ersatzvorschau. Die
Inspektion verändert weder Bewegungsdefinition noch Simulation.

**Abnahme:** Achse, Bezugssystem, Normalstellung, Prozent-/Gradsemantik,
vorhandene Grenzen und transformierte Meshpunkte werden gegen bestehende
Core-Bewegungsdefinitionen geprüft. Bei nicht modellierten Grenzen bleibt
der jeweilige Wert `null`/nicht vorhanden. JSON und C++ liefern dieselben
Referenz-, Achs- und Pose-Daten.

Die Meterteilung des Darstellungsrasters bleibt eine GUI-Aufgabe auf Basis
der bestehenden Meter-/Koordinatenkonvention. Sie ändert keine
Core-Stützstellen oder Rechennetze und benötigt kein physikalisches Raster.
