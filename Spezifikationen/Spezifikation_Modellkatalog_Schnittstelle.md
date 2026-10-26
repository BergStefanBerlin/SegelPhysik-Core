# Spezifikation Modellkatalog-Schnittstelle Core → GUI

**Status: FINAL – freigegeben am 26.10.2026, umgesetzt und per CTest nachgewiesen in C-0.2 (uncommittet); Nutzerabnahme offen.**
**Anforderungen:** A-C-00042 bis A-C-00046
**Umsetzungsplan:** [Umsetzungsplan C-0.2](../Umsetzungsplaene/Umsetzungsplan_C-0.2.md)
**Entwurfspunkte:** [Modellkatalog-Schnittstelle](Entwurfspunkte/Modellkatalog_Schnittstelle.json) (IDEE-C-00020 bis IDEE-C-00026, entschieden)

## 1. Zweck und Abgrenzung

Die GUI liest Modelle nicht mehr selbst aus Dateien. Der Core liest den
Modellordner, prüft die Modelle und reicht über eine einfache Schnittstelle
ausschließlich GUI-relevante Informationen weiter. Damit gibt es nur einen
Leser des Modellformats, und eine spätere Web-GUI kann dieselbe
Schnittstelle über eine dünne Transportschicht nutzen.

Die Schnittstelle ist zweistufig:

1. **Katalog:** der gesamte Objektkatalog des Ordners, nur Hauptobjekte
   (Modelle) mit wenigen Kopfangaben.
2. **Details auf Abruf:** Struktur, Darstellungsgeometrie und Diagnosen
   eines Modells erst bei Anforderung durch die GUI.

**Bei der GUI bleiben:** Ordnerwahl, Ordnerüberwachung, Anzeige, Auswahl
und Zwischenspeicher. Der Core öffnet keine Dialoge.

**Nicht Teil:** Netzwerktransport, Modellbearbeitung, Simulationsergebnisse
([Schnittstelle Core → GUI](../Dokumentationen/Schnittstelle_Core_GUI.md))
und Kraftdaten ([Kraft- und Momentenbilanz](Spezifikation_Kraft_und_Momentenbilanz.md)).

Bezug: [Modellbeschreibung](Spezifikation_Modellbeschreibung.md)
(A-C-00001, A-C-00003, A-C-00030), [Hydrostatikkern](Spezifikation_Hydrostatikkern.md)
(A-C-00017, A-C-00028, A-C-00031), IDEE-C-00006.

## 2. Anforderungen

| ID | Kurztitel | Version | Status |
|---|---|---|---|
| A-C-00042 | Nur GUI-relevante Informationen | C-0.2 | umgesetzt |
| A-C-00043 | Objektkatalog mit Hauptobjekten | C-0.2 | umgesetzt |
| A-C-00044 | Details auf Abruf | C-0.2 | umgesetzt |
| A-C-00045 | Bezug zwischen Katalog und Simulation | C-0.2 | umgesetzt |
| A-C-00046 | Transportunabhängige, versionierte Schnittstelle | C-0.2 | umgesetzt |

### A-C-00042: Nur GUI-relevante Informationen

**Version:** C-0.2 · **Status:** umgesetzt

Über die Schnittstelle gehen ausschließlich Angaben, die die GUI für
Auswahl, Anzeige und Bedienung braucht. Volumennetze für die Hydrostatik,
Trägheitstensoren, Schnittpuffer und andere interne Rechendaten werden nicht
übergeben. Dateipfade sind kein Bestandteil der Antworten; Modelle werden
über ihre Modell-ID angesprochen.

**Abnahme:** Vertragstests stellen fest, dass Katalog- und Detailantworten
nur die dokumentierten Felder enthalten.

### A-C-00043: Objektkatalog mit Hauptobjekten

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** IDEE-C-00020, -00021, -00022, -00025

Aufruf `modellkatalog(ordner)` mit dem von der GUI gewählten Ordner. Ein
Hauptobjekt ist ein Modell, also eine `modell.json` direkt im Ordner oder in
einem unmittelbaren Unterordner. Das Ergebnis ist ein Katalogobjekt, das die
GUI hält; der Core führt keinen globalen Zustand. Je Hauptobjekt:

| Feld | Bedeutung |
|---|---|
| `modell_id` | ID aus der Modellbeschreibung |
| `name` | Anzeigename aus der Modellbeschreibung |
| `beschreibung` | Optionaler Kurztext, sofern im Modell vorhanden |
| `modell_fassung` | Inhaltsfingerprint wie im Ergebnisvertrag |
| `status` | `verwendbar` oder `fehlerhaft` |
| `fehleranzahl` | Anzahl der Diagnosen; Details auf Abruf |

Die Reihenfolge ist stabil (nach Ordnername). Fehlerhafte Modelle erscheinen
im Katalog als `fehlerhaft` (A-C-00031). Tragen mehrere Modelle dieselbe
`modell_id`, ist das ein Fehler mit Fehler-ID; alle betroffenen Modelle
werden als `fehlerhaft` geführt. Ein nicht lesbarer Ordner liefert eine
Fehlerantwort mit Fehler-ID, keine leere Liste.

**Abnahme:** Der Beispielordner liefert alle Modelle in stabiler
Reihenfolge; ein fehlerhaftes Modell und ein ID-Duplikat erscheinen als
`fehlerhaft` mit `fehleranzahl > 0`.

### A-C-00044: Details auf Abruf

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** IDEE-C-00023, -00024

| Aufruf | Liefert |
|---|---|
| `modellstruktur(modell_id)` | Gruppen und Elemente mit IDs und Namen, Gesamtmasse, Hüllquader im Modellbezug |
| `darstellungsgeometrie(modell_id[, element_id])` | Je Element Eckpunkte und Dreiecke im Modellbezug in Metern, Farbe sofern im Modell angegeben, Herkunft `darstellung` oder `volumen_ersatz` |
| `modelldiagnose(modell_id)` | Diagnosen mit Fehler-ID, Text und Fundstelle |

Fehlt ein Darstellungsnetz, liefert der Core das Volumennetz und kennzeichnet
dies als `volumen_ersatz`. Für fehlerhafte Modelle liefert der Core die
Geometrie, soweit sie lesbar ist („nur Ansicht“ in der GUI). Die
Darstellungsgeometrie hat keinen Einfluss auf die Physik (A-C-00017).
Die Daten stehen als C++-Strukturen für die Desktop-GUI und als
JSON-Serialisierung für Web und Tests bereit; eine Binärform folgt erst bei
gemessenem Bedarf.

**Abnahme:** Die Geometrie der Referenzmodelle stimmt mit der heutigen
GUI-Darstellung überein (gleiche Eckpunkt- und Dreieckszahl, gleiche Lage).

### A-C-00045: Bezug zwischen Katalog und Simulation

**Version:** C-0.2 · **Status:** umgesetzt

Prüfung und Simulation lassen sich über das Katalogobjekt und die Modell-ID
starten. Jede Detail- oder Laufanfrage nennt die erwartete `modell_fassung`.
Hat sich die Modelldatei inzwischen geändert, weist der Core die Anfrage
mit eindeutiger Fehler-ID zurück.

**Abnahme:** Nach Änderung einer Modelldatei liefert eine Anfrage mit der
alten Fassung die Fehler-ID; ein neuer Katalogabruf liefert die neue Fassung.

### A-C-00046: Transportunabhängige, versionierte Schnittstelle

**Version:** C-0.2 · **Status:** umgesetzt · **Herkunft:** IDEE-C-00024, -00026

Die Schnittstelle ist eine Bibliotheksfunktion ohne Netzwerkanteil. Die
JSON-Antworten tragen das Feld `katalogversion` (zunächst `"1"`), unabhängig
von der Vertragsversion der Simulationsergebnisse. Eine spätere
Web-Anbindung reicht dieselben Antworten ohne eigene Fachlogik weiter.
Die Beschreibung steht in `Dokumentationen/Schnittstelle_Core_GUI.md`
(A-C-00028); neue Fehler-IDs stehen im Fehlerkatalog.

**Abnahme:** Vertragstests prüfen die JSON-Antworten ohne GUI; Doku,
Beispiele und Fehlerkatalog stimmen mit der Implementierung überein.
