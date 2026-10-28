# Spezifikation: Hydrodynamische Zusatzmasse und Dämpfung

**Status:** Fachlich finalisiert am 28.10.2026 (Anforderungen A-C-00086 bis A-C-00092). Umsetzung und Abnahme sind offen. Keine Commit-, Push-, Tag- oder Release-Freigabe.  
**Zielversion:** C-0.7 (laufender Versionsschritt; früher als C-0.9 geführt) · **Umsetzungsplan:** [Umsetzungsplan C-0.7](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md), Teil C  
**Herkunft:** [Entwurfspunkteliste](Entwurfspunkte/Hydrodynamik_Zusatzmasse_Daempfung.json) (IDEE-C-00051 bis IDEE-C-00059)

Diese Spezifikation ergänzt die [Core-Hydrodynamik-Spezifikation](Spezifikation_Hydrodynamik.md),
die Zusatzmassen und Dämpfung bisher ausdrücklich als getrennte spätere Themen
ausschließt. Sie ist die einzige maßgebliche Spezifikation für diesen Gegenstand.
Wellenwiderstand, Rumpfnachlauf, Rumpf-Schwert-Ruder-Interaktion, freie
Oberfläche, Seegang, Frequenzabhängigkeit der Zusatzmasse, Segelkräfte sowie
CFD- oder Modellversuchsvalidierung sind nicht Teil dieses Umfangs.

**GUI:** Es sind keine GUI-Anforderungen und kein GUI-Plan erforderlich
(keine A-G-/U-G-Kennungen). Der Ergebnisvertrag wird nur additiv erweitert; die
Verträglichkeit mit der GUI wird im Core-Plan über den integrierten
GUI/Core-Testlauf nachgewiesen (siehe A-C-00092).

## 1. Ziel und Ausgangslage

Das Boot beschleunigt und dreht heute mit reiner Körpermasse und
Körperträgheit. Real bewegt es zusätzlich Wasser mit und erfährt eine
Dämpfung der Rotations- und Hubbewegungen, die das Panelmodell nicht liefert
(z. B. Abstrahlungsdämpfung).

Ausgangslage im Core:

- Translation: skalare Masse, Beschleunigung im Weltrahmen.
- Rotation: Euler-Gleichung im Körperrahmen mit der Trägheitsmatrix um den
  Gesamtschwerpunkt.
- Panelkräfte enthalten bereits translatorischen Widerstand (Reibung,
  Druckwiderstand Cd = 1,0, Folienkräfte) und die lokale Rotationsanströmung.
  Eine zusätzliche translatorische Dämpfung würde diese Anteile doppelt zählen
  und ist ausgeschlossen.

Alle Koeffizienten sind Schätzungen ohne Messdaten. Sie werden als solche mit
Quelle und Unsicherheit ausgewiesen; es wird keine quantitative Genauigkeit
behauptet.

## 2. Anforderungen

### A-C-00086: Bestimmung der Zusatzmassen

*Herkunft: IDEE-C-00052, IDEE-C-00053, IDEE-C-00055.*

Der Core bestimmt eine diagonale Zusatzmassenmatrix mit sechs Werten
(A11, A22, A33 in kg, A44, A55, A66 in kg·m²) um den Gesamtschwerpunkt.

- Rumpf: Ableitung aus dem Rumpfmesh mit Ellipsoid- oder Streifennäherung aus
  Abmessungen, Verdrängung und Referenzwasserlinie.
- Ruder und Schwert: zusätzlicher Beitrag in der Querrichtung ihrer Profile
  über eine Plattennäherung (Zusatzmasse je Spannweite ≈ ρ·π/4·c²), umgerechnet
  auf Masse und Trägheit um den Gesamtschwerpunkt.
- Die Zusatzmassen sind konstant und gelten am Referenzzustand (aufrecht,
  Konstruktionswasserlinie). Eine Änderung mit Krängung oder Eintauchung wird
  nicht modelliert und als Einschränkung dokumentiert.
- Jeder Wert trägt Status `geschaetzt`, Methode, Quelle und Unsicherheit.
- Ein nicht endlicher oder negativer Wert und ein nicht bestimmbarer Wert
  führen zu einer ausdrücklichen Diagnose statt zum stillen Wert null.

**Abnahme:** Kugel (A11 = ½·ρ·V), Ellipsoid (Lamb-Koeffizienten) und Platte
liefern innerhalb dokumentierter Toleranzen die analytischen Referenzwerte;
Optimist, 420er und Folkeboot liefern endliche, positive und plausibel
gewichtete Werte; Spiegelsymmetrie des Bootes bleibt erhalten.

### A-C-00087: Modellvertrag für Zusatzmasse und Dämpfung

*Herkunft: IDEE-C-00052, IDEE-C-00056.*

Die Modellbeschreibung erhält einen optionalen, rückwärtskompatiblen Block
für Zusatzmassen und Dämpfung.

- Je Zusatzmassenwert, je Dämpfungskoeffizient und je Dämpfungsverhältnis ist
  ein Überschreiben in `modell.json` möglich; ohne Angabe gelten die
  abgeleiteten Werte.
- Einheiten, Wertebereiche (endlich, nicht negativ) und gültige
  Freiheitsgrade werden beim Import validiert. Ein ungültiger Block wird
  ausdrücklich diagnostiziert, nicht als „nicht vorhanden“ gewertet.
- Modelle ohne den Block bleiben gültig und erhalten die abgeleiteten Werte.
- Ein Überschreiben wird im Ergebnis als Herkunft `modell` ausgewiesen.

**Abnahme:** Gültige, ungültige, partielle und fehlende Blöcke sowie Altmodelle
sind getestet.

### A-C-00088: Bewegungsgleichung mit Zusatzmasse

*Herkunft: IDEE-C-00054.*

Der Integrator löst im Körperrahmen mit ν = (u, v, w, p, q, r):

```text
(M_RB + M_A) · dν/dt + C_RB(ν)·ν + C_A(ν)·ν = τ
```

- `M_A` ist die Zusatzmassenmatrix aus A-C-00086, `C_A` sind die zugehörigen
  Coriolis-/Munk-Terme.
- Beide vorhandenen Integratorstufen verwenden dieselbe Gleichung; ihr
  Zustandsvertrag und ihre Reihenfolge bleiben konsistent.
- Die statische Gleichgewichts- und Ruhelagenrechnung bleibt unverändert; die
  Zusatzmasse wirkt nur bei Beschleunigung.
- Ist die effektive Massen- bzw. Trägheitsmatrix nicht positiv definit, bricht
  der Schritt mit ausdrücklicher Diagnose ab, wie bei der bisherigen Trägheit.
- Der Testantrieb und seine Geschwindigkeitsbegrenzung bleiben unverändert.

**Abnahme:** Freie Beschleunigung eines Körpers aus Kraft F ergibt
a = F/(m + A11); Munk-Terme erzeugen keine Energie (Energiebilanz über eine
Drehung); Statik und Ruhelage sind mit und ohne Zusatzmasse identisch.

### A-C-00089: Dämpfungsmodell

*Herkunft: IDEE-C-00056.*

Eine zusätzliche Dämpfung wirkt je Freiheitsgrad als additiver Beitrag:

```text
τ_D,i = −(B1,i + B2,i·|ν_i|) · ν_i
```

- Freiheitsgrade: Gieren, Rollen, Stampfen und Tauchen. Eine zusätzliche
  translatorische Dämpfung in Längs- und Querrichtung ist ausgeschlossen.
- Rollen, Stampfen und Tauchen: linearer Anteil über das Dämpfungsverhältnis
  `B1 = 2·ζ·√(C_ii·(M_ii + A_ii))` mit der aus der Hydrostatik abgeleiteten
  Rückstellsteifigkeit `C_ii`. Die Zahlenwerte für ζ werden nach A-C-00090
  festgelegt.
- Gieren (ohne Rückstellung): quadratischer Anteil aus Rumpflänge und Fläche
  abgeleitet, linearer Anteil als geschätzter Anteil der Zusatzträgheit pro
  Sekunde.
- Der quadratische Anteil darf nur den Teil enthalten, den die Panelkräfte
  nicht schon liefern.
- Die Dämpfung entzieht nur Energie, sie erzeugt nie welche.

**Abnahme:** Abklingversuche für Rollen, Stampfen, Tauchen und Gieren zeigen
die erwartete Abklingrate; keine Instabilität; Energie nimmt monoton ab.

### A-C-00090: Messung der Ist-Abklingraten und Festlegung der Koeffizienten

*Herkunft: IDEE-C-00056, IDEE-C-00057.*

Vor der Festlegung von ζ und der Gierkoeffizienten wird am heutigen Core
(ohne Zusatzmasse und Dämpfung) für Optimist, 420er und Folkeboot gemessen,
wie schnell Rollen, Stampfen, Tauchen und Gieren aus einer Auslenkung bzw.
Anfangsgeschwindigkeit abklingen.

- Die Messung wird dokumentiert (Szenario, Ergebnis, Datum, Codestand).
- Die endgültigen ζ-Werte und Gierkoeffizienten werden so festgelegt, dass
  nur der fehlende Anteil ergänzt und keine Doppelzählung erzeugt wird.
- Die festgelegten Werte, ihre Begründung und ihre Unsicherheit stehen im
  Plan und im Ergebnis. Eine Abweichung von typischen Jollenwerten
  (Rollen ζ ≈ 0,1–0,3, Stampfen/Tauchen ≈ 0,2–0,4) wird ausdrücklich
  begründet.

**Abnahme:** Messprotokoll und festgelegte Werte liegen vor; nach Einbau
entspricht die Gesamtabklingrate dem dokumentierten Ziel.

### A-C-00091: Ergebnisvertrag und Bilanz

*Herkunft: IDEE-C-00058.*

Der Ergebnisvertrag wird additiv erweitert; bestehende Felder behalten ihre
Bedeutung.

- Neu: effektive Masse bzw. Trägheit, Zusatzmassenwerte mit Herkunft sowie die
  Dämpfungsbeiträge als eigene Einzelbeiträge in der Kraft- und
  Momentenbilanz mit Status, Methode, Quelle, Unsicherheit und Parametern.
- Die Bilanz weist aus, dass `Σ τ = (M_RB + M_A)·dν/dt + …` gilt und nicht
  mehr `m·a`; die Beiträge werden nicht als bestätigte Messung
  dargestellt.
- Die Beiträge sind in der Gesamtkraft und im Gesamtmoment nicht doppelt
  gezählt.
- Alte Konsumenten (insbesondere die GUI) müssen ohne Änderung weiterlaufen.

**Abnahme:** Ergebnisvertragstests für neue Felder und unveränderte Altfelder;
Bilanz-Konsistenz über Zeitschritte; integrierter GUI/Core-Lauf ohne
Anpassung der GUI.

### A-C-00092: Nachweise und Regression

*Herkunft: IDEE-C-00051, IDEE-C-00059.*

- Analytische Referenzen (Kugel, Ellipsoid, Platte), Energie- und
  Symmetrietests, unveränderte Statik, Abklingversuche, Live-/Batch-Parität
  und Regression der bestehenden Ruder-/Schwert-Szenarien.
- Referenzmodelle Optimist, 420er und Folkeboot.
- Verändertes Steuerverhalten (z. B. Dreh- und Beschleunigungsverhalten des
  420ers beim Ruderversuch) ist erwartet; es wird dokumentiert und kein
  Testwert stillschweigend gelockert.
- Vollständiger nativer Release-Build und CTest im Core; der integrierte
  GUI/Core-Lauf wird als Verträglichkeitsnachweis ausgeführt. Messgating
  wird mit `SEGELPHYSIK_TEST_INSTRUMENTATION=ON` mitgeprüft, falls berührt.
- Es wird keine quantitative Bootsgenauigkeit behauptet. Build- und
  Testerfolg, Messung, externe Validierung und Nutzerabnahme werden getrennt
  ausgewiesen.

## 3. Bekannte Einschränkungen und Risiken

- Eingriff in die Integratorgleichung (Körperrahmen-Translation statt
  skalarer Masse im Weltrahmen); beide Integratorstufen müssen konsistent
  bleiben.
- Alle Koeffizienten sind Schätzungen ohne Messdaten; Abweichungen von
  mehreren zehn Prozent sind möglich.
- Doppelzählung mit den Panelkräften wird nur über die Messung nach
  A-C-00090 begrenzt.
- Konstante Zusatzmasse ignoriert den Einfluss von Krängung und Eintauchung.
- Frequenzabhängigkeit der Zusatzmasse (Wellenbildung) ist nicht modelliert.
- Diagonale Matrix: Kopplungen (z. B. Querrichtung und Gieren) fehlen.

## 4. Herkunft der Anforderungen

| IDEE-ID | A-ID | Gegenstand |
|---|---|---|
| IDEE-C-00051 | A-C-00092 | Ort, Zielversion und Nachweise (Zielversion C-0.7) |
| IDEE-C-00052 | A-C-00086, A-C-00087 | Bestimmung der Zusatzmassen und Überschreiben |
| IDEE-C-00053 | A-C-00086 | Beitragende Körper |
| IDEE-C-00054 | A-C-00088 | Diagonalmatrix, Munk-Terme, Integratoreingriff |
| IDEE-C-00055 | A-C-00086 | Konstanz am Referenzzustand |
| IDEE-C-00056 | A-C-00087, A-C-00089, A-C-00090 | Dämpfungsmodell, Gieren, ζ |
| IDEE-C-00057 | A-C-00090 | Messung und Doppelzählung |
| IDEE-C-00058 | A-C-00091 | Ergebnisvertrag und Bilanz |
| IDEE-C-00059 | A-C-00092 | Abnahme ohne Messdaten |
