# Spezifikation der einheitlichen Modellbeschreibung

## Status und Geltungsbereich

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
