# Umsetzungsplan C-0.6: Hydrodynamik im CORE

**Status: freigegeben und in Umsetzung; keine Git-Veröffentlichungsfreigabe.**

**Stand: 27.10.2026.** C-0.6 ist der nächste CORE-Arbeitsschritt nach C-0.5.

Dieser Plan beschreibt die schrittweise Umsetzung der
[CORE-Spezifikation Hydrodynamik](../Spezifikationen/Spezifikation_Hydrodynamik.md).
Er ist der einzige maßgebliche Umsetzungsplan für C-0.6. Die fachlichen
Entscheidungsherkünfte und technischen Restpunkte stehen in der
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Hydrodynamik.json).
Die Anforderungen A-C-00062 bis A-C-00072 sowie die Schritte U-C-000045 bis
U-C-000053 folgen projektweit auf A-C-00061 und U-C-000044. A-C-00061 ist die
Modellvertragsanpassung in der
[Modellbeschreibungsspezifikation](../Spezifikationen/Spezifikation_Modellbeschreibung.md).

Es gibt keine GUI-spezifische Physik oder neue Darstellung in diesem Plan.
Eine kleine Anpassung des bestehenden GUI-Core-Adapters ist erforderlich,
damit Live-Läufe denselben Hydrodynamikkontext wie dynamische Core-Läufe
verwenden. Die bestehende generische Kraftanzeige bleibt unverändert. Das
Ruder bleibt relativ zum Rumpf fest; Ruderbewegung, Steuerung und
Aktuatordynamik sind ausgeschlossen.

## 1. Verbindliche Arbeitsgrundlagen

- Referenzmodelle sind der Optimist und der 420er als Jollen sowie das
  Folkeboot als Kielboot. Es werden keine besseren oder später zu
  beschaffenden Geometrien vorausgesetzt.
- Die Qualität der tatsächlich vorhandenen physischen Meshes wird geprüft
  und dokumentiert. Fehlende Daten werden nicht durch erfundene Werte oder
  Anzeigegeometrie ersetzt.
- Alle drei Modellverzeichnisse liegen außerhalb dieses CORE-Repositories.
  Änderungen an der bereits nutzerseitig geänderten Folkeboot-Modellbeschreibung
  werden erhalten und getrennt ausgewiesen.
- Die erste Physikstufe ist der isolierte ITTC-1957-Reibungsanteil mit dem
  vorläufigen Arbeitsbereich `10^6 <= Re <= 10^9`. Außerhalb des Bereichs
  wird bei positiver Geschwindigkeit kein Reibungsbeitrag berechnet; die
  Bilanz weist die Lücke mit einer Diagnose aus. Bei `V = 0` wird ein
  gültiger Nullbeitrag ohne `Cf`-Auswertung geliefert.
- Der ITTC-Schritt ist Integrationsnachweis, keine fachliche
  Bewegungsabnahme. Diese erfolgt erst nach validiertem
  Rumpf-Restwiderstand und Kiel-Seitenkraft.
- Hydrodynamik- und Profilangaben gehören in den bestehenden
  `modell.json`-Vertrag und dessen Spezifikation. Zuverlässig aus physischen
  Meshes ableitbare Werte werden nicht redundant gespeichert.
- Laufbezogene Fluiddaten werden in einem typisierten Core-Kontext
  transportiert. Der bestehende Simulationsstart bleibt zunächst erhalten.
- Bestehende Ergebnisfelder werden additiv genutzt. Neue Quellen- oder
  Gruppenwerte allein lösen keine neue Ergebnisversion aus; inkompatible
  Änderungen an Pflichtfeldern oder deren Bedeutung benötigen eine neue
  Version.
- Der erste Geschwindigkeitsansatz nutzt die Translation des
  Gesamtschwerpunkts. Rotationsbedingte lokale Strömung und daraus
  resultierende Dämpfung sind nachgelagerte Optimierungen.

## 2. Geordnete Arbeitsschritte

Die Schritte sind mit projektweit eindeutigen U-C-Kennungen versehen und den
freigegebenen A-C-Anforderungen zugeordnet. Ihre Statusangaben werden mit
tatsächlichen Tests und Datenbelegen fortgeschrieben.

| U-ID | Anforderung(en) | Inhalt und Ergebnis | Abhängigkeit | Nachweis/Abnahmekriterium |
|---|---|---|---|
| U-C-000045 | A-C-00062 | Modell- und Meshinventar aller drei Referenzen | Keine | Physische Mesh- und Modellkennwerte, Provenienz und bekannte Unsicherheiten pro Boot; keine GUI-Mesh-Nutzung |
| U-C-000046 | A-C-00061, A-C-00072 | Modell-/API-Vertrag mit Elementrolle, Importvalidierung, typisiertem Laufkontext, Einheiten und Fehlerbehandlung | U-C-000045 | Schema-Beispiele, Positiv-/Negativfälle, Modellbindung und kompatibler Simulationsstart |
| U-C-000047 | A-C-00064 | Isolierter ITTC-Reibungsrechner ohne Formfaktor oder Restwiderstand | U-C-000046 | Formelreferenzen, Nullfluss, Re-Grenzen, Vorzeichen und ungültige Eingaben |
| U-C-000048 | A-C-00063, A-C-00067, A-C-00070, A-C-00071 | Hydrodynamik in dynamische Integration und Ergebnisbeiträge einbinden | U-C-000047 | Zwischenstufen, Ergebnisversionen, Rückwärtskompatibilität, Bilanz und GUI-Adapter-Lesbarkeit |
| U-C-000049 | A-C-00065 | Rumpf-Rest-/Wellenwiderstand anhand validierter Methode ermitteln | U-C-000048 | Methode, Datenquelle, Anwendbarkeitsdomäne und unabhängige Referenzen; andernfalls mit konkretem Grund blockiert |
| U-C-000050 | A-C-00066 | Kiel-Seitenkraft und Polarenalternative untersuchen | U-C-000046, U-C-000048, U-C-000049 | Belegte Profile/Kennwerte, Streckung, Kraft, Widerstand und Ablösegrenzen; andernfalls blockiert |
| U-C-000051 | A-C-00069 | Fachliche Bewegungsabnahme | U-C-000049, U-C-000050 | Unabhängig abgeleitete Toleranzen und getrennte Referenzfälle; ITTC allein reicht nicht |
| U-C-000052 | A-C-00066, A-C-00068 | Festes Ruder und einzelne Interaktionen | U-C-000051 | Nachgelagert; kein bewegliches Ruder und keine unbelegten Zuschläge |
| U-C-000053 | A-C-00062 bis A-C-00072 | Abschluss, Schnittstellendokumentation und Rückverfolgbarkeit | U-C-000048; für Gesamtfreigabe U-C-000051, U-C-000052 | Statusmatrix mit Tests, Modellbelegen, Einschränkungen und Nutzerabnahmegrenze |

U-C-000049 und U-C-000050 dürfen nicht als fachlich abgenommen markiert
werden, solange ihre Quellen- und Datenanforderungen nicht erfüllt sind.
Dadurch bleiben U-C-000051 und nachgelagerte Schritte blockiert, falls die
erforderliche fachliche Grundlage fehlt.

## 3. Meilensteine

| Meilenstein | Eintritt | Überprüfbare Abnahme |
|---|---|---|
| Daten- und Schnittstellenvertrag | Modellinventar für alle drei Referenzen liegt vor | Modellfelder, Herkunft, Einheiten, Frames, Fehlerfälle, API und additive Ergebnisform sind abgestimmt und dokumentiert |
| ITTC-Integration | Vertragsmeilenstein bestanden | Isolierte Referenzen und dynamische Integrationsprüfungen bestehen; keine fachliche Gesamtbewegungsabnahme behauptet |
| Rumpfmodell | ITTC-Integration bestanden | Belegter Rest-/Wellenwiderstand mit Referenzen für die Modellklassen integriert |
| Kiel und Bewegung | Rumpfmodell vorhanden | Kiel-Seitenkraft nachgewiesen; fachliche Bewegungsfälle und Toleranzen für Jollen sowie Kielboot bewertet |
| Erweiterte Bauteile/Interaktionen | Einzelmodelle separat abgenommen | Feste Ruder- und Interaktionsbeiträge nur soweit Daten und Nachweise dies tragen |
| Lieferung | Alle für die freigegebene Stufe aktiven Anforderungen umgesetzt | CORE-Build, CTest, Regressionen, Schnittstellenbeschreibung und Rückverfolgbarkeitsmatrix vollständig; offene Grenzen und Nutzerabnahme getrennt ausgewiesen |

## 4. Test- und Integrationsregeln

- Mit jeder Berechnungsstufe entstehen gezielte Tests; der Abschlusstest
  ersetzt keine bausteinbegleitenden Tests.
- Prüfen: analytische oder unabhängige Referenzwerte, Null- und Grenzfälle,
  nicht endliche/ungültige Eingaben, Kraftrichtung, Frame-Transformation,
  Angriffspunkt, Moment um den Gesamtschwerpunkt, Summenparität,
  Integratorstufen, Ergebnisserialisierung und Bilanzstatus.
- Für Bewegungsfälle müssen gleiche Modellfassung, Hydrodynamikdaten,
  Fluidkontext und Schrittfolge reproduzierbare Ergebnisse innerhalb der
  festgelegten Toleranzen ergeben.
- Die bestehenden V2-Aufrufe und der separate V3-Testantrieb werden
  regressiv geprüft. Es werden keine vorhandenen Pflichtfelder oder
  Bedeutungen still geändert.
- Die vorhandene GUI darf die generischen Beiträge lesen; neue GUI-
  Darstellungen und GUI-Physik sind kein Abnahmekriterium dieses Core-Plans.
- Für den nativen Core-Pfad gelten Build und CTest des Projekts. Bei einer
  späteren Commit-Freigabe gelten zusätzlich die projektweiten
  Commit-Prüfregeln.
- Technische Testergebnisse, Integrationsnachweise und fachliche
  Nutzerabnahme werden getrennt dokumentiert. Keine erreichte Teilstufe wird
  als vollständige Yacht-Hydrodynamik bezeichnet.

## 5. Statusgrenze

Dieser Plan beauftragt die Core-Implementierung für C-0.6. Er erteilt keine
GUI-, Commit-, Push-, Tag- oder Release-Freigabe. Technische Umsetzung,
Modellabdeckung und fachliche Abnahme werden getrennt ausgewiesen.

## 6. Tatsächlicher Umsetzungsstand

| U-ID | Status | Tatsächlicher Nachweis / offene Grenze |
|---|---|---|
| U-C-000045 | technisch abgeschlossen; fachliche Modellfreigabe offen | Rollen in den drei vorhandenen Modellpaketen ergänzt; physische Rumpfmeshes inventarisiert (Gesamtoberfläche ca. 6,42/16,07/28,78 m²; X-Bounds ca. 2,36/4,20/7,68 m). Diese Gesamtwerte sind weder benetzte Fläche noch unabhängige Messdaten. Alle drei Modelle bleiben als Entwurf gekennzeichnet. |
| U-C-000046 | technisch abgeschlossen | Strikter JSON-Rollenimport mit Mesh-Pflicht, typisierter Fluidkontext, Modellfassungsbindung, Laufoption und Ergebnisserialisierung implementiert; ungültige Rollen/Konfigurationen werden diagnostiziert. CPPI-IDs sind registriert. |
| U-C-000047 | technisch abgeschlossen | ITTC-1957-Reibung aus poseabhängig ausgeschnittener physischer Rumpffläche und Wasserlinienlänge; Tests für analytischen Wert, Nullfluss, Re-Grenzen, Vorzeichen und Eingabefehler bestehen. |
| U-C-000048 | technisch abgeschlossen; keine fachliche Bewegungsabnahme | ITTC-Kraft und Moment wirken in Start-, Mittel- und Endauswertung des dynamischen Integrators. Beiträge und Laufkontext stehen additiv in Ergebnisversion 2; Standardwasserprofil aktiviert die Core-Stufe ohne GUI-Eingabe. Der GUI-Core-Adapter initialisiert bei Rumpfmodellen denselben Hydrodynamikkontext für Live-Läufe. Der aktuelle GUI-/Core-Build sowie alle 29 Tests bestehen, darunter Modellkatalog, Adapter und dynamische Referenzmodelle. Die GUI-Oberfläche und Kraftdarstellung blieben unverändert; die drei Bootmodelle sind im Katalogtest explizit abgesichert. |
| U-C-000049 | blockiert: anwendbare Referenzdaten fehlen | Ein Optimist-bezogener Schleppversuch ist bibliografisch nachgewiesen, aber Messkurven, getestete Rumpfvariante und Übertragbarkeit auf das vorhandene Mesh sind nicht verifiziert. Für 420er und Folkeboot fehlen verwendbare Schleppdaten; die DSYHS-Domäne für die drei Modelle ist ebenfalls nicht nachgewiesen. Es wurde kein Rest-/Wellenkoeffizient extrapoliert. |
| U-C-000050 | blockiert: Polaren/Kennwerte fehlen | Für Kiel/Schwert und Ruder liegen keine validierten Modellpolaren oder Ablösegrenzen vor. Es wird keine Seitenkraft oder Ruderwirkung erfunden. |
| U-C-000051 | blockiert durch U-C-000049 und U-C-000050 | ITTC-Reibung allein belegt keine realistische Yachtbewegung. Es gibt deshalb keine fachliche Gesamtbewegungsabnahme und keine Toleranzbehauptung. |
| U-C-000052 | blockiert durch U-C-000051 | Das Ruder bleibt wie beauftragt relativ zum Rumpf fest; hydrodynamische Ruderkräfte und Interaktionen werden erst nach fachlichem Bewegungsnachweis ergänzt. |
| U-C-000053 | teilweise abgeschlossen; Gesamtschluss blockiert | Spezifikation, Schnittstellenvertrag, CPPI-Register und dokumentierte Status-/Nachweismatrix aktualisiert. Der Liefermeilenstein bleibt wegen U-C-000049 bis U-C-000052 offen. |

Die Re-Untergrenze hat eine praktische Wirkung: Bei `nu=1e-6 m²/s` muss
`V*Lwl >= 1 m²/s` gelten. Außerhalb `[1e6,1e9]` wird kein
ITTC-Reibungswert extrapoliert; der Einzelbeitrag trägt `CORE-HYD-0001` und
den Zustand `nicht_berechnet`. Die dynamische Integration läuft weiter, aber
die Ergebnisbilanz bleibt ausdrücklich eine Teilsumme. Exakt null bleibt ein
gültiger Nullbeitrag.

Verifikation am 28.10.2026: nativer Core-Release-Build erfolgreich; **16/16
Core-CTest-Prüfungen** und **29/29 GUI-/Core-CTest-Prüfungen** bestanden,
darunter GUI-Modellkatalog und -Adapter sowie dynamische Referenzmodelle.
Diese Tests bestätigen die Softwareintegration und die endliche
Reibungsberechnung in den gewählten Pose-/Geschwindigkeitsfällen, nicht die
reale Widerstandsgenauigkeit, Kielwirkung oder Nutzerabnahme.
