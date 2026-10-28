# Umsetzungsplan C-0.7: Dreidimensionale Hydrodynamik

**Status: in Umsetzung; Build und 16/16 Core-CTest-Prüfungen bestanden.
Technische Gesamt- und fachliche Abnahme sind noch offen.**

**Stand: 28.10.2026.** C-0.7 ist der geplante Folgearbeitsschritt nach
C-0.6. Die Versionsbezeichnung ordnet diesen Plan im CORE-Arbeitsverlauf
ein; sie erstellt oder autorisiert keinen Git-Tag oder Release.

Dieser Plan ist der einzige maßgebliche Umsetzungsplan für den Versionsschritt
C-0.7. Er ist in vier Teile gegliedert: **Teil A** (Abschnitte 1 bis 7)
Dreidimensionale Hydrodynamik, **Teil B** bewegliche Ruder und Schwerter
(früher als „C-0.8“ geführt), **Teil C** Zusatzmasse und Dämpfung (früher
„C-0.9“) und **Teil D** Lateraldruckpunkt. Die Versionsbezeichnungen
„C-0.8“ und „C-0.9“ sind aufgehoben; es gilt durchgehend C-0.7. Die
Kennungen A-C und U-C bleiben unverändert.
Die Aussagen zum Umfang in den Abschnitten 1 bis 7 gelten für Teil A.
Maßgebliche fachliche Grundlage von Teil A ist die
[Hydrodynamik-Spezifikation](../Spezifikationen/Spezifikation_Hydrodynamik.md),
insbesondere Abschnitt 12.
Die zugehörigen Herkunftsentscheidungen und Restpunkte stehen in der
[Entwurfspunkteliste Hydrodynamik](../Spezifikationen/Entwurfspunkte/Hydrodynamik.json).
Der C-0.6-Plan bleibt als historischer Plan für seine eigene Version
unverändert.

C-0.7 umfasst ausschließlich Core-Hydrodynamik. Keine GUI-Physik,
GUI-Eingaben oder spezielle GUI-Kraftdarstellung sind vorgesehen. Das
Ruder bleibt starr am Rumpf; Rudersteuerung, freie Oberfläche mit
Wasseraufprall sowie Wellenbildung und Wellenwiderstand sind ausgeschlossen.
Dieser Plan erteilt keinen Implementierungs-, Commit-, Push-, Tag- oder
Release-Auftrag.

## 1. Anforderungen und Rückverfolgbarkeit

Die fachlichen Anforderungen A-C-00073 bis A-C-00078 stehen ausschließlich
in der Hydrodynamik-Spezifikation. Ihre Herkunftszuordnung ist:

| IDEE-ID | Anforderung | Fachlicher Gegenstand |
|---|---|---|
| IDEE-C-00044 | A-C-00073 | Körperumfang und Bauteilrollen |
| IDEE-C-00045 | A-C-00074 | 3D-Zustand, lokale Anströmung und benetztes Mesh |
| IDEE-C-00046 | A-C-00075 | Kraftmodelle, Kennfelder und Gültigkeitsregeln |
| IDEE-C-00047 | A-C-00076 | Objektparameter, Provenienz und Materialreferenz |
| IDEE-C-00048 | A-C-00077 | Isolierte Tests und fachliche Referenzen |
| IDEE-C-00049 | A-C-00078 | Dynamikintegration, Endgeschwindigkeit und Abdrift |

Die Umsetzungsschritte U-C-000054 bis U-C-000059 sind die nächsten
projektweit freien Kennungen nach U-C-000053. Jeder Schritt verweist auf
mindestens eine aktive Anforderung. Die Statuswerte werden erst nach
tatsächlicher Umsetzung und belegtem Nachweis fortgeschrieben. Ein
technisch bestandener Test allein gilt nicht als fachliche Validierung.

## 2. Voraussetzungen und Grenzen

- C-0.6 bleibt die technische Ausgangsbasis. Vor Implementierungsbeginn
  Build, CTest, benutzte API und Ergebnisvertrag feststellen; vorhandene
  Änderungen oder abweichende Ergebnisse dokumentieren, nicht verwerfen.
- Die acht einfachen Referenzkörper werden als Rümpfe behandelt.
  Optimist und 420er umfassen jeweils Rumpf, Schwert und Ruderblatt;
  das Folkeboot umfasst Rumpf, Kiel, separates Bulb-Teilobjekt mit
  eigenem Mesh und Ruderblatt. Die konkret verfügbaren Modell- und
  Geometriedateien sind zu inventarisieren; Anzeigegeometrie ist keine
  Physikquelle.
- Hydrostatik bleibt separat und unverändert. Hydrodynamik wird additiv in
  die bestehende Starrkörperdynamik integriert.
- Objektabhängige Parameter und Kennfelder werden am Modellelement unter
  `Hydrodynamik` geführt. Laufweite Fluiddaten bleiben im Laufkontext.
  Materialgebundene Stoffwerte dürfen referenziert werden; der
  Materialkatalog muss vor Core-Nutzung einen validierten Modellvertrag
  erhalten. Modell- und Materialvertrag werden in bestehende
  Spezifikationen integriert, keine parallele Spezifikation anlegen.
- Werte können nutzerseitig vorgegeben oder geschätzt sein, müssen dann
  aber als `geschaetzt` gekennzeichnet bleiben. Wenn belastbare
  Referenzwerte fehlen, werden geeignete Näherungen aus verfügbaren
  Quellen, physikalischen Beziehungen und dokumentierten Annahmen
  entwickelt. Sie dürfen nicht als Messwerte oder validierte Werte
  ausgegeben werden.
- Kennfelder werden innerhalb der belegten Domäne stückweise linear bzw.
  bei zwei Tabellenachsen bilinear interpoliert. Außerhalb wird nicht
  extrapoliert; der Beitrag ist sichtbar `nicht_berechnet` und die Bilanz
  unvollständig.
- Fehlende unabhängige Referenzdaten und quantitative Toleranzen blockieren
  nicht die technische Umsetzung oder technische Abnahme. Sie begrenzen
  die Aussagekraft über quantitative Genauigkeit und müssen als offene
  Validierungslücke dokumentiert werden. Fehlende Kraftbeiträge dürfen
  weiterhin nicht als Nullbeiträge behandelt werden.

## 3. Geordnete Umsetzungsschritte

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit | Nachweis und Abnahmekriterium |
|---|---|---|---|---|
| U-C-000054 | A-C-00073, A-C-00074 | Modell-, Komponenten- und Meshinventar; Rollen, Koordinaten, physische Geometrie, Benetzungs- und Angriffspunktverfahren sowie nichtüberlappende Kiel-/Bulb-Aufteilung festlegen | Keine | Vollständiges Inventar für einfache Körper, Optimist, 420er und Folkeboot; Meshherkunft, Qualitätsgrenzen und offene Geometrieprobleme dokumentiert; Rotation, Frames und Integratorstufen mit unabhängigen kinematischen/geometrischen Fällen geprüft |
| U-C-000055 | A-C-00076 | Zielvertrag für `Hydrodynamik`, Parameter- und Kennfelddatensätze, Provenienz, strikte Diagnosen und optionale Materialreferenz in die bestehende Modellbeschreibungsspezifikation überführen | U-C-000054 | Modellbeispiele sowie gültige/ungültige JSON-Testfälle; Einheiten, Status, Quellen, Unsicherheit, Gültigkeitsdomänen und Materialreferenzen werden ohne stille Defaults geprüft |
| U-C-000056 | A-C-00075, A-C-00076, A-C-00077 | Für Hautreibung, richtungsabhängigen Rumpf-Druckwiderstand sowie Seitenkraft/Widerstand von Kiel/Schwert, Bulb und Ruder geeignete Methoden, Datensätze und Domänen auswählen und rückverfolgbar ablegen; bei Datenlücken begründete Schätzwerte entwickeln | U-C-000054, U-C-000055 | Gleichungen, Bezugsflächen/-längen, Vorzeichen, Kennfeldanteile, Quellen oder Herleitungen, Annahmen, Anwendungsgrenzen und Unsicherheiten dokumentiert; jeder Schätzwert als `geschaetzt` markiert; Doppelzählung ausgeschlossen. Externe Validierungslücken blockieren den Schritt nicht |
| U-C-000057 | A-C-00074, A-C-00075, A-C-00077 | Benetzte Meshflächen, lokale Relativströmung, Hautreibung, Druckwiderstand und Seitenkraft als getrennte Core-Beiträge berechnen und isoliert testen | U-C-000054, U-C-000055, U-C-000056 | Tests für X/Y/Z, Translation/Rotation, Nullfluss, Meshschnitt, Vorzeichen, Angriffspunkt/Moment, Kennfeldinterpolation, Domänengrenzen, `nicht_berechnet`, Bilanzlücke und Widerstandsanteile bestehen |
| U-C-000058 | A-C-00074, A-C-00078 | Gültige hydrodynamische Einzelbeiträge je Integratorstufe additiv in die bestehende Dynamik einbinden; Kraft-/Momentenbilanz, Diagnosen und Batch-/Live-Parität sicherstellen | U-C-000057 | Integratorstufen verwenden denselben Stufenzustand; gültige Beiträge werden summiert, fehlende Beiträge bleiben als Bilanzlücke sichtbar; Modelle ohne Hydrodynamik und bestehende API-/Ergebnisverträge behalten ihr Verhalten |
| U-C-000059 | A-C-00073 bis A-C-00078 | Dynamische Bewegungsfälle, technische Wirkungsprüfung, soweit möglich Referenzabgleich, Abschlussmatrix und Dokumentation durchführen | U-C-000058 | Freies Sinken und Querfahrt für einfache Körper, Jollen und Folkeboot geprüft; Endgeschwindigkeit entsteht aus Kräftegleichgewicht ohne Begrenzer; Abdrifthemmung durch die modellierten Seitenkräfte. Geschätzte Werte und fehlende externe Validierung sind am Ergebnis sichtbar; quantitative Genauigkeit wird nur bei unabhängigen Referenzen und erfüllten Toleranzen behauptet |

U-C-000056 ist mit dokumentierten, fachlich begründeten Schätzwerten
abschließbar, wenn belastbare Quellen fehlen; Status und Datenlücken bleiben
sichtbar. U-C-000059 ist ohne externe Referenzen technisch abschließbar,
wenn die Bewegungs- und Bilanztests bestanden sind. In diesem Fall bleibt
die quantitative fachliche Validierung ausdrücklich offen und darf nicht
als bestanden ausgewiesen werden.

## 4. Meilensteine

| Meilenstein | Eintrittskriterium | Abnahmekriterium |
|---|---|---|
| M1 – Geometrie- und Zustandsbasis | U-C-000054 abgeschlossen | Körper-/Meshinventar, Rollen, Frames, Rotationszustand und Benetzungsstrategie sind nachvollziehbar und getestet |
| M2 – Daten- und Methodenvertrag | U-C-000055 und U-C-000056 abgeschlossen; bei Datenlücken liegen begründete Schätzwerte vor | Modellvertrag ist strikt validiert; jede verwendete Methode und jeder Datensatz hat Quelle oder Herleitung, Status, Annahmen und Gültigkeitsdomäne |
| M3 – Isolierte Kraftmodelle | U-C-000057 abgeschlossen | Kraftarten und Kennfeldgrenzen einzeln getestet; keine Doppelzählung und keine stillen Nullbeiträge |
| M4 – Dynamikintegration | U-C-000058 abgeschlossen | Additive Kraft-/Momentenbilanz, Integratorstufen, Diagnosen und Batch-/Live-Parität nachgewiesen |
| M5 – Bewegungs- und Wirkungsprüfung | U-C-000059 abgeschlossen | Sink- und Querfahrtfälle bestehen die technischen Kraft-, Bilanz- und Wirkungsprüfungen; Schätzwerte und fehlende externe Validierung sind ausgewiesen |
| M6 – C-0.7 Abschluss | Alle aktiven Anforderungen U-Schritten und Nachweisen zugeordnet | Core-Build, CTest, Rückverfolgbarkeit und getrennte technische/Validierungs-Statusmatrix vollständig; Grenzen und offene Punkte ausdrücklich dokumentiert |

## 5. Test- und Integrationsregeln

- Mit jeder Stufe gezielte Tests ergänzen; abschließende Bewegungsfälle
  ersetzen keine isolierten Tests der Kraftmodelle.
- Prüfen: analytische oder unabhängige Referenzwerte, Null- und Grenzfälle,
  Kraftrichtung, Frame-Transformation, benetzte Fläche, Angriffspunkt,
  Moment, Summenparität, Interpolation, Gültigkeitsdomäne und
  Bilanzvollständigkeit.
- Für Dynamikfälle müssen Modellfassung, Hydrodynamikdaten, Fluidkontext und
  Schrittfolge reproduzierbare Ergebnisse innerhalb der für technische
  Regressionen festgelegten Toleranzen liefern. Diese Toleranzen sind von
  empirischen Genauigkeitstoleranzen gegenüber realen Bootsdaten zu trennen.
- Bestehendes Verhalten für Modelle ohne Hydrodynamik, C-0.6-APIs und
  Ergebnisverträge regressiv prüfen. Keine inkompatible Änderung von
  Pflichtfeldern oder deren Bedeutung ohne passende Versionierung.
- Die Umsetzung erfolgt im Core. GUI-Änderungen und spezielle GUI-
  Darstellungen sind nicht Teil dieses Plans.
- Für den nativen Core-Pfad Build und CTest verwenden. Bei einer späteren
  Commit-Freigabe gelten zusätzlich die projektweiten Commit-Prüfregeln.
- Testergebnisse, technische Integration und fachliche Nutzerabnahme
  getrennt dokumentieren. Unbelegte oder geschätzte Parameter sind kein
  Nachweis fachlicher Genauigkeit.

## 6. Abschluss- und Validierungsstatus

C-0.7 ist technisch abgeschlossen, wenn alle Anforderungen implementiert,
die dazugehörigen Core-Tests bestanden und Diagnosen sowie Datenherkunft
nachvollziehbar dokumentiert sind. Für fehlende Referenzdaten werden
begründete Schätzwerte verwendet und als solche gekennzeichnet; dies
blockiert weder technische Implementierung noch technischen Abschluss.
Quantitative fachliche Validierung bleibt ein eigener Status und kann
nachträglich mit unabhängigen Referenzfällen und Toleranzen ergänzt werden.

Keine Spezifikation, kein Modellbeispiel und kein Test darf einen geschätzten
Wert als validiert ausgeben. Ein nicht berechneter Beitrag ist ausdrücklich
als solcher zu serialisieren und in der Bilanz zu kennzeichnen. Planstatus,
Umsetzungsstatus, Testergebnisse und fachliche Abnahme sind getrennt zu
führen. Änderungen an fremden Arbeitsbäumen oder Modell-Repositories,
Commit, Push, Tag und Release sind nicht durch diesen Plan freigegeben.

## 7. Tatsächlicher Umsetzungsstand

| U-ID | Stand | Nachweis bzw. offener Punkt |
|---|---|---|
| U-C-000054 | teilweise umgesetzt | Meshpanels, Wasserlinienschnitt, lokale Rotation und alle acht einfachen Meshmodelle werden in Core-Tests geprüft; der Core-Importer ordnet die acht kanonischen Modell-IDs als Rumpf zu. Beim Folkeboot ist kein separates Bulb-Element mit eigener Hydrodynamikrolle/Geometrie vorhanden; Rollen-/Meshinventar ist daher nicht vollständig. |
| U-C-000055 | teilweise umgesetzt | Strikter Import für Rollen, Modellparameter, Kennfeldmetadaten, vollständige Kennfeldrechtecke und Widerstandsanteile ist implementiert und getestet. `materialreferenz` wird als Text importiert, aber mangels freigegebenem Materialkatalogvertrag nicht aufgelöst. |
| U-C-000056 | technisch umgesetzt, extern nicht validiert | Geschätzte Reibungs-, Druck- und Seitenkraftmodelle samt Gültigkeitsgrenzen und Provenienz sind dokumentiert und im Ergebnis ausgewiesen. Für konkrete Bootsmeshes fehlen weiterhin unabhängige Messdaten und quantitative Genauigkeitstoleranzen. |
| U-C-000057 | technisch umgesetzt | Core-Tests decken X/Y/Z, lokale Rotation, Wasserlinienschnitt, Formwiderstand, Seitenkraft, Kennfeldinterpolation/-grenzen, ungültige Daten und Bilanzlücken ab. |
| U-C-000058 | teilweise umgesetzt | Hydrodynamik ist in die dynamischen Core-Integratorstufen integriert; Beiträge, Momente, Provenienz und Parameter werden serialisiert. Batch-Referenzläufe für die Bootsmodelle bestehen; getrennte Live-/Batch-Paritätsabnahme ist noch offen. |
| U-C-000059 | teilweise umgesetzt | Synthetische Fälle zeigen eine sich einstellende Sinkgeschwindigkeit und Abdrifthemmung; einfache Geometrien und drei Bootsmodelle werden geprüft. Vollständige Bewegungsabnahme aller Bootsbauteile ist wegen fehlender Bulb-/Rollenmodellierung offen. Externe quantitative Validierung bleibt ebenfalls offen. |

Der letzte ausgeführte Standardlauf bestand aus dem nativen Release-Build und
allen 16 Core-CTest-Prüfungen. Diese Ergebnisse belegen technische
Regressionen für den aktuellen Core-Arbeitsstand, nicht die Vollständigkeit
der Referenzmodelle oder eine fachliche Vorhersagegenauigkeit. Es wurden
weder GUI- noch externe Modell-Repositories geändert.

# Teil B: Bewegliche Ruder und Schwerter (ehemals Plan C-0.8)

Teil B ist aus dem früheren Plan „C-0.8“ übernommen. Der Versionsschritt
ist C-0.7 (es gab keinen eigenen Tag für 0.8); die Kennungen
U-C-000060 bis U-C-000066 bleiben unverändert. Maßgebliche fachliche
Grundlage ist Abschnitt 13 der
[Core-Hydrodynamik-Spezifikation](../Spezifikationen/Spezifikation_Hydrodynamik.md#13-dynamische-verstellung-von-ruder-und-schwert);
Herkunft: [Entwurfspunkteliste Hydrodynamik](../Spezifikationen/Entwurfspunkte/Hydrodynamik.json).
Teil B enthält ausschließlich Core-Arbeiten; die GUI-Seite steht im
[GUI-Plan G-0.7](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.7_Ruder_Schwertsteuerung.md).

## B.1 Umfang und Rückverfolgbarkeit

Der Plan setzt die Core-Bewegungsdefinition, Stellzustände und deren
hydrodynamische Verwendung um. Bewegte Elemente ändern weder
Massenverteilung noch Schwerpunkt, Trägheit, Auftrieb oder Hydrostatik.
Hub-Kiele, Aktuatordynamik, Bauteilinterferenz, Wasserzutritt durch
Rumpfdurchbrüche und Strömungsnachläufe sind ausgeschlossen.

| IDEE-ID | Core-Anforderung | Gegenstand |
|---|---|---|
| IDEE-C-00050 | A-C-00079 | JSON-Bewegungsvertrag und Importvalidierung |
| IDEE-C-00050 | A-C-00080 | Modellachsen, Drehpunkte und projektweite Stellkonvention |
| IDEE-C-00050 | A-C-00082 | Zeitgeordnete Eingaben und Hydrodynamikpose |
| IDEE-C-00050 | A-C-00084 | Invariante Masse, Schwerpunkt, Trägheit und Hydrostatik |
| IDEE-C-00050 | A-C-00085 | Core-Bewegungstests, Integrations- und Regressionstests |

A-C-00081 und A-C-00083 sind GUI-seitige Liefergegenstände (A-G-00046 bis
A-G-00051 im GUI-Plan).

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit |
|---|---|---|---|
| U-C-000060 | A-C-00080, A-C-00085 | Bestehende Modelle, Meshachsen, Ruderstock-/Schwertlager und relevante Dreh-/Hubräume inventarisieren; Pivot-Startwerte und Vorzeichen gegen Modellframe prüfen, Abweichungen dokumentieren | Keine |
| U-C-000061 | A-C-00079, A-C-00080 | Rückwärtskompatiblen JSON-Bewegungsvertrag, Einheiten, gültige Bewegungsarten, Vektor-/Pivotvalidierung und robuste Hubwegableitung in die bestehende Modellbeschreibung integrieren | U-C-000060 |
| U-C-000062 | A-C-00079, A-C-00080, A-C-00085 | Modellfeste Pose-/Transformationslogik für Drehen und Heben umsetzen; projektweite Ruderwinkel- und Schwertstellungsabbildung samt Endstellungen isoliert testen | U-C-000061 |
| U-C-000063 | A-C-00082, A-C-00085 | Typisierte Stellereignisse, Simulationszeitbezug, stabile Ereignisfolge, Bestätigung und reproduzierbare Zustands-/Ergebnisbindung implementieren | U-C-000061, U-C-000062 |
| U-C-000064 | A-C-00082, A-C-00084 | Core-Bewegungszustand in allen Integratorstufen verwenden; benetzte bewegte Simulationsgeometrie und Hydrodynamikbeiträge aktualisieren, Massen- und Hydrostatikwerte invariabel halten | U-C-000062, U-C-000063 |
| U-C-000065 | A-C-00079 bis A-C-00085 | Referenzmodelle, Fehlerszenarien, dynamische Ereignisfolgen und Live-/Batch-Parität integrieren; Ergebnis-/Diagnoseverträge regressionsprüfen | U-C-000064 |
| U-C-000066 | A-C-00079 bis A-C-00085 | Rückverfolgbarkeitsmatrix, technische Nachweise, Modell-/Pivotgrenzen und offene Validierungslücken vervollständigen | U-C-000065 |

Umsetzungsschritte werden nur nach tatsächlicher Bearbeitung als
abgeschlossen markiert; Geometrieinventar, Import, Transformation,
Ereignisfolge, Dynamikintegration und Abschlussnachweise sind getrennte
Prüfgegenstände.

## B.2 Voraussetzungen und Grenzen

- Der Arbeitsbaum enthält fremde Änderungen; sie dürfen nicht überschrieben,
  zurückgesetzt oder pauschal gestagt werden.
- Die Pivotkoordinaten sind meshbasierte Startnäherungen, keine vermessenen
  Lagerdaten, und müssen mit Darstellungsmesh und Skizzen plausibilisiert
  werden. Anpassungen erfolgen in den Modell-JSONs.
- Achsen im modellfesten Frame: +X zum Bug, +Y nach Backbord, +Z nach oben.
  Optimist-Schwert: Heben +Z. 420er-Schwert: Drehung +Y, 0-%-Stellung 90°
  nach achtern gegenüber der 100-%-Meshlage. Ruder: Drehung um +Z, Grenzen
  -90° bis +90°.
- Altmodelle ohne Bewegungsblock bleiben unverändert; eine Bewegung wird nie
  aus Namen oder Meshs erraten. Ein ungültiger Bewegungsblock wird
  ausdrücklich diagnostiziert.
- Die Stellposition ist für alle Berechnungen eines Integratorzeitschritts
  konsistent; die Ereignisreihenfolge bei gleicher Simulationszeit ist
  stabil und reproduzierbar.
- Verstellungen verändern keine Masse, Schwerpunkt, Trägheit oder
  Hydrostatik. Das Hydrodynamik-Simulationsmesh darf von der Darstellung
  getrennt sein; Rumpfdurchbrüche werden nur dort geschlossen.
- Nachläufe, Abschirmung, Kollisionen, Wasseraufprall und Betätigungsdynamik
  sind keine stillen Ersatzmodelle. Fehlende Messdaten begrenzen die
  fachliche Genauigkeit, blockieren aber technische Tests nicht.

## B.3 Abhängigkeiten, Meilensteine und Prüfstrategie

| Stufe | Eingang | Ausgang / Freigabekriterium |
|---|---|---|
| A. Baseline und Geometrie | Core-Build/API, Referenzmodelle | Achsen, Meshfassungen, plausible Drehpunkte und Hubendstellungen verifiziert |
| B. Datenvertrag | Stufe A | Strikter, rückwärtskompatibler Import; Hubweg reproduzierbar |
| C. Kinematik | Stufe B | Testbare Dreh-/Hubtransformation ohne Masse-/Hydrostatikmutation |
| D. Eingabe und Laufzustand | Stufe C | Stellereignisse deterministisch angenommen, bestätigt und reproduzierbar |
| E. Hydrodynamikkopplung | Stufe D | Konsistente bewegte Geometrie in jedem Integratorsample |
| F. Integration und Übergabe | Stufe E | Regressionen, Live-/Batch-Parität, dokumentierte GUI-Schnittstelle |

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| M7 – Geometrie und Baseline (B) | U-C-000060 begonnen | Baseline und Modellfassungen bekannt; Pivots/Achsen als plausibel, zu korrigieren oder offen markiert |
| M8 – Modellvertrag und Kinematik (B) | M7 | Strikte Validierung; Altmodelle laden; Transformationen und Endpunkte analytisch getestet |
| M9 – Dynamische Stellereignisse (B) | M8 | Ereignisannahme, Zeitbezug, Reihenfolge und Bestätigung reproduzierbar |
| M10 – Hydrodynamik und Invarianten (B) | M9 | Dynamische Pose ändert erwartete Hydrodynamik; Masse, Schwerpunkt, Trägheit, Hydrostatik unverändert |
| M11 – Integration und Übergabe (B) | M10 | Build, CTest, Live-/Batch-Parität; GUI-Vertrag dokumentiert |

Prüfstrategie: Import/Vertrag (gültig, ungültig, Altmodell); Kinematik (-90°,
0°, +90° Ruder; 0 %, Zwischenwert, 100 % Schwert; Drehung um Pivot);
Ereignisfolge (gleiche und verschiedene Zeitpunkte, Reset); Dynamik und
Hydrodynamik (alle Integratorstufen, Angriffspunkt, Moment, Bilanz);
Invarianten und Regression; nativer Release-Build und CTest.

## B.4 Rückverfolgbarkeit und tatsächlicher Stand

| A-ID | Core-Arbeitspaket(e) | Nachweis | Status |
|---|---|---|---|
| A-C-00079 | U-C-000061–000063, U-C-000065–000066 | JSON-Import, Referenzmodelle und gezielte Validierungstests; weitere Fehler-/Altmodellfälle offen | teilweise |
| A-C-00080 | U-C-000060–000062, U-C-000065–000066 | Achsen, Transformationen und Hubweg technisch umgesetzt; Pivot-Näherungen noch nicht visuell/fachlich abgenommen | teilweise |
| A-C-00082 | U-C-000063–000065 | Ereignisannahme und V4-Zustand vorhanden; Ereignisreihenfolge, Replay und Live-/Batch-Parität noch nicht vollständig geprüft | teilweise |
| A-C-00084 | U-C-000064–000066 | Bewegungsabhängige Hydrodynamik; Ruderseitenkraft und Giermoment im lokalen Profilrahmen mit synthetischem Ruder und 420er-Rudermesh auf Vorzeichen geprüft. Umfassende Hydrostatik-/Integratorregression und quantitative Validierung offen | teilweise |
| A-C-00085 | U-C-000060–000066 | Core-Release-Build und Core-Tests bestanden; integrierter GUI/Core-CTest-Lauf bestanden (29/29, nach C-0.9-Umfang 30/30). Fachliche Pivot-/Modellabnahme und vollständige Rückverfolgbarkeitsmatrix offen | teilweise |

| U-ID | Tatsächlicher Stand | Nachweis / offener Punkt |
|---|---|---|
| U-C-000060 | teilweise umgesetzt | Achsen/Pivots in Optimist und 420 JSON ergänzt; Pivotlagen sind Näherungen und warten auf Sicht-/Geometrieprüfung. Folkeboot: Ruder-`Bewegung` (Drehachse [0,0,1], Drehpunkt [-3.36,0,-0.47], aus der Geometrie geschätzt) ergänzt, Kiel starr; GUI-Probelauf offen |
| U-C-000061 | technisch umgesetzt | Bewegungsdefinitionen, Importvalidierung und Hubwegableitung; Core-Build und Referenzmodelltests bestanden |
| U-C-000062 | teilweise umgesetzt | Dreh- und Hubtransformationen einschließlich Ruder-/Schwertrotation und Hubtranslation getestet; vollständige Endlagen-/Pivotabnahme offen |
| U-C-000063 | teilweise umgesetzt | Stellereignisse, Bestätigung und Ergebnisvertrag V4 umgesetzt; zeitgleiche Reihenfolge, Wiederholung und Live-/Batch-Parität nicht vollständig nachgewiesen |
| U-C-000064 | teilweise umgesetzt | Bewegte Geometrie wirkt auf Hydrodynamik; Ruderseitenkraft nutzt den mitbewegten lokalen Profilrahmen. Korrektur 28.10.2026: Profilabmessungen werden am unverstellten Mesh bestimmt (ein stark ausgelenktes Ruder galt zuvor als Nicht-Tragfläche und lieferte keine Seitenkraft), Bezugsfläche folgt der lokalen Profilnormalen, oberhalb des Ablösewinkels geht die Kraft in ein Flachplatten-Normalkraftmodell über (cn = (1,1 + 0,018·AR)·sin α, geschätzt, nicht kalibriert). Vorzeichen von Seitenkraft und Giermoment für ±20°, ±60° sowie ±85° (Moment nur bis 60°) am 420er-Ruder getestet; Core-Szenario 420er (100 N bis 1,9 m/s, Kraft weg, Ruder −20°/0°/+20°) zeigt Drehung des Bugs in die erwartete Richtung. Erste Masse-/Hydrostatik-Invariante getestet; Integratorstufen und quantitative Kraft-/Momentenabnahme offen |
| U-C-000065 | teilweise umgesetzt | Referenzmodelle, Import-/Hydrodynamiktests und Regressionen bestanden; integrierte GUI/Core-CTest-Suite bestanden; fachliche Pivotabnahme und vollständige Live-/Batch-Nachweise offen |
| U-C-000066 | teilweise umgesetzt | Core-/Schnittstellendokumentation ergänzt; Core-CTest und integrierter Lauf bestanden. Vollständige Rückverfolgbarkeit, Pivotvalidierung, quantitative Bootsvalidierung und offene Datenlücken noch abzuschließen |

# Teil C: Hydrodynamische Zusatzmasse und Dämpfung (ehemals Plan C-0.9)

Teil C ist aus dem früheren Plan „C-0.9“ übernommen. Der Versionsschritt
ist C-0.7; die Kennungen U-C-000067 bis U-C-000073 bleiben unverändert.
Fachliche Grundlage ist die
[Spezifikation Hydrodynamische Zusatzmasse und Dämpfung](../Spezifikationen/Spezifikation_Hydrodynamik_Zusatzmasse_Daempfung.md)
(A-C-00086 bis A-C-00092); Herkunft:
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Hydrodynamik_Zusatzmasse_Daempfung.json).
Es gibt keinen GUI-Plan; die GUI-Verträglichkeit wird über den integrierten
GUI/Core-Lauf nachgewiesen.

## C.1 Umfang und Rückverfolgbarkeit

| A-ID | Gegenstand |
|---|---|
| A-C-00086 | Bestimmung der Zusatzmassen (Rumpf, Ruder, Schwert) |
| A-C-00087 | Modellvertrag (optionaler Block, Überschreiben, Validierung) |
| A-C-00088 | Bewegungsgleichung mit Zusatzmasse und Munk-Termen |
| A-C-00089 | Dämpfungsmodell (Gieren, Rollen, Stampfen, Tauchen) |
| A-C-00090 | Messung der Ist-Abklingraten, Festlegung ζ und Gierkoeffizienten |
| A-C-00091 | Ergebnisvertrag und Bilanz (additiv) |
| A-C-00092 | Nachweise und Regression |

Ausgeschlossen: Wellenwiderstand, Nachlauf, Interaktion Rumpf/Schwert/Ruder,
freie Oberfläche, Frequenzabhängigkeit, Eintauchabhängigkeit, Kopplungsterme,
translatorische Zusatzdämpfung.

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit |
|---|---|---|---|
| U-C-000067 | A-C-00090, A-C-00092 | Baseline: Core bauen, CTest sichern. Ist-Abklingraten für Rollen, Stampfen, Tauchen und Gieren an Optimist, 420er und Folkeboot messen und dokumentieren | Keine |
| U-C-000068 | A-C-00086, A-C-00092 | Zusatzmassenberechnung (Rumpf per Ellipsoid-/Streifennäherung, Ruder/Schwert per Plattennäherung) mit Diagnose; isoliert gegen Kugel, Ellipsoid, Platte testen | U-C-000067 |
| U-C-000069 | A-C-00087, A-C-00092 | Optionalen, rückwärtskompatiblen Modellblock in Modellbeschreibung und Import integrieren; Validierung, Herkunft `modell`; Altmodelle unverändert | U-C-000068 |
| U-C-000070 | A-C-00088, A-C-00092 | Bewegungsgleichung mit effektiver Masse und Munk-/Coriolis-Termen im Körperrahmen in beiden Integratorstufen; Positiv-Definit-Prüfung; Statik und Testantrieb unverändert | U-C-000068 |
| U-C-000071 | A-C-00089, A-C-00090, A-C-00092 | Dämpfungsbeitrag je Freiheitsgrad; ζ und Gierkoeffizienten aus der Messung festlegen und begründen; Abklingversuche, Energiemonotonie, Stabilität | U-C-000069, U-C-000070 |
| U-C-000072 | A-C-00091, A-C-00092 | Additive Ergebniserweiterung (effektive Masse/Trägheit, Zusatzmassen mit Herkunft, Dämpfungsbeiträge in der Bilanz); Ergebnisvertrags- und Bilanztests; Schnittstellenregister | U-C-000071 |
| U-C-000073 | A-C-00086 bis A-C-00092 | Referenzmodelle, bestehende Ruder-/Schwert-Szenarien, Live-/Batch-Parität; Core-Build, vollständiger CTest, integrierter GUI/Core-Lauf; Rückverfolgbarkeit und offene Punkte | U-C-000072 |

## C.2 Voraussetzungen, Meilensteine und Prüfstrategie

- Die Abklingmessung (U-C-000067) erfolgt vor jeder Änderung an Integrator
  oder Kräften. Alle Koeffizienten sind Schätzungen.
- Ein verändertes Steuerverhalten (z. B. im 420er-Ruderversuch) ist erwartet
  und wird dokumentiert; Testwerte werden nie stillschweigend gelockert.
- Der Ergebnisvertrag wird nur additiv erweitert.

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| M12 – Baseline und Messung (C) | Start Teil C | Build/CTest grün, Messprotokoll der Ist-Abklingraten |
| M13 – Zusatzmassen und Modellvertrag (C) | M12 | Analytische Referenzen und Importtests bestanden |
| M14 – Integrator (C) | M13 | Beschleunigungs-, Energie- und Statiktests; beide Stufen konsistent |
| M15 – Dämpfung (C) | M14 | Koeffizienten festgelegt und begründet; Abklingtests stabil |
| M16 – Ergebnisvertrag und Integration (C) | M15 | CTest vollständig, integrierter GUI/Core-Lauf, Live-/Batch-Parität |

Prüfstrategie: Zusatzmasse (Kugel, Ellipsoid, Platte, Symmetrie); Import
(gültig, ungültig, partiell, Altmodell); Integrator (a = F/(m + A11),
Energiebilanz, Statik); Dämpfung (Abklingversuche, Energiemonotonie);
Ergebnisvertrag (neue und unveränderte Felder, Bilanz); Regression und
Integration (Ruder-/Schwert-Szenarien, Release-Build, CTest, ggf.
`SEGELPHYSIK_TEST_INSTRUMENTATION=ON`, integrierter Lauf). Quantitative
Bootsgenauigkeit wird nicht behauptet.

## C.3 Rückverfolgbarkeit und tatsächlicher Stand

| A-ID | Arbeitspaket(e) | Nachweis | Status |
|---|---|---|---|
| A-C-00086 | U-C-000068, U-C-000073 | Referenztests, Modellwerte | umgesetzt, getestet; Werte geschätzt |
| A-C-00087 | U-C-000069, U-C-000073 | Importtests, Altmodelle | umgesetzt, getestet |
| A-C-00088 | U-C-000070, U-C-000073 | Beschleunigungs-, Energie-, Statiktests | umgesetzt, getestet |
| A-C-00089 | U-C-000071, U-C-000073 | Abklingversuche | umgesetzt, getestet; ζ geschätzt |
| A-C-00090 | U-C-000067, U-C-000071, U-C-000073 | Messprotokoll, Koeffizientenbegründung | umgesetzt; Stampfmessung Optimist/420er unzuverlässig |
| A-C-00091 | U-C-000072, U-C-000073 | Ergebnisvertrags- und Bilanztests | umgesetzt, getestet |
| A-C-00092 | U-C-000067 bis U-C-000073 | Gesamtregression, integrierter Lauf | teilweise |

| U-ID | Tatsächlicher Stand | Nachweis / offener Punkt |
|---|---|---|
| U-C-000067 | erledigt (Messung) | Baseline Codestand `9bb7f04`, Probe `abklingprobe.cpp`, dt=5 ms, 30 s, Start: Rollen 8°, Stampfen 4°, Tauchen 3 cm, Gieren r0=0,5 rad/s. ζ_ist (Rollen/Stampfen/Tauchen) und Gier-Abklingrate: Optimist 0,063 / 0,047 / 0,054 / 0,42 s⁻¹; 420er 0,070 / n.a. (kein Maximum erkennbar, x_end=-0,027) / 0,051 / 0,47 s⁻¹; Folkeboot -0,012 / 0,034 / 0,007 / 0,61 s⁻¹ (Rollen leicht angefacht: Messartefakt oder numerische Anregung, offen). Perioden: Optimist 0,79/0,57/0,47 s, 420er 1,14/–/0,55 s, Folkeboot 2,42/0,82/1,00 s. Core-Baseline CTest 16/16. |
| U-C-000068 | umgesetzt, getestet | `hydrotraegheit.cpp` (Lamb-Doppelkörper-Ellipsoid aus Wasserlinie/Tiefgang/Verdrängung, Steiner-Terme, Plattennäherung für Ruder/Schwert, Diagnose). `traegheit_tests`: Kugel A11=½ρV, Spheroid k1=0,209/k2=0,702, Axialsymmetrie, drei Referenzboote endlich/positiv. Offen: Werte sind Schätzungen, nicht gegen Messdaten validiert; Kopplungsterme fehlen. |
| U-C-000069 | umgesetzt, getestet | Optionaler Block `Hydrodynamische_Traegheit` (modell.cpp), Fehler CORE-MOD-0002, Herkunft `modell`/`geschaetzt`. Tests: gültig, partiell, negativ, unbekannter Schlüssel/Gruppe, Text statt Zahl, Altmodell. |
| U-C-000070 | umgesetzt, getestet | Kirchhoff-Gleichung im Körperrahmen beider Integratorstufen mit Munk-Termen, Positiv-Definit-Prüfung, nur bei aktivem Hydrokontext (Schalter `zusatzmasse_daempfung`). Tests: A=0 ⇒ identisch zu vorher (1e-9), a=F/(m+a33) (Verhältnis 0,5±1e-3). Testantrieb-Begrenzer nutzt unverändert die Körpermasse. |
| U-C-000071 | umgesetzt, getestet | Dämpfungsbeiträge `daempfung:{rollen,stampfen,gieren,tauchen}`; Rückstellung aus Wasserlinienschnitt; B=2ζ√(C(I+A)). Gewählt nach Messung (Zusatz-ζ): Rollen 0,10, Stampfen 0,35, Tauchen 0,30; Gieren linear 0,1 1/s·(Izz+a66), quadratisch 0,1·ρ·T·L⁴/64. Gemessene Gesamt-ζ (20-s-Probe, Optimist / 420er / Folkeboot): Rollen 0,129 / 0,115 / 0,117; Tauchen 0,322 / 0,217 / 0,301; Stampfen 0,049 / n.a. / 0,232; Gier-Abklingrate 0,32 / 0,42 / 0,47 1/s. Offen: Die Stampf-Messung des Optimist (0,049) und des 420ers ist unzuverlässig (Restversatz, Maximumsuche relativ zu null); Stampfziel 0,2–0,4 damit nur für das Folkeboot belegt. Wahl der ζ ist Schätzung. |
| U-C-000072 | umgesetzt, getestet | Additiver Ergebnisblock `hydrodynamische_traegheit` (Zusatzmassen mit Herkunft, Dämpfung, effektive Masse/Trägheit, Rückstellung, Diagnosen), Bilanz erwartet `daempfung:*`. CPPI-0026 bis -0030 im Register. Test auf neue und unveränderte Altfelder. |
| U-C-000073 | teilweise | Core Release 17/17 CTest, integriert (GUI/Core, `C:\spdeps\guibuild`) 30/30. Bestehende Ruder-/Schwert-/Testantriebstests unverändert grün. Nicht erledigt: instrumentierter Build (Messgating nicht berührt, nicht ausgeführt), erneute Szenario-Messung des 420er-Ruderversuchs, fachliche Abnahme durch den Nutzer. |

# Teil D: Lateraldruckpunkt

Fachliche Grundlage ist die
[Spezifikation Lateraldruckpunkt](../Spezifikationen/Spezifikation_Lateraldruckpunkt.md)
(A-C-00093 bis A-C-00097); Herkunft:
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Lateraldruckpunkt.json).
Teil D enthält ausschließlich Core-Arbeiten. Die Darstellung steht im
[GUI-Plan G-0.7](../../GUI/Umsetzungsplaene/Umsetzungsplan_G-0.7_Ruder_Schwertsteuerung.md),
Teil B. Der Segeldruckpunkt ist ausgeschlossen.

## D.1 Umfang und Rückverfolgbarkeit

| IDEE-ID | A-ID | Gegenstand |
|---|---|---|
| IDEE-C-00061 | A-C-00093 | Definition und Quellen |
| IDEE-C-00062 | A-C-00094 | Berechnung |
| IDEE-C-00063 | A-C-00095 | Status und Schwelle |
| IDEE-C-00064 | A-C-00096 | Ergebnisvertrag |
| IDEE-C-00065 | A-C-00097 | Wirkung und Nachweise |

| U-ID | Anforderung(en) | Arbeitspaket und Ergebnis | Abhängigkeit |
|---|---|---|---|
| U-C-000074 | A-C-00093, A-C-00094, A-C-00095 | Auswertung `lateraldruckpunkt` aus den hydrodynamischen Seitenkraftbeiträgen (Modell-y, Angriffspunkt), kraftgewichteter Mittelpunkt, Status mit Schwelle 5 % von Σ|Fy,i| und 1 N; keine Rückwirkung auf Kräfte | Keine (Baseline: Build und CTest sichern) |
| U-C-000075 | A-C-00097 | Isolierte Core-Tests: Kraft nur am Ruder, nur am Schwert, zwei gleichgerichtete Beiträge, Gegenkräfte, Querkraft ≈ 0, Schwellenwerte, Kräfte mit/ohne Auswertung identisch | U-C-000074 |
| U-C-000076 | A-C-00096 | Additiver Ergebnisblock `druckpunkte.lateral` in `zustand_json` (Live und Batch gleich), `vertragsversion` unverändert; Schnittstellenregister (CPPI ab 0031), Schnittstellendokumentation | U-C-000074 |
| U-C-000077 | A-C-00093 bis A-C-00097 | Referenzmodelle Optimist, 420er, Folkeboot bei Querfahrt; Regression, Altfelder, Live-/Batch-Parität; Core-Release-Build, vollständiger CTest, integrierter GUI/Core-Lauf; Rückverfolgbarkeit | U-C-000075, U-C-000076 |

## D.2 Voraussetzungen, Meilensteine und Prüfstrategie

- Der Lateraldruckpunkt ist eine reine Auswertegröße; Kräfte, Momente und
  Zustand bleiben bitgleich.
- Nur Meter im Modellrahmen; keine Prozentangabe im Core.
- Fremde, uncommittete Änderungen werden nicht überschrieben.
- Messgating ist nicht berührt; ein instrumentierter Build ist nur bei
  Eingriffen in das Messgating nötig.

| Meilenstein | Eintritt | Abnahme |
|---|---|---|
| M17 – Auswertung (D) | Start Teil D | Berechnung und Status implementiert; isolierte Tests bestehen |
| M18 – Ergebnisvertrag (D) | M17 | Block in Live und Batch, Altfelder unverändert, CPPI eingetragen |
| M19 – C-0.7 Abschluss | M18, Teile A bis C erledigt | Core-CTest, integrierter GUI/Core-Lauf, Rückverfolgbarkeit; offene Punkte ausgewiesen |

## D.3 Rückverfolgbarkeit und tatsächlicher Stand

| A-ID | Arbeitspaket(e) | Nachweis | Status |
|---|---|---|---|
| A-C-00093 | U-C-000074, U-C-000075, U-C-000077 | Tests zu Quellen | offen |
| A-C-00094 | U-C-000074, U-C-000075, U-C-000077 | Handrechnungsvergleich | offen |
| A-C-00095 | U-C-000074, U-C-000075, U-C-000077 | Statustests | offen |
| A-C-00096 | U-C-000076, U-C-000077 | Vertragstest, Parität, Register | offen |
| A-C-00097 | U-C-000075, U-C-000077 | Gesamttests, integrierter Lauf | offen |

| U-ID | Tatsächlicher Stand | Nachweis / offener Punkt |
|---|---|---|
| U-C-000074 | teilweise umgesetzt | Auswertung `lateraldruckpunkt` in `hydrostatik.cpp`; Abweichung von A-C-00095: zusätzlicher Status `geometrisch` (Flächenschwerpunkt der benetzten Lateralfläche, wenn Seitenkräfte nicht ausreichen, z. B. in Ruhe/statisch) – Nutzerfreigabe der Spezifikationsänderung steht aus |
| U-C-000075 | teilweise umgesetzt | `lateraldruckpunkt_tests` (Quader: Ruhe geometrisch, Querfahrt kraftgewichtet, JSON, Modell ohne Hydrodynamik); Ruder/Schwert-Einzelfälle und Schwellen offen |
| U-C-000076 | umgesetzt (Register offen) | Block `druckpunkte.lateral` in `zustand_json` immer bei Hydrodynamik-Elementen, auch ohne Hydrodynamik-Kontext (statische Simulation); CPPI-0031 siehe Register |
| U-C-000077 | offen | Referenzmodelle bei Querfahrt und Sichtprüfung nicht durchgeführt; Core-CTest 18/18; integrierter Lauf 28/31, die 3 Fehlschläge `gui_graphics_render_*` ("420 disabling course trail must restore the exact image") bestehen unabhängig vom Lateraldruckpunkt |

Statusänderungen erfolgen erst nach tatsächlichem Ergebnis. Build-/Test-
Erfolg, Messung, externe Validierung und Nutzerabnahme werden getrennt
ausgewiesen. Commit, Push, Tag und Release bleiben nicht beauftragt.
