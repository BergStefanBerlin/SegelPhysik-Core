# Spezifikation der einheitlichen Modellbeschreibung

## Status und Geltungsbereich

**Bisheriger Montage-Ablagevertrag (vor C-0.21-Cutover):** Dateien heissen
`modells_information.json`. Jedes vorhandene Element darf ein `setting` direkt
neben `id` und `Darstellung` tragen: `translation_m`, `rotation_xyzw`,
`konfiguriert`. Default ist Identitaet/false; endliche Translation und
normiertes Quaternion werden fuer alle Elemente validiert. Unbekannte
Setting-Felder bleiben beim Speichern erhalten. Nur Segel ergaenzen
`original_buglage` und `quellen_fingerprint`; ihre intrinsische Quelle
und Lookup werden genau einmal als `Segel.quelle_datei` und
`Segel.lookup_datei` am bestehenden Element referenziert. Ohne tatsaechliche
Lookup bleibt `lookup_datei` aus; es wird keine neue Kopplung erfunden.
Es entstehen keine neuen Objekt-IDs oder zentralen Elementmaps.
Gemeinsame Revision/Fingerprints liegen in `konfiguration`.
Der verbindliche Persistenz-/Montagevertrag steht in
[Spezifikation Segelkonfigurator, Abschnitt 4](Spezifikation_Segelkonfigurator.md).
Bestaetigte generische Montageposen werden in der Darstellung angewendet.
420-Segelquellen bleiben fuer die bestehende GUI-Kopplung unmontiert und
werden in der Segel-API montiert; Folk-/Opti-OBJ-Segel sind im
Inspektionsmesh bereits montiert. Die Segel-API laedt fuer diese Modelle
die unveraenderte Quellgeometrie erneut und montiert sie genau einmal.
`Montage` am bestehenden Element dokumentiert verifizierte OBJ-Eckbezüge
der neutralen Mittschiffsbasis; Mast-/Vorliekneigung bleibt erhalten.
Physische Netze, Massen und Traegheit
werden dadurch nicht implizit transformiert; dynamische Winkel bleiben
getrennt. Dieser Abschnitt beschreibt den vorhandenen Legacy-Vertrag bis zum
separat geregelten C-0.21-Cutover; danach ist für den aktiven Modellkatalog
ausschließlich A-C-00148 maßgeblich. Der gemeinsame Basisfingerprint blendet nur echte
Element-Settings und gemeinsame Konfigurationsmetadaten aus.

Die Koordinatenanforderungen A-C-00001 bis A-C-00003 wurden am
26.10.2026 vom Nutzer ausdrücklich final freigegeben.
Zusätzlich wurde am selben Tag der vorhandene Beispielaufbau als
Grundlage des ersten Modellimports bestätigt (A-C-00030).
Damit werden weder die physikalische Richtigkeit der Beispieldaten
noch eine Implementierung freigegeben oder nachgewiesen.

Dieses Dokument ist die maßgebliche Spezifikation für das sachlogische
Thema Modellbeschreibung in SegelPhysik-Core. Die Modellkonventionen
sind für die gemeinsame Verwendung der Modelldaten durch Core und GUI
vorgesehen. Ein GUI-Release oder Änderungen im GUI-Repository werden
dadurch nicht beauftragt.

Noch nicht finalisierte Themen stehen ausschließlich in der
[Entwurfspunkteliste](Entwurfspunkte/Modellformat.json). Die
[Beispielsammlung](Beispiele_Modellformat/README.md) veranschaulicht
Arbeitsstände und ist keine zusätzliche normative Spezifikation.

## Freigegebene Anforderungen

### A-C-00001: Einheitliches modellfestes Koordinatensystem

Alle Modelle verwenden ein rechtshändiges, modellfestes Koordinatensystem:

| Achse | Positive Richtung |
|---|---|
| X | Nach vorn, bei Booten zum Bug |
| Y | Nach links, bei Booten nach Backbord |
| Z | Nach oben in der aufrechten Referenzlage des Modells |

Die Achsen sind mit dem Modell verbunden und bewegen bzw. drehen sich
mit ihm. Bei einfachen Körpern ohne Bug oder Backbord gelten dieselben
Achsen als konstruktive Richtungen ihrer Referenzlage.

Die Konvention gilt zentral für alle Modelle. Ein Modell darf keine
abweichende Achsenkonvention voraussetzen; ein zusätzlicher
Koordinatensystemblock pro Modelldatei ist nicht erforderlich.

**Abnahmekriterium:** An einem Referenzmodell liegen vorwärts, links
und oben jeweils auf der positiven X-, Y- bzw. Z-Achse. Es gilt
X × Y = Z. Bei einer Drehung des Modells drehen die modellfesten Achsen
mit; die Modelldateien benötigen dafür keinen individuellen
Achsenkonventionsschalter.

**Herkunft:** Koordinatenklärung und ausdrückliche Nutzerfreigabe am
26.10.2026; Teilaspekt von IDEE-C-00004.

### A-C-00002: Ortsfestes Weltsystem und Ruhewasseroberfläche

Das Weltsystem ist vom modellfesten System zu unterscheiden. Es ist
ortsfest und rechtshändig; seine positive Z-Achse zeigt nach oben.
Die horizontale Ruhewasseroberfläche liegt im Weltsystem bei Z = 0.

Eine Kurs- oder Lageänderung eines Modells dreht nicht die Weltachsen.
Insbesondere ist die positive Welt-X-Achse nicht dauerhaft durch die
jeweils aktuelle Bugrichtung definiert.

Eine geografische Zuordnung der horizontalen Weltachsen, etwa zu
Nord und Ost, wird mit dieser Anforderung nicht festgelegt.

**Abnahmekriterium:** Bei einer Modellrotation bleibt das Weltachsensystem
unverändert. Punkte der horizontalen Ruhewasseroberfläche haben im
Weltsystem Z = 0, unabhängig von der Lage des Modells.

**Herkunft:** Koordinatenklärung und ausdrückliche Nutzerfreigabe am
26.10.2026; Teilaspekt von IDEE-C-00004.

### A-C-00003: Fester Konstruktionsursprung des Modells

Der Ursprung des modellfesten Systems ist ein fester Konstruktionsbezug.
Er darf weder implizit mit dem veränderlichen Gesamtschwerpunkt
gleichgesetzt noch aus der aktuellen Wasserlinie abgeleitet werden.

Eine Änderung der Massenverteilung, der Beladung oder der Eintauchtiefe
verschiebt den Konstruktionsursprung nicht relativ zur Modellgeometrie.
Der Modellursprung muss nicht auf der Ruhewasseroberfläche liegen.

Die konkrete geometrische Wahl dieses Bezugspunkts, beispielsweise
Rumpfunterkante oder ein Konstruktionsdatum, wird hier noch nicht
modellübergreifend vereinheitlicht. Auch ein gegebenenfalls nötiger
Höhenversatz zum Weltsystem darf nicht mit einer Änderung der
Modellgeometrie verwechselt werden.

**Abnahmekriterium:** Werden Besatzungsposition oder Beladung verändert,
kann sich der Gesamtschwerpunkt ändern, während Ursprung und Koordinaten
der unveränderten Modellgeometrie gleich bleiben. Dasselbe gilt bei
veränderter Eintauchtiefe.

**Herkunft:** Koordinatenklärung und ausdrückliche Nutzerfreigabe am
26.10.2026; Teilaspekt von IDEE-C-00004.

### A-C-00030: Beispielstruktur als Grundlage des ersten Modellimports

Der erste Modellimport übernimmt den gemeinsamen JSON-Aufbau der
Beispielsammlung: Modellidentität, Gruppen als Organisationsknoten,
Elementlisten mit modellweit eindeutiger lesbarer `id`,
zweckgebundener `Darstellung` und/oder `Hydrostatik`.
`Darstellung.datei` referenziert das Grafikmesh.
`Hydrostatik.masse` ist verpflichtend, `Hydrostatik.volumen` optional.
Dateireferenzen sind relativ zur Modelldatei aufzulösen.

Unter `masse` werden die bestehenden Varianten `punktmasse`
(`masse_kg`, `position_m`), `vollkoerper` (`dichte_kg_pro_m3`, `datei`)
und `schale` (`dichte_kg_pro_m3`, `wanddicke_m`, `datei`) verwendet.
`volumen.datei` referenziert ausschließlich die Verdrängungsgeometrie.
Gruppenpositionen bestimmen kein physikalisches Kraftmodell.
Ohne Volumen gilt exakt null Verdrängung, ohne Geometrieersatz.

Die Struktur bleibt ausdrücklich weiterentwickelbar.
Unterstützte Schemaversionen, Feldtypen, Pflicht-/Optionalfelder und
Änderungsregeln sind vor dem Import zu dokumentieren und zu testen.
Temporäre Entwurfsmetadaten der Beispiele sind keine Berechnungsparameter.
Insbesondere wird eine bestehende Kennzeichnung fehlender
Simulationsfreigabe nicht automatisch zur erteilten Freigabe umgedeutet.

Modelle können falsch oder ungenau sein. Die Übernahme des Aufbaus ist
keine Bestätigung ihrer Eignung; ungeeignete Modelle werden mit
eindeutigen Diagnosen abgelehnt und später separat korrigiert.

**Abnahmekriterium:** Tests decken die drei Massenvarianten, reine
Darstellung, optionale Verdrängung, Dateiauflösung und Modell-IDs ab.
Ungültige Eingaben werden nicht still normalisiert oder umgedeutet.
Der tatsächlich unterstützte Feld- und Versionsvertrag ist dokumentiert.

**Herkunft:** Nutzerentscheidung zur ersten Importgrundlage am
26.10.2026; Teilaspekte von IDEE-C-00001, IDEE-C-00002 und IDEE-C-00006.

### A-C-00048: Optionale Darstellungsfarbe je Element

Ein Element darf in `Darstellung` das optionale Feld `farbe` im Format
`"#rrggbb"` (hexadezimal, Groß-/Kleinschreibung beliebig) tragen. Der
Core reicht die Farbe über `darstellungsgeometrie` als `farbe_rgb` mit
Werten 0..1 weiter (A-C-00044). Ohne Angabe ist `farbe_rgb` `null` und
die GUI verwendet ihre Standardfarbe. Die Farbe hat keinen Einfluss auf
die Physik. Ein ungültiger Wert wird nicht still ersetzt: Das Modell ist
fehlerhaft (CORE-MOD-0002 mit Fundstelle `Darstellung.farbe`).

**Abnahmekriterium:** Ohne Angabe keine Farbe; `#FF8000` ergibt
(1, 128/255, 0) in Struktur und JSON; ein ungültiger Wert macht das
Modell fehlerhaft und liefert keine Farbe.

**Herkunft:** Nutzerauftrag vom 26.10.2026 (Optimist einfärben).

### A-C-00049: Optionale Darstellungsoberfläche je Element

Ein Element darf in `Darstellung` das optionale Feld `oberflaeche` mit
einem der Werte `"matt"`, `"glaenzend"`, `"metall"`, `"chrom"`, `"lack"`,
`"glas"`, `"leuchtend"`, `"schillernd"`, `"facettiert"`, `"drahtgitter"`,
`"perlmutt"`, `"unsichtbar"` (Teil wird nicht gezeichnet, z. B.
Luftkörper nach A-C-00051), `"stoff"`, `"holz"` oder
`"metall_gebuerstet"` tragen. Die letzten drei Werte bezeichnen explizit
Gewebe, Holz und gebürstetes Metall; `"metall"` und alle bisherigen Werte
behalten ihre Bedeutung. Die Bezeichner sind stabile, kleingeschriebene
Darstellungsmetadaten, keine physikalischen Materialdefinitionen. Der Core
reicht den Wert unverändert über `darstellungsgeometrie` als
`oberflaeche` weiter (A-C-00044). Ohne Angabe ist `oberflaeche` `null`
und die GUI verwendet ihr Standardmaterial. Die Oberfläche hat keinen
Einfluss auf die Physik. Ein ungültiger Wert wird nicht still ersetzt:
Das Modell ist fehlerhaft (CORE-MOD-0002 mit Fundstelle
`Darstellung.oberflaeche`).

Der bestehende optionale Stringtransport genügt; Schema und öffentliche
Datentypen bleiben unverändert. UV-Koordinaten oder Texturdateien sind nicht
vorgeschrieben. Die GUI darf eine geeignete prozedurale Projektion aus den
lokalen Modellkoordinaten ableiten. Die Wahl der Oberfläche darf Masse,
Schwerpunkt, Trägheit, Volumennetze, verdrängtes Volumen, Kräfte und Momente
nicht verändern. Die Modellfassung ändert sich bei geänderter JSON-Datei
weiterhin zur Cache-Invalidierung.

**Abnahmekriterium:** Ohne Angabe keine Oberfläche; jeder zugelassene Wert
erscheint unverändert nach erneutem Einlesen in Beschreibung, Katalogteil
und JSON. Gegenüber demselben Modell ohne Oberfläche bleiben Massenwerte,
Volumennetze und eine Hydrostatikauswertung identisch. Ein ungültiger Wert
(z. B. `"regenbogen"`) macht das Modell mit CORE-MOD-0002 und passender
Fundstelle fehlerhaft und liefert keine Oberfläche.

**Herkunft:** Nutzerauftrag vom 27.10.2026 (Grafikeffekte, Materialien);
Effektwerte ergänzt auf Nutzerauftrag vom 26.10.2026 (einfache Körper
grell und je mit eigenem Effekt); `"unsichtbar"` ergänzt auf
Nutzerauftrag vom 28.10.2026 (Luft muss durchsichtig sein).
Nachtrag vom 27.10.2026 für GUI G-0.5: ausdrücklicher Nutzerauftrag zur
vollständigen Umsetzung einschließlich persistenter Stoff-, Holz- und
gebürsteter Metalloberflächen; bestehender Vertrag A-C-00049 wird erweitert,
keine neue Anforderung oder parallele Spezifikation.

### A-C-00061: Explizite Hydrodynamikrolle eines Modellelements

Ein Modellelement darf ein optionales Geschwisterfeld `Hydrodynamik` neben
`Darstellung` und `Hydrostatik` tragen. Es ist ein Objekt mit dem Pflichtfeld
`rolle`, dessen zulässige Werte `rumpf`, `kiel` und `ruder` sind. Das Feld
kennzeichnet ausschließlich die physikalische Funktion des Elements für die
Core-Hydrodynamik; es ersetzt keine Masse oder Verdrängungsdefinition.

Die Rolle bezieht sich auf das physische `Hydrostatik.volumen.datei`-Mesh.
Ein Element mit Hydrodynamikrolle benötigt deshalb ein solches Mesh. Die
`Darstellung`-Geometrie und der Dateiname liefern keine Hydrodynamikrolle.
Ein Modellelement darf höchstens eine Rolle tragen; unbekannte Rollen und
unbekannte Schlüssel werden mit konkreter Modellfundstelle abgelehnt. Modelle
ohne dieses Feld behalten unverändertes bisheriges Verhalten.

Die Rolle wird in der importierten Modellfassung berücksichtigt. Der
Hydrodynamikrechner leitet daraus weder Profilkennwerte noch empirische
Widerstandskoeffizienten ab. Solche Rechenparameter bedürfen eigener
validierter Verträge und dürfen nicht aus der bloßen Rollenangabe erfunden
werden.

**Abnahmekriterium:** Import akzeptiert die drei Rollen nur bei vorhandenem
physischem Volumenmesh, weist sie dem richtigen Element zu und bindet die
Modelldatei in die Modellfassung ein. Fehlendes Mesh, ungültige Rolle oder
zusätzliche unbekannte Schlüssel werden mit Fundstelle abgelehnt. Modelle
ohne `Hydrodynamik` importieren weiterhin wie zuvor.

**Herkunft:** Finalisierung und Implementierungsauftrag Hydrodynamik,
27.10.2026; Entwurfspunkt IDEE-C-00033.

### Zielerweiterung des Hydrodynamikblocks für den Folgeumfang

A-C-00061 beschreibt den mit C-0.6 eingeführten Rollenvertrag. Für den
fachlich finalisierten Hydrodynamik-Folgeumfang darf das optionale Feld
`Hydrodynamik` am selben Modellelement zusätzlich `modelle` und eine optionale
`materialreferenz` enthalten. Die verbindliche Struktur, Pflichtmetadaten,
Herkunftsstatus, Gültigkeitsbereiche und Kennfeldregeln sind in
[A-C-00076 der Hydrodynamik-Spezifikation](Spezifikation_Hydrodynamik.md#a-c-00076-objektparameter-provenienz-und-materialreferenzen)
festgelegt. Sie ändert den C-0.6-Importvertrag nicht rückwirkend. Der
Core-Importer unterstützt die optionalen Modellparameter, Kennfelder und
Materialreferenz im C-0.7-Folgeumfang; Schema-, Provenienz- und
Hydrodynamiktests decken die Implementierung ab. Die vollständige technische
und fachliche Abnahme ist im zugehörigen Umsetzungsplan separat
nachzuweisen.

## Quellen und Abgrenzung zur Historie

Die historische Spezifikation vF1 in SegelPhysik3 beschreibt ein
rechtshändiges System mit X zum Bug, Y nach Backbord und Z nach oben
sowie der Wasseroberfläche bei Z = 0. Ihre Bezeichnung als
„Weltkoordinaten“ mit X in Bugrichtung wird hier ausdrücklich durch
die Trennung von modellfestem und ortsfestem System präzisiert.

Die historische Spezifikation V0.9 zu Modellpaketen beschreibt einen
rumpffesten Referenzpunkt unabhängig vom Gesamtschwerpunkt. Dessen
damalige konkrete Lage an der Rumpfunterkante ist mit der vorliegenden
Freigabe nicht als allgemeine Ursprungsregel übernommen.

Historische Dateien sind Herkunftsbelege, keine zusätzliche normative
Spezifikation dieses Themas.

## Nachverfolgbarkeit und Umsetzungsstand

| Anforderung | Ursprung | Fachlicher Status | Umsetzung und Nachweis |
|---|---|---|---|
| A-C-00001 | IDEE-C-00004, Teilaspekt Modellachsen | Freigegeben | C-0.1: Hydrostatikplan U-C-000017; pose_transformations und hydrostatik_referenzen |
| A-C-00002 | IDEE-C-00004, Teilaspekt Weltsystem | Freigegeben | C-0.1: Hydrostatikplan U-C-000008 und U-C-000017; Schnitt- und Transformationstests |
| A-C-00003 | IDEE-C-00004, Teilaspekt Konstruktionsursprung | Freigegeben | C-0.1: Hydrostatikplan U-C-000007 und U-C-000017; Schwerpunkt-, Pose- und Dynamiktests |
| A-C-00030 | IDEE-C-00001, IDEE-C-00002, IDEE-C-00006, Teilaspekte Importgrundlage | Als Grundlage bestätigt | C-0.1: Import implementiert, Test modell_import; unterstützte Grenzen im GUI-Vertrag dokumentiert |
| A-C-00048 | Nutzerauftrag 26.10.2026 (Farben) | Freigegeben | C-0.2 Nachtrag: Plan C-0.2 U-C-000032; Test katalog (`farben`) |
| A-C-00049 | Nutzerauftrag 27.10.2026 (Materialien) | Freigegeben; technisch nachgewiesen, keine ästhetische Nutzerabnahme | C-0.2 Nachtrag: Plan C-0.2 U-C-000033; G-0.5-Vertragserweiterung am 27.10.2026: vollständiger nativer Release-Build und gesamte gemeinsame CTest-Suite 22/22 in 64,55 s, einschließlich aller CORE-Tests, `modellkatalog_schnittstelle` (alle 15 Namen, CLI-Validierungsfunktion, Physikinvarianz) und drei GPU-Tests; GUI-Modelladapter mit sechs Materialbezeichnern bestanden; Details im Plan |
| A-C-00061 | IDEE-C-00033 | Freigegeben für C-0.6; Core-/Modellvertrag | C-0.6: U-C-000045 und U-C-000046; Modellimport- und Geometrietests |
Die Umsetzung ist inzwischen über den
[Hydrostatikplan](../Umsetzungsplaene/Umsetzungsplan_Hydrostatikkern.md)
beauftragt. Die Abnahmekriterien sind noch nicht als bestanden
nachgewiesen. Rotationsdarstellung, Gelenke, lokale Elementbezüge,
konkrete Ursprungswahl und weitere Modellthemen bleiben offen.

## C-0.8-Erweiterung: konsolidierte Quellgeometrie (02.11.2026)

Vom Nutzer zur Umsetzung freigegeben: Alternative 2 mit stabiler,
rollenunabhängiger Identität. Dieser Abschnitt erweitert dieselbe
maßgebliche Themenspezifikation; die bisherigen Anforderungen und das
Altformat bleiben bestehen. Persönliche GUI-Abnahme bleibt separat offen.

### A-C-00105: Ein OBJ mit expliziten Teilnetzen, Schema `"2"`

Die JSON-Datei führt Gruppen, Elemente und ihre fachlichen Eigenschaften
weiter. Genau ein oberster Geometrieverweis
`"geometrie": {"datei": "boot.obj", "laengeneinheit": "m"}` benennt die
gemeinsame OBJ-Datei. Der relative Dateiname ist frei wählbar, Endung `.obj`.
`Darstellung`, `Hydrostatik.masse` (außer Punktmasse) und
`Hydrostatik.volumen` verwenden jeweils `"mesh": "<o-ID>"` statt `datei`.
Ein Teilnetz beginnt mit `o <stabile-ID>`. `g`, Material- oder Dateinamen
sind keine Ersatzidentitäten. Mehrere fachliche Verwendungen dürfen
dasselbe Teilnetz referenzieren. Auch unbenutzte Quellvarianten sind
inspektierbar. Fehlende, leere oder doppelte `o`-IDs, Flächen ohne `o`,
fehlende/ungültige Referenzen und andere Längeneinheiten werden abgelehnt.
Globale Vertex-Indizes, negative Indizes und vorhandene `v/vt/vn`-Verweise
bleiben korrekt; Teilnetze werden mit lokalen Indizes bereitgestellt.
Vertices unter einem `o` sowie aus anderen Bereichen referenzierte
Vertices gehören zum Teilnetz; die Reihenfolge ist die globale Quellreihenfolge.

**Abnahme:** Import-/Negativtests, globale/negative Indizes und geteilte
Referenzen in `modell_geometrie_v2`.

### A-C-00106: Identität und hydrodynamische Verwendung getrennt

Element-ID und Mesh-ID sind unabhängig voneinander und von Anzeigenamen.
Nur ein expliziter Block `Hydrodynamik` aktiviert die hydrodynamische
Verwendung; darin ist `rolle` verpflichtend und ausschließlich `rumpf`,
`kiel` oder `ruder`. Die Rolle benötigt wie bisher ein physisches Volumen.
Das Fehlen des gesamten Blocks bedeutet bewusst keine hydrodynamische
Verwendung (z. B. Luftkörper); es ist keine Aufforderung zur Rolleninferenz.
Namen wie `rumpf`, `quader` oder `ruder` lösen im Schema 2 keine Rolle aus.
Mehrere Elemente dürfen dieselbe Rolle haben; Rollen sind keine IDs.
Die historische Sonderregel der acht einfachen Körper bleibt ausschließlich
im Altformat erhalten. Der Konverter materialisiert deren bisher implizite
Rolle ausdrücklich im neuen JSON.

**Abnahme:** Umbenannte Elemente/Modelle behalten explizite Rollen;
fehlende/ungültige Rollen innerhalb eines Blocks werden abgelehnt; kein
Block erzeugt keine Rolle, auch beim Modellnamen `quader`.

### A-C-00107: Gemeinsamer Import ohne Änderung der Physik

Altformat `0.1-entwurf` und Schema 2 verwenden dieselben Core-Bausteine
für OBJ-Lesen, Geometrievalidierung, Massenintegration, Bewegung und
Simulation. Für alte Darstellungsdateien bleibt der bisherige tolerante
Modus einschließlich Fan-Triangulierung erhalten; Schema 2 fordert
Dreiecke in allen Teilnetzen. Fachliche Rollen wählen vorhandene
Quellgeometrie aus und reparieren, schließen oder erfinden keine Flächen.

**Notwendige Präzisierung zu „ein Mesh pro Bauteil“:** Ein OBJ pro Modell
ist verbindlich, aber ein physisches Element kann mehrere benannte
Quellvarianten referenzieren. Die vorhandenen Referenzboote besitzen teils
unterschiedliche Darstellungs-, offene Schalenmassen- und geschlossene
Verdrängungsnetze. Deren Zusammenlegung würde Masse, Trägheit, Flutrand
oder Verdrängung ändern. Deshalb bleiben die real vorhandenen Varianten
als getrennte `o`-IDs in derselben Datei erhalten. Keine automatische
Ableitung eines geschlossenen Volumens aus einer offenen Schale.

**Abnahme:** Alle elf unveränderten Modelle werden über Alt-/Neuimport
auf exakte Massen, Schwerpunkte, Trägheiten, Netze, Flutränder, Bewegungen,
Darstellung, Struktur und Diagnose-IDs verglichen; dynamische
Simulationszustände bleiben bis auf formatbedingt geänderte
Modellfassungskennungen identisch.

### A-C-00108: Quellregister und Herkunft im unveränderlichen Snapshot

`getMesh(snapshot, "quelle")` liefert jedes Quellteilnetz genau einmal,
auch ohne Verwendung. `source_id` ist im Schema 2 die stabile `o`-ID;
im Altformat vergibt Core eine innerhalb der Beschreibung stabile
Quellkennung pro normalisierter Dateireferenz. Dateipfade werden nicht
als öffentliche Identität verwendet. `element_ids` enthält alle
Verwendungen; `element_id` bleibt kompatibel die erste oder leer.
Elementfilter berücksichtigen jede Verwendung.

Alle Mesh-Antworten führen `provenance` (`quelle` oder `abgeleitet`),
`source_id` (gegebenenfalls leer) und lesbare `derivation`.
Fachnetze sind unveränderte Quellkopien. Massenpunkte sind eine
abgeleitete Punktprojektion der Quellvertices, keine integrierten
Punktmassen; bei Punktmassen stammen sie aus expliziten JSON-Positionen.
Der Flutrand enthält tatsächlich durch Kanteninzidenz ermittelte offene
Schalenrandknoten, keine erfundene geschlossene Begrenzung.
`InspectionNode.mesh_ids` verbindet vorhandene Elementknoten mit den
Mesh-Antworten; die GUI darf daraus Quelle-/Berechnet-Unterknoten bilden.
Die bestehenden Fachtypen und JSON-Hüllen bleiben kompatibel, neue
Metadaten sind additiv. Dateiänderungen nach Erstellung verändern den
besitzenden Snapshot nicht und erzeugen keine späteren Dateizugriffe.
Der Schema-2-Fassungsabdruck berücksichtigt das ganze einzelne OBJ
einschließlich Darstellungs- und unbenutzter Varianten.

**Abnahme:** Quellen-/Filter-/Verknüpfungsprüfungen, Variantenvollständigkeit,
Snapshotabfragen nach Quelldateiänderung und Fassungsablehnung neuer
Abfragen mit alter Katalogfassung.

### A-C-00109: Explizite, geschützte Migration

`konvertiere_modell_v2(quelle, zielordner)` sowie das Kommando
`segelphysik_konvertieren <modells_information.json> <neuer Zielordner>` lesen über die
gemeinsamen Importer und erzeugen ausschließlich einen neuen Zielordner
mit `modells_information.json` und `boot.obj`. Ein existierender Zielordner wird
abgelehnt, auch bei identischem Inhalt. Originaldateien bleiben
unverändert. Der Konverter übernimmt vorhandene Geometrievarianten,
Material-/Farbmetadaten, Bewegungen und explizite physikalische Daten.
Punktmassen benötigen kein erfundenes Quellnetz. Nicht lesbare
Darstellungsquellen werden nicht ersetzt, sondern verhindern die Migration.
Schreib-/Speicherfehler werden gemeldet; ein dann gegebenenfalls bereits
angelegter unvollständiger Zielordner wird nicht stillschweigend entfernt.

**Abnahme:** Konvertierung aller elf Modelle, Schutz vorhandener Ziele
und unveränderte Originaldateien.

### Rückverfolgbarkeit der Erweiterung

| Anforderung | Plan C-0.8 | Nachweis |
|---|---|---|
| A-C-00105 | U-C-000086, U-C-000089 | `modell_geometrie_v2`, Objekt-/Referenz-Negativfälle |
| A-C-00106 | U-C-000086, U-C-000089 | explizite/fehlende Rolle, unabhängige IDs |
| A-C-00107 | U-C-000086, U-C-000089 | elf Alt-/Neumodelle, dynamischer Simulationsvergleich |
| A-C-00108 | U-C-000087, U-C-000089 | Quellregister, Herkunft, Filter, Snapshotpersistenz |
| A-C-00109 | U-C-000088, U-C-000089 | Migration, Original-/Überschreibschutz |

## C-0.8-Erweiterung: eine geometrische Quelle je Element

**Status: fachlich finalisiert am 02.11.2026; Umsetzung gemäß dem
maßgeblichen C-0.8-Plan beauftragt.** Die Herkunftszuordnung steht in
[Modellformat.json](Entwurfspunkte/Modellformat.json).

### A-C-00110: Ein-Quellmesh-Vertrag (Schema 3)

Schema 3 führt den Vertrag „eine Quelle je geometrischem Element“ ausdrücklich
ein. Modellordner und Schema 0.1 sowie Schema 2 bleiben lesbar und behalten
ihre bisherige Semantik. Schema 3 nutzt ein gemeinsames OBJ mit benannten
Elementnetzen; für jedes Element referenzieren Darstellung, Massenintegration,
Hydrostatikvolumen und Hydrodynamik dieselbe Mesh-ID. Die Mesh-ID ist unabhängig
von Element-ID, Anzeigename und expliziter physikalischer Rolle. Nicht
geometrische Punktmassen bleiben ohne künstliches Mesh.

Mehrere Verwendungen derselben Quelle sind zulässig; getrennte, konkurrierende
Quellnetze für ein Element sind im Schema 3 unzulässig. Das gemeinsame Mesh
muss für jede deklarierte Verwendung geeignet und gemäß den bereits geltenden
Import-/Physikregeln valide sein. Unvereinbare offene Schalen und geschlossene
Volumen werden nicht automatisch repariert, geschlossen oder ersetzt.

**Präzisierung nach Nutzerfreigabe vom 02.11.2026:** Die bestehende Physik
soll ausreichend genau, nicht bitgenau erhalten bleiben. Verwendungen dürfen
deshalb explizite, reproduzierbare Teilgeometrien derselben Quelle bestimmen.
Dies ist keine zweite editierbare Quelle und keine allgemeine Netzreparatur.
Schema 3 erlaubt optional `ableitung` in `Darstellung`,
`Hydrostatik.masse` und `Hydrostatik.volumen`:

```json
{
  "mesh": "element_mesh_1",
  "ableitung": {
    "art": "z_schnitt",
    "z_m": -0.47,
    "steigung_x": 0.0,
    "steigung_y": 0.0
  }
}
```

- `z_schnitt` verwendet den Quellteil unterhalb der modellfesten Ebene
  `z = z_m + steigung_x*x + steigung_y*y`; Steigungen sind optional und
  standardmäßig 0. Die deklarierte Schnittfläche wird reproduzierbar
  trianguliert und geschlossen. Das Quellnetz muss bereits ein valider
  geschlossener Körper sein. Das ist eine explizite Verwendung, kein
  automatisches Schließen beschädigter Eingaben.
- `ohne_deckflaeche` mit `z_m` entfernt ausschließlich die horizontalen,
  in dieser Höhe liegenden Dreiecke aus der Quelle. Es ist eine
  Oberflächenauswahl für Darstellung oder Schalenmasse, nicht für Volumen
  oder Vollkörpermasse. Die offenen Kanten der ausgewählten Masseschale
  liefern weiterhin über den bestehenden Algorithmus die Flutrandpunkte.
- Ein Schnitt muss die Quelle tatsächlich teilen. Unbekannte Felder,
  nicht endliche Werte, leere Auswahlen, ungültige Topologie sowie mehrere
  Schnittringe oder Löcher werden ausdrücklich abgelehnt. Kein heuristisches
  Füllen mehrerer Ringe. Geometrische Ebenentoleranz:
  `1e-9 * max(1 m, Z-Ausdehnung der Quelle)`.
- Ableitungen bleiben ausschließlich Core-Aufgabe. Die Inspektion zeigt
  Quelle und verwendete Teilgeometrie getrennt mit `source_id`,
  `provenance = abgeleitet` und tatsächlicher Ableitungsdefinition.
  Massenstützstellen werden aus der ausgewählten Massengeometrie extrahiert;
  kein neuer Schwerpunkt-, Trägheits-, Flutungs- oder Stützstellenalgorithmus.

**Abnahme:** Schema-3-Validierung weist Mehrfachquellen pro Element ab und
nimmt zulässige gemeinsame Referenzen an. Legacy- und Schema-2-Importe
bleiben regressionsgeprüft; bestehende Massen-, Schwerpunkt-, Trägheits-,
Volumen- und Stützstellenalgorithmen werden nicht geändert.

### Zielvertrag und Abgrenzung zum bisherigen Schema 2

Für jedes flächengeometrisch definierte Element existiert genau ein
maßgebliches, bearbeitbares Quellmesh. Eine gemeinsame OBJ-Datei enthält
die benannten Elementnetze; sie ist nicht ein einziges ungetrenntes Netz
für das gesamte Modell. ID, Anzeigename und explizite physikalische Rolle
bleiben voneinander unabhängig.

Darstellung, Massenintegration, Hydrostatik und Hydrodynamik referenzieren
dieselbe Quelle des Elements. Mehrere Verwendungen erzeugen weder weitere
Quellidentitäten noch unabhängig gepflegte Geometriekopien. Die JSON
enthält Identitäten, Eigenschaften, Verwendungen und gegebenenfalls
ausdrückliche Ableitungsparameter, keine konkurrierenden Quellnetze für
dieses Element. Rein geometriefreie Punktmassen bleiben ohne künstliches
Mesh mit ihrer bestehenden Position und Massendefinition darstellbar.

Das ist eine geplante Verschärfung gegenüber der in A-C-00107 dokumentierten
Bestandskompatibilität mit mehreren Quellvarianten. Der historische
Vertrag und seine Nachweise werden nicht rückwirkend umgedeutet:
Das bisher implementierte Schema 2 erfüllt dieses neue Ziel noch nicht.
Vor einer Dateivertragsänderung sind Versions-/Kompatibilitätsregeln
festzulegen; kein stilles Neuinterpretieren vorhandener Dateien.

### Bestehende Berechnungen und abgeleitete Daten

In diesem Umfang keine Meshvereinfachung, keine neue Stützstellenerzeugung
und keine Neufassung der Schwerpunkt-/Massen-/Trägheitsberechnung.
Vorhandene Algorithmen und Semantik der Stützstellen bleiben unverändert.
Insbesondere sind extrahierte Meshvertices keine neue diskretisierte
Massenverteilung. Vorhandene Punkte, Flutränder und Berechnungsergebnisse
bleiben nachvollziehbare Ableitungen oder explizite Parameter, keine
zusätzlichen bearbeitbaren Meshquellen.

Später mögliche Vereinfachungen sind reproduzierbare Ableitungen derselben
Quelle, nicht Teil dieser Umstellung. Fehlende Flächen oder physikalische
Information dürfen nicht durch automatische Schließung, Reparatur oder
Raten ergänzt werden. Ist die gemeinsame Geometrie für eine Verwendung
ungeeignet, wird die Unvereinbarkeit ausdrücklich diagnostiziert.

### A-C-00111: Nachweisbare Migration und Konfliktbehandlung

Ein ausdrücklicher Konverter darf alte Modellversionen nur in einen neuen,
noch nicht vorhandenen Zielordner überführen. Er liest über den gemeinsamen
Importer, verändert keine Originaldateien und gibt ein Schema-3-Modell nur
aus, wenn sich alle Geometrieverwendungen eines Elements ohne fachliche
Auswahl auf eine Quelle zurückführen lassen. Geometrievergleich erfolgt
auf eingelesenen Koordinaten und Dreiecksbeziehungen, nicht anhand von
Dateipfad- oder Textgleichheit. Bei nicht auflösbaren Abweichungen muss die
Konvertierung mit Diagnose abbrechen; sie darf keinen als erfolgreich
migriert aussehenden Ersatzstand oder automatische Ersatzgeometrie liefern.

Die Inventur der elf Bestandsmodelle ergibt acht ohne Abweichung zwischen
Darstellung, Masse und Volumen: `bleistift`, `kegel`, `kugel`, `platte`,
`pyramide`, `quader`, `rolle`, `unregelmaessig`. Für diese ist die
Ein-Quellmesh-Migration geometrisch eindeutig.

Beim ursprünglichen strikten Migrationsversuch waren drei Modelle wegen
konkreter Geometrie-/Topologieabweichungen gesperrt. Dieser Ausgangsbefund
bleibt als Herkunftsnachweis erhalten; die nachfolgend ausdrücklich
freigegebenen Ableitungen lösen diese Fälle. Originale werden nicht geändert:

| Modell / Element | Identische Verwendungen | Abweichende Verwendung(en) | Befund |
|---|---|---|---|
| `420` / `Rumpf` | Masse = Volumen (34 530 Punkte, 69 056 Dreiecke) | Darstellung (36 168 Punkte, 72 332 Dreiecke) | unterschiedliche Geometrien |
| `420` / `Cockpit` | Darstellung = Volumen (12 232 Punkte, 24 460 Dreiecke) | Masse (6 490 Punkte, 12 604 Dreiecke; 374 offene Kanten) | Schale gegenüber geschlossenem Volumen |
| `420` / `Schwert` | Darstellung = Masse (14 Punkte, 24 Dreiecke) | Volumen (12 Punkte, 20 Dreiecke) | unterschiedliche Geometrien |
| `folkeboot` / `Schwert` | Darstellung = Masse (12 Punkte, 20 Dreiecke) | Volumen (12 Punkte, 20 Dreiecke) | Koordinaten-/Dreiecksnetz verschieden |
| `folkeboot` / `Ruder` | Darstellung = Masse (10 Punkte, 16 Dreiecke) | Volumen (10 Punkte, 16 Dreiecke) | Koordinaten-/Dreiecksnetz verschieden |
| `optimist` / `Rumpf` | Darstellung = Masse (310 Punkte, 574 Dreiecke; 44 offene Kanten) | Volumen (311 Punkte, 618 Dreiecke) | offene Schale gegenüber geschlossenem Volumen |
| `optimist` / `Schwert` | Darstellung = Masse (63 Punkte, 122 Dreiecke) | Volumen (49 Punkte, 94 Dreiecke) | unterschiedliche Geometrien |

Die Zählungen stammen aus der geometrischen Normalisierung der eingelesenen
OBJ-Koordinaten und Dreiecke; die identischen Netze wurden auch bei
unterschiedlichen Quelldateipfaden als identisch erkannt. Die betroffenen
Elemente erfordern eine fachliche Quellentscheidung, bevor ihre vollständige
Migration freigegeben werden kann. Insbesondere werden die offenen
Schalen von `420/Cockpit` und `optimist/Rumpf` nicht zur Volumengeometrie
geschlossen.

**Abnahme des strikten Aufrufs:** Die acht eindeutigen Modelle werden vollständig konvertiert und
physikalisch regressionsgeprüft. `420`, `folkeboot` und `optimist` werden
mit konkreter Konfliktdiagnose abgelehnt. Jede spätere Auflösung erfordert
eine begründete Nutzerentscheidung und einen Vorher-/Nachhervergleich.

**Auflösung nach der anschließenden Nutzerentscheidung:** Der Auftrag,
die bisherige Physik über eine Quelle und explizite Ableitungen ausreichend
genau zu erhalten, ersetzt die oben dokumentierte Entscheidungssperre.
Der ursprüngliche strikte Aufruf `--schema3` lehnt abweichende Quellen
weiterhin ab. Der bewusst gewählte Aufruf `--schema3-ableitungen`
beziehungsweise `konvertiere_modell_v3(quelle, ziel, true)` darf:

- das geschlossene Volumennetz als Quelle wählen und dessen vorhandene
  Deckfläche für die alte offene Masseschale ausschließen
  (420/Cockpit, Optimist/Rumpf; bei letzterem auch für die Darstellung);
- vollständige Massennetze als Quelle nutzen und bisherige Auftriebsteile
  durch aus den alten Schnittflächen bestimmte Ebenen ableiten
  (420/Schwert, Folkeboot/Schwert und Ruder, Optimist/Schwert);
- beim 420/Rumpf das vorhandene gemeinsame Massen-/Volumennetz als Quelle
  nutzen. Die alte Darstellung ist geringfügig anders; es wird kein Netz
  vereinfacht, sondern die physikalische Bestandsquelle übernommen.

Der Konverter prüft die Ableitung vor Anlage des Zielordners und validiert
das gesamte Zielmodell einschließlich Überschneidungen über denselben
Importer. Er findet Quellen/Teilflächen geometrisch, nicht anhand von
Bootsnamen. Nicht passende Fälle werden weiterhin mit CORE-MOD-0009
abgelehnt. Insbesondere wäre der einfache horizontale Schnitt am 420-Schwert
falsch: Der alte obere Abschluss ist geneigt, sonst entstünde eine
Überschneidung mit dem Rumpf.

Vorläufige, bewusst konservative Migrationsgrenzen:
Hüllgrenzen je Koordinate 1 µm; geschlossene Ableitungen Volumen und zentrale
Trägheitskomponenten höchstens 0,1 % (Tensor absolut relativ zum größten
Bestandsbetrag), Schwerpunkt 0,1 mm. Für die reine Darstellungsübernahme:
Oberfläche höchstens 0,1 %, Flächenschwerpunkt 1 mm, Flächenträgheit 1 %.
Offene Masseschalen müssen weiterhin geometrisch übereinstimmen.

Die Referenzabnahme ist strenger als diese Konvertergrenzen:
Masse relativ `1e-9`, Massenschwerpunkt `1e-8 m`, Trägheit `1e-8`
relativ mit Skalenuntergrenze 1; Flutrandkoordinaten als Menge exakt.
Vier Wasserlagen einschließlich Krängung: verdrängtes Teilvolumen
`1e-8 m³ + 1e-6 * Bestand`, Auftriebsschwerpunkt `1e-6 m`.
Gesamtkräfte und Momente in zwei Lagen: `0,001 N` beziehungsweise
`0,001 Nm`; gleiche Flutungsentscheidungen.
Dynamischer Vergleich über 0,2 s mit 0,02-s-Schritten und nichttrivialer
Anfangsgeschwindigkeit: Position, Quaternion, Geschwindigkeit und Drehrate
je Komponente höchstens `1e-4` in ihrer Einheit. Diese kurzen Regressionen
sind kein allgemeiner Langzeit-Stabilitätsnachweis.

### Migration und prüfbare Abnahme

- Bisherige Quellen je Element inventarisieren und geometrisch vergleichen;
  Referenzgleichheit allein genügt nicht zur Erkennung gleicher Netze.
- Identische Geometrie zusammenführen, physikalische Verwendungen erhalten.
  Bei abweichenden Netzen eine begründete maßgebliche Quelle bestimmen;
  höhere Dreieckszahl allein belegt keine fachliche Eignung.
- Unterschiede zwischen offener Schale und geschlossenem Volumen nicht als
  reine Deduplikation behandeln. Nicht verlustfrei auflösbare Fälle mit
  Geometrie-/Ergebnisvergleich sichtbar zur fachlichen Entscheidung stellen.
  Ohne Entscheidung kein als vollständig migriert ausgegebenes Ersatzmodell.
- Originale und Altimport erhalten; Migration ausschließlich explizit in
  neue Ziele. Kein Editor oder automatisches Speichern im GUI-Auftrag.
- Für alle elf Referenzmodelle Zuordnung je Element sowie vor/nach
  Geometrie, Masse, Schwerpunkt, Trägheit, Volumen, Stützstellen/Flutränder,
  Bewegung und repräsentative Simulationszustände vergleichen.
  Bei identischen Eingangsgeometrien bisherige Gleichheitsnachweise erhalten.
  Bei geänderter Geometrie Abweichungen messen und fachlich freigeben;
  keine erfundenen Toleranzen und keine Behauptung identischer Ergebnisse.

**Abnahmeziel:** Jedes geometrische Element besitzt genau eine Quelle;
alle Verwendungen und Ableitungen sind darauf zurückführbar. Verbleibende
Migrationskonflikte außerhalb der ausdrücklich freigegebenen und geprüften
Ableitungen verhindern die vollständige Abnahme.

Die gemeinsame Objektinspektion und Bewegungsdaten sind im
[Katalog-/Inspektionsvertrag](Spezifikation_Modellkatalog_Schnittstelle.md)
spezifiziert. Rasterlinien und GUI-Layout sind ausschließlich Gegenstand
der [GUI-Spezifikation](../../GUI/Spezifikationen/Spezifikation_GUI_Bootsauswahl.md).

## Modellformat 0.3, Fingerprints und Modellpfade

**Status:** Fachlich finalisiert am 10.11.2026; Zielversion C-0.21.
Core-Umsetzung und technische Abnahme sind offen.
**Anforderungen:** A-C-00148 bis A-C-00153.
**Entwurfsquellen:** [Migrationslog](../../Modelle/Modelle-NEW/MIGRATIONSLOG.md),
[Datenschema](../../Modelle/Modelle-NEW/schema_data.json) und
[Validierungsschema](../../Modelle/Modelle-NEW/schema_validation.json).
Technische Restdetails und Abnahmeschritte stehen im
[Umsetzungsplan C-0.21](../Umsetzungsplaene/Umsetzungsplan_C-0.21.md).
Nicht normativ offene Implementierungsdetails stehen in der
[Entwurfspunkteliste Modellformat](Entwurfspunkte/Modellformat.json).

Nach dem in A-C-00153 geregelten Cutover verwendet der aktive Modellbestand
ausschließlich das Dateipaar `modells_data.json` und
`modells_validation.json`. Der C-0.21-Core-Katalog erhält keinen stillen
Legacy-Fallback. Historische und synthetische Fixtures werden im Rahmen der
Umsetzung auf den aktuellen Vertrag migriert; getrennte Legacy-Konverter
bleiben außerhalb des aktiven Katalog-/Ladepfads.

### A-C-00148: Schema-geprüfter Import von Modellformat 0.3

Ein Katalogmodell besteht ausschließlich aus `modells_data.json` und
`modells_validation.json`. Der Core validiert beide vor der Deserialisierung
gegen die Draft-2020-12-Schemas `schema_data.json` und
`schema_validation.json` am Modellkatalog-Wurzelordner. Schema-Auflösung ist
unabhängig vom Arbeitsverzeichnis. Fehlende/zusätzliche Dateien, fehlende
oder unbekannte Schemas, Validatorfehler und Schema-Verletzungen erzeugen
eindeutige Diagnosen und sperren Nutzung; es gibt keinen stillen Rückfall auf
ein anderes Modellformat.

Der Core prüft zusätzlich schemaübergreifende Bedingungen: `schema` und
`identity` müssen in beiden Modelldateien übereinstimmen, `identity.id` muss
dem Modellordnernamen entsprechen, und Ressourcenreferenzen müssen
vorhanden, typgerecht und innerhalb des Modellordners auflösbar sein.
Modellgruppen, Elemente, `placement`, `display`, `physics` und
Gruppen-`movement` werden gemäß Schema 0.3 semantikerhaltend eingelesen.
`schema.release` ist Metadatum und weder Simulationsfreigabe noch
Ersatz für bestehende Simulationsprüfungen. Teilweise geparste Daten werden
nicht als erfolgreiches Modell ausgegeben.

**Abnahme:** Alle elf Modellordner werden eingelesen. Negativtests decken
fehlende/zusätzliche Dateien, fehlerhaftes JSON, Schemafehler, unbekannte
Version, Abweichungen zwischen `identity`/`schema`, Ordner-ID-Konflikte,
fehlende Ressourcen und unzulässige Pfadfluchten ab. Kein Fehlerfall startet
Simulation oder liefert eine erfolgreiche Modellansicht.

### A-C-00149: Deterministische Bereichs-Fingerprints

Der Core berechnet und vergleicht SHA-256 für genau die fünf
Schema-Bereiche:

| Eintrag in `model.fingerprints` | Quelle |
|---|---|
| `visualisation` | Dateien unter `modells/visualisation/` |
| `mass` | Dateien unter `modells/mass/` |
| `hydrostatics` | Dateien unter `modells/hydrostatics/` |
| `aerodynamics` | Dateien unter `modells/aerodynamics/` |
| `modelData` | Inhalt von `modells_data.json` |

Jeder Bereichs-Hash umfasst alle regulären Dateien rekursiv, einschließlich
versteckter Dateien. Symlinks werden abgelehnt. Dateien werden nach
normalisiertem, relativem UTF-8-Pfad mit `/` byteweise sortiert; der
Hashstrom codiert pro Datei eine 64-Bit-unsigned little-endian Pfadlänge,
Pfadbytes, eine 64-Bit-unsigned little-endian Inhaltslänge und die exakten
Datei-Bytes.
Ein leerer Ressourcenbereich ergibt SHA-256 des leeren Eingabestroms.
`modells_validation.json`, Schema-Dateien und Sperrdateien werden nicht in
Bereichshashes aufgenommen. Absolute Pfade, Zeitstempel und
Betriebssystemmetadaten sind nicht Bestandteil der Hashes.

Bei jedem Katalog-/Ladevorgang vergleicht der Core alle fünf gespeicherten
Werte mit den aktuellen Hashes, ohne Dateien zu ändern. Fehlende,
unlesbare oder abweichende Hashwerte werden je Bereich diagnostiziert.
Jede Abweichung sperrt Simulation. Ein aktueller Visualisierungs-Hash
erlaubt die Ansicht bei schema-valider Modellbeschreibung und gültigen
Darstellungsressourcen auch dann, wenn andere Bereiche fehlerhaft sind.
Eine Visualisierungsabweichung sperrt die Ansicht bis zur ausdrücklichen
Aktualisierung. Diagnosen bleiben in allen Fällen sichtbar.

**Abnahme:** Wiederholte Läufe auf identischem Inhalt liefern bytegleiche
Hashes. Tests variieren Dateiinhalt, Pfad, Verschachtelung, Reihenfolge,
versteckte Dateien, leere Bereiche und Symlinks; nur die vorgesehenen
Bereichshashes ändern sich. Änderungen an Validierungs-/Schema-Dateien
ändern keinen Bereichs-Hash. Fingerprintfehler lösen keinen Schreibzugriff
aus.

### A-C-00150: Ausdrückliche atomare Fingerprint-Aktualisierung

Eine Fingerprint-Aktualisierung erfolgt ausschließlich durch einen
ausdrücklichen API-Aufruf, der den Modellordner und eine nichtleere,
duplikatfreie Auswahl aus den fünf Bereichen erhält. Die Funktion validiert
Eingaben, Schemas und Ressourcen, aktualisiert ausschließlich die
angeforderten `.hash`-Felder und setzt `modifiedAt` nur bei tatsächlich
geändertem Hash auf das UTC-Datum des Schreibvorgangs. Die Ausgabe wird
schema-validiert und atomar ersetzt. Unbeteiligte Bereiche und sonstige
Validierungsmetadaten bleiben unverändert. Bei einem Fehler bleibt die
bestehende Datei unverändert; der Aufrufer erhält eine konkrete Core-
Diagnose. Eine ungültige oder fehlende Validierungsdatei wird nicht
automatisch neu angelegt.

**Abnahme:** Tests aktualisieren einzelne und mehrere Bereiche, bestätigen
idempotentes Verhalten und Datumsregeln, erhalten fremde Metadaten und
simulieren Schreib-/Validierungsfehler sowie parallele Schreibkonflikte.
Fehler lassen Dateiinhalt und Zeitstempel bytegleich unverändert.

### A-C-00151: Öffentliche C++-Schnittstelle für Fingerprints

Der Core exportiert die in
[Schnittstelle Core → Modellwerkzeuge](../Dokumentationen/Schnittstelle_Core_Modellwerkzeuge.md)
festgelegte C++-Bibliotheksfunktion. Modellwerkzeuge wie Segelmacher
übergeben den Modellordner und die gewünschten Bereiche; sie implementieren
keinen eigenen Hash-, JSON- oder Schemapfad. Die API liefert die berechneten
Hashwerte und Aktualisierungsinformation zurück und meldet Fehler über
dokumentierte Core-Diagnosen. Ein CLI-Zweitvertrag ist nicht Bestandteil.

**Abnahme:** Core- und unabhängige Verbraucher-Builds binden dieselbe
öffentliche API ein. Vertragstests prüfen alle Bereiche, Teilauswahl,
Fehler- und Ergebniswerte; CPPI-0140, CPPI-0142, CPPI-0143 und CPPI-0144
sind im Register nachvollziehbar.

### A-C-00152: Gemeinsame, zur Laufzeit änderbare Defaults

Modellbasis, Schema-Wurzel und weitere veränderbare Standardpfade liegen in
`%APPDATA%\SegelPhysik\defaults.json`. Der gemeinsame Dateivertrag enthält
mindestens `modelRoot`, `schemaRoot` und ein erweiterbares `defaults`-Objekt.
`defaults` ist offen: Verbraucher weisen unbekannte Schlüssel nicht zurück
und erhalten sie beim Schreiben; Standardwerte können ohne Neu-Build
geändert oder ergänzt werden. Pfade sind absolut oder relativ zur
Konfigurationsdatei aufzulösen.
Priorität: explizite Aufruf-/Benutzerauswahl, dann Test-Override, dann
entsprechende Umgebungsvariable, dann Datei. Fehlende oder ungültige
Konfiguration wird sichtbar diagnostiziert; es gibt keinen stillen
Festplatten- oder Quellcode-Fallback. Änderungen sind ohne Neu-Build wirksam.
Die GUI und externe Modellwerkzeuge verwenden dieselbe Datei und Feldsemantik.

**Abnahme:** Tests prüfen fehlende, ungültige und unlesbare Dateien,
Relative-/Absolute-Pfade, Prioritätsreihenfolge, Laufzeitänderung und
isolierte Test-Overrides. Produktions-Builddefinitionen enthalten keinen
festcodierten aktiven Modellwurzelpfad.

### A-C-00153: Sicherer Cutover des aktiven Modellbestands

Erst nach bestandener Core- und GUI-Vorprüfung wird der bisherige
`Segelphysik\Modelle`-Ordner vollständig in `Segelphysik\Modelle_alt`
umbenannt und `Segelphysik\Modelle\Modelle-NEW` an den freien Zielpfad
`Segelphysik\Modelle` verschoben. Keine Quelldaten werden gelöscht; ein
bereits vorhandener Zielordner führt zum Abbruch ohne Überschreiben.
Core-Katalog, Schemaauflösung, Tests, Werkzeuge, Start-/Entwicklungsprofile,
CMake und Dokumentation beziehen aktive Pfade aus A-C-00152. Fehler beim
Cutover dürfen keinen teilweisen aktiven Modellbestand hinterlassen.

**Abnahme:** Vor dem Verschieben bestehen Core- und GUI-Builds und
vollständige Tests gegen die neue Quelle. Danach bestehen sie erneut gegen
den endgültigen `Modelle`-Pfad; alle elf Modelle sind über den Katalog
erreichbar und werden schema-/fingerprintgeprüft. `Modelle_alt` bleibt
inhaltlich erhalten; Datei- und Hashinventare werden vor/nachgewiesen.
Die GUI-Abnahme bleibt separat in ihrer Spezifikation und ihrem Plan.

### C-0.21 Nachverfolgbarkeit

| Anforderung | Umsetzungsschritte |
|---|---|
| A-C-00148 | U-C-000131, U-C-000132, U-C-000136 |
| A-C-00149 | U-C-000133, U-C-000135 |
| A-C-00150 | U-C-000133, U-C-000135 |
| A-C-00151 | U-C-000132, U-C-000133 |
| A-C-00152 | U-C-000134, U-C-000136 |
| A-C-00153 | U-C-000136, U-C-000137 |
