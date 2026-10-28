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
| CPPI-0011 | `Bewegungsart` | implementiert | C-0.8 / A-C-00079 | Bewegungsarten `drehen` und `heben` |
| CPPI-0012 | `Bewegungsdefinition` | implementiert | C-0.8 / A-C-00079 | Importierte Achse, Pivot, Hubrichtung/-weg und gemeinsam bewegte Darstellungselemente |
| CPPI-0013 | `Element::bewegung` | implementiert | C-0.8 / A-C-00079 | Optionale, strikt validierte JSON-Bewegungsdefinition |
| CPPI-0014 | `ElementBeschreibung::bewegung` | implementiert | C-0.8 / A-C-00079 | Bewegungsmetadaten für die Katalog-/GUI-Vorschau |
| CPPI-0015 | `StrukturElement::bewegung` | implementiert | C-0.8 / A-C-00081 | Bedienart und Kinematikdaten in der Modellstruktur |
| CPPI-0016 | `Bewegungsstellung` | implementiert | C-0.8 / A-C-00082 | Bestätigter Winkel bzw. Stellungsprozentsatz je Element |
| CPPI-0017 | `Bewegungsereignis` | implementiert | C-0.8 / A-C-00082 | Geordnete, elementbezogene Stellwertänderung |
| CPPI-0018 | `BewegungsUebergang` | implementiert | C-0.8 / A-C-00082 | Ereignisfolge mit Schritt-, Zeit- und Schrittweitenbezug |
| CPPI-0019 | `Zustand::bewegungsstellungen` | implementiert | C-0.8 / A-C-00082 | Historischer Core-Stellzustand |
| CPPI-0020 | `Zustand::letzte_bewegungsereigniskennung` | implementiert | C-0.8 / A-C-00082 | Monotone Ereignisfolge über Startzustandsübernahmen |
| CPPI-0021 | `Zustand::bewegungsuebergang` | implementiert | C-0.8 / A-C-00082 | Ereignisprotokoll des letzten Integratorschritts |
| CPPI-0022 | `kraefte` mit Bewegungsstellungen | implementiert | C-0.8 / A-C-00082 | Hydrodynamik berücksichtigt bewegte Netze; Hydrostatik bleibt unverändert |
| CPPI-0023 | `bewegungstransformation` | implementiert | C-0.8 / A-C-00083 | Zeilenweise 4×4-Transformation je Modell-Element für die Darstellung |
| CPPI-0024 | `dynamikschritt` mit Stellereignissen | implementiert | C-0.8 / A-C-00082 | Übernimmt Stellwerte deterministisch vor allen Auswertungen des Zeitschritts |
| CPPI-0025 | `Laufoptionen::bewegungsereignisse` | implementiert | C-0.8 / A-C-00082 | Batch-Ereignisse nach Quellschritt und Kennung |

Die Quellenkennung `Quelle::hydrodynamik` und die Ergebnisgruppe
`hydrodynamik` sind additive Werte des bestehenden Beitragsvertrags.
Für die erste Core-Stufe wird nur die Rumpfrolle mit dem ITTC-1957-
Reibungsmodell berechnet. Kiel- und Ruderrollen sind importierbar, aber
erzeugen noch keine Seitenkraft.
