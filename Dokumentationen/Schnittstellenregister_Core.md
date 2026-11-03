# Schnittstellenregister CORE

Projektweites Register dauerhafter C++-Interfacekennungen (`CPPI-*`).
Die Kennungen ergänzen A-/U-Verweise und bleiben bei Umbenennung oder
Weiterentwicklung erhalten.

| CPPI-ID | C++-Schnittstelle | Status | Version/Anforderung | Bemerkung |
|---|---|---|---|---|
| CPPI-0001 | `HydrodynamikRolle` | implementiert | C-0.6 / A-C-00061 | Rollen `rumpf`, `kiel`, `ruder` |
| CPPI-0002 | `Element::hydrodynamik_rolle` | implementiert | C-0.6 / A-C-00061 | Optionale importierte Rolle aus `modell.json` |
| CPPI-0003 | `HydrodynamikKonfiguration` | implementiert | C-0.6 / A-C-00072 | Fluiddichte, kinematische Viskosität und Weltgeschwindigkeit |
| CPPI-0004 | `HydrodynamikKontext` | implementiert | C-0.6 / A-C-00072 | Unveränderliche Konfiguration mit Modellfassung und Auswertungszustand |
| CPPI-0005 | `Zustand::hydrodynamik` | implementiert | C-0.6 / A-C-00072 | Historischer, unveränderlicher Laufkontext |
| CPPI-0006 | `Zustand::hydrodynamik_erforderlich` | implementiert | C-0.6 / A-C-00072 | Erkennt verlorene Pflichtkontexte |
| CPPI-0007 | `hydrodynamik_start` | implementiert | C-0.6 / A-C-00072 | Validiert und bindet den Laufkontext an ein Modell |
| CPPI-0008 | `kraefte` mit Hydrodynamikkontext und Translationsgeschwindigkeit | implementiert | C-0.6 / A-C-00063, A-C-00067 | Additive Einzelbeiträge; bestehende Aufrufe bleiben quellkompatibel |
| CPPI-0009 | `bilanz` mit `hydrodynamik_aktiv` | implementiert | C-0.6 / A-C-00067 | Erwartete Rumpfbeiträge werden auf Vollständigkeit geprüft |
| CPPI-0010 | `Laufoptionen::hydrodynamik` | implementiert | C-0.6 / A-C-00072 | Optionale explizite Konfiguration; dynamische Rollenmodelle nutzen sonst das dokumentierte Standardwasserprofil |
| CPPI-0011 | `Bewegungsart` | implementiert | C-0.7 / A-C-00079 | Bewegungsarten `drehen` und `heben` |
| CPPI-0012 | `Bewegungsdefinition` | implementiert | C-0.7 / A-C-00079 | Importierte Achse, Pivot, Hubrichtung/-weg und gemeinsam bewegte Darstellungselemente |
| CPPI-0013 | `Element::bewegung` | implementiert | C-0.7 / A-C-00079 | Optionale, strikt validierte JSON-Bewegungsdefinition |
| CPPI-0014 | `ElementBeschreibung::bewegung` | implementiert | C-0.7 / A-C-00079 | Bewegungsmetadaten für die Katalog-/GUI-Vorschau |
| CPPI-0015 | `StrukturElement::bewegung` | implementiert | C-0.7 / A-C-00081 | Bedienart und Kinematikdaten in der Modellstruktur |
| CPPI-0016 | `Bewegungsstellung` | implementiert | C-0.7 / A-C-00082 | Bestätigter Winkel bzw. Stellungsprozentsatz je Element |
| CPPI-0017 | `Bewegungsereignis` | implementiert | C-0.7 / A-C-00082 | Geordnete, elementbezogene Stellwertänderung |
| CPPI-0018 | `BewegungsUebergang` | implementiert | C-0.7 / A-C-00082 | Ereignisfolge mit Schritt-, Zeit- und Schrittweitenbezug |
| CPPI-0019 | `Zustand::bewegungsstellungen` | implementiert | C-0.7 / A-C-00082 | Historischer Core-Stellzustand |
| CPPI-0020 | `Zustand::letzte_bewegungsereigniskennung` | implementiert | C-0.7 / A-C-00082 | Monotone Ereignisfolge über Startzustandsübernahmen |
| CPPI-0021 | `Zustand::bewegungsuebergang` | implementiert | C-0.7 / A-C-00082 | Ereignisprotokoll des letzten Integratorschritts |
| CPPI-0022 | `kraefte` mit Bewegungsstellungen | implementiert | C-0.7 / A-C-00082 | Hydrodynamik berücksichtigt bewegte Netze; Hydrostatik bleibt unverändert |
| CPPI-0023 | `bewegungstransformation` | implementiert | C-0.7 / A-C-00083 | Zeilenweise 4×4-Transformation je Modell-Element für die Darstellung |
| CPPI-0024 | `dynamikschritt` mit Stellereignissen | implementiert | C-0.7 / A-C-00082 | Übernimmt Stellwerte deterministisch vor allen Auswertungen des Zeitschritts |
| CPPI-0025 | `Laufoptionen::bewegungsereignisse` | implementiert | C-0.7 / A-C-00082 | Batch-Ereignisse nach Quellschritt und Kennung |
| CPPI-0026 | `HydrodynamikKonfiguration::zusatzmasse_daempfung` | implementiert | C-0.7 / A-C-00087 | Schalter (Standard an); aus = Verhalten ohne Zusatzmasse |
| CPPI-0027 | `HydrodynamischeTraegheit`, `HydrodynamikKontext::traegheit` | implementiert | C-0.7 / A-C-00086, A-C-00090 | Zusatzmassen und Dämpfung mit Herkunft, bei `hydrodynamik_start` berechnet |
| CPPI-0028 | `berechne_hydrodynamische_traegheit`, `ellipsoid_zusatzmasse` | implementiert | C-0.7 / A-C-00086 | Lamb-Doppelkörper-Ellipsoid, Plattennäherung für Ruder/Schwert |
| CPPI-0029 | Modellblock `Hydrodynamische_Traegheit`, `Modell::hydrodynamische_traegheit` | implementiert | C-0.7 / A-C-00087 | Optionales Überschreiben von Zusatzmassen/Dämpfung; Fehler CORE-MOD-0002 |
| CPPI-0030 | Ergebnisfeld `hydrodynamische_traegheit`, Beiträge `daempfung:*` | implementiert | C-0.7 / A-C-00091 | Additiv; `vertragsversion` und Altfelder unverändert |
| CPPI-0031 | Ergebnisfeld `druckpunkte.lateral` (`status` gueltig/geometrisch/nicht_bestimmbar, `position_modell_m`, `querkraft_modell_n`, `lateralflaeche_m2`, `quellen`) | implementiert | C-0.7 / A-C-00093 bis A-C-00096 | Additiv; `vertragsversion` unverändert; Status `geometrisch` ergänzt A-C-00095 (Freigabe offen) |

Die Quellenkennung `Quelle::hydrodynamik` und die Ergebnisgruppe
`hydrodynamik` sind additive Werte des bestehenden Beitragsvertrags.
Für die erste Core-Stufe wird nur die Rumpfrolle mit dem ITTC-1957-
Reibungsmodell berechnet. Kiel- und Ruderrollen sind importierbar, aber
erzeugen noch keine Seitenkraft.

## C-0.8: Implementiert; technische Core-Tests bestanden

Maßgeblich ist [Modellkatalog-Spezifikation, Abschnitt 3.7](../Spezifikationen/Spezifikation_Modellkatalog_Schnittstelle.md#37-normativer-daten--und-api-vertrag).
Jeder nachfolgende Export hat eine eigene Kennung. Die Inspektionsfunktionen
und kompatiblen Snapshotprojektionen sind in Core-CTest abgedeckt; die
GUI-Integration und Nutzerabnahme bleiben getrennte Schritte.

| CPPI-ID | C++-Schnittstelle | Status | Anforderung |
|---|---|---|---|
| CPPI-0032 | `InspectionMatrix` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0033 | `InspectionValue` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0034 | `InspectionType` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0035 | `InspectionNode` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0036 | `InspectionProperty` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0037 | `InspectionMesh` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0038 | `InspectionInfo` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0039 | `ModelInspection` | implementiert | C-0.8 / A-C-00099, A-C-00102 |
| CPPI-0040 | `ModelInspection::info` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0041 | `readModelInspection` | implementiert | C-0.8 / A-C-00099 |
| CPPI-0042 | `getAvailableObjectTreeTypes` | implementiert | C-0.8 / A-C-00098 |
| CPPI-0043 | `getObjectTree` | implementiert | C-0.8 / A-C-00098 |
| CPPI-0044 | `getObjectProperties` | implementiert | C-0.8 / A-C-00100 |
| CPPI-0045 | `getAvailableMeshTypes` | implementiert | C-0.8 / A-C-00101 |
| CPPI-0046 | `getMesh` | implementiert | C-0.8 / A-C-00101 |
| CPPI-0047 | `inspectionInfoJson` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0048 | `inspectionTreeTypesJson` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0049 | `inspectionTreeJson` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0050 | `inspectionPropertiesJson` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0051 | `inspectionMeshTypesJson` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0052 | `inspectionMeshJson` | implementiert | C-0.8 / A-C-00102 |
| CPPI-0053 | `modellstruktur(const ModelInspection&)` | implementiert | C-0.8 / A-C-00104 |
| CPPI-0054 | `darstellungsgeometrie(const ModelInspection&, optional<Element-ID>)` | implementiert | C-0.8 / A-C-00104 |
| CPPI-0055 | `Element::massen_mesh` | implementiert | C-0.8 / A-C-00101 |
| CPPI-0056 | `Element::massenart` | implementiert | C-0.8 / A-C-00098 |
| CPPI-0057 | `Element::dichte_kg_pro_m3` | implementiert | C-0.8 / A-C-00098 |
| CPPI-0058 | `Element::wanddicke_m` | implementiert | C-0.8 / A-C-00098 |
| CPPI-0059 | `ModellBeschreibung::dokument` | implementiert | C-0.8 / A-C-00098 |
| CPPI-0060 | `QuellMesh` | implementiert | C-0.8 / A-C-00105, A-C-00108; unveränderte Quellgeometrie |
| CPPI-0061 | `ElementBeschreibung::darstellung_mesh` | implementiert | C-0.8 / A-C-00105, A-C-00108 |
| CPPI-0062 | `ElementBeschreibung::volumen_mesh` | implementiert | C-0.8 / A-C-00105, A-C-00108 |
| CPPI-0063 | `ElementBeschreibung::masse_mesh` | implementiert | C-0.8 / A-C-00105, A-C-00108 |
| CPPI-0064 | `ModellBeschreibung::quellmeshes` | implementiert | C-0.8 / A-C-00108; auch unbenutzte OBJ-Varianten |
| CPPI-0065 | `konvertiere_modell_v2` | implementiert | C-0.8 / A-C-00109; neuer Zielordner, kein Überschreiben |
| CPPI-0066 | `InspectionNode::mesh_ids` | implementiert | C-0.8 / A-C-00108; direkte Mesh-Verweise eines Elements |
| CPPI-0067 | `InspectionMesh::source_id` | implementiert | C-0.8 / A-C-00108; stabile o-ID im Schema 2 |
| CPPI-0068 | `InspectionMesh::provenance` | implementiert | C-0.8 / A-C-00108; `quelle` oder `abgeleitet` |
| CPPI-0069 | `InspectionMesh::derivation` | implementiert | C-0.8 / A-C-00108; lesbare Herkunft/Ableitung |
| CPPI-0070 | `InspectionMesh::element_ids` | implementiert | C-0.8 / A-C-00108; vollständige Verwendungen einer Quelle |
| CPPI-0071 | `QuellMesh::id` | implementiert | C-0.8 / A-C-00105, A-C-00108 |
| CPPI-0072 | `QuellMesh::ursprung` | implementiert | C-0.8 / A-C-00108; logische Quelle ohne absolute Pfade |
| CPPI-0073 | `QuellMesh::mesh` | implementiert | C-0.8 / A-C-00105, A-C-00108; modellfeste Meter |
| CPPI-0074 | `InspectionProperty::area` | implementiert | C-0.8 / A-C-00112; stabile fachliche Bereichszuordnung |
| CPPI-0075 | `InspectionMovementPose` | implementiert | C-0.8 / A-C-00113; typisierte Snapshot-Meshes einer Bewegungsstellung |
| CPPI-0076 | `InspectionMovementPreview` | implementiert | C-0.8 / A-C-00113; Referenz, Achse, optionale Grenzen und Posen |
| CPPI-0077 | `getMovementPreview` | implementiert | C-0.8 / A-C-00113; Core-transformierte Normal-/Grenzgeometrie |
| CPPI-0078 | `movementPreviewJson` | implementiert | C-0.8 / A-C-00113; JSON-Projektion der Bewegungs-Vorschau |
| CPPI-0079 | `konvertiere_modell_v3` | implementiert | C-0.8 / A-C-00111; Schema-3-Migration mit Konfliktabbruch; optional `ableitungen_zulassen=false`, explizit `true` für geprüfte Ebenenschnitte/Flächenauswahl |
| CPPI-0080 | `StationaereGleichgewichtKonfiguration` | implementiert | C-0.8 / A-C-00114; eine optionale, feste Fremdkraft und Elementstellungen |
| CPPI-0081 | `StationaeresGleichgewichtErgebnis` | implementiert | C-0.8 / A-C-00116; vollständige sechs Residuen, Toleranzen, Status und Ergebniszustand |
| CPPI-0082 | `stationaeres_gleichgewicht` | implementiert | C-0.8 / A-C-00114 bis A-C-00117; begrenzte stationäre Suche mit Hydrodynamik |
| CPPI-0083 | `Laufoptionen::stationaer` | implementiert | C-0.8 / A-C-00114; optionale Belegung aktiviert den stationären Lauf, `aktiv` schaltet allein die Fremdkraft |
| CPPI-0084 | `zustand_json(..., StationaeresGleichgewichtErgebnis*)` | implementiert | C-0.8 / A-C-00116; additive Ergebnisvertragsversion 5 und `stationaer`-Snapshot |
| CPPI-0085 | `Quelle::externe_kraft` | implementiert | C-0.8 / A-C-00115; Herkunft der konfigurierten stationären Fremdkraft |
| CPPI-0086 | `hydrostatik(..., bewegungsstellungen)` | implementiert | C-0.8 / A-C-00115; konsistente bewegte Verdrängungs-/Massenlage |
| CPPI-0087 | `StationaeresGleichgewichtErgebnis::flutungswiederholungen` | implementiert | C-0.8 / A-C-00116; Anzahl automatischer Neuberechnungen nach neu erkannter Flutung |

| CPPI-0088 | `StationaereGleichgewichtKonfiguration::kurvenfahrt` | implementiert | C-0.8 / A-C-00118 | Expliziter additiver Dreh-/Kurvenmodus; Standard `false`, fehlendes JSON-Eingabefeld bedeutet `false` |
| CPPI-0089 | `ErgebnisJsonFormat` | implementiert | C-0.8 / A-C-00121 | `Kompatibel` erhaelt V1-V5 automatisch; `V6` liefert stationaere Bewegungsbilanz und Kursrate |
| CPPI-0090 | `Laufoptionen::ergebnis_json_format` | implementiert | C-0.8 / A-C-00121 | Standard `ErgebnisJsonFormat::V6` |
| CPPI-0091 | `zustand_json(..., ErgebnisJsonFormat)` | implementiert | C-0.8 / A-C-00121 | Standard V6; `Kompatibel` erhaelt bisherige V1-V5-Projektion |
| CPPI-0092 | `StationaeresGleichgewichtErgebnis::loesungstyp` | implementiert | C-0.8 / A-C-00121 | `geradeaus`, `kurvenfahrt` oder `reine_drehung` |
| CPPI-0093 | `StationaeresGleichgewichtErgebnis::kursdrehrate_rad_s` | implementiert | C-0.8 / A-C-00121 | Signierte Weltvertikalrate; nicht Betrag von `omega` |
| CPPI-0094 | `StationaeresGleichgewichtErgebnis::aeussere_kraft_welt_n` | implementiert | C-0.8 / A-C-00119, A-C-00121 | aeussere Lastsumme getrennt von Bewegungsresiduum |
| CPPI-0095 | `StationaeresGleichgewichtErgebnis::aeusseres_moment_modell_nm` | implementiert | C-0.8 / A-C-00119, A-C-00121 | aeussere Momentensumme getrennt von Bewegungsresiduum |
| CPPI-0096 | `StationaeresGleichgewichtErgebnis::residuum_bewegung_kraft_welt_n` | implementiert | C-0.8 / A-C-00119 | Fext minus Starrkoerper-/Fluidtraegheitsbeitrag |
| CPPI-0097 | `StationaeresGleichgewichtErgebnis::residuum_bewegung_moment_modell_nm` | implementiert | C-0.8 / A-C-00119 | Mext minus Traegheits-/Kreiselbeitrag |
| CPPI-0098 | `StationaeresGleichgewichtErgebnis::radius_m` | implementiert | C-0.8 / A-C-00120, A-C-00121 | Radius im mitstroemenden Wasserrahmen; leer bei r=0 oder reiner Drehung |
| CPPI-0099 | `StationaeresGleichgewichtErgebnis::omega_modell_rad_s` | implementiert | C-0.8 / A-C-00121 | Modellachsenraten aus kinematischer Weltvertikal-Drehung |
| CPPI-0100 | `StationaeresGleichgewichtErgebnis::kurvenfahrt` | implementiert | C-0.8 / A-C-00118, A-C-00121 | Effektiver Modus im Ergebnis-Snapshot |

## C-0.8: Druckpunktauswertung und Ersatz-SDP

Die konkreten additiven Ergebnisfelder und die für die Panelbilanz nötigen
C++-Exporte sind vor Implementierung registriert. Quelle:
[Druckpunktspezifikation Abschnitt 5](../Spezifikationen/Spezifikation_Lateraldruckpunkt.md#5-c-08-korrekter-ldp-und-ersatz-sdp).
Die bestehende Kennung CPPI-0031 bleibt fuer `druckpunkte.lateral`
erhalten. Weitere konkrete C++-Exports erst nach Festlegung ihrer
Signatur einzeln registrieren, nicht unter einer Familienkennung.

| CPPI-ID | Schnittstelle | Status | Anforderung / Bedeutung |
|---|---|---|---|
| CPPI-0101 | Ergebnisfeld `druckpunkte.lateral.auswertung_version` | implementiert | C-0.8 / A-C-00125; Wert 2 kennzeichnet korrigierte Panelmomente und Panelbetragsschwelle |
| CPPI-0102 | Ergebnisblock `druckpunkte.ersatz_segel` | implementiert | C-0.8 / A-C-00124, A-C-00125; Status, Quelle `fremdkraft` und optionaler modellfester Ansatzpunkt, keine Aerodynamik |
| CPPI-0103 | `LateraldruckpunktPanelbilanz` | implementiert | C-0.8 / A-C-00123; aggregierte signierte Panelwerte Q/B/X/Z |
| CPPI-0104 | `Beitrag::lateraldruckpunkt_panelbilanz` | implementiert | C-0.8 / A-C-00123; vor Bauteilaggregation berechnete Panelbilanz |

## C-0.9: Dynamische Fremdkraft – freigegebener Core-Mindestvertrag

Maßgeblich sind A-C-00126 bis A-C-00128 in der
[Spezifikation Dynamische Fremdkraft](../Spezifikationen/Spezifikation_Dynamische_Fremdkraft.md).
Nur der Core-Mindestvertrag ist implementiert; GUI-Integration,
Vollständigkeitsmatrix und Performancevergleich bleiben offen.

| CPPI-ID | C++-Schnittstelle | Status | Anforderung |
|---|---|---|---|
| CPPI-0105 | `DynamischeFremdkraftKonfiguration` | implementiert | C-0.9 / A-C-00126; Aktivflag, modellfester Kraftvektor und Ansatzpunkt |
| CPPI-0106 | `DynamischeFremdkraftKontext` | implementiert | C-0.9 / A-C-00126; unveränderlicher Lauf- und Snapshotkontext |
| CPPI-0107 | `Zustand::dynamische_fremdkraft` | implementiert | C-0.9 / A-C-00126; optionaler historischer Kontext |
| CPPI-0108 | `Zustand::dynamische_fremdkraft_erforderlich` | implementiert | C-0.9 / A-C-00126; erkennt verlorenen Pflichtkontext |
| CPPI-0109 | `dynamische_fremdkraft_start` | implementiert | C-0.9 / A-C-00126; Validierung und Bindung an Modell und Startzustand |
| CPPI-0110 | `kraefte(..., DynamischeFremdkraftKontext*)` | implementiert | C-0.9 / A-C-00127; genau ein Beitrag mit Modellvektor, Ansatzpunkt und Moment |
| CPPI-0111 | `Laufoptionen::dynamische_fremdkraft` | implementiert | C-0.9 / A-C-00126; optionale Batch-Laufkonfiguration |
| CPPI-0112 | `Kraefte::schwerpunkt_modell_m` | implementiert | C-0.9 / A-C-00128; physischer Gesamtschwerpunkt fuer dieselbe Elementstellung |
