# Umsetzungsplan C-0.7: Dreidimensionale Hydrodynamik

**Status: in Umsetzung; Build und 16/16 Core-CTest-Prüfungen bestanden.
Technische Gesamt- und fachliche Abnahme sind noch offen.**

**Stand: 28.10.2026.** C-0.7 ist der geplante Folgearbeitsschritt nach
C-0.6. Die Versionsbezeichnung ordnet diesen Plan im CORE-Arbeitsverlauf
ein; sie erstellt oder autorisiert keinen Git-Tag oder Release.

Dieser Plan ist der einzige maßgebliche Umsetzungsplan für den
Hydrodynamik-Folgeumfang C-0.7. Maßgebliche fachliche Grundlage ist die
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
