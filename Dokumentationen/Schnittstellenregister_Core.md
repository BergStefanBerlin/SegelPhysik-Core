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

Die Quellenkennung `Quelle::hydrodynamik` und die Ergebnisgruppe
`hydrodynamik` sind additive Werte des bestehenden Beitragsvertrags.
Für die erste Core-Stufe wird nur die Rumpfrolle mit dem ITTC-1957-
Reibungsmodell berechnet. Kiel- und Ruderrollen sind importierbar, aber
erzeugen noch keine Seitenkraft.
