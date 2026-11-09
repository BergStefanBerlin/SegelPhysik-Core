# Core-Spezifikation: persistente starre Segel-Einbaulage

**Status:** FINAL fachlich spezifiziert auf Nutzerauftrag vom 06.11.2026.
**Arbeitszielversion:** C-0.12. Core-Implementierung und Debug-/Release-
Feature-Nachweise sind vorhanden; sechs native Bestandsfehler wurden auf
archiviertem HEAD reproduziert und sind in Plan/Nachweisen beschrieben.
GUI-Integration und Nutzerabnahme bleiben offen; kein Commit, Tag, Push
oder Release beauftragt.
**Plan:** [C-0.12](../Umsetzungsplaene/Umsetzungsplan_C-0.12.md).
**Herkunft:** Anforderungen dieser Sitzung sowie ausdrueckliche Bestaetigung
der separaten persistenten Einbaulage nach Pruefung des SegelMacher-Vertrags.

Einzige thematische Spezifikation fuer die Segel-Einbaulage.
Der [Modellkatalogvertrag](Spezifikation_Modellkatalog_Schnittstelle.md)
bleibt erhalten; neu ist eine ausdrueckliche Konfigurationsoperation
neben dem weiterhin unveraenderlichen Inspektions-Snapshot.
Die [GUI-Spezifikation](../../GUI/Spezifikationen/Spezifikation_GUI_Segelkonfigurator.md)
regelt ausschliesslich Bedienung, Kamera und Darstellung.
Keine GUI-Abhaengigkeit im Core und keine Aenderungen im SegelMacher.

## 1. Datenverantwortung und Referenzen

Der Core liest und validiert allein Modell, Segelquellen und Konfiguration.
Identitaet wird getrennt fuer Modell, Element, intrinsische Segelquelle,
Darstellungsmesh und Lookup gefuehrt; keine Gleichsetzung der ID `Großsegel`
mit der intrinsischen ID `420_grosssegel`.

Pro konfigurierbarem Segel sind erforderlich:

- explizite Zuordnung Element-ID zu intrinsischer Segel-JSON,
  Darstellungsmesh und vorhandener Lookup-Identitaet;
- semantische Ecken: Hals, Schothorn, Kopf am Vorliek und bei viereckigem
  Segel hintere Kopfecke; keine Eckenermittlung anhand Punktreihenfolge;
- gerichtete Vorliek-/Unterliekreferenzen aus der intrinsischen Kontur;
- Modellbezugspunkte und Richtungen fuer Mast, Grossbaum und Vorstag,
  einschliesslich Mast-Grossbaum-Anschlag und Deck-Vorstag-Anschlag.

Fehlende Bezugspunktkoordinaten oder Quellenzuordnungen sind vor
Implementierung anhand der realen 420er-Daten zu ermitteln und als
nachvollziehbare Modellmetadaten festzuhalten, nicht numerisch zu erraten.
Unklare Referenzen blockieren die betroffene Automatik mit Diagnose.
Dies ist ein technischer Datenbereitstellungsschritt, kein offener
fachlicher Wunsch. Originalgeometrien werden dabei nicht umgeschrieben.

**Umgesetzte 420er-Referenzherkunft:** Riggeometrien werden anhand von
`bootsmodell.json` und der `Darstellung.datei`-Referenz der semantischen
Elemente `Mast`, `Großbaum`, `Vorstag` aufgeloest. Die 420-Dateipfade
werden explizit geprueft und alle drei Rig-OBJs sind im Quellfingerprint
enthalten. OBJ-Konvention: Meter, +X Bug, +Y Backbord, +Z oben.
`mast.obj`-Querschnitte bestehen aus 24 Vertices; die Mittellinie wird
zwischen den Abschnittszentren 1–24 und 25–48 auf die von den
Grossbaum-Endkappen definierte z-Hoehe interpoliert. `grossbaum.obj`
enthaelt Querschnitte 1–24 und 25–48 sowie Endkappenzentren 49/50; deren
Abweichung von den jeweiligen Ringzentren darf 1e-6 m nicht ueberschreiten.
`vorstag.obj` verwendet die Zentren der Abschnitte 1–8 und 9–16 fuer
Deckanschlag und lokale Vorstagrichtung. Daraus ergeben sich die
reproduzierbaren Geometriebezuge Mast/Baumanschlag `[0.85070675, 0, 1.35] m`,
Baumrichtung zum Schothorn `[-2.338488, 0.539883, 0] m`, Fockdeck
`[2.00570675, 0, 0.616428] m` und lokaler Vorstagvektor
`[-0.1475, 0, 0.510447] m`. Der unabhaengige 420-Kopietest liest die
Rig-OBJ-Vertices selbst und vergleicht Anschlaege/Achsen sowie automatisch
platzierte Segellieken mit diesen Geometrien; die Werte sind keine
Messgenauigkeitsbehauptung des Entwurfs.

## 2. Starre Transformation und automatische Platzierung

Einbaulage im Modellbezug:

```text
p_modell = R * p_segel + t
R^T * R = I, det(R) = +1
```

Intern Meter und endliche Gleitkommazahlen; keine Skalierung, Spiegelung,
Kontur-, Profil-, Woelbungs- oder Verwindungsaenderung.
Eine einzelne Lage gilt konsistent fuer alle zum Segel gehoerenden
Darstellungsteile; keine doppelte Anwendung vorhandener Transformationen.

**Autorisierte Grundstellungsrevision 06.11.2026:** Alle drei aktiven
Boote (420, Folkeboot, Optimist) werden in einer dauerhaften
0-Grad-Mittschiffsbasis gefuehrt. Das ist nicht zwingend ein
Identitaetsquaternion: Mast-/Vorstag-/Vorliekneigung bleiben erhalten.
Montagebasis und spaeterer Betriebswinkel delta bleiben getrennt.
Automatik stellt die neutrale Basis wieder her, nicht die Oeffnung eines
historisch ausgeschwenkten Roh-OBJ. Großbaum und gegebenenfalls Spriet
werden ueber Settings entsprechend neutral gefuehrt; der Mast bleibt fest.

**420-Grosssegel:** Hals auf Mast-Grossbaum-Anschlag; gerichtetes Vorliek
parallel zum Mast nach oben, Unterliek mittschiffs achterlich parallel zur
neutralisierten Grossbaumachse. Die vorherige Formulierung, einem bereits
ausgeschwenkten Rohbaum zu folgen, wird durch diese Nutzerfreigabe ersetzt.
**Fock:** Hals auf Deck-Vorstag-Anschlag; gerichtetes Vorliek entlang
Vorstag Richtung Mast. Die verbleibende Rotation um das Vorstag wird
ueber die Bootsmittschiffsebene festgelegt: Unterliek/Schothorn achterlich,
keine willkuerliche Rotation um das Vorliek.

Die Automatik ist deterministisch und idempotent; sie ersetzt manuelle
Lage und setzt den Fock-Versatz auf 0. Lassen sich die Grosssegelrichtungen
wegen inkompatibler Winkel nicht gleichzeitig starr ausrichten, ist die
Operation mit Diagnose abzulehnen, nicht das Segel zu verformen.
Bezugspunkte sind semantische Anschlaege, nicht ungepruefte Mesh-AABB-Mitten.
Folkeboot und Optimist verwenden explizit belegte OBJ-Montagereferenzen
gemaess Abschnitt 2a, keine angeblich kompatible intrinsische Zuordnung.

Numerische Abnahme: Halsabweichung hoechstens 1e-6 m, gerichtete
Winkelabweichung hoechstens 1e-6 rad, Orthogonalitaets- und
Determinantenabweichung hoechstens 1e-10; negative Parallelauslegung
ist unzulaessig. Diese Grenzen sind Geometriepruefungen, keine
Messgenauigkeitsbehauptung der rekonstruierten Segel.

## 2a. Belegte OBJ-Basis und sichtbare Quellenkonflikte

**Massgebliche Revision 06.11.2026, einheitliche Objektmontage:**
Die untenstehenden alten Vertexzuordnungen beschreiben ihren historischen
Quellenstand, nicht automatisch die inzwischen rekonstruierten GUI-OBJs.
Alle Objekte behalten ihre vorhandene Element-ID und genau eine aktuelle
Darstellungsreferenz. Physische Volumen-/Massen-/Quellmeshes bleiben
unveraendert als Quellen verfuegbar, nicht als zusaetzliche Objektklone.
`InspectionMesh.montage` verwendet fuer alle Objekte denselben Typ
`MontageReferenz`: lokaler Bezugspunkt/Achsen, starre Lage, Herkunft und
Quellenfingerprint. Ohne besondere Metadaten gilt der deklarierte
Modellkoordinatenbezug; keine AABB-Mitte oder geratenen Eckpunkte.
Eine deklarierte Punktmasse ohne OBJ wird durch genau einen Punkt an
`Hydrostatik.masse.position_m` dargestellt; sie bleibt dasselbe Objekt
und erhaelt keine Ersatzbox oder neue Geometrieidentitaet.
Explizite Objektanschlag-/Endindices werden aus der deklarierten OBJ-Quelle
gelesen. Mehrdeutige aktuelle Darstellungsreferenzen werden abgewiesen.

Segelart, Quellpfad, Lookup und Ecken werden ausschliesslich aus
`Segel`, `grundkontur.punkte` und den benannten Randkanten gelesen.
Es gibt keine Verzweigung nach Boots-ID oder festes Segelmanifest.
Neue explizite OBJ-Vertexreferenzen benoetigen `Montage.quellen_sha256`;
ein abweichender gespeicherter Quellencheck wird mit `CORE-SEG-0031`
abgewiesen. `Montage.bezug:"kontur"` benutzt die semantische Kontur;
optional geben `rotation_xyzw`/`translation_m` die starre Zielbasis vor.

Belegter Legacy-Fallback: Eine OBJ-Quelldeklaration `# Source: <JSON>;`
sowie im aktuellen Mesh vorhandener semantischer Hals und Vorliekkopf
belegen den Konturbezug. Alte ungepruefte Segelindices werden **nicht**
auf das neue Mesh angewandt. Sind ihre nichtentarteten semantischen
Ecken in der deklarierten Massenquelle erhalten, wird daraus eine starre
Quellbasis abgeleitet. Bei fehlender expliziter Massenreferenz darf das
vorhandene standardisierte Schwesterobjekt `mass/<OBJ-Dateiname>` im
gleichen `modells`-Teilbaum als Legacy-Beleg dienen; ohne diesen Beleg
wird die Automatik diagnostisch abgewiesen. Keine Skalierung:
abweichende historische Segelabmessungen werden nicht kuenstlich angepasst.
Die bestehende gespeicherte Lage wird mit dieser Basis zusammengesetzt;
Halsposition, gerichtete Vorliekachse, Originalseite und Revision bleiben
erhalten (`CORE-SEG-0032`). Die neutrale Automatik verwendet dieselbe
belegte Montagebasis.

Das Einlesen schreibt niemals Modell- oder Quelldaten. Erst die
bestaetigte Aktion bindet die aktuellen OBJ-/Kontur-SHA256-Werte unter
`setting.montage_referenz`; dadurch bleibt die Basisfassung stabil und
weitere Aktionen koennen denselben unveraenderlichen Snapshot verwenden.
Nach einer solchen Bestaetigung werden geaenderte Quellen nicht erneut
als Legacy interpretiert, sondern ausdruecklich abgewiesen.
Die elf Originalmodelle sind fuer diesen Auftrag strikt read-only;
Persistenztests arbeiten ausschliesslich auf eigenen Modellkopien.

**Nutzerentscheidung 06.11.2026:** 0-Grad-Grundstellung anhand belegter
OBJ-Referenzen umsetzen; Quellenkonflikte sichtbar dokumentieren.
`Montage` liegt direkt am vorhandenen Element. `schema_version:1`,
`basis:"mittschiffs"` und `bezug:"obj"` kennzeichnen die Referenz;
Vertexindices sind explizit 1-basiert und bezeichnen den Mittelwert der
genannten Vertices, keine heuristische Eckenerkennung:

| Element | Hals/Anschlag | Schothorn/Ende | Kopf Vorliek | Kopf Achterliek |
|---|---|---|---|---|
| 420 Großbaum | 50 (Endkappe am Mast) | 49 (achterliche Endkappe) | – | – |
| Folk Großbaum | 2,3,6,7 (vorderer Schaftquerschnitt) | 1,4,5,8 (achterlicher Querschnitt) | – | – |
| Folk Großsegel | 1,5 | 2,6 | 4,8 | 3,7 |
| Folk Fock | 2,5 | 1,4 | 3,6 | – |
| Opti Großbaum | ungerade 1–63 (vorderer Ring) | gerade 2–64 (achterlicher Ring) | – | – |
| Opti Großsegel | 3,21 | 1,19 | 6,24 (Kehle) | 2,4,20,22 (Peak) |

Segelfelder heissen `hals_vertex_indices`, `schothorn_vertex_indices`,
`kopf_vorliek_vertex_indices`, optional `kopf_achterliek_vertex_indices`.
Baumfelder heissen `anschlag_vertex_indices`, `ende_vertex_indices`.
Opti-Spriet traegt `mitbewegt_mit:"Großsegel"` und dieselbe starre Pose
wie das Segel; dadurch bleibt der gemessene Peakanschlag unveraendert.
Diese Metadaten und die einmaligen `Segel.quelle_datei`-Referenzen sind
Teil des Modellfingerprints. Keine neuen Element-IDs, OBJ-Aenderungen
oder Skalierung.

OBJ-Segelbasis: Die Vorliek-/Unterliekrichtungen definieren ein
orthonormales Quellbasisdreibein. Eine eigentliche Rotation fuehrt dieses
in die XZ-Mittschiffsebene; die Vorliekneigung gegen +Z und alle
Formabstaende bleiben erhalten. Der Hals behaelt X und Z und liegt auf
Y=0. Baumrotation um +Z stellt die Schaftachse achterlich neutral,
bei erhaltenen Endpunkthoehen. Vorhandene konstruktive Abstaende zwischen
Baum-Endkappe und Segelhals werden nicht kuenstlich auf Null gezogen.
Die Messpruefung verlangt Erhalt der belegten Anschlagabstaende und
0-Grad-Oeffnung der Referenzebene, nicht das Glattdruecken von Woelbung.

**Quellenkonflikte, keine Physikfreigabe:** Folk-Großsegel OBJ-Unterliek
3,22 m gegen intrinsisch 3,30 m, OBJ-Vorliek 9,42 m gegen 8,75 m;
Folk-Fock OBJ-Vorliek 9,69645 m gegen 5,50 m. Opti-Unterliek OBJ
1,98971 m gegen 1,90 m; OBJ-Vorliek etwa 1,62877 m gegen 1,73 m.
Die intrinsischen JSONs bleiben unveraendert, es gibt dort keine
vorhandenen kompatiblen Lookups. `CORE-SEG-0030` im gueltigen
Montagesnapshot weist die inkompatible intrinsische Kopplung und fehlende
Physikfreigabe sichtbar aus. Der verschachtelte Alt-Sidecar unter
`optimist/modells/aerodynamics/segelkonfiguration.json` enthaelt kopierte
420-Werte (inklusive nicht existierender Opti-Fock); er ist kein
freigegebener Montagevertrag und wird weder gelesen noch umgeschrieben.

## 3. Manuelle Operationen

Die GUI liefert Modellachsen und eine Translations-/Winkelanforderung,
keine Dateipfade oder umgeschriebenen Netze. Der Core prueft:

- Translation nur in XY, XZ oder YZ; senkrechte Koordinate und R erhalten.
- Rotation um die Normalachse derselben Ebene und einen semantischen
  Pivot: Hals fuer alle anderen Ecken; vorderer Kopf beim Ziehen des Halses.
  Der Pivot bleibt innerhalb 1e-6 m fest; Form und Groesse bleiben erhalten.
- Alle Punkte werden gemeinsam transformiert. Fuer beliebige Punktpaare
  bleibt der Abstand bis auf 1e-10 m + 1e-10 relativ erhalten.

Fock-cm-Eingabe: fester Grundpositionshals `a`, normierte Vorstagrichtung
`d` vom Deck Richtung Mast und aktueller Hals `h`:

```text
versatz_cm = 100 * dot(h - a, d)
h_neu = h + (eingabe_cm / 100 - dot(h - a, d)) * d
```

R und die zu d senkrechte manuelle Korrektur bleiben erhalten.
Die feste Referenz d aendert sich durch manuelle Rotation nicht.
Auch 0 cm setzt nur diesen axialen Anteil zurueck; erst die Automatik
setzt die gesamte Lage zurueck. Kein Aufsummieren absoluter Eingaben.
Der ausgegebene Versatz wird aus der Lage abgeleitet und widerspricht
keinem separat gespeicherten Stellwert.

## 4. Dauerhafte Ablage und Fehlervertrag

**Autorisierte Ablageaenderung vom 06.11.2026:** Die aktive Modelldatei
heisst **bootsmodell.json**. Montageposen stehen ausschliesslich als
`setting` direkt am bestehenden Element neben `id` und `Darstellung`.
`konfiguration` enthaelt ausschliesslich gemeinsame Metadaten, keine
Elementmap oder neuen Objekt-IDs. Montagepose bleibt vom Betriebstrimm getrennt.
Keine separate
Segelkonfigurationsdatei mehr; eine vorhandene Legacy-Datei
`segelkonfiguration.json` erfordert explizite Migration und wird abgelehnt,
insbesondere bei gleichzeitiger neuer Konfiguration.
Der Block hat `schema_version: 1`, ganzzahlige nichtnegative `revision`,
`modell_id`, `modell_fassung`, `quellen_fingerprint`.
Jedes bestehende Element verwendet dasselbe `setting`-Format:
`translation_m`, `rotation_xyzw`, `konfiguriert`; nur Segel ergaenzen direkt
`original_buglage` (`unbekannt`, `backbord`, `steuerbord`) und
`quellen_fingerprint`. Der Fockversatz wird aus der Pose und festen
Vorstagreferenz abgeleitet, nie redundant gespeichert.
Intrinsische Segelquelle und Lookup sind genau einmal am selben Element
als `Segel.quelle_datei` und bei vorhandenem Lookup `Segel.lookup_datei` referenziert;
`Darstellung.datei` bleibt die einzige Meshreferenz.
Persistierte intrinsische IDs, Mesh-IDs und Lookup-IDs entfallen;
die API kann die Identitaeten weiterhin aus Quellen ausgeben.
Fehlende Segel-Settings bleiben unkonfiguriert;
bereits bestaetigte Eintraege werden nicht zurueckgesetzt.
Nicht-Segel erhalten dieselbe validierte starre Montagepose.
Initial sind Identitaetstranslation/-rotation und `konfiguriert:false`
vorgesehen. Bestaetigte Nicht-Segel-Settings wirken genau einmal in der
Core-Darstellungsaufbereitung. Die lokale 420-Quelle wird nur in der
Segel-API montiert; Folk-/Opti-OBJ-Segel erscheinen bereits neutral in
der generischen Inspektion. Deren Segel-API liest fuer Betriebswinkel
die unveraenderten Rohmeshes und montiert genau einmal; kein doppeltes
Transformieren der bereits montierten Inspektionspunkte.
Inspektions-Quellmeshes und bereits geladene Snapshots bleiben
unveraendert. Dynamische Bewegungs-/Stellwinkel bleiben getrennt.
Diese Stufe konfiguriert Montage/Darstellung, nicht die vorhandene
hydrostatische/hydrodynamische Physik: physische Netze, Massenschwerpunkte
und Traegheit werden nicht implizit transformiert. Eine Darstellungspose
ist damit keine behauptete physikalische Bauteilverstellung.
Eine fehlende Konfiguration bedeutet noch nicht konfiguriert; ein
vorhandener unlesbarer/ungueltiger Block erzeugt dagegen Diagnose.
Beim Speichern bleiben sonstiger Modellinhalt, nicht betroffene Eintraege
und unbekannte Felder einschliesslich segelspezifischer Erweiterungen erhalten.

`modells_information.lock` schuetzt Revisions-/Quellenpruefung und atomare Ersetzung
der gesamten Modelldatei. Der Modellfingerprint schliesst ausschliesslich
den obersten `konfiguration`-Block und `setting` unmittelbar an benannten
Elementobjekten unter `gruppen.*.elemente[]` aus und verwendet die kanonische
nlohmann-JSON-Serialisierung aller anderen Inhalte sowie referenzierte
Dateipfade und Bytes der referenzierten Dateien (`datei`, `quelle_datei`,
`lookup_datei`). Gleichnamige `setting`-Felder ausserhalb der echten
Elemente bleiben im Fingerprint. Keine rekursive Hash-Inventarisierung
aller Lookup-Produkte; nur explizite Referenzdateien werden gelesen.
`lade_modell(...).fassung` und `modell_fassung(...)`
liefern denselben Fingerprint, auch nach Speichern und Neuformatierung.
Segel-/Rigg-/Lookupquellen werden zusaetzlich mit SHA-256 validiert.
Initial ist ein ungebundener leerer Block
`{"schema_version":1,"revision":0}` erlaubt. Beim ersten
Speichern ergaenzt der Core Modell-/Quellenidentitaet. Fehlende Identitaet
bei bestaetigten Segel-Settings, positiven Revisionen oder teilweise gesetzter
Identitaet ist dagegen kein unkonfigurierter Zustand, sondern ein Fehler.

Jede abgeschlossene Operation wird ohne extra Speicheraufruf des Nutzers
als Transaktion validiert, atomar geschrieben und erst danach bestaetigt.
Temporaere Ziehvorschauen werden nicht als dauerhaft gespeichert bezeichnet.
Keine Speicherung erst beim Programmende und kein Ablaufdatum.
Die letzte bestaetigte Konfiguration uebersteht Neustart und Modellwechsel.
Loeschen, Verlust des Modellordners und Hardwareausfall sind keine
garantierbare Persistenz und werden nicht als solche versprochen.

Schreibfehler, beschaedigte Daten, nicht endliche Werte, ungueltige
Rotationen, fremde IDs oder veraenderte geometrische Quellen werden
explizit diagnostiziert. Keine stillen Defaults oder Ueberschreibungen.
Bei Fehlschlag bleiben Datei und letzter bestaetigter Zustand unveraendert.
Parallel arbeitende Prozesse duerfen sich nicht unbemerkt ueberschreiben:
Revision/Fassung unter geeigneter Schreibsperre pruefen, veraltete
Aenderungsanforderungen mit Konfliktdiagnose ablehnen.

## 5. Lookup-Kompatibilitaet

Normative Grundlage:
[SegelMacher-Lookupvertrag, Abschnitt 8](../../../SegelMacher/Spezifikationen/Spezifikation_Lookup_Generator.md).
Im tatsaechlichen Dateisystem liegt dieses getrennte Projekt neben
Segelphysik; die technische Quellenpruefung verwendet dessen reale Pfade.
Die portable Laufzeit benoetigt das SegelMacher-Repository nicht.

Einzel-Lookups verwenden das lokale Segelsystem, Momente um den Hals,
lokale Angriffspunkte und SHA-256-Pruefsummen. Die intrinsische JSON,
OBJ-Netze, Auftraege und vorhandenen Lookup-Ergebnisse bleiben bei jeder
Platzierung **byteidentisch**. Kein Neuberechnen oder Invalidieren eines
geometrisch unveraenderten Einzel-Lookups durch reine Einbaulage.
Lookup-Identitaet und intrinsischer Geometriefingerprint sind von der
Konfigurationsrevision zu unterscheiden.

Fuer die spaetere Physik muss dieselbe gespeicherte Lage verfuegbar sein:
Punkte mit R und t, Kraefte und freie Momente nur mit R; das Moment um
einen Bootspunkt erhaelt zusaetzlich den korrekten Hebelarmanteil.
Kein doppeltes Einrechnen von Resultierender und Einzelkraeften.
Das ist ein Kompatibilitaetsvertrag, keine jetzt implementierte
Lookup-Auswertung, Windphysik oder gekoppelte Mehrsegelaerodynamik.

## 6. Funktionaler Liefervertrag

Typisierte, transportunabhaengige Core-Daten und Operationen fuer:
Referenzen/semantische Eckpunkte lesen, bestaetigte Lage lesen,
Automatik/Translation/Rotation/absoluten Fock-Versatz als Vorschau
validieren, Lage mit erwarteter Fassung/Revision dauerhaft bestaetigen
und aktualisierte Inspektionsgeometrie beziehen.
Antworten unterscheiden Vorschau, gespeichert und fehlgeschlagen;
Diagnosen bleiben der GUI zuordenbar. Physikalischer Quellenstand,
Inspektions-Snapshot und Konfigurationsrevision sind getrennte Identitaeten.
Bestehende Inspektionsfunktionen veraendern ihren Snapshot nicht nachtraeglich.
Konkrete C++-Exports und ihre einzelnen CPPI-Kennungen werden im ersten
Umsetzungsschritt vor Headerimplementierung im Core-Register festgelegt;
hier werden keine unbestimmten Interfaces mit Sammel-IDs vorweggenommen.

## 7. Anforderungen und Abnahme

Alle Anforderungen sind fachlich final. Der Core-Umsetzungsplan fuehrt den
technischen Erfuellungs-/Offenstatus und die gezielten Testbelege; GUI-
Integration, Renderer-Normalen und Nutzerabnahme bleiben davon getrennt.

| ID | Verbindliche Anforderung | Abnahme |
|---|---|---|
| A-C-00136 | Einheitliche Montage-/Quellreferenz vorhandener Objekte; datengetriebene Segel-/Lookup-Zuordnung, semantische Ecken und Rigg-Referenzen. | Alle elf Modelle und alle fuenf Segel ohne Boots-ID-Sonderzweig; gepruefte Vertex-/Konturreferenzen; fehlende, veraltete oder mehrdeutige Daten diagnostizieren. |
| A-C-00137 | Deterministische Grosssegelautomatik mit Mast-/Baumbezug. | Hals/Liek-Toleranzen aus Abschnitt 2; Wiederholung unveraendert; inkompatible Richtungen ablehnen. |
| A-C-00138 | Deterministische Fockautomatik mit Deck-/Vorstagbezug und achterlicher Orientierung. | Hals/Liek-Toleranzen, eindeutige Restrotation, Wiederholung und Versatz 0. |
| A-C-00139 | Formstabile Achsebenentranslation und Eckrotation mit Hals-/Kopfpivot. | Drei-/Viereckreferenzen in XY/XZ/YZ; Abstand, Pivot, Orientierung/Tiefe gemessen; Skalierung/Spiegelung abweisen. |
| A-C-00140 | Absoluter vorzeichenbehafteter Fock-Versatz in cm. | 0/+10/-10, geneigtes Vorstag, wiederholter Wert und vorab gedrehte/seitlich verschobene Fock nach Formel pruefen. |
| A-C-00141 | Sofortige atomare dauerhafte modellbezogene Speicherung ohne Speicherknopf. | Jede Operation nach Prozessneustart identisch; Schreibfehler/Unterbrechung ohne beschaedigten letzten Stand; Revisionskonflikt explizit. |
| A-C-00142 | Lookup- und intrinsische Quellen unveraendert; Lage fuer spaetere Physik nutzbar. | SHA-256 vor/nach allen Operationen fuer Quellen und Lookup-Dateien identisch; getrennte Geometrie-/Lageidentitaet; synthetische Punkt-/Vektor-/Momentenbezuege pruefen, keine neue Physik. |
| A-C-00143 | Typisierter Vorschau-/Bestaetigungs-/Diagnosevertrag neben unveraenderlicher Inspektion mit einer aktuellen Darstellung je Objekt. | GUI-unabhaengige Vertragstests fuer Status, Fassung/Revision, Fremd-ID, NaN, stale Montage, beschaedigte Daten und lesbaren gespeicherten Stand; physische Quellen bleiben getrennt erhalten. |
| A-C-00144 | Nur Konfiguration fuer Bootsauswahl, keine Aenderung bestehender Physikvertraege. | Bestehende Katalog-/Inspektions-/Simulationstests; 420er-Simulationsfreigabe bleibt unveraendert; kein Qt/VTK im Core. |
| A-C-00145 | Reale 420er-Integration und nachverfolgbare Core-Nachweise. | Neue Grosssegel-/Fockquellen samt Lookups als Referenz; alle aktiven Anforderungen mit Tests/Belegen und separatem Nutzerabnahmestatus. |
| A-C-00146 | Einheitlichen Segelstellwinkel und gerichtete Stellachse nach Abschnitt 8 im Datenvertrag festlegen. | Vertrags-/synthetische Rotationstests: 0 mittschiffs, +90 Backbord, -90 Steuerbord fuer beide Segel; lokale +Z und Lookup-Konvention unveraendert, keine zweite Stellwinkelkonvention. |
| A-C-00147 | Original-Buglage je Segel persistent verwalten und bedingte lokale Formspiegelung gemaess Abschnitt 9 bereitstellen. | Beide Originalseiten gegen positive/negative/Nullstellung pruefen, fehlende Angabe diagnostizieren; Kontur/Identitaet/Quellbytes erhalten, Normalen und Dreiecksorientierung konsistent. |

## 8. Einheitlicher Segelstellwinkel

**Nutzerfreigabe:** 06.11.2026, 10:11 Uhr. Verbindliche
Kompatibilitaetsfestlegung fuer die Modellhandhabung; keine neue
Simulationssteuerung, Lookup-Auswertung oder Windberechnung in C-0.12.

Bootsachsen bleiben +X voraus, +Y Backbord, +Z oben in Normallage.
Jedes Segel hat seinen eigenen lokalen Bezug mit Halsursprung und +Z
entlang des Vorlieks vom Hals zum Kopf. Rechtshaendigkeit und vorhandene
intrinsische Geometrie-/Lookup-Achsen bleiben erhalten.

Je Segel gibt es genau einen Stellwinkel delta in Grad:
0 bedeutet Grundsegelebene mittschiffs mit Schothorn achterlich,
positiv oeffnet nach Backbord, negativ nach Steuerbord.
+90 und -90 Grad bezeichnen die entsprechenden Oeffnungsstellungen.
Diese Winkel sind Drehwinkel um das Vorliek, nicht bei geneigtem Vorstag
mit einem horizontal gemessenen Schothorn-Azimut gleichzusetzen.

Die normierte **positive Stell-Drehachse a** zeigt vom Kopf zum Hals:
lokal a = (0, 0, -1). Grosssegel nutzt die definierte
Mast-/Vorliekachse, Fock die Vorstag-/Vorliekachse durch den Hals.
Mit der mathematischen Rechte-Hand-Regel gilt unmittelbar:

```text
R_stell = Rotation(a, delta)
p_modell = R_einbau * R_stell * p_segel + t_einbau
```

Die Drehachse geht durch den lokalen Halsursprung. Die automatische oder
manuell korrigierte Einbaulage (R_einbau, t_einbau) definiert delta = 0.
Die Stellbewegung ist davon getrennt und verschiebt den Hals nicht.
Es gibt keine zweite gespeicherte Winkelgroesse mit umgekehrtem Vorzeichen.
Eine interne Umrechnung Grad/Radiant aendert die Winkelbedeutung nicht.

Die Referenzdrehrichtung gilt in Normallage; Krängung oder ein beliebiges
manuelles Drehen darf das Vorzeichen nicht anhand von Welt-Y neu definieren.
Bei stark veraenderter Einbaulage gelten weiterhin die gerichtete lokale
Achse und deren Nullstellung. Die Achse wird mit R_einbau ins Bootssystem
transformiert, nicht als neue globale Vertikalachse interpretiert.

Der Lookup-Anstroemwinkel alpha ist ein anderer fachlicher Wert
(Windherkunft im lokalen Segelsystem), kein zweiter Stellwinkel.
Seine bestehende Definition und die intrinsischen Quellen bleiben
unveraendert. Die spaetere Windauswertung verwendet die vollstaendige
Segellage, nicht eine allgemeine Subtraktion zweier horizontaler Winkel.

## 9. Original-Buglage und lokale Formspiegelung

**Nutzerfreigabe:** 06.11.2026. Pro Segel wird die Buglage gespeichert,
fuer die die unveraenderte intrinsische Geometrie und ihr zugehoeriges
Darstellungsnetz gezeichnet wurden: **Backbordbug** (Baum/Schothorn
Backbord, Wind von Steuerbord) oder **Steuerbordbug** (umgekehrt).
Das ist eine Quellorientierungsangabe, keine Zielstellung.
Fehlende Angabe bleibt explizit unbekannt; keine Ableitung allein aus
Woelbungsvorzeichen, Elementnamen oder aktuellem Stellwinkel.

Die Angabe gehoert zum `setting` des bestehenden Segel-Elements in `bootsmodell.json` und zur
Quellzuordnung, wird getrennt fuer jedes Segel gespeichert und unterliegt
denselben atomaren Speicher-/Revisions-/Fehlerregeln wie die Einbaulage.
Eine vorhandene Konfiguration ohne diese neue optionale Angabe behaelt
ihre Lage und meldet die Originalseite als unbekannt.
Ein Seitenwechsel darf die starre Einbaulage nicht aendern.

Die Zielseite folgt spaeter aus delta: positiv Backbordbug, negativ
Steuerbordbug. Bei delta = 0 gilt die gespeicherte neutrale Montagebasis
mit unveraendertem Originalbauch, nicht die ausgeschwenkte Roh-OBJ-Stellung;
keine letzte Seite als stiller Ersatz. Eine unbekannte Zielseite ist
keine Freigabe, auf eine historisch geoeffnete Position zurueckzufallen.
Bei bekannter Zielseite und unbekannter Originalseite ist eine benoetigte
Spiegelentscheidung nicht moeglich und explizit zu diagnostizieren.

Bei gleicher Seite ist S = I; bei verschiedenen Seiten ist lokal
S = diag(1, -1, 1). Die vollstaendige Darstellungsabbildung lautet:

```text
p_modell = R_einbau * R_stell * S * p_original + t_einbau
```

Diese Kurzform gilt fuer die lokale 420-Quelle mit Hals im Ursprung.
Bei bereits in Modellkoordinaten gezeichneten Folk-/Opti-OBJs werden
Stellrotation und Formspiegelung um den expliziten Hals und die
OBJ-Vorlieksachse ausgefuehrt; danach gilt die gespeicherte Montagepose.
Positives delta wirkt weiterhin um die gerichtete Achse Kopf-zum-Hals.

S wird aus Quell-/Zielseite abgeleitet, nicht kumulativ auf die zuletzt
gerenderten Punkte angewendet. Damit fuehrt Wiederholung nicht zu einer
erneuten Rueckspiegelung. Die Stellachse Kopf zum Hals und der delta-Vertrag
bleiben unveraendert. Punkte der Grundkontur in Y = 0 bleiben fest;
Woelbung und raeumliche Verwindungsform werden gemeinsam gespiegelt.
Eckgriffe, Picking und angezeigte Geometrie muessen dieselbe Abbildung nutzen.
Normalen werden passend transformiert, Dreiecksorientierungen der
gespiegelten Darstellung korrigiert; keine falsche Beleuchtung/Culling.

Diese ausdruecklich freigegebene Formspiegelung ist eine Ausnahme vom
bisherigen Spiegelungsverbot: S hat Determinante -1 und wird **getrennt**
von der weiterhin eigentlichen starren Rotation R mit Determinante +1
gefuehrt. Keine negative Skalierung als vermeintlich gueltige Einbaulage.
Abstaende und Formgroesse bleiben erhalten; die intrinsischen Quellen,
Konturrollen, Originalseitenangabe und Lookup-Identitaeten bleiben erhalten.

Original-JSON, OBJ, Auftraege und Lookups werden nicht umgeschrieben.
Die spaetere Physik muss die Formparitaet ausdruecklich auswerten:
Eine optische Spiegelung allein macht einen Lookup nicht physikalisch
passend. Bei einer begruendeten Spiegelabbildung sind Kraefte/Ortsvektoren
polare Vektoren, Momente dagegen axiale Vektoren (unter S: det(S) * S * M).
Auch Anstroemung, Verwindung, Gueltigkeitsbereich und Modellsymmetrie
muessen geprueft werden; kein pauschales Spiegeln von Kraftspalten oder
unerlaubtes Erweitern des vorhandenen Lookup-Rasters.
Diese spaetere Aerodynamik ist nicht Bestandteil C-0.12.
