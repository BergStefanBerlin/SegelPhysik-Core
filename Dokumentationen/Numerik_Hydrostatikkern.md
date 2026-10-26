# Numerischer Arbeitsstand des Hydrostatikkerns

Diese Implementierungsbeschreibung ergänzt die
[Spezifikation](../Spezifikationen/Spezifikation_Hydrostatikkern.md).
Sie ändert keine fachlichen Anforderungen und ersetzt keine
fachliche Abnahme.

## Vorläufige technische Entscheidungen

- Alle Rechengrößen sind SI-Größen; Rotationen verwenden rad bzw.
  normierte Quaternionen in xyzw-Reihenfolge, Modell nach Welt.
- Der Rechenpfad verwendet derzeit `double` und präzise
  Gleitkommaoptionen. Ein einfacher Vergleichsbenchmark prüft zusätzlich
  `float`; eine vollständige Mehrformat-Physikbibliothek wird nicht
  allein für diesen Vergleich aufgebaut.
- Die statische Rechnung löst Höhe, Krängung und Trimm mit numerisch
  bestimmter Jacobi-Matrix und gedämpfter Newtoniteration.
  Die Anfangshöhe wird zunächst vertikal eingeklammert.
  Welt-X/Y und Kurs bleiben fest.
- Kraft- und Momentengrenzen sind in der Solver-API getrennt
  konfigurierbar, im ersten CLI-Lauf jeweils 0,001 N bzw. 0,001 N·m.
  Es gibt keinen erfolgsförmigen Stagnations-Fallback.
- Die Dynamik verwendet einen expliziten Mittelpunktansatz mit
  zwei Kraftauswertungen pro Schritt. Der Schwerpunkt wird integriert;
  daraus wird die Lage des festen Konstruktionsursprungs rekonstruiert.
  Rotation wird mit Quaternioninkrementen fortgeschrieben.
  Es gibt keine künstliche Dämpfung.
- Der Wasserschnitt integriert orientierte Dreiecksmomente relativ
  zu einem Referenzpunkt auf der Schnittebene nahe der Geometrie.
  Schnittdeckflächen liefern in diesem Bezug keinen Volumenbeitrag.
  Trocken- und Vollnassfälle besitzen eigene Pfade.
  Die Volumensumme verwendet kompensierte Summation.

## Genauigkeit und Grenzen

Zentimeter und Kilogramm bleiben die vereinbarten fachlichen
Orientierungen, keine Rasterung der Eingaben oder Zwischenrechnungen.
Für diese Referenzfälle werden deutlich strengere numerische Grenzen
geprüft: analytische Quaderschnitte und Schwerpunktlagen, freier Fall
und Schrittweitenkonvergenz einer ungedämpften hydrostatischen Bewegung.
Eine Grenze von 1 mm wird im dynamischen Höhenvergleich gegen eine
feinere Rechnung geprüft. Zusätzlich ist eine Verringerung des Fehlers
bei halbierter Schrittweite nachzuweisen.

Diese Tests sind kein universeller Beweis für jede Geometrie, Beladung
oder Schrittweite. Insbesondere ersetzen sie keine gemeinsam
abgestimmten Genauigkeitsgrenzen für alle übrigen Größen. Eine
Faktor-10-Gesamtabnahme bleibt an den definierten Anwendungsbereich,
unabhängige Referenzen und vereinbarte Fehlerbudgets gebunden.

Bei praktisch singulärer Jacobi-Matrix wird Nichtkonvergenz gemeldet.
Eine dynamische Trägheit muss positiv definit sein; sie wird nicht
künstlich regularisiert. Statische Konvergenz weist ein Kräftegleichgewicht
nach, aber noch nicht automatisch dessen dynamische Stabilität.
Eine große Schrittweite kann auch bei gültigem Modell instabil sein.

## Performancebelege

`core_benchmark` führt Aufwärmung und neun Messwiederholungen mit
je 500 Hydrostatikauswertungen aus und gibt Median, Minimum, Maximum
und Auswertungen pro Sekunde aus. Die Kontrollsumme verhindert,
dass Ergebnisse unbenutzt bleiben.
Ein kleiner Zahlenformatvergleich erfasst Laufzeit und
Volumenproduktfehler bei gegenüber der Geometrie großen Koordinaten.
Er ist ein Indikator für Auslöschung, kein vollständiger Solververgleich.

JSONL und ein JSON-Array werden beim Zusammenstellen von 1000
identischen, tatsächlich serialisierten Zuständen verglichen.
Dieser Test misst Zusammenstellung, nicht Parsergeschwindigkeit,
Dateisystem oder GUI-Rendering. Ergänzend misst der Suite-Treiber
vollständige kleine Dateiaufzeichnungen mit 1000 Schritten in sieben
Wiederholungen. Die Mediane von etwa 33,8 ms (JSONL) und 34,7 ms
(JSON) besitzen überlappende Streuungen. JSONL bleibt deshalb ein
vorläufiger Arbeitsstandard, kein Beweis universeller Formatoptimalität.
Die [aktuellen Nachweise](Umsetzungsnachweise_Hydrostatikkern.md)
verlinken die Rohdaten beider Messläufe.
