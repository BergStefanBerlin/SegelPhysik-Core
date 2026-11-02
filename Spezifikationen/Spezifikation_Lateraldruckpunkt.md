# Spezifikation: Lateraldruckpunkt

**Status:** Abschnitte 1–4 am 28.10.2026 für C-0.7 finalisiert (A-C-00093 bis A-C-00097); Abschnitt 5 am 02.11.2026 für C-0.8 finalisiert (A-C-00123 bis A-C-00125). Umsetzung und technische Abnahme: siehe Plan. Keine Commit-, Push-, Tag- oder Release-Freigabe.
**Zielversion:** C-0.7 und C-0.8 · **Umsetzungspläne:** [C-0.7](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md), Teil D; [C-0.8](../Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md), Abschnitt „Korrekter LDP und Ersatz-SDP“
**Herkunft:** [Entwurfspunkteliste](Entwurfspunkte/Lateraldruckpunkt.json) (IDEE-C-00060 bis IDEE-C-00065 und IDEE-C-00085 bis IDEE-C-00087)
**GUI-Gegenstück:** [Spezifikation GUI Lateraldruckpunkt, Abschnitt 5](../../GUI/Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md#5-ldp-und-ersatz-sdp-als-punkte-02112026)

Diese Spezifikation beschreibt den Lateraldruckpunkt (CLR, Center of Lateral
Resistance) als abgeleitete Auswertegröße des Cores. Sie ist die einzige
maßgebliche Spezifikation für diesen Gegenstand. Der Segeldruckpunkt (CE) ist
nicht Teil dieses Umfangs, wird aber in der Struktur des Ergebnisvertrags
vorbereitet. Segelkräfte, Vorlage und Gierbilanz gegen den CE folgen mit der
Aerodynamik.

## 1. Ziel

Der Lateraldruckpunkt ist der Angriffspunkt der resultierenden
hydrodynamischen Seitenkraft des Unterwasserschiffs. Zusammen mit dem
Segeldruckpunkt bestimmt er die Luv- bzw. Leegierigkeit. Der Core liefert
ihn als Ergebnisgröße; die GUI zeichnet ihn nur (siehe GUI-Spezifikation).

Der Lateraldruckpunkt ändert weder Kräfte noch Momente noch den
Simulationszustand. Er entsteht ausschließlich aus bereits berechneten
Beiträgen.

## 2. Anforderungen

Entscheidung IDEE-C-00060: eigene Spezifikation; Zielversion ist der
laufende Versionsschritt C-0.7 (kein neuer Versionsschritt, kein eigener Plan).

### A-C-00093: Definition und Quellen

*Herkunft: IDEE-C-00061.*

Berücksichtigt werden die hydrodynamischen Seitenkraftbeiträge der Elemente
mit Rolle `rumpf` (Druck), `kiel` und `ruder` (Seitenkraft), jeweils nur die
Komponente quer zur Bootslängsachse (Modell-y).

Nicht berücksichtigt werden Reibung, Zusatzdämpfung (`daempfung:*`),
Hydrostatik, Gewicht, Testantrieb.

**Abnahme:** Tests belegen, dass nur die genannten Beiträge eingehen und
Dämpfung, Reibung und Hydrostatik den Wert nicht verändern.

### A-C-00094: Berechnung

*Herkunft: IDEE-C-00062.*

Mit Querkraft F_y,i und Angriffspunkt (x_i, y_i, z_i) im Modellrahmen:

- Querkraft gesamt: F_y = Σ F_y,i
- Längsposition: x_CLR = Σ(F_y,i · x_i) / F_y
- Höhenlage: z_CLR = Σ(F_y,i · z_i) / F_y (nur Auskunft)
- Position quer: y_CLR = 0 (Mittschiffsebene, nur Darstellung)

Die Summen laufen über alle Seitenkraftbeiträge in Modell-y, auch über
solche mit entgegengesetztem Vorzeichen (z. B. Bug und Heck beim reinen
Drehen). Heben sich diese weitgehend auf, ist x_CLR nicht sinnvoll bestimmbar.

**Abnahme:** Kraft nur am Ruder liefert dessen Angriffspunkt; zwei gleichgerichtete
Beiträge liefern den kraftgewichteten Mittelwert; Ergebnis stimmt mit
einer unabhängigen Handrechnung überein.

### A-C-00095: Status und Schwelle

*Herkunft: IDEE-C-00063.*

- `gueltig`, wenn |F_y| ≥ 5 % von Σ|F_y,i| und |F_y| ≥ 1 N.
- Sonst `nicht_bestimmbar`, ohne Position. Eine Zahl wird nie erfunden.
- Ist keine Hydrodynamik aktiv, fehlt der Block.

**Abnahme:** Gegenkräfte, Querkraft null und Schwellenwerte (knapp darüber
und darunter) liefern den erwarteten Status; bei `nicht_bestimmbar` gibt es
keine Position.

### A-C-00096: Ergebnisvertrag

*Herkunft: IDEE-C-00064.*

Additiver Block im Zustands-JSON neben `hydrodynamik`; `vertragsversion`
bleibt unverändert, alle Altfelder bleiben. Live und Batch verwenden
dieselbe Serialisierung.

```
"druckpunkte": {
  "lateral": {
    "status": "gueltig" | "nicht_bestimmbar",
    "position_modell_m": [x, 0, z],
    "querkraft_modell_n": F_y,
    "quellen": ["Rumpf:hydrodynamik:druck", "Ruder:hydrodynamik:seitenkraft", ...]
  }
}
```

Bei `nicht_bestimmbar` fehlen `position_modell_m` und `quellen` oder sind
leer; `querkraft_modell_n` bleibt. Der Block `druckpunkte` ist bewusst
allgemein angelegt; ein Eintrag `segel` folgt später. Alle Längen sind in
Metern im Modellrahmen; Prozent der Wasserlinienlänge liefert der Core nicht
(Umrechnung ist Sache der GUI).

**Abnahme:** Ergebnisvertragstest für beide Status, unveränderte Altfelder,
Live-/Batch-Parität; Eintrag im Schnittstellenregister (neue CPPI ab 0031).

### A-C-00097: Wirkung und Nachweise

*Herkunft: IDEE-C-00065.*

- Reine Auswertegröße, keine Rückwirkung auf Kräfte/Momente.
- Tests: Kraft nur am Ruder (x_CLR = Ruderposition), nur am Schwert,
  zwei Beiträge gleicher Richtung (Mittelwert), Gegenkräfte (nicht bestimmbar),
  Querkraft ≈ 0, Live-/Batch-Parität, Altfelder unverändert.
- Referenzmodelle Optimist, 420er und Folkeboot liefern bei Querfahrt einen
  Wert im Rumpfbereich.
- Integrierter GUI/Core-Lauf bleibt grün.

**Abnahme:** Alle genannten Tests bestehen; Kräfte und Momente mit und ohne
Auswertung sind identisch.

## 3. Entscheidungen zur Finalisierung

- Zielversion: laufender Versionsschritt C-0.7 (Nutzer, 28.10.2026).
- Schwelle 5 % von Σ|F_y,i| und mindestens 1 N bestätigt.
- Nur Meter im Core; kein Prozentanteil.
- Live und Batch führen den Block gleich.

## 4. Bekannte Einschränkungen

- Das Panelmodell ist eine Näherung; der CLR ist so genau wie die
  Seitenkräfte der Hydrodynamik (geschätzte Koeffizienten).
- Rumpf und Kiel/Schwert-Interaktion wird nicht modelliert; der Punkt
  folgt aus den Einzelbeiträgen.
- Keine Aussage über die Gierstabilität ohne Segeldruckpunkt.

## 5. C-0.8: korrekter LDP und Ersatz-SDP

**Status:** Am 02.11.2026 auf ausdrückliche Nutzerfreigabe finalisiert.
Diese Anforderungen ergänzen die historischen C-0.7-Verträge aus
Abschnitten 1–4; sie ändern weder die Physik noch die Bedeutung der
bestehenden Ergebnisvertragsversion. Maßgeblich ist ausschließlich der
[C-0.8-Plan](../Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md).
Herkunft und IDEE-zu-A-Zuordnung: [Entwurfspunkteliste](Entwurfspunkte/Lateraldruckpunkt.json).

### A-C-00123: Panelgenaue Lateraldruckpunktauswertung

*Herkunft: IDEE-C-00085.*

LDP bezeichnet den aequivalenten Angriffspunkt der resultierenden
hydrodynamischen Modell-Y-Seitenkraft, nicht den geometrischen
Flaechenschwerpunkt. Quellen bleiben Rumpfdruck und Kiel-/Ruderseitenkraft;
Reibung, Daempfung, Gewicht, Auftrieb und Fremdkraft bleiben ausgeschlossen.

Die Auswertung erfolgt aus den tatsaechlich verwendeten Panelkraeften:

- `Q = Summe(F_y,p)`
- `B = Summe(abs(F_y,p))`
- `X = Summe(x_p * F_y,p)`
- `Z = Summe(z_p * F_y,p)`
- bei bestimmbarer Resultierenden: `LDP = (X/Q, 0, Z/Q)`.

Panelpositionen und Kraefte muessen vor der Summation im selben
Modellrahmen vorliegen; Pose, benetzte Geometrie, bewegte Elemente und
lokale Rotationsanstroemung entsprechen exakt der Kraftberechnung.
Alle implementierten Seitenkraftmethoden einschliesslich Kennfeldpfad
verwenden dieselbe Auswertung. Signierte erste Momente und die Summe der
Panelbetraege bleiben bei jeder Aggregation erhalten. Insbesondere nicht
aus einem mit Gesamtkraftbetraegen gewichteten Bauteilmittelpunkt oder
dem vollen Giermoment rekonstruieren: letzteres enthaelt auch `-y*F_x`.
Die Zusatzdaten sind reine Auswertung ohne Einfluss auf Physik/Solver.

Bestimmbar ist der kraftbasierte Punkt ab `abs(Q) >= 1 N` und
`abs(Q) >= 0.05 * B`. `B` ist die Summe der Beträge der einzelnen
Panelkräfte, auch wenn sie sich innerhalb eines Bauteils aufheben.
Bei zu starker Aufhebung wird kein kraftbasierter Punkt ausgewiesen.
Ein separater geometrischer Ersatzpunkt darf nicht als kraftbasierter
LDP erscheinen. Ein gültiger Punkt außerhalb des Boots wird weder
geklemmt noch auf die Rumpfoberfläche projiziert.

**Abnahme:** Das Zweipanel-Beispiel `+100 N bei x=+1 m`,
`-80 N bei x=-1 m` ergibt `Q=20 N`, `X=180 Nm` und `x_LDP=9 m`.
Gegenkräfte innerhalb und zwischen Bauteilen sowie beide Schwellen
exakt, knapp oberhalb und knapp unterhalb werden geprüft. Modell-X-
und Modell-Y-Kraft dürfen nicht über das volle Giermoment verwechselt
werden. Pose, bewegte Geometrie und Kennfeldpfad verwenden dieselbe
Panelauswertung. Die Auswertung hat keine Rückwirkung auf Physik oder
Solver.

### A-C-00124: Ersatz-SDP aus der Fremdkraft

*Herkunft: IDEE-C-00086.*

Bis zur Einfuehrung der Segelphysik bezeichnet Ersatz-SDP genau den
einen modellfesten Ansatzpunkt der konstanten Fremdkraft. Keine
Ableitung aus Segelmesh, Mastposition oder Namen; keine Aerodynamik.
Quelle ist ausschließlich der eingefrorene Core-Ergebniszustand des
angezeigten Laufs, nicht eine später geänderte GUI-Eingabe.
Der stationäre Ergebnis-Snapshot muss zum serialisierten Zustand passen;
ein veralteter oder abweichender Snapshot wird zurückgewiesen und nicht
an einen anderen Zustand angehängt.

Bei aktiver, endlicher und nicht verschwindender Fremdkraft ist der
Punkt gueltig, unabhaengig davon, ob die Kraft modell- oder weltfest
definiert wurde; der Ansatzpunkt bleibt in beiden Faellen modellfest.
Inaktive oder exakt verschwindende Kraft liefert keinen wirksamen
Ersatz-SDP. Fehlt im Zustand ein stationärer Lastkontext, wird kein
Punkt hinzuerfunden. Ein nichtkonvergiertes Ergebnis bleibt über
seinen Status als Näherung erkennbar. Bezeichnung und Herkunft dürfen
keinen bereits berechneten echten Segeldruckpunkt behaupten.

Gleiche Laengsposition von SDP und LDP ist keine Core-Erfolgsbedingung.
Vollständige Bewegungsbilanz und bestehende stationäre Verträge bleiben
maßgeblich; keine automatische Last-, Ruder- oder Schwertkorrektur.

**Abnahme:** Aktive, inaktive und exakt verschwindende Fremdkraft sowie
modell- und weltfester Kraftframe liefern erwarteten Status und
modellfesten Ansatzpunkt. Alte Zustände ohne stationären Lastkontext
enthalten keinen Block. Änderungen nach dem Snapshot ändern den
ausgegebenen Punkt nicht; ein Snapshot für einen abweichenden Zustand
wird abgelehnt.

### A-C-00125: Additiver Ergebnisvertrag und Legacyprojektion

*Herkunft: IDEE-C-00087.*

- Bestehendes `druckpunkte.lateral` mit Metern im Modellrahmen behalten.
  Additive Kennzeichnung `auswertung_version: 2` für die korrigierte
  Panel-Auswertung; `position_modell_m` ausschließlich bei gültigem
  kraftbasiertem Punkt als LDP nutzbar. Historisches `geometrisch`
  bleibt als eigener Status unterscheidbar, nicht als LDP umdeuten.
- Zusätzlicher, getrennt benannter Block `druckpunkte.ersatz_segel`:
  `status` (`gueltig`, `inaktiv`, `kraft_null`),
  `quelle: fremdkraft`, bei gültigem Status `position_modell_m`.
  Ohne entsprechenden Lastkontext fehlt der Block.
  Kein Feld `segel` mit vorgetaeuschter Aerodynamik belegen.
- V6 sonst unveraendert. Neue GUI liest alte Zustaende weiterhin,
  weist aber nicht zertifizierte alte LDP-Auswertung kenntlich aus;
  sie rekonstruiert keine Ersatzphysik. Fehlende neue Daten sind
  explizit nicht verfuegbar. Unbekannte/malformed neue Pflichtwerte
  werden nach bestehenden Fehlerwegen gemeldet.
- `Kompatibel` behält seine bisherige Vertragsversionswahl und
  Feldprojektion; `auswertung_version` und ein nur mit passendem
  Stationsnapshot vorhandenes `ersatz_segel` sind additive Felder.
  V6-spezifische Felder werden nicht in ältere Projektionen kopiert.
- Additive Exportfelder und neue C++-Datentypen sind einzeln im
  Core-Schnittstellenregister erfasst: CPPI-0101/0102 für die
  Ergebnisfelder und CPPI-0103/0104 für die Panelbilanz im öffentlichen
  Beitragsvertrag. CPPI-0031 bleibt erhalten. Darstellung und
  Grafikpersistenz bleiben GUI-Aufgaben. Keine Core-Grafikoptionen oder
  weiteren Physikparameter hinzufügen.

**Abnahme:** JSON enthält die explizite LDP-Auswertungsversion 2 und
den separaten Ersatz-SDP-Block gemäß Kontextverfügbarkeit. V6 und alle
Altfelder bleiben unverändert; ältere geometrische Werte bleiben als
`geometrisch` unterscheidbar und werden nicht als gültiger LDP
umgedeutet. Ein fehlender Lastkontext ergibt keinen Block. Live,
Batch und Serialisierung sind konsistent. Kräfte, Momente,
Solverergebnisse und Zustandsverlauf sind mit und ohne
Druckpunktauswertung identisch. Die Regression enthält einen echten
420er-Lauf mit vollständigen Eingaben; ein Screenshot allein ist kein
vollständiger numerischer Nachweis.
