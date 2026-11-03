# Core: Performance der Modell-, Geometrie- und Ergebnisaufbereitung

**Status:** Finalisiert und zur vollstaendigen Umsetzung freigegeben,
03.11.2026, Nutzerauftrag 20:45 Uhr. Zielversion C-0.10 bestaetigt.
Einzige Spezifikation dieses Optimierungsthemas; offene Punkte und
Entwurfskennungen stehen in der
[Entwurfspunkteliste](Entwurfspunkte/Performance_Modellaufbereitung.json).
Zielversion C-0.10, ohne Git-/Release-Auftrag.
Einziger zugehoeriger [Plan](../Umsetzungsplaene/Umsetzungsplan_C-0.10.md).

## 1. Ziel und Abgrenzung

Die nachgewiesenen Wiederholungen der Modell-/Geometrievorbereitung
vermeiden, ohne physikalische Berechnungen abzuschwaechen. Unveraenderte
Modellinformationen einmal vorbereiten und teilen; nur von geaenderten
Eingaben abhaengige Daten erneuern. Der Nutzen muss auch fuer komplexe
Meshes wie den 420er nachgewiesen werden.

Ausgangspunkt sind die [Befunde C-0.9, 4.1-4.3](../Umsetzungsplaene/Umsetzungsplan_C-0.9.md#41-modellinventar-und-vergleichsgrenzen).
Diese Analyse ist Quelle, kein paralleler Optimierungsplan.
GUI-Arbeit steht ausschliesslich in der
[GUI-Spezifikation](../../GUI/Spezifikationen/Spezifikation_Performance_Darstellung.md).

**Ausgeschlossen:** Optimierung des statischen Gleichgewichts- und
Kurvenfahrtsolvers (Suchverfahren, Jacobi, Startwerte, Iterationszahl,
Toleranzen, Abbruchbedingungen), neuer Integrator, Windphysik,
Meshreduktion, groebere Schnitte, geaendertes dt, weniger Core-Schritte
oder weniger vorgeschriebene Ergebnisdatensaetze. Statische Pfade duerfen
gemeinsame unveraenderliche Aufbereitung nutzen, ohne ihren fachlichen
Algorithmus zu aendern; sie sind als Regression mitzupruefen.

## 2. A-C-00129: Einmalige Modellbasis

Nach erfolgreicher Modellvalidierung eine unveraenderliche Modellbasis
fuer Topologie, Originalmeshes, Ableitungen, lokale Geometriedaten und
modellfeste Massenintegrale halten. Unveraenderte grosse Mesharrays nicht
bei jeder Kraftauswertung oder Integratorstufe tief kopieren.
Gleiche Daten duerfen innerhalb eines Rechenfalls von Integrator,
Kraftauswertung und Ergebnisausgabe gemeinsam gelesen werden.

Keine globale, unbegrenzt wachsende Cacheverwaltung einfuehren.
Lebensdauer und Eigentum an Modell/Lauf binden; abgeschlossene Laeufe
muessen freigebbar bleiben. Verschiedene Modelle/Laeufe und parallele
Worker duerfen keine veraenderlichen Daten teilen. Modellfassung,
Geometrieinhalt, Ableitungsdefinition und relevante Konfiguration sind
Teil der Identitaet; Dateiname oder Element-ID allein reicht nicht.
Bestehende Validierung und Diagnosen unveraendert erhalten.

## 3. A-C-00130: Stellungsabhaengige Vorbereitung

Das physische Modell aus festen Ruder-/Schwertstellungen einmal je
tatsaechlich benoetigtem Stellungszustand vorbereiten. Bewegte Geometrie,
Massenmittelpunkt und Traegheit muessen dieselbe Modell-/Stellungsfassung
verwenden. Unveraenderte Stellungen erzeugen keine neue tiefe Modellkopie,
Meshtransformation oder Massenaggregation pro Integratorstufe.

Bei Bewegungsereignissen nur geaenderte Elemente und ihre transitiven
Abhaengigkeiten erneuern: mitbewegte Kinder, Gruppen sowie globale
Schwerpunkt-/Traegheitsaggregate. Ein identischer effektiver Stellwert
darf keine Geometrieneuberechnung ausloesen; Ereignisquittierung und
Historie muessen trotzdem korrekt fortgeschrieben werden.
Modellwechsel, Konfigurationswechsel und geaenderte Ableitungen
invalidieren die jeweils betroffenen Daten.

## 4. A-C-00131: Zwingend zustandsabhaengige Arbeit

| Datenklasse | Wiederverwendung | Erneuerung |
|---|---|---|
| Modelltopologie, lokale konstante Integrale | gleiche validierte Modellbasis | neue Modell-/Geometriefassung |
| Bewegte lokale Meshes, physische Masse/CG/Traegheit | gleiche Modellbasis und effektive Stellungen | Stellung oder abhaengiges Element geaendert |
| Weltgeometrie, Wasserlinienschnitt, Benetzung | nur exakt identische wirksame Eingaben | Pose, Wasserebene, Stellung oder relevante Fluidkonfiguration geaendert |
| Kraefte/Bilanz | exakt gleicher Auswertungszustand und Kontext | unter anderem Geschwindigkeit, Rotation, Zeit, Kraft-/Hydro-/Flutungskontext geaendert |

Ein gleicher Stellwert ist kein gleicher Bootszustand. Anfang,
Mittelpunkt und Ende eines Integrationsschritts nicht gleichsetzen.
Nasspanels, Flutungsstatus und lokale Rotationsanstroemung nicht aus
veralteten Posen wiederverwenden. Die bestehende Aktualisierungssemantik
der Zusatztraegheit bleibt erhalten; dies ist keine Freigabe zur
physikalischen Neudefinition ihrer Pose-/Ereignisabhaengigkeit.

## 5. A-C-00132: Komplexe Meshes

Exakte Vorarbeiten wie lokale Bounds und topologische Hilfsdaten nur
erneuern, wenn ihre Eingaben wechseln. Kopien grosser Arrays und
mehrfache identische Transformationen vermeiden. Geometrie, Topologie,
Ableitungen, Orientierung, Schnittgenauigkeit und Zahl der physikalisch
wirksamen Flaechen bleiben unveraendert.
Kein approximatives Culling, LOD oder Austausch von Physikmeshes.
Ein Cache fuer alle Posen einer Historie ist nicht das Ziel: nur
begrenzt benoetigte Auswertungsdaten halten.

## 6. A-C-00133: Kraftauswertung und Ergebnisaufbereitung

Eine bereits vorliegende vollstaendige Auswertung darf fuer Ausgabe
und Bilanz desselben Zustands genutzt werden, anstatt `kraefte()`
erneut aufzurufen. Voraussetzung ist explizite Identitaet aller
wirksamen Eingaben, nicht nur Zeitstempel oder Schrittindex.
Integrator-Mittelpunktkraefte duerfen nicht als Endzustandskraefte
ausgegeben werden. Liegt keine passende Auswertung vor, muss der
Zustand regulaer ausgewertet werden.

Batch und GUI-Livepfad abdecken. JSON-Felder, Reihenfolge der
Ergebniszustaende, Kontexte, Diagnoseverhalten und Vollstaendigkeit von
Beitraegen/Summen/Druckpunkten bleiben kompatibel. Rueckblick nach
spaeteren Bewegungsereignissen muss den damaligen Zustand auswerten,
nicht die aktuelle Worker-Geometrie. Bestehende oeffentliche APIs
moeglichst erhalten; konkrete neue exportierte Interfaces erst nach
Finalisierung im projektlokalen CPPI-Register vergeben.

## 7. A-C-00134: Nachweise und Abnahme

- Testzaehler unterscheiden Basisaufbau, Stellungsaufbereitung,
  Meshkopie/-transformation, Kraftauswertung und Ergebniswiederverwendung.
  Sie duerfen Zeitmessungen nicht dominieren; Zaehlerlauf und Release-
  Timing getrennt dokumentieren.
- Feste Stellungen: eine logische Vorbereitung je benoetigter
  Modell-/Stellungsfassung, keine erneute tiefe Vorbereitung fuer
  unveraenderte Integratorstufen; notwendige Schnitt-/Kraftauswertungen
  bleiben nachweisbar vorhanden.
- Ereignisse, identische Stellwerte, Modellaustausch, Konfigurationswechsel,
  Rueckblick, parallele Laeufe und Fehlerpfade pruefen.
- Vorher/nachher dieselben aktuellen Schema3-Modelle 420/Folkeboot,
  Startzustaende, Kraefte, dt und Horizonte; ein Warmup, mindestens fuenf
  Wiederholungen, Rohwerte/Median/Min/Max. Laden, Schritte,
  Kraftauswertung/JSON und Gesamtlauf getrennt ausweisen.
- Gemeinsame Altfaelle free/drive/hydro/hydro_drive sowie heutige
  Fremdkraft aktiv/inaktiv und feste/geaenderte Stellungen abdecken.
  Kurzlauf 0,5 s mit dt 0,01 s und laengerer Lauf 5 s; instabile oder
  ungueltige Faelle explizit melden, nicht stillschweigend austauschen.
- Numerische Referenzen fuer Position, Orientierung, Geschwindigkeiten,
  Kraefte, Momente, CG/Traegheit, Nassgeometrie und Kontexte mit bestehenden
  fachlichen Toleranzen vergleichen; keine Toleranzlockerung fuer Tempo.
  Bestehende offene Physikabnahmen bleiben offen.
- Performanceziel: Wiederholungsarbeit per Zaehler beseitigt und
  messbarer Nutzen in den betroffenen grossen Modellfaellen, ohne
  reproduzierbare Regression anderer Faelle. Kein erfundener Sollfaktor.
  Liegt eine Differenz in der Streuung, ist Beschleunigung nicht belegt.

## 8. Freigabegate

Die Nutzerfreigabe vom 03.11.2026, 20:45 Uhr finalisiert saemtliche
Punkte einschliesslich der 5-s-Matrix ohne Mindestbeschleunigungsfaktor.
Die Entwurfspunkteliste bewahrt IDEE-C-00093..00098 und ihre Zuordnung
zu A-C-00129..00134. Abnahmekriterien stehen jeweils im zugehoerigen
Abschnitt und gemeinsam in Abschnitt 7. Keine Git-Veroeffentlichung.
