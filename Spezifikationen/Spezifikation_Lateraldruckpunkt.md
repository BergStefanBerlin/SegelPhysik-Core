# Spezifikation: Lateraldruckpunkt

**Status:** Fachlich finalisiert am 28.10.2026 (Anforderungen A-C-00093 bis A-C-00097). Umsetzung und Abnahme sind offen. Keine Commit-, Push-, Tag- oder Release-Freigabe.  
**Zielversion:** C-0.7 (laufender Versionsschritt) · **Umsetzungsplan:** [Umsetzungsplan C-0.7](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md), Teil D  
**Herkunft:** [Entwurfspunkteliste](Entwurfspunkte/Lateraldruckpunkt.json) (IDEE-C-00060 bis IDEE-C-00065)  
**GUI-Gegenstück:** [Spezifikation GUI Lateraldruckpunkt](../../GUI/Spezifikationen/Spezifikation_GUI_Lateraldruckpunkt.md)

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
