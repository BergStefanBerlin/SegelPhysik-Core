# Spezifikation Kraft- und Momentenbilanz im Ergebnisvertrag

**Status: FINAL – freigegeben am 26.10.2026, umgesetzt und per CTest nachgewiesen in C-0.2 (uncommittet); Nutzerabnahme offen.**
**Anforderungen:** A-C-00034 bis A-C-00041
**Umsetzungsplan:** [Umsetzungsplan C-0.2](../Umsetzungsplaene/Umsetzungsplan_C-0.2.md)
**Entwurfspunkte:** [Kraft- und Momentenbilanz](Entwurfspunkte/Kraft_und_Momentenbilanz.json) (IDEE-C-00011 bis IDEE-C-00019, entschieden)

## 1. Zweck und Abgrenzung

Die GUI soll Kräfte und Momente verständlich darstellen: welche Beiträge auf
einen Körper wirken, woher Momente stammen und wie die Einzelbeiträge die
Gesamtbilanz bilden. Diese Spezifikation legt fest, welche
**physikalischen Daten** der Core dafür liefert. Die grafische Darstellung
(Pfeile, Bögen, Skalierung, Farben, Wertefenster) beschreibt das GUI-Projekt
in der „Spezifikation GUI: Kraft- und Momentendarstellung“.

**Grundsatz der Aufteilung:** Alles, was physikalisches Wissen oder
Solverwissen braucht, liegt im Core. Die GUI transformiert und zeichnet nur
gelieferte Werte und rechnet keine Physik.

| Thema | Core (diese Spezifikation) | GUI |
|---|---|---|
| Beiträge und Quellen | Stabile IDs, Quelle, Gruppe, Rolle, Punktart | Farbe und Linienart je Gruppe |
| Momente | Moment jedes Beitrags um S, Summen | Bogen, Radius, Orientierung |
| Summen | Gruppen- und Gesamtsummen mit Quellverweisen | Kennzeichnung als Summe |
| Datenzustände | gueltig, null, deaktiviert, nicht_berechnet, ungueltig | Textanzeige statt Leerzelle |
| Vollständigkeit | Erwarteter Quellenumfang, Bilanzstatus | „Gesamtbilanz“ oder „Teilsumme“ |
| Gleichgewicht | Toleranzen, Residuen, Freiheitsgrade, Iterationen | Anzeige im Detailbereich |
| Bilanzparität | Beiträge rekonstruieren die Solverbilanz | – |
| Wirkungslinie, Hebelarm | Wrench, Bezug, echte Angriffspunkte | Geometrische Hilfsdarstellung |

Herkunft: Altprojekt-Spezifikation „V0.11: Kraft- und Momentendarstellung“
(SegelPhysik3, Abschnitte 2, 3, 7.1 und 7.6). Altkennungen A-V0.11-… sind nur
Herkunftsverweise. Altprojekt-Code wird nicht übernommen.

Diese Spezifikation erweitert den Ergebnisvertrag aus der
[Spezifikation Hydrostatikkern](Spezifikation_Hydrostatikkern.md)
(A-C-00016, A-C-00017, A-C-00028), ohne diese Anforderungen zu ändern.

Nicht Teil dieser Spezifikation: neue Kraftgesetze (Hydrodynamik,
Widerstand, Wind, Added Mass), erfundene Gegen- oder Ersatzkräfte,
grafische Größen, die Berechnung äquivalenter Wirkungslinien (IDEE-C-00016:
Die GUI konstruiert sie bei Bedarf geometrisch). Spätere Kraftmodelle
(IDEE-C-00003) liefern ihre Beiträge nach diesem Vertrag.

## 2. Anforderungen

| ID | Kurztitel | Version | Status |
|---|---|---|---|
| A-C-00034 | Beitragsregister mit stabiler Identität | C-0.2 | umgesetzt |
| A-C-00035 | Moment jedes Beitrags um den Schwerpunkt | C-0.2 | umgesetzt |
| A-C-00036 | Summen mit Quellverweis und einmaliger Bilanzierung | C-0.2 | umgesetzt |
| A-C-00037 | Unterscheidbare Datenzustände | C-0.2 | umgesetzt |
| A-C-00038 | Vollständigkeit der Bilanz | C-0.2 | umgesetzt |
| A-C-00039 | Gleichgewichtsangaben des statischen Solvers | C-0.2 | umgesetzt |
| A-C-00040 | Parität zur Solverbilanz | C-0.2 | umgesetzt |
| A-C-00041 | Vertragsversion 2 und Dokumentation | C-0.2 | umgesetzt |

### A-C-00034: Beitragsregister mit stabiler Identität

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00004, -00005, -00007; IDEE-C-00012, -00016

Jeder gelieferte Kraft- oder Momentenbeitrag besitzt:

| Feld | Bedeutung |
|---|---|
| `id` | Über alle Zustände eines Laufs stabile, eindeutige ID (bestehendes Schema `<Element-ID>:gewicht`, `<Element-ID>:auftrieb`, `gesamt:moment` bleibt gültig) |
| `element_id` | Element oder leere ID für den Gesamtmodellbezug |
| `quelle` | Physikalische Quelle: zunächst `schwerkraft`, `hydrostatik`; unbekannte Quellen `nicht_zugeordnet` mit Originalnamen |
| `gruppe` | Fachliche Gruppe: zunächst `gewicht_auftrieb`; die Liste ist im Ergebnisvertrag erweiterbar |
| `rolle` | `einzelbeitrag`, `gruppensumme` oder `gesamtsumme` |
| `punktart` | `angriffspunkt` (realer Kraftangriffspunkt) oder `bezugspunkt` (Momentenbezug bzw. Darstellungsbezug ohne physikalischen Angriff) |

Mehrere Beiträge derselben Quelle bleiben einzeln identifizierbar. Jeder
Beitrag gehört genau einer Gruppe an. Kein Beitrag wird weggelassen.

**Abnahme:** Zwei aufeinanderfolgende dynamische Zustände liefern dieselben
IDs. Jede ID ist eindeutig; Quelle, Gruppe, Rolle und Punktart sind gesetzt.

### A-C-00035: Moment jedes Beitrags um den Schwerpunkt

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00006; IDEE-C-00011, -00018

Für jeden Kraftbeitrag Fᵢ mit Angriffspunkt Aᵢ und gegebenenfalls eigenem
Moment Mᵢ,A liefert der Core das Moment um den Momentenbezug P:

```text
Mᵢ,P = (Aᵢ − P) × Fᵢ + Mᵢ,A
```

Der Momentenbezug P ist der aktuelle Gesamtschwerpunkt S (wie heute
`moment_bezug_modell_m`) und wird je Zustand genannt. Liegt ein Beitrag nur
als Wrench um einen Punkt Q vor, gilt `Mᵢ,P = Mᵢ,Q + (Q − P) × Fᵢ`; derselbe
Hebelarm wird nicht doppelt addiert. Alle Beiträge, Momente und Summen
werden im Modellframe geliefert; die GUI transformiert mit der Pose. S ist
bei einem frei schwimmenden Körper kein fester Drehpunkt.

**Abnahme:** Die algebraischen Referenzfälle in Abschnitt 3 werden mit
`rtol = 1e-10` und `atol = 1e-9` (SI) erfüllt.

### A-C-00036: Summen mit Quellverweis und einmaliger Bilanzierung

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00007

Der Core liefert je Zustand die Gesamtkraft ΣF und das Gesamtmoment ΣM um S
sowie je Gruppe die Gruppensumme aus Kraft und Moment, jeweils im
Modellframe. Jede Summe verweist auf die IDs ihrer Einzelbeiträge und ist
kein zusätzlicher Summand. Gewicht wird genau einmal berücksichtigt.
Ungleiche Beträge von Gewicht und Auftrieb bleiben erhalten; es gibt keine
Zwangsangleichung oder erfundene Gegenkraft.

**Abnahme:** Die Summe der Einzelbeiträge ergibt die gelieferten Gruppen-
und Gesamtsummen. Kein Beitrag wird doppelt gezählt.

### A-C-00037: Unterscheidbare Datenzustände

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00041; IDEE-C-00013

Jeder Beitrag trägt das Feld `zustand` mit einem der Werte `gueltig`,
`null` (gültiger Nullvektor), `deaktiviert`, `nicht_berechnet` oder
`ungueltig`. Bei `ungueltig` enthält das Feld `fehler_id` eine ID aus dem
Fehlerkatalog. Bei `deaktiviert`, `nicht_berechnet` und `ungueltig` sind
`vektor` und `punkt_m` JSON-`null`. Fehlende oder ungültige Werte erscheinen
nie als Nullvektor. Kleine endliche Werte werden nicht auf null gesetzt.

**Abnahme:** Ein Element ohne eingetauchtes Volumen liefert einen Auftrieb
mit `zustand = null`. Ein absichtlich ungültiger Testwert erzeugt
`ungueltig` mit Fehler-ID; die ID steht im Fehlerkatalog.

### A-C-00038: Vollständigkeit der Bilanz

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00039; IDEE-C-00014

Jeder Zustand nennt den erwarteten Quellenumfang. Er ergibt sich aus Modell
und Rechenmodus (heute je Element Gewicht und Auftrieb). Daraus folgt der
Bilanzstatus:

- `vollstaendig`: alle erwarteten Beiträge sind `gueltig` oder `null`,
- `teilsumme`: mindestens ein erwarteter Beitrag ist `nicht_berechnet` oder
  `ungueltig`; die betroffenen IDs werden genannt,
- `ungeklaert`: der erwartete Umfang ist unbekannt.

`deaktiviert` ist keine Lücke. Ausblenden in der GUI ändert den Umfang nicht.

**Abnahme:** Ein Test mit erzwungen fehlendem Beitrag meldet `teilsumme`
mit der fehlenden ID; der Referenzquader meldet `vollstaendig`.

### A-C-00039: Gleichgewichtsangaben des statischen Solvers

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00040; IDEE-C-00015

Statische Ergebnisse liefern die tatsächlich verwendeten Kriterien:
`krafttoleranz_n`, `momenttoleranz_nm`, `residuum_kraft_n`,
`residuum_moment_nm`, `freiheitsgrade` (geprüfte Freiheitsgrade) und
`iterationen`. Dynamische Zustände tragen keine Gleichgewichtsaussage;
die Felder fehlen dort bzw. sind ausdrücklich `null`.

**Abnahme:** Der statische Quader-Referenzfall liefert Toleranzen und
Residuen unterhalb der Toleranzen. Ein nicht konvergierter Fall liefert
Residuen oberhalb der Toleranz.

### A-C-00040: Parität zur Solverbilanz

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-V0.11-00048; IDEE-C-00019

Die gelieferten Einzelbeiträge rekonstruieren für denselben Zustand,
dieselbe Modellfassung, denselben Frame und Bezug die Kraft- und
Momentensumme, die der Solver bzw. Integrator verwendet. Bei Dynamik werden
die Beiträge für den gelieferten Zustand (Ende des Schritts) ausgewertet und
so gekennzeichnet.

**Abnahme:** Tests vergleichen für Statik und Dynamik die Beitragssumme mit
der Solverbilanz desselben Zustands, Toleranzen wie Abschnitt 3.

### A-C-00041: Vertragsversion 2 und Dokumentation

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** A-C-00016, A-C-00028; IDEE-C-00017

Die Erweiterung wird als Vertragsversion `"2"` eingeführt. Alle Felder der
Version 1 bleiben mit unveränderter Bedeutung erhalten; neue Felder kommen
hinzu. `Dokumentationen/Schnittstelle_Core_GUI.md` beschreibt Felder,
Beispiele und Kompatibilitätsregeln (A-C-00028).

**Abnahme:** Vertragstests und aktualisierte Referenzaufzeichnungen für
Statik und Dynamik liegen vor; ein Leser für Version 1 kann Version-2-Daten
weiterhin auswerten.

## 3. Algebraische Referenzfälle

Bezug P = S, Werte im Modellframe.

| Fall | Erwartung |
|---|---|
| Einzelkraft durch S: F = (0, 0, 10) N | M_S = (0, 0, 0) N·m; keine erfundene Gegenkraft |
| Kraft bei A − S = (0; 0,35; 0) m, F = (0, 0, 10) N | M_S = (3,5; 0; 0) N·m |
| Kräftepaar an verschiedenen Punkten | ΣF = 0; Moment unabhängig vom Bezug |
| Gewicht ≠ Auftrieb | Resultierende Kraft bleibt erhalten |
| A − S = (0,10; 0,35; −0,30) m, F = (0, 0, 49 050) N | M_S = (17 167,5; −4 905; 0) N·m |
| Wrench um Q, Bezugswechsel nach P | M_P = M_Q + (Q − P) × F |
| Körper-/Weltframe | Punkte rotiert und verschoben, Vektoren nur rotiert; Bezugs- und Framewechsel vertauschbar |

## 4. Quellen

- Altprojekt `SegelPhysik3/doc/WegZuV1.0/Revisionslog/zuV0.11/Spezifikation_V0.11.md`
  (Herkunft, nicht normativ).
- [Spezifikation Hydrostatikkern](Spezifikation_Hydrostatikkern.md).
- [Schnittstelle Core → GUI](../Dokumentationen/Schnittstelle_Core_GUI.md).
