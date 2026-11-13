# Core-Umsetzungsplan C-0.23: Windfeld und Windvertrag

**Stand:** 13.11.2026 · **Status:** Umsetzung begonnen; Gesamtumsetzung,
Tests und Abnahme offen. Vorgesehener nächster Core-Versionsschritt nach C-0.22;
Umsetzung am 13.11.2026 beauftragt. Kein Tag oder Release.
Maßgeblich sind die [Windspezifikation](../Spezifikationen/Spezifikation_Windmodell.md)
und die [Referenzpunkt-Spezifikation](../Spezifikationen/Spezifikation_Referenzpunkte.md).
Der GUI-Plan liegt getrennt in
[G-0.23](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.23.md).

## 1. Umfang, Voraussetzungen und Grenzen

Der Core liefert Windfeld, Referenzprofil, lokalen scheinbaren Wind an der
Mastspitze, bestätigte zeitbezogene Einstellungen und Status für GUI und
Wiedergabe. Eine gemeinsame Punktesammlung bündelt die in der
Referenzpunkt-Spezifikation genannten Punkte einschließlich beider
Koordinatensysteme und verfügbarer Geschwindigkeiten. Er implementiert
keine Segelkräfte, keinen echten SDP und keine
Windphysik für weitere Punkte oder bewegliche Segel. Fremdkraft und
bisherige Solverphysik bleiben unverändert. GUI, Modellrepository und deren
Git-Stände sind keine Core-Arbeitsprodukte dieses Plans.

Vor Vergabe wurden in Spezifikationen, Plänen und Dokumentationen die
höchsten bisherigen Kennungen A-C-00157, U-C-000140 und im
Schnittstellenregister CPPI-0152 geprüft. Dieser Plan nutzt A-C-00158
bis A-C-00168 und U-C-000141 bis U-C-000148. Die konkretisierten
Schnittstellen CPPI-0153 bis CPPI-0169 sind im Register erfasst. Sollten
weitere öffentliche Typen oder Funktionen benötigt werden, erhält
jedes einzelne Interface **vor Implementierung** eine neue CPPI-Kennung
oberhalb des dann geprüften Höchststands im bestehenden Register.

Die aktuelle `Modellstruktur` stellt Gruppen und Elemente bereit, aber
keinen ausdrücklichen Mastspitzenvertrag. Ist keine Spitze definiert, wird
der höchste Punkt des eindeutigen Mastes aus dessen neutraler
Darstellungsgeometrie nach der Referenzpunkt-Spezifikation bestimmt.
Nicht den höchsten Punkt eines anderen Objekts verwenden. Fehlt der Mast,
seine verwendbare Geometrie oder bei mehreren Masten eine eindeutige
Hauptmastzuordnung, bleibt der Punkt mit Diagnose nicht verfügbar.

## 2. Umsetzungsschritte und Abhängigkeiten

| U-ID | Schritt und fachlicher Umfang | Anforderungen | Abhängigkeiten | Nachweis |
|---|---|---|---|---|
| U-C-000141 | Bestehende Modell-, Zustands-, Live- und Wiedergabeverträge untersuchen; Mastidentität, Spitze, Schwerpunkt und ihre Frames sowie vorhandene Ergebnisformate prüfen. Fachlichen Datenaustausch, Fehler-/Nichtverfügbarkeitsstatus und Snapshotzuordnung konkretisieren; jedes geplante öffentliche Interface einzeln registrieren. Lücken ausdrücklich melden. | A-C-00163, A-C-00164, A-C-00165, A-C-00166 bis A-C-00168 | keine | Vertragsinventar mit Datenherkunft, vorhandenen Schnittstellen und CPPI-Zuordnung; fehlende oder mehrdeutige Mastdaten ergeben Blocker statt Ersatzdaten. |
| U-C-000142 | Referenzeinstellungen validieren und das logarithmische Höhenprofil mit Mindestwind auswerten. Core-Eingaben außerhalb der Grenzen oder nicht endlich sowie unzulässige Höhen explizit ablehnen; lokale Werte nicht auf 100 m/s deckeln. | A-C-00160, A-C-00164 | U-C-000141 | C23-01 und C23-02; reproduzierbare numerische Toleranzen und Diagnosen dokumentiert. |
| U-C-000143 | Weltkompass, horizontale Strömungsrichtung und statische sowie dynamische Richtungsmodi einschließlich Umschalten implementieren. Nicht bestimmbare horizontale Bootsausrichtung explizit diagnostizieren. | A-C-00158, A-C-00159, A-C-00162 | U-C-000141, U-C-000142 | C23-03 und C23-04; keine vertikale wahre Windkomponente bei Krängung/Stampfen. |
| U-C-000147 | Gemeinsame Punktesammlung und bestehende Punktarten mit stabiler Identität, Herkunft und getrenntem Status für Position und Geschwindigkeit bereitstellen; den Mastspitzeneintrag für U-C-000144 vorsehen. Positionen und bestimmbare weltbezogene Geschwindigkeiten in Welt- und Bootsachsen aus demselben Zustand ausgeben; wandernde Resultierende nicht als feste Punkte behandeln. | A-C-00166, A-C-00167 | U-C-000141 | C23-11 und C23-12 für bestehende Punktarten und fehlende Werte; Mastspitzenfall nach U-C-000144. |
| U-C-000144 | Vorhandene Mastspitze übernehmen oder aus dem höchsten Punkt der eindeutigen Mast-Darstellungsgeometrie bestimmen und über die Punktesammlung bereitstellen. Aus Pose, Schwerpunkttranslation und Welt-Winkelgeschwindigkeit Punktgeschwindigkeit und lokalen scheinbaren Wind berechnen. Fehlende/mehrdeutige Masten und ungültige Höhen kennzeichnen. | A-C-00161, A-C-00162, A-C-00164, A-C-00165, A-C-00167 | U-C-000141, U-C-000142, U-C-000143, U-C-000147 | C23-05 bis C23-07 und C23-12; höchster Mastpunkt und posierte Lage prüfen. |
| U-C-000145 | Bestätigte Live-Änderungen und Richtungsmodus zeitlich konsistent in Laufzuständen und Wiedergabe abbilden; `h0` im Lauf nicht ändern. Vorhandene Ergebnisformate und Verbraucher kompatibel halten, für alte Zustände fehlende Winddaten explizit kennzeichnen. | A-C-00159, A-C-00163, A-C-00164 | U-C-000141 bis U-C-000144 | C23-08 und C23-09; Thread-/Zustandsgrenzen und historische Zuordnung nachweisen. |
| U-C-000148 | Punktesammlung und bestehende Punkt-/Beitragsansichten mit Livezustand und Wiedergabe konsistent integrieren; bisherige Felder für bestehende Verbraucher als kompatible Sicht desselben Ergebnisses erhalten. Fehlende neue Daten in alten Ergebnissen explizit ausweisen. | A-C-00166, A-C-00168 | U-C-000147, U-C-000145 | C23-11 und C23-13; bestehende LDP-, Ersatz-SDP- und Kraftverträge bleiben erhalten. |
| U-C-000146 | Core-Regression, Schnittstellenregister, Core–GUI-Dokumentation und Übergabenachweise vervollständigen. Anforderungen gegen tatsächliche Tests und Ergebnisse abgleichen. | A-C-00158 bis A-C-00168 | U-C-000141 bis U-C-000145, U-C-000147, U-C-000148 | C23-10, nativer Build und vollständiger CTest erfolgreich; ansonsten offener Fehler und kein Abschluss. |

## 3. Testanforderungen

| Fall | Eingabe und erwartetes Ergebnis |
|---|---|
| C23-01 Profil | `h0=10 m`, `v0=6 m/s`: bei 1/5/10/20 m rund 4,72/5,62/6,00/6,38 m/s; numerische Referenz mit dokumentierter Toleranz vergleichen. Bei gleicher Höhe und unterschiedlichen horizontalen Positionen gleiche Werte. |
| C23-02 Grenzen | `h=z0` ergibt mindestens 0,01 m/s; `h<z0`, NaN/Inf und Werte außerhalb `v0=0,01–100 m/s`, `h0=0,01–100 m` liefern Fehler. Bei `v0=100 m/s`, `h0=10 m`, `h=20 m` liegt der Wert bei etwa 106,41 m/s und wird nicht gekappt. Grenzwerte selbst sind gültig. |
| C23-03 Weltachse | N/O/S/W und 0°/360°: Herkunft korrekt, Luftströmung um 180° entgegengesetzt; Welt-X Nord, Welt-Y West. Krängung und Stampfen erzeugen keine Welt-Z-Komponente des wahren Windes. |
| C23-04 Modi | Kursänderung in Statik und bootmitgeführter Dynamik gegenüber weltfester Dynamik; bei beiden Umschalt-Richtungen unmittelbar kein Sprung der Weltrichtung. Nicht bestimmbare horizontale Bootsausrichtung ergibt Diagnose. |
| C23-05 Relativwind | Bei ruhender Mastspitze sind wahre und scheinbare Strömung gleich; bekannte Translation wird subtrahiert; Rotation verwendet `omega × Hebelarm` im Weltframe. Heben/Rotation können scheinbaren vertikalen Wind erzeugen. |
| C23-06 Punktverfügbarkeit | Explizite Mastspitze und ersatzweise höchste Modell-Z-Höhe des Mastmeshes, einschließlich mehrerer gleich hoher Vertices und Krängung; kein Mast, mehrere Masten mit eindeutigem Hauptmast und ohne eindeutige Zuordnung. Korrekter Punkt oder explizit nicht verfügbar, nie geratener Ersatz-SDP. Lokale Höhe aus Weltpose über Wasser. |
| C23-07 Richtungsstatus | Exakte Vektorgleichheit und rein vertikaler scheinbarer Vektor; Betrag bleibt physikalisch korrekt. Für wahren und scheinbaren Wind horizontale und räumliche Beträge bei 0,1 m/s und knapp darüber getrennt auswertbar. |
| C23-08 Zeit | Richtung und `v0` im Lauf ändern, beide Modi wechseln, `h0`-Änderung im Lauf zurückweisen; vor und nach bestätigtem Änderungszeitpunkt abfragen und alten Stand wiedergeben. Start/Neustart verwenden bestätigte Vorgaben. |
| C23-09 Vertrag | Zeit, Modellidentität, Pose, Einstellungen, Punkt, Profil und Vektoren gehören demselben Ergebnisstand an; Diagnose statt gültig aussehender Null bei Fehler. Historische Ergebnisse ohne Windvertrag sind explizit nicht verfügbar. |
| C23-10 Regression | Nativer Core-Build und komplette CTest-Suite; bestehende Statik, Dynamik, Testantrieb, Fremdkraft, Modellimport und Ergebnisverbraucher bleiben unverändert. Keine Änderung von Modellquelldaten oder GUI. |
| C23-11 Sammlung | Mastspitze, Schwerpunkt, einzelne Auftriebs- und weitere Kraftangriffspunkte, LDP und Fremdkraft-/Ersatz-SDP korrekt identifizieren und Herkunft/Status mit bisherigen Ergebnissen vergleichen. Mehrere Beiträge derselben Art unterscheiden; inaktive Fremdkraft, fehlende Punkte und unbekannte Angriffspunkte nicht als Nullpunkt ausgeben. |
| C23-12 Frames und Geschwindigkeit | Bekannte Pose, Translation und Rotation: Position in Welt- und Bootsframe und dieselbe weltbezogene Geschwindigkeit in beiden Achsensystemen prüfen. Für einen festen Mastpunkt Rotationsanteil nachweisen; bei null Betrag keine Richtung. Wandernder oder nur rechnerischer Resultierendenpunkt ohne bestimmbaren Bewegungsbezug: Position und Geschwindigkeit unabhängig behandeln, keine starre Geschwindigkeit erfinden. |
| C23-13 Migration und Historie | Neue Sammlung und bisherige Feldsichten eines Zustands stimmen bei IDs, Positionen, Status und Zuordnung überein; alte Ergebnisformate und Verbraucher bleiben lesbar. Fehlende historische Geschwindigkeiten sind nicht verfügbar und werden nicht aus aktuellen Zuständen ersetzt. |

## 4. Meilensteine, Übergabe und Validierung

| Gate | Eintritt | Abnahme |
|---|---|---|
| C1 Vertrag | U-C-000141 | Datenherkunft und konkrete Interfaces geprüft/registriert; für Mastbezug nötige Modelldaten eindeutig oder Blocker dokumentiert. |
| C2 Windfeld | C1; U-C-000142–000143 | C23-01 bis C23-04 bestanden. |
| C3 Lokaler Wind, Punkte und Zeit | C2 und Mastdaten-Gate; U-C-000147, U-C-000144–000145, U-C-000148 | C23-05 bis C23-09 und C23-11 bis C23-13 bestanden, einschließlich Live/Wiedergabe und Fehlerfällen. |
| C4 Core-Übergabe | C3; U-C-000146 | C23-10 und vollständige Rückverfolgung bestanden; GUI kann gegen den tatsächlichen Vertrag integrieren. |

Vom Core-Repository-Stamm bei Umsetzung ausführen:

```powershell
cmake -S Code -B build\standard -DBUILD_TESTING=ON
cmake --build build\standard --config Release
ctest --test-dir build\standard -C Release --output-on-failure
```

Gezielte neue Windtests zuerst ausführen; konkrete Selektoren nach
Testregistrierung dokumentieren. Instrumentierten Build zusätzlich prüfen,
falls Messgating geändert wird. Testergebnisse, konkrete Toleranzen,
Umgebung und offene Abweichungen hier nachtragen; keine geplanten Tests
als bestanden markieren.

## 5. Rückverfolgbarkeit und Status

| Anforderung | Schritte | Tests | Status |
|---|---|---|---|
| A-C-00158 | U-C-000143, U-C-000146 | C23-03, C23-10 | offen |
| A-C-00159 | U-C-000143, U-C-000145, U-C-000146 | C23-03, C23-04, C23-08, C23-10 | offen |
| A-C-00160 | U-C-000142, U-C-000146 | C23-01, C23-02, C23-10 | offen |
| A-C-00161 | U-C-000144, U-C-000146 | C23-05, C23-06, C23-10 | offen |
| A-C-00162 | U-C-000143, U-C-000144, U-C-000146 | C23-04, C23-07, C23-10 | offen |
| A-C-00163 | U-C-000141, U-C-000145, U-C-000146 | C23-08 bis C23-10 | offen |
| A-C-00164 | U-C-000141, U-C-000142, U-C-000144 bis U-C-000146 | C23-02, C23-06, C23-09, C23-10 | offen |
| A-C-00165 | U-C-000141, U-C-000144, U-C-000146 | C23-06, C23-10 | offen |
| A-C-00166 | U-C-000141, U-C-000147, U-C-000148, U-C-000146 | C23-11, C23-13, C23-10 | offen |
| A-C-00167 | U-C-000141, U-C-000147, U-C-000144, U-C-000146 | C23-05, C23-12, C23-10 | offen |
| A-C-00168 | U-C-000141, U-C-000148, U-C-000146 | C23-09, C23-13, C23-10 | offen |

**Tatsächliche Nachweise:** Die Hilfsfunktion CPPI-0154 berechnet aus dem
vom Aufrufer vorgegebenen Element die geometrische Mastspitze; CPPI-0153
meldet Ergebnis oder Diagnose. Der Integrationstest
`modellformat_03_source_integration` prüft die drei aktuellen Segelmodelle
und das Fehlen eines Mastes beim Quader; die automatische Auswahl wurde
für diese Modelle ebenfalls geprüft. Das ist nur ein Teil von C23-06:
die vollständige Abnahme aller Punkt- und Historienfälle bleibt offen.
Ein isoliertes 420-Testmodell prüft zusätzlich mehrere Masten ohne eindeutigen
Hauptmast, einen eindeutig benannten Hauptmast sowie mehrere Hauptmasten;
`modellformat_03_tests.exe` bestand diese Fälle am 13.11.2026.
Ein separater nativer Release-Build mit lokal vorhandenen
Abhängigkeiten und alle 30 CTests bestanden auch nach der Änderung am
Mastkontext, darunter `wind_contract` und die vier Modellformat-Tests.
Der erneute vollständige Release-Build und CTest-Lauf am 13.11.2026
bestand nach Ergänzung der Mehrmast- und Windgrenzentests ebenfalls
mit 30 von 30 Tests.
Nach dem ergänzten Mehrmasttest wurde `modellformat_03_fingerprints` im
Core-Build erneut erfolgreich ausgeführt.
`wind_contract` bestand nach ergänzten Fällen für mehrere unterscheidbare
Auftriebspunkte, unbekannten Angriffspunkt sowie inaktiven und aktiven
Ersatz-SDP ohne erfundene Geschwindigkeit erneut.
Ergänzte Grenzen für nicht endliche Eingaben und eine nicht bestimmbare
horizontale Bootsausrichtung bestanden ebenfalls in `wind_contract`.
Der Test prüft zudem `0°/360°` sowie beide Windsichtbarkeiten bei genau
`0,1 m/s` und knapp darüber.
Die Windtests decken Profil, Richtungswechsel, Punktgeschwindigkeit
und historische JSON-Werte teilweise ab. Die Sammlung kann seit der jüngsten
Änderung eine explizit übergebene Mastreferenz auch ohne Windkontext ausgeben;
`wind_contract` und `simulation_gesamtlauf` prüfen Herkunft, Rotation,
Fassungsprüfung und Erhalt über einen Dynamikschritt. Die automatische
Übernahme einer expliziten Spitze aus einem Modell bleibt offen: Im aktuellen
Modellschema wurde kein entsprechendes Feld gefunden; ein explizit übergebener
Mastpunkt wird aber mit seiner Herkunft erhalten. Sämtliche Anforderungen
und Gates bleiben bis zum vollständigen Nachweis offen. Persönliche
fachliche Abnahme bleibt ausstehend.

**Testmigration auf Modellformat 0.3:** `modellaufbereitung`,
`hydrodynamik_referenzen`, `hydrodynamik_referenzmodelle` und
`kraft_momentenbilanz` bestanden nach Umstellung auf aktuelle Modelle.
Die Hydrodynamikprüfung behält dabei die analytischen Fälle und die
Referenzmodell- und Ruderprüfungen. `temporaerer_testantrieb` bestand mit
dem aktuellen Quader und 420 in einem isolierten Ausgabeverzeichnis;
die CTest-Registrierung verwendet diese Modelle. Der frühere 30/30-Lauf
ist kein Nachweis für die noch unvollständige Migration. Weitere Tests
verwenden weiterhin Altformatdateien. Insbesondere können die
analytischen Importreferenzen für Hydrodynamik-Polaren und
Trägheitsblöcke nicht unverändert in das 0.3-Schema übertragen werden,
das diese Datenfelder derzeit nicht vorsieht. Physikalische Abdeckung und
gleichwertige Import-/Batchnachweise sind vor Abschluss zu klären und
nachzuweisen. `Modelle_alt` war bei der Prüfung am bisherigen Ort nicht
vorhanden; eine Löschung durch diesen Arbeitsschritt erfolgte nicht.
