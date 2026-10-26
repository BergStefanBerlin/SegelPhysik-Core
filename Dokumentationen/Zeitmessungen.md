# Implementierte Zeitmessungen

Stand: 26.10.2026. Register zu A-C-00021 bis A-C-00024.
Dieses Register beschreibt Code, keine geplanten Timer.

## Aktivierung und Messgrenzen

`simulieren` misst jeden begonnenen Gesamtlauf mit
`std::chrono::steady_clock`, einschließlich Parameterprüfung, Modellimport,
Berechnung und Schließen der Ergebnisdatei. Auch Fehler und Abbruch
liefern die vergangene Zeit. Der anschließende CLI-Bericht ist nicht
enthalten; Prozessstart/-ende ebenfalls nicht. Einheit: Sekunden.
Simulationszeit wird davon unabhängig geführt.

Detailtimer sind durch `SEGELPHYSIK_TEST_INSTRUMENTATION` im normalen
Build vollständig aus den instrumentierten Funktionen entfernt.
Im gesonderten Testbuild muss zusätzlich
`testmessungen_aktivieren(true)` aufgerufen werden. Die CLI aktiviert
diese Funktion nicht. Der Zustand ist threadlokal; ein Lauf setzt
die gesammelten Werte zurück und liefert Anzahl und Summendauer.

| Kennung | Funktionalität / Messgrenze | Aktivierung | Quellfundstelle | Nachweis |
|---|---|---|---|---|
| gesamtlauf | Eintritt in `simulieren` bis nach Dateischluss bzw. Fehlerbehandlung | Immer | [simulation.cpp](../Code/src/simulation.cpp) | `simulation_gesamtlauf`, einschließlich Modellfehler und Abbruch |
| modellvorbereitung | Gesamter Import einschließlich JSON, Meshprüfung und Massenvorbereitung | Nur expliziter Testmodus | [modell.cpp](../Code/src/modell.cpp), `lade_modell` | `simulation_gesamtlauf` |
| masseneigenschaften | Masseneigenschaften eines Geometrieelements | Nur expliziter Testmodus | [modell.cpp](../Code/src/modell.cpp), Massenintegration | `simulation_gesamtlauf` |
| ebenenschnitt | Vollständige Bestimmung eines eingetauchten Volumens samt erstem Moment | Nur expliziter Testmodus | [hydrostatik.cpp](../Code/src/hydrostatik.cpp), `wasseranteil` | `hydrostatik_referenzen` |
| hydrostatikauswertung | Alle Elementgewichte/-auftriebe und Summen eines Zustands | Nur expliziter Testmodus | [hydrostatik.cpp](../Code/src/hydrostatik.cpp), `hydrostatik` | `hydrostatik_referenzen` |
| gleichgewichtssuche | Gesamter statischer Solver einschließlich Initialisierung | Nur expliziter Testmodus | [hydrostatik.cpp](../Code/src/hydrostatik.cpp), `gleichgewicht` | `hydrostatik_referenzen` |
| dynamischer_zeitschritt | Ein vollständiger Integrationsschritt samt Kraftauswertungen | Nur expliziter Testmodus | [hydrostatik.cpp](../Code/src/hydrostatik.cpp), `dynamikschritt` | `hydrostatik_referenzen` |
| ergebnisaufzeichnung | Aufbau und Schreiben eines vollständigen Zustands; endgültiger Dateischluss im Gesamttimer | Nur expliziter Testmodus | [simulation.cpp](../Code/src/simulation.cpp), Schreibfunktion innerhalb `simulieren` | `simulation_gesamtlauf` |

Messungen sind verschachtelt: Ein Solver enthält Hydrostatikauswertungen,
diese wiederum Schnitte. Eine Aufzeichnung enthält eine erneute
Kraftauswertung für genau den aufgezeichneten Zustand. Summen dürfen
deshalb nicht zu einer angeblichen Gesamtlaufzeit addiert werden.
Kein Timer liegt innerhalb einer Dreiecks-/Vektoroperation.
Benchmarkwiederholungen werden separat als Median, Minimum und Maximum
berichtet; Einzelaufrufe werden nicht laufend protokolliert.

Noch fehlende Messpunkte werden nicht als implementiert ausgewiesen.
