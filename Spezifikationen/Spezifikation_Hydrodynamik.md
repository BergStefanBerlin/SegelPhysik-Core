# Spezifikation CORE: Hydrodynamik des Segelfahrzeugs

**Status: C-0.6 umgesetzt; Anforderungen A-C-00073 bis A-C-00078 fachlich finalisiert und in Umsetzung. Anforderungen A-C-00079 bis A-C-00085 zur dynamischen Verstellung von Ruder und Schwert sind fachlich finalisiert; Umsetzung und Abnahme sind offen. Keine Commit-, Push-, Tag- oder Release-Freigabe.**

**Stand:** 28.10.2026

**Nachtrag:** Abschnitt 12 enthält die finalisierten Anforderungen
A-C-00073 bis A-C-00078 für den Folgeumfang nach C-0.6. Abschnitt 13 enthält
die Anforderungen A-C-00079 bis A-C-00085 für die spätere Bewegungsstufe.
Diese Anforderungen erweitern nicht rückwirkend den abgeschlossenen
C-0.6-Plan. Der
[Umsetzungsplan C-0.6](../Umsetzungsplaene/Umsetzungsplan_Hydrodynamik.md)
gilt für seine eigene Version; der Folgeumfang ist im
[Umsetzungsplan C-0.7](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md)
geordnet. Die Umsetzung erfolgt im Core. GUI-, Commit-, Push-, Tag- oder
Release-Aktionen sind hiervon nicht freigegeben.

Abschnitt 13 ergänzt den späteren Umfang um bewegliche Ruder und Schwerter,
einschließlich eines GUI-Steuer-Overlays. Er erweitert nicht rückwirkend
C-0.7 und ist nicht im C-0.7-Umsetzungsplan enthalten. Die nötige GUI-
Integration wird durch diese Core-Spezifikation fachlich beschrieben, aber
nicht zur Umsetzung freigegeben. Die bisherigen Festlegungen zum festen
Ruder gelten für C-0.7; für den späteren Bewegungsumfang gilt Abschnitt 13.
Der separate
[Umsetzungsplan C-0.7, Teil B](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md)
ordnet die Core-Arbeiten; ein GUI-Plan liegt im GUI-Repository.

**Entscheidungsherkunft und Restpunkte:** [Hydrodynamik](Entwurfspunkte/Hydrodynamik.json)

Diese Spezifikation beschreibt die schrittweise Erweiterung des eigenständigen
SegelPhysik-CORE um hydrodynamische Kräfte. Abschnitt 11 enthält die
Anforderungen der umgesetzten Stufe C-0.6; Abschnitt 12 enthält die
fachlich finalisierten Anforderungen für C-0.7; Abschnitt 13 enthält
Anforderungen für eine nachfolgende Bewegungsstufe. Herkunft und
technische Restpunkte stehen in der verlinkten Entwurfspunkteliste. Der
bestehende Umsetzungsplan gilt für C-0.6; der Folgeumfang ist im separaten
C-0.7-Plan geordnet. Abschnitt 13 beschreibt einen danach liegenden
Bewegungsumfang. Die fachliche Finalisierung erteilt weder einen
Umsetzungs-, GUI-, Commit-, Push-, Tag- oder Release-Auftrag.

## 1. Ziel und Architekturgrenze

Die hydrodynamische Physik, ihre Auswertung und die daraus entstehenden
Kraft- und Momentenbeiträge gehören vollständig in den CORE. Die Berechnung
muss headless und unabhängig von Python, Qt, GUI, Rendering und
Darstellungsdateien funktionieren. Die GUI bleibt Konsument der Core-Zustände
und berechnet oder korrigiert keine Hydrodynamik.

Im Umfang C-0.7 ist keine neue GUI-Eingabe oder Rudersteuerung vorgesehen:
Die GUI übergibt das Modell und verwendet Pose und Geschwindigkeit aus den
bestehenden Simulationsergebnissen. Der CORE muss die neuen Kraftbeiträge
dennoch über einen dokumentierten, stabilen Vertrag bereitstellen, damit
eine spätere GUI-Ausbaustufe diese anzeigen kann.
Spezielle Darstellung und Bedienung der neuen Kraftgruppen sind nicht Teil
dieses Umfangs.

Die aktuelle GUI liest die generische Vektorliste und behandelt unbekannte
Kraftquellen als „weitere Quelle“. Neue Kraftbeiträge können deshalb bereits
mit der vorhandenen generischen Darstellung sichtbar werden. Dieses
Bestandsverhalten bleibt unverändert. Neue GUI-Eingaben, eine speziell
gestaltete Hydrodynamikgruppe oder zusätzliche Darstellungen sind zunächst
nicht vorgesehen; die Körperbewegung und Kraftberechnung erfolgen im CORE.

Die bestehende Hydrostatik bleibt erhalten. Hydrodynamische Beiträge dürfen
Gewicht, Auftrieb oder andere Beiträge weder ersetzen noch doppelt zählen.
Ein fehlender oder ungültiger hydrodynamischer Beitrag darf nicht still als
Nullkraft erscheinen.

## 2. Festes Ruder und Bewegungsgrenze im C-0.7-Umfang

Das Ruder bleibt im C-0.7-Umfang in seiner derzeitigen Modellkonfiguration.
Es gibt in diesem Umfang keine unabhängige Ruderbewegung, kein Stellgesetz, keine
Ruderwinkelsteuerung und keine Ruderaktuatordynamik. Das Ruder ist relativ zum
Rumpf fest; bewegt oder dreht sich das Gesamtmodell, folgt es ausschließlich
dieser Starrkörperbewegung. Die spätere Bewegungsstufe ist in Abschnitt 13
mit separaten Anforderungen definiert.

Das Ruder folgt der starren Rumpfbewegung und behält seine vorhandene
Modellorientierung. Es gibt keinen hydrodynamischen Ruderwinkel als Eingabe.
Hydrodynamische Kräfte am feststehenden Ruder werden erst nach dem
fachlichen Bewegungsnachweis mit validiertem Rumpf-Restwiderstand und
Kiel-Seitenkraft in einer eigenen Stufe untersucht. Die Bewegungsgrenze
bedeutet nicht, dass Ruderkräfte bereits im ersten Rechenschritt enthalten
sind.

## 3. Fachliche Grundlage und Quellenbewertung

Als Quellen dienen:

- [Hydrodynamik eines Bootes](../../Ideen/hydrodynamik_boot.pdf): kompakter
  Vorschlag für Rumpf, Kiel, feststehendes Ruder, stationäre Fahrt und
  Simulation.
- [Berechnungsvorschrift Hydrodynamik Rev. 2](../../Ideen/Berechnung_Hydrodynamik_2_2.md):
  differenziert zwischen stumpfem Einzelkörper, Rumpf, Tragflächen und
  Bauteilbilanzen und beschreibt Anwendbarkeit und Unsicherheiten.

Die Markdown-Quelle ist die bessere fachliche Grundlage für die Trennung der
Bauteilmodelle. Das PDF ist nützlich als anschauliche Reihenfolge und
Rechenmodell, seine vereinfachten Platzhalter sind aber keine freigegebenen
CORE-Verträge. Insbesondere ist der Ansatz `Cr = c_rest * Fr^4` ohne
festgelegte und validierte Koeffizienten keine belastbare
Wellenwiderstandsvorhersage. Die Partikelkorrelation aus Teil A der
Markdown-Quelle liefert Widerstand, aber keine Seitenkraft und ist daher
kein Ersatz für ein Kiel- oder Rudermodell.

Die dokumentierten Genauigkeiten sind konzeptuelle Erwartungen, keine
Abnahmetoleranzen. Eine Korrelation darf nur innerhalb ihres belegten
Gültigkeitsbereichs benutzt werden. Quellen, Modellgrenzen und bekannte
Unsicherheiten müssen in der späteren Spezifikation und den Nachweisen
nachvollziehbar bleiben.

Für die ITTC-1957-Reibungslinie ist die
[ITTC Recommended Procedure 7.5-02-02-01](https://ittc.info/media/11780/75-02-02-01.pdf)
die maßgebliche Formelsammlung. Für DSYHS ist die Regression von
[Remmlinger, Bare Hull Resistance Prediction Based on the Delft Systematic
Yacht Hull Series (2014)](https://www.remmlinger.com/yacht/delftseries/BareHullResistancePredictionBasedOnTheDSYHS.pdf)
eine zugängliche Methodenquelle. Eine Regression darf nur innerhalb der
belegten Parameterdomäne der Versuchsserie verwendet werden.

Öffentlich geführte Referenzdaten nennen für das Folkeboot etwa 6,0 m
Wasserlinienlänge; daraus folgt die bekannte theoretische
Verdränger-Rumpfgeschwindigkeit von etwa 5,94 kn
([SailboatLab](https://sailboatlab.com/data_sheet/5206/0/)). Das ist keine
garantierte Höchstgeschwindigkeit und kein Widerstands- oder Polardatensatz.
Das vorhandene Folkeboot-Modell hat eine physische Mesh-Länge von 7,68 m und
ist als Entwurf gekennzeichnet. Die externen Abmessungen dienen deshalb nur
als Plausibilitätskontrolle, nicht als Ersatz für das Mesh oder als
Kalibrierwert. Ebenso sind die vorhandenen Optimist- und 420er-Meshes
Entwurfsdaten und nicht als Schleppversuch validiert.

Als verfügbare Referenzmodelle stehen der Optimist und der 420er als Jollen
sowie das Folkeboot als Kielboot zur Verfügung. Dies sind die besten Modelle,
die für die Arbeit zu erwarten sind; die Umsetzung darf weder realistischere
Geometrien voraussetzen noch deren spätere Beschaffung als Voraussetzung
setzen. Modellqualität und physische Mesh-Eignung werden für genau diese
Modelle untersucht und ihre Grenzen dokumentiert. Ergebnisse sind zwischen
Jollen und Kielboot getrennt zu bewerten; eine einzelne Modellklasse
repräsentiert nicht automatisch die andere.

## 4. Vorgeschlagene schrittweise Reihenfolge

Die Reihenfolge ist im maßgeblichen
[Umsetzungsplan C-0.6](../Umsetzungsplaene/Umsetzungsplan_Hydrodynamik.md)
in abhängigkeitgerechte Arbeitsschritte zerlegt.
Jede Stufe muss für sich fachlich und numerisch prüfbar sein. Nicht
implementierte spätere Stufen dürfen weder durch stillschweigende Ersatzwerte
noch durch als vollständig bezeichnete Gesamtergebnisse vorgetäuscht werden.

### Schritt 0: Verträge und Eingangsgrößen klären

Vor produktiver Physik werden die bestätigten Daten- und Ergebnisverträge
technisch konkretisiert. Dauerhafte Hydrodynamik- und Profilangaben gehören
in den bestehenden `modell.json`-Vertrag und werden in die vorhandene
Modellbeschreibungsspezifikation integriert. Geometriewerte, die sich
zuverlässig aus physischen Meshes ableiten lassen, werden nicht redundant
gespeichert. Die konkrete Zuordnung zu den aktuellen Elementen und die
Qualität der drei verfügbaren Referenzmodelle sind zu prüfen.

Physik darf nicht aus Anzeigegeometrie oder Dateinamen abgeleitet werden.
Fehlende benetzte Fläche, LWL, Profil- oder Bauteilzuordnung muss explizit
festgestellt werden, nicht durch erfundene Ersatzwerte geschlossen werden.
Unzureichende Modelldaten sind als Einschränkung oder konkreter
Validierungsfehler auszuweisen; es wird keine bessere Geometrie vorausgesetzt.

### Schritt 1: Isolierter Rumpf-Reibungsbeitrag

Als kleinster prüfbarer Einstieg wird ein eigenständiger CORE-Rechenbaustein
für den Reibungswiderstand des Rumpfs vorgeschlagen. Dafür wird die
ITTC-1957-Reibungslinie verwendet:

```text
Re = V * Lwl / nu
Cf = 0.075 / (log10(Re) - 2)^2
Rf = 0.5 * rho * V^2 * Sw * Cf
```

`V` ist der Betrag der Relativgeschwindigkeit am Gesamtschwerpunkt bei
räumlich gleichförmiger Fluidgeschwindigkeit. `Lwl` bezeichnet die
Wasserlinienlänge, `Sw` die benetzte Fläche, `rho` die Fluiddichte und `nu`
die kinematische Viskosität. Das Ergebnis ist ein Widerstand entgegen der
Relativströmung. Ein Formfaktor, Wellenwiderstand
oder empirischer Zuschlag ist für diesen isolierten ersten Nachweis nicht
enthalten. Als vorläufiger Arbeitsbereich gilt `10^6 <= Re <= 10^9`. Bei
positiver Geschwindigkeit außerhalb dieses Bereichs wird der
Reibungsbeitrag als `nicht_berechnet` mit `CORE-HYD-0001` ausgegeben; die
Simulation kann fortgesetzt werden, aber die Kraftbilanz muss als unvollständig
gekennzeichnet bleiben. Es wird weder extrapoliert noch still ein Ersatzwert
verwendet. Bei `V = 0` entsteht ein gültiger Nullbeitrag ohne Auswertung von
`Cf`. Fluideigenschaften und Geometrie müssen endlich und physikalisch
zulässig sein.

Die Berechnung verwendet ausdrücklich dokumentierte, prüfbare Geometriewerte.
LWL und benetzte Fläche dürfen aus einem physischen Mesh abgeleitet werden,
wenn das Verfahren für das betreffende Modell nachweislich geeignet ist;
andernfalls müssen sie als dauerhafte Modellangaben im bestehenden
`modell.json`-Vertrag vorliegen. Eine Ableitung aus Darstellungsmeshes oder
Dateinamen ist ausgeschlossen. Die Datenfelder werden in die bestehende
Modellbeschreibungsspezifikation integriert.

Der isolierte Rumpfbeitrag wird über die bestehende Core-Kraftauswertung in
die dynamische Starrkörperintegration eingespeist. So werden
Kraftberechnung, Momentenübertragung, Integratoraufrufe und
Ergebnisserialisierung von Anfang an gemeinsam getestet. Dieser
Integrationsnachweis ist noch keine fachliche Bewegungsabnahme der Yacht;
diese erfolgt erst, wenn zusätzlich ein belegter Rumpf-Restwiderstand und
die Kiel-Seitenkraft verfügbar sind.

### Schritt 2: Rumpf-Rest- und Wellenwiderstand

Ein Rest- oder Wellenwiderstand wird ergänzt, sobald Methode, Datenquelle,
Parameter, Gültigkeitsbereich und Referenzfälle festgelegt und geprüft sind.
Für die vorhandenen Jollen und das Folkeboot wird eine belegte,
DSYHS-basierte Yachtmethode bevorzugt, sofern deren Anwendbarkeit für den
jeweiligen Rumpf nachgewiesen ist. Andernfalls ist ein anderer validierter
Ansatz auszuwählen. Der unkalibrierte `Cr = c_rest * Fr^4`-Platzhalter ist
keine Vorhersage und darf nicht als solche ausgegeben werden.

### Schritt 3: Seitenkräfte und Widerstand an festen Anhängen

Für den Kiel wird zunächst ein einfaches, nachvollziehbares Kennwertmodell
mit Seitenkraft sowie Profil- und induziertem Widerstand umgesetzt. Als
Alternative darf ein im `modell.json`-Vertrag angegebenes 2D-Profil
verwendet werden, das zunächst über die gesamte Spannweite beziehungsweise
Höhe als gleichförmig angenommen wird. Dafür sind eine geeignete Polare und
explizite Ablösegrenzen erforderlich; ein Profilname allein genügt nicht.
Die Modellwahl berücksichtigt Streckung, Anstellwinkel und die
Starrbefestigung. Wechselwirkungen wie Nachlauf, Abwind, Spiegeleffekt,
Krängung, Ablösung und Junction-Zuschläge werden einzeln begründet und
schrittweise validiert. Die Partikelkorrelation aus Teil A wird dafür nicht
verwendet. Das Ruder bleibt unverändert relativ zum Rumpf fest und wird erst
in einer nachgelagerten Stufe hydrodynamisch berücksichtigt.

Dieser Schritt legt keine bewegliche Ruderstellung fest. Ein variabler
Ruderwinkel oder dessen Optimierung bleibt ausgeschlossen.

### Schritt 4: Beiträge, Momente und Solverintegration

Jeder validierte Kraftbeitrag wird mit stabiler Identität, Quelle und
Angriffspunkt im bestehenden CORE-Kraft-/Momentenmodell geführt. Momente
werden konsistent auf den festgelegten Bezugspunkt übertragen; die
Bilanzierung muss Hydrostatik- und Hydrodynamikanteile je genau einmal
berücksichtigen. Bereits der erste isolierte Rumpfbeitrag wird an die
Dynamikintegration angeschlossen. Mit jeder späteren Komponente wird
dieselbe gemeinsame Schnittstelle erweitert und erneut gegen die tatsächlich
verwendete Kraftauswertung geprüft.

Die bestehenden Ergebnisbeitragsfelder werden additiv genutzt. Bestehende
Pflichtfelder und ihre Bedeutungen bleiben unverändert; die Ergebnisversion
wird nicht allein wegen neuer Quellen- oder Gruppenwerte erhöht. Eine neue
Version ist nur erforderlich, wenn eine inkompatible Änderung an Pflichtfeld
oder Bedeutung unvermeidbar ist. V3 bleibt der separate Testantriebvertrag.
Der Kompatibilitätsnachweis gehört zur Implementierung.

### Spätere, ausdrücklich getrennte Themen

Bewegliches Ruder, VPP-Gleichgewicht mit Segelantrieb, Manöverregelung,
Zusatzmassen, hydrodynamische Dämpfung, freie Oberfläche, Wellenzusatzwiderstand
in Seegang, Wandnähe, Kavitation sowie CFD oder Modellversuche sind nicht Teil
des ersten Einstiegs. Sie dürfen nicht durch pauschale Zuschläge als bereits
modelliert gelten. Zusatzmassen und hydrodynamische Dämpfung werden in der
[Spezifikation Hydrodynamische Zusatzmasse und Dämpfung](Spezifikation_Hydrodynamik_Zusatzmasse_Daempfung.md)
(A-C-00086 bis A-C-00092) geführt.

## 5. Prüf- und Abnahmeprinzipien für die spätere Finalisierung

Vor einer Freigabe werden für jeden Rechenschritt unabhängige Referenzfälle,
Einheiten, Frame- und Vorzeichenkonventionen, Wertebereiche und
Toleranzen benötigt. Tests sollen mindestens abdecken:

- analytische oder unabhängig nachgerechnete Referenzwerte der isolierten
  Formeln;
- Null- und Grenzgeschwindigkeit sowie unzulässige oder nicht endliche
  Eingaben;
- Kraftrichtung entgegen der Wasserrelativströmung;
- Momentenübertragung auf den bestehenden Bezugspunkt und Summenparität;
- Gültigkeitsbereich jeder verwendeten Korrelation;
- Altverhalten und Ergebnisvertragskompatibilität ohne GUI-Änderungen.

Keine konzeptuelle Genauigkeitsangabe aus den Quellen ersetzt diese
Abnahmekriterien. Technische Tests, Integration und fachliche Abnahme sind
getrennt auszuweisen.

## 6. Implementierungsrestpunkte und Nachweisgrenze

Die neun besprochenen fachlichen Entscheidungen sind in der
[Entwurfspunkteliste Hydrodynamik](Entwurfspunkte/Hydrodynamik.json)
dokumentiert; die Liste weist die Herkunft jeder nachfolgenden A-Kennung
aus. Technische Restpunkte sind insbesondere die Eignung konkreter
Geometriedaten, die Anwendbarkeit von DSYHS, fehlende Profilpolaren und
fachliche Toleranzen. Sie heben den erteilten Implementierungsauftrag nicht
auf; wo ein belastbarer Nachweis fehlt, ist die betreffende Modellstufe
explizit als nicht abgenommen beziehungsweise blockiert zu kennzeichnen.

## 7. Soll-Vertrag für Core-Kräfte und -Momente

Dieser Abschnitt konkretisiert den vorgesehenen Core-Vertrag. Er ist
**fachlicher Entwurf**, keine bereits verfügbare API. Feld-/Typnamen sind
stabile Namensvorschläge; ihre endgültige Freigabe erfolgt mit der
Finalisierung. Der tatsächliche Schnittstellenstand steht ausschließlich in
der [Core-GUI-Schnittstellenbeschreibung](../Dokumentationen/Schnittstelle_Core_GUI.md).

### 7.1 Berechnungs- und Integrationsgrenze

Der Hydrodynamikbaustein wertet einen vollständigen, expliziten
Rechenzustand aus: unveränderliche Modellfassung, Modellpose,
Translationsgeschwindigkeit des Gesamtschwerpunkts und konfigurierte
Fluid-/Hydrodynamikdaten. In der ersten Stufe wird die Relativgeschwindigkeit
am Gesamtschwerpunkt verwendet; lokale Geschwindigkeitsanteile aus der
Körperrotation werden als spätere Optimierung zurückgestellt. Die
Integratorzwischenstufen müssen jeweils mit ihrer eigenen Pose und
Translationsgeschwindigkeit ausgewertet werden; das Auswerten nur der Start-
oder Endpose für alle Kraftstufen ist unzulässig.

Der Baustein liefert seine tatsächlichen Einzelbeiträge an die gemeinsame
Core-Kraftauswertung. Der Integrator verwendet deren Summe gemeinsam mit den
bereits bestehenden Kräften. Es gibt keine zweite, nur für Hydrodynamik
geführte Bewegungsgleichung und keinen GUI-seitigen Kraftzuschlag.
Für Modelle ohne aktivierte Hydrodynamik bleibt das bestehende Verhalten
unverändert.

Die betroffenen heutigen Erweiterungspunkte im Core sind:

| Vorhandener Core-Vertrag | Vorgesehene Erweiterung |
|---|---|
| `Zustand` | Als Quelle für Pose sowie lineare und Winkelgeschwindigkeit auswerten; keine GUI-Zwischenrechnung |
| `Kraefte` | Hydrodynamische Einzelbeiträge an dieselbe Kraftauswertung anhängen und in Kraft/Moment einrechnen |
| `Beitrag` | Vorhandene Punkt-, Vektor-, Frame-, ID-, Gruppen- und Momentenfelder wiederverwenden |
| `kraefte(...)` / Dynamikintegrator | Expliziten Hydrodynamik-Kontext und den jeweils aktuellen Integratorzustand berücksichtigen |
| `bilanz(...)` | Erwartete hydrodynamische Beiträge ergänzen und fehlende Berechnung als Teilsumme melden |
| `zustand_json(...)` | Die berechneten Beiträge im dokumentierten Core-Ergebnisvertrag veröffentlichen |

Die API-Erweiterung erfolgt additiv; bestehende Aufrufe ohne
Hydrodynamikkonfiguration müssen weiter das bisherige Verhalten liefern. Die
Implementierung verwendet intern einen typisierten Core-Kontext und erhält
zunächst den bestehenden Simulationsstart. Dauerhafte Yacht- und
Profilparameter gehören in `modell.json`, laufbezogene Fluiddaten werden
explizit an den typisierten Kontext gebunden. Die konkrete öffentliche
Funktionsform wird bei der technischen Vertragskonkretisierung festgelegt.

Der Umfang des ersten Rechenschritts soll auf die Dynamik beschränkt bleiben.
Die vorhandene hydrostatische Statik darf durch einen bei Geschwindigkeit
null nicht definierten Widerstandsbeiwert nicht verändert werden. Ob und wie
später stationäre Strömung oder ein hydrodynamisches Gleichgewicht behandelt
wird, bleibt offen.

### 7.2 Eingaben und Zustandsbezug

Der spätere öffentliche C++-Vertrag soll einen benannten, validierten
Hydrodynamik-Kontext statt globaler Variablen oder impliziter
Modellmetadaten verwenden. Inhaltlich benötigt dieser Kontext mindestens:

| Eingabe | Einheit/Bezug | Vertrag |
|---|---|---|
| Fluiddichte `rho_kg_m3` | kg/m³ | endlich, positiv und für alle Berechnungen des Laufs fest |
| Kinematische Viskosität `nu_m2_s` | m²/s | endlich, positiv und für alle Berechnungen des Laufs fest |
| Fluidgeschwindigkeit | m/s, Weltframe | über den typisierten Kontext; ruhendes Wasser wird explizit als Nullgeschwindigkeit dargestellt |
| Bauteilrolle und Geometrieparameter | SI, Modellframe | eindeutig einem physikalischen Modellelement zugeordnet |
| Referenzlängen/Flächen | m bzw. m² | Herkunft und Gültigkeit müssen benannt sein; keine GUI-Geometrie |
| Pose und Starrkörpergeschwindigkeit | bestehender `Zustand` | Translation im Weltframe; Rotation ist in der ersten Geschwindigkeitsnäherung kein lokaler Strömungsanteil |

Bauteilrollen und dauerhaft zur Yacht gehörende hydrodynamische
Profilangaben werden im bestehenden `modell.json`-Vertrag geführt; die
Modellbeschreibungsspezifikation ist dafür maßgeblich. Verlässlich aus
physischen Meshes ableitbare Geometrie wird nicht dupliziert. Fluiddaten und
die Laufbindung werden in einem typisierten Core-Kontext transportiert; der
bestehende Simulationsstart bleibt zunächst erhalten. Zeitlich veränderliche
Strömungen sind nicht Teil der ersten Stufe. Fehlende Pflichtgeometrie wird
nicht mit erfundenen Defaultwerten ergänzt.

Die GUI benötigt für die erste Core-Bewegungsstufe keine neuen
Hydrodynamik-Eingabefelder oder physikalischen API-Aufrufe. Die Erweiterung
des strikten Modellimporters und die Pflege gültiger Modelldaten sind
Core-/Modellaufgaben. Neue Modellfelder werden ausschließlich in die
vorhandene [Spezifikation der Modellbeschreibung](Spezifikation_Modellbeschreibung.md)
integriert; diese Hydrodynamikspezifikation begründet keine parallele
Modellformatspezifikation.

In der ersten Stufe wird die Relativgeschwindigkeit am Gesamtschwerpunkt S
gebildet. Für die spätere lokale Erweiterung an einem Angriffspunkt A gilt
kinematisch; in der folgenden Form sind alle Geschwindigkeitsterme im
Weltframe:

```text
r_AS_welt       = Pose.modellvektor_nach_welt(A_modell - S_modell)
omega_welt      = Pose.modellvektor_nach_welt(omega_modell)
v_Körper(A)_welt = v_S_welt + omega_welt × r_AS_welt
u_rel(A)_welt    = v_Fluid(A)_welt - v_Körper(A)_welt
```

`A_modell` ist der Angriffspunkt und `S_modell` der in der bestehenden
Kraft-/Momentenbilanz verwendete Gesamtschwerpunkt, beide im Modellframe.
In Stufe 1 ist `u_rel` am Schwerpunkt die maßgebliche Geschwindigkeit;
`omega x r_AS` ist nur die dokumentierte spätere Erweiterung. Ein Widerstand
muss der verwendeten Relativströmung entgegenwirken. Derselbe
Geschwindigkeitsanteil darf nicht durch getrennte Rumpf- und Anhangsmodelle
doppelt angesetzt werden, sofern keine ausdrücklich spezifizierte
Interaktion vorliegt.

### 7.3 Rückgabevertrag und Kraftvektoren

Jeder berechnete physikalische Beitrag wird im Core als Einzelbeitrag des
bestehenden `Beitrag`-Vertrags repräsentiert. Für hydrodynamische Beiträge
ist folgendes Schema vorgesehen:

| Feld | Sollinhalt |
|---|---|
| `id` | deterministische ID, stabil über Zustände desselben Laufs, z. B. `Rumpf:hydrodynamik:reibung` |
| `element_id` | ID des zugeordneten Rumpf-, Kiel- oder Ruderelements; kein erfundenes Element |
| `art` / `einheit` | `kraft` / `N`; eigenständige Momente verwenden `moment` / `N·m` |
| `quelle` | `hydrodynamik` |
| `gruppe` | `hydrodynamik`; Untergruppen nur nach ausdrücklicher Festlegung |
| `rolle` | `einzelbeitrag`; Summen werden getrennt ausgewiesen |
| `punktart` | `angriffspunkt` bei echter Kraftangriffsstelle, sonst `bezugspunkt` |
| `frame` | `modell` für Punkt und Vektor, gemäß bestehendem Ergebnisvertrag |
| `punkt_m` | physikalischer Angriffspunkt im Modellframe; `null`, wenn nicht bekannt |
| `vektor` | tatsächlicher Kraftvektor im Modellframe; keine GUI-Skalierung |
| `moment_s_nm` | genau einmal berechnetes Moment desselben Beitrags um S |
| `zustand` | bestehender Datenzustand; gültige Nullkraft ist `null`, nicht fehlender Eintrag |
| `fehler_id` | katalogisierte ID bei `ungueltig`, sonst `null` gemäß bestehendem Vertrag |

Als Momentenregel gilt:

```text
M_i,S = (A_i - S) × F_i + M_i,A
```

Ein Moment ohne separate Kraft darf nicht zusätzlich als resultierender
Kraftbeitrag summiert werden. Gruppen- und Gesamtsummen sind Verweise auf
ihre Einzelbeiträge und keine zusätzlichen Summanden. Die CORE-Kraftsumme
und ihr Moment müssen aus denselben Beiträgen rekonstruierbar sein, welche
der Integrator im betreffenden Auswertungsschritt tatsächlich verwendet.

Die genauen IDs und die Zuordnung mehrerer Widerstandsanteile je Element
werden im Schritt der jeweiligen Komponente festgelegt. IDs dürfen nicht
aus gerundeten Kraftwerten abgeleitet werden und müssen bei wiederholter
Auswertung stabil bleiben.

### 7.4 Bilanzstatus und fehlende Berechnung

Der erwartete Bilanzumfang umfasst genau die für Modellfassung,
Hydrodynamikkonfiguration und Rechenmodus vorgesehenen Einzelbeiträge.
Ein erfolgreich berechneter Widerstand bei `|u_rel| = 0` ist ein gültiger
Nullbeitrag. Ein nicht aktiviertes Modell ist keine Bilanzlücke. Ein
erwarteter, aber nicht berechenbarer Beitrag darf hingegen nicht als Null
maskiert werden: Er wird mit `nicht_berechnet` oder `ungueltig`, einer
konkreten Diagnose und einer unvollständigen Bilanz ausgewiesen.

Für den ITTC-1957-Reibungsbeitrag ist der vorläufige Arbeitsbereich
`10^6 <= Re <= 10^9` festgelegt. Bei `V > 0` außerhalb dieses Bereichs wird
der Beitrag mit `nicht_berechnet` und `CORE-HYD-0001` markiert; die
Simulation darf fortfahren, aber die Bilanz muss unvollständig bleiben.
Es wird nicht extrapoliert. Bei `V = 0` wird `Cf` nicht ausgewertet und ein
gültiger Nullbeitrag geliefert. Ein Teilergebnis darf nie als vollständige
Physikbilanz bezeichnet werden.

### 7.5 Ergebnisserialisierung, Versionierung und GUI-Verwendung

Die vorhandene V2-Ergebnisform enthält bereits Beitragsfelder, Summen,
Frames, Angriffspunkte, Momente und Bilanzstatus. V3 ergänzt ausschließlich
den separaten Testantriebskontext und bleibt davon unberührt.
Hydrodynamik nutzt die vorhandene Form additiv; bestehende Felder und
Bedeutungen bleiben unverändert. Neue Quellen- und Gruppenwerte allein
erfordern keine neue Vertragsversion. Eine neue Version wird nur nötig,
wenn eine inkompatible Änderung an Pflichtfeldern oder ihrer Bedeutung
unvermeidbar ist; das ist mit einem Kompatibilitätsnachweis zu belegen.

Die vorhandene GUI liest bereits generische Kraft-/Momentenbeiträge,
Angriffspunkte und Momente und klassifiziert unbekannte Quellen als
generische weitere Quelle. Eine besondere Hydrodynamik-Darstellung, neue
Farben, Filter oder ein neuer Kraftgruppen-Dialog sind ausdrücklich nicht
Voraussetzung für die erste Core-Bewegungsstufe. Die vorhandene generische GUI-Anzeige bleibt unverändert und ist keine
hydrodynamische Fachabnahme.

Spätere GUI-Nutzung muss `quelle`, `gruppe`, stabile `id`, `element_id`,
`punkt_m`, `vektor`, `moment_s_nm`, `frame` und `zustand` aus dem
Core-Vertrag beziehen. Die GUI leitet keine hydrodynamischen Kräfte,
Momente, Profile oder Summen selbst her.

### 7.6 Fehlermeldungen und Laufatomizität

Ungültige oder nicht endliche Eingaben, fehlende Pflichtgeometrie,
unzulässige Reynolds-Zahl und numerischer Überlauf benötigen konkrete
Core-Diagnosen. Fehlermeldungen müssen Quelle und betroffene Modellelemente
benennen, soweit verfügbar. Fehler werden nicht still verschluckt und
erzeugen keinen Null- oder Erfolgssurrogatwert.

Ein fehlgeschlagener Integratorschritt veröffentlicht keinen teilweise
aktualisierten Zustand. Der letzte gültige Zustand bleibt verwendbar. Neue
hydrodynamische Fehler-IDs werden erst bei Finalisierung und Aufnahme in den
Fehlerkatalog verbindlich nummeriert.

## 8. Umsetzungs- und Nachweisstufen

Jede Stufe benötigt einen eigenen, später freizugebenden Umsetzungsschritt
und bleibt bis zu dessen Abnahme aus dem Anspruch einer vollständigen
Hydrodynamik heraus.

| Stufe | Inhalt | Mindestnachweis |
|---|---|---|
| 0 | Eingabe-/Geometrie-, Frame-, Fehler- und Bilanzvertrag für vorhandene Referenzmodelle | Modell-/API-Negativtests; Nachweis, welche Werte aus physischen Meshes ableitbar sind |
| 1 | Isolierter Rumpf-Reibungsbeitrag nach ITTC-1957 und Integration in die dynamische Bewegung | Unabhängige Referenzwerte, Nullfluss, Re-Grenzen, Kraftrichtung, Moment, Integrator- und Ergebnisparität; noch keine Bewegungsabnahme |
| 2 | Validierter Rumpf-Rest-/Wellenwiderstand, bevorzugt DSYHS bei belegter Anwendbarkeit | Quellen-/Datensatzprüfung und Referenzfälle für Optimist, 420er und Folkeboot innerhalb ausgewiesener Grenzen |
| 3 | Kiel-Seitenkraft und zugehöriger Widerstand, zunächst Kennwertmodell; alternativ dokumentierter 2D-Profilpfad | Profil-/Streckungsreferenzen, Polare, Kleinwinkeltests, Ablösegrenzen und getrennte Bewertung der Jollen und des Folkeboots |
| 4 | Fachliche Bewegungsabnahme mit Rumpf-Reibung, validiertem Restwiderstand und Kiel-Seitenkraft | Repräsentative Fälle und verbindliche Toleranzen für Optimist/420er und Folkeboot; keine bessere Geometrie vorausgesetzt |
| 5 | Nachgelagerte Kräfte am vorhandenen, relativ festen Ruder | Starrbefestigungs-, Angriffspunkt- und Momentenreferenzen; kein Winkelstellglied |
| 6 | Einzelne Wechselwirkungen und dynamische Gesamtbilanz | Parität zwischen allen Integratorauswertungen und ausgelieferter Endzustandsbilanz |
| 7 | Optional: spätere GUI-Kraftdarstellung | Getrennte GUI-Anforderungen und GUI-Tests; nicht Teil der ersten Core-Freigabe |

Ergebnisse dürfen nicht über eine tatsächlich erreichte Stufe hinaus als
vollständige Yacht-Hydrodynamik ausgewiesen werden.

## 9. Abnahmematrix

Je Referenzfall sind vor der fachlichen Bewegungsabnahme verbindliche
absolute und relative Toleranzen aus unabhängigen Referenzdaten festzulegen.
Mindestens folgende Eigenschaften müssen geprüft werden:

| Bereich | Prüfkriterium |
|---|---|
| Eingaben | Fehlende, nicht endliche, null-/negativ-unzulässige und außerhalb des festgelegten Gültigkeitsbereichs liegende Parameter werden explizit behandelt |
| Nullfluss | Endlicher gültiger Nullbeitrag; keine Division durch null und keine unbestimmte Widerstandsrichtung |
| Vorzeichen | Bei `u_rel = v_fluid - v_s` zeigt der Widerstand mit `u_rel` und damit entgegen der Körperbewegung relativ zum Fluid |
| Bezug/Frame | Vektoren werden rotiert, Punkte rotiert und verschoben; `moment_s_nm` entspricht der Hebelarmregel genau einmal |
| Summen | Einzelbeiträge ergeben Gruppen- und Gesamtkraft sowie Gesamtmoment ohne Doppelzählung |
| Bewegung | Dynamik verwendet die Hydrodynamikkraft in allen erforderlichen Integratorauswertungen und ändert Pose/Geschwindigkeit nachvollziehbar |
| Endzustand | Serialisierte Einzelbeiträge beziehen sich auf dieselbe Endpose und Geschwindigkeit wie der gelieferte Zustand |
| Rückwärtskompatibilität | Ohne Hydrodynamikdaten bleiben bisherige Testfälle und Ergebnisse fachlich unverändert |
| GUI-Kompatibilität | Bestehende Bewegungsanzeige funktioniert; generische Kraftanzeige unbekannter Quelle wird dokumentiert und geprüft |
| Referenzmodelle | Optimist und 420er als Jollen sowie Folkeboot als Kielboot getrennt geprüft; Datenmängel und Gültigkeitsgrenzen dokumentiert, ohne bessere Geometrie vorauszusetzen |
| Fachliche Bewegung | Erst nach validiertem Rumpf-Restwiderstand und Kiel-Seitenkraft anhand der verfügbaren Referenzmodelle bewerten; keine pauschale Genauigkeitsbehauptung |
| Reproduzierbarkeit | Gleiche Modellfassung, Konfiguration und Schrittfolge ergeben stabile IDs und reproduzierbare Ergebnisse innerhalb der Toleranzen |

## 10. Explizite Nichtziele der ersten Stufen

Nicht enthalten sind unabhängige Ruderbewegung, variable Ruderwinkel,
Ruderstellgesetz, Autopilot, Segelantrieb, stationäres VPP,
hydrodynamische Gleichgewichtssuche, automatische CAD-/Darstellungsanalyse,
beliebige CFD-Genauigkeit, freie Oberfläche, Wellen in Seegang, Wandnähe,
Kavitation und pauschale, nicht belegte Interaktionszuschläge.

Eine spätere Aufnahme dieser Themen erfordert eine eigene Entscheidung und
eine nachvollziehbare Erweiterung der Spezifikation. Die gegenwärtige
Starrkörperdynamik des CORE bleibt die Bewegungsebene.

## 11. Freigegebene Anforderungen für C-0.6

Die folgenden Anforderungen sind auf den ausdrücklichen Implementierungsauftrag
vom 27.10.2026 freigegeben. Ihre Herkunftszuordnung steht in der
[Entwurfspunkteliste](Entwurfspunkte/Hydrodynamik.json). Das Wort
„freigegeben“ erteilt keine Git-Veröffentlichungsfreigabe.

### A-C-00062: Referenzmodelle und Geometriegrenzen

Optimist und 420er werden als Jollen, das Folkeboot als Kielboot untersucht.
Diese vorhandenen Modelle sind die verfügbare Referenzgrundlage; realistischere
Geometrie oder ihre spätere Beschaffung darf nicht vorausgesetzt werden.
Physikwerte dürfen aus physischem `Hydrostatik.volumen`-Mesh nur abgeleitet
werden, wenn das Verfahren geometrisch definiert und reproduzierbar ist.
Darstellungsmeshes und Dateinamen sind keine Physikquellen. Jede
Mesh-/Modellunsicherheit und jeder fehlende Datenbestand wird dokumentiert.

Rotationsbedingte lokale Strömungsanteile und daraus folgende zusätzliche
Dämpfung sind Optimierungen nach dem ersten Bewegungsnachweis. Die verfügbaren
Referenzmodelle sind die Abnahmegrundlage; eine Verbesserung oder Ersetzung
ihrer Geometrie ist keine Voraussetzung.

**Abnahmekriterium:** Drei getrennte Modellinventare beschreiben tatsächlich
verwendete physische Meshes, Geometriekennwerte, Herkunft und erkennbare
Grenzen; keine Behauptung einer nicht nachgewiesenen Modellgenauigkeit.

### A-C-00063: Relativgeschwindigkeit und Bezugsrahmen

Die erste Stufe verwendet die Translationsgeschwindigkeit des
Gesamtschwerpunkts und eine explizite, räumlich gleichförmige
Fluidgeschwindigkeit. `u_rel = v_fluid - v_s` wird im Weltframe gebildet;
Kraftvektoren werden in das Modellframe transformiert. Jede erforderliche
Integratorstufe verwendet ihren eigenen Zustand. Rotationsbedingte lokale
Geschwindigkeitsanteile werden ausdrücklich zurückgestellt.

Für den Reibungswiderstand gilt `F = u_rel/|u_rel| * Rf`: Die Kraft folgt
der Fluidgeschwindigkeit relativ zum Körper und wirkt damit entgegen der
Körpergeschwindigkeit relativ zum Fluid.

**Abnahmekriterium:** Translation, Fluidströmung, Orientierungstransformation
und Integratorzwischenstufen sind durch Tests mit erwarteten Richtungen und
Beträgen abgedeckt.

### A-C-00064: ITTC-1957-Reibungsbeitrag

Der erste Rumpfbeitrag verwendet ausschließlich
`Re = V*Lwl/nu`, `Cf = 0.075/(log10(Re)-2)^2` und
`Rf = 0.5*rho*V^2*Sw*Cf`. Es gibt keinen Formfaktor und keinen
Restwiderstandszuschlag in dieser Stufe. Arbeitsbereich: `10^6 <= Re <= 10^9`.
Bei `V=0` wird `Cf` nicht ausgewertet und ein gültiger Nullbeitrag geliefert.
Bei `V>0` außerhalb des Arbeitsbereichs wird der Reibungsbeitrag mit
`nicht_berechnet` und `CORE-HYD-0001` ausgewiesen; der dynamische Lauf darf
fortgesetzt werden, muss die Bilanz aber als unvollständig markieren.
Außerhalb des Arbeitsbereichs wird kein Widerstand extrapoliert.

**Abnahmekriterium:** Unabhängige numerische Referenzen, Nullfluss, beide
Re-Grenzen, Fälle außerhalb des Bereichs, Eingabevalidierung, Kraftrichtung
und Ergebnisendlichkeit werden geprüft.

### A-C-00065: Rumpf-Rest- und Wellenwiderstand

Ein Rest-/Wellenwiderstand darf erst berechnet und als physikalischer
Modellbeitrag ausgegeben werden, wenn Methode, Datenquelle und
Anwendbarkeitsdomäne für das jeweils verwendete Referenzmodell nachgewiesen
sind. DSYHS wird bevorzugt, aber nicht ungeprüft auf Jollen oder ein Modell
außerhalb ihrer Daten-/Parameterdomäne extrapoliert. Der unkalibrierte
`Cr = c_rest * Fr^4`-Platzhalter wird nicht als Vorhersage verwendet.

**Abnahmekriterium:** Quelle, Methodenversion, Parameterdomäne und unabhängige
Referenzfälle sind dokumentiert. Wo diese Belege fehlen, wird der Beitrag
sichtbar als nicht implementiert/nicht abgenommen geführt; keine erfundene
Regression.

### A-C-00066: Kiel-Seitenkraft und festes Ruder

Die Kiel-/Schwertstufe ist vom Rumpfwiderstand getrennt. Das erste
Kennwertmodell sowie die Alternative eines über Höhe/Spannweite einheitlichen
2D-Profils benötigen explizit dokumentierte Auftriebs-/Widerstandskennwerte
und Ablösegrenzen. Ein Profilname oder Mesh allein belegt keine Polare. Das
Ruder bleibt starr relativ zum Rumpf und wird erst nach fachlichem
Bewegungsnachweis mit validiertem Rumpf-Restwiderstand und Kiel-Seitenkraft
hydrodynamisch einbezogen.

**Abnahmekriterium:** Isolierte Kleinwinkel-, Streckungs-, Ablöse-,
Kraft-/Widerstandsrichtungs-, Angriffspunkt- und Momententests mit belegten
Kennwerten; andernfalls als blockiert/nicht abgenommen dokumentieren.

### A-C-00067: Additive Kraft-/Momentenbeiträge

Hydrodynamische Einzelbeiträge verwenden die bestehenden Core-Beitragsfelder
für ID, Element, Angriffspunkt, Frame, Kraftvektor, Moment, Quelle, Gruppe
und Status. Momente werden genau einmal auf den aktuellen
Gesamtschwerpunkt bezogen; Gruppen- und Gesamtsummen sind keine zusätzlichen
Summanden. Fehlende erwartete Beiträge dürfen nicht als Nullbeiträge
maskiert werden.

**Abnahmekriterium:** Beitragssummen stimmen mit der tatsächlich vom
Integrator verwendeten Kraft-/Momentensumme überein; Bilanzlücken und
ungültige Beiträge sind explizit.

### A-C-00068: Interaktionen

Bauteile werden zunächst isoliert berechnet. Nachlauf, Abwind,
Spiegeleffekt, Krängung, Ablösung und Junction-Zuschläge werden nur einzeln
und nach einem eigenen Nachweis ergänzt. Unbelegte pauschale Zuschläge sind
ausgeschlossen.

**Abnahmekriterium:** Jeder aktivierte Interaktionsterm besitzt eine eigene
Quelle, Referenzfälle, Gültigkeitsgrenze und isolierbare Tests.

### A-C-00069: Referenzfälle und fachliche Abnahme

ITTC-Tests belegen Rechenintegration, nicht das vollständige Bewegungsverhalten
einer Yacht. Die fachliche Bewegungsabnahme erfolgt erst mit validiertem
Rumpf-Restwiderstand und Kiel-Seitenkraft. Optimist und 420er sowie Folkeboot
werden getrennt bewertet. Absolute und relative Toleranzen müssen vor der
fachlichen Abnahme aus unabhängigen Referenzdaten stammen.

**Abnahmekriterium:** Eine Rückverfolgbarkeitsmatrix ordnet jede aktive
Anforderung geeigneten Tests und Mess-/Referenzbelegen zu; Unsicherheiten und
nicht erreichte Kriterien bleiben sichtbar.

### A-C-00070: Dynamische Core-Integration

Hydrodynamische Kräfte wirken in der dynamischen Starrkörperintegration.
Hydrostatische Statik und bisherige Aufrufe ohne Hydrodynamikkonfiguration
bleiben unverändert. VPP, stationäres Gleichgewicht und Segelantrieb sind
ausgeschlossen.

**Abnahmekriterium:** Dynamische Integratorstufen verwenden ihre zugehörigen
Zustände; ohne Hydrodynamik sind bestehende Regressionen und statische
Lösungen unverändert.

### A-C-00071: GUI-Grenze

Die erste Lieferung ändert weder GUI-Physik noch GUI-Eingaben oder
spezielle Hydrodynamikdarstellungen. Die vorhandene generische GUI-Anzeige
bleibt unverändert; ihre Anzeige ist keine fachliche Hydrodynamikabnahme.

**Abnahmekriterium:** Bestehender Zustandsadapter kann Core-Beiträge lesen;
keine GUI-Dateien oder spezielle Hydrodynamikdarstellungen sind erforderlich.

### A-C-00072: Modell- und Laufdatenvertrag

Dauerhafte Bauteilrollen und physikalische Modellangaben werden im
bestehenden `modell.json`-Vertrag geführt. Geometriewerte werden nicht
redundant gespeichert, wenn sie verlässlich aus physischen Meshes ableitbar
sind. Fluidparameter und gleichförmige Fluidgeschwindigkeit werden in einem
typisierten Core-Laufkontext transportiert. Ein dynamischer Lauf mit
Rumpfrolle und ohne explizite Laufkonfiguration erhält das dokumentierte
Standardwasserprofil `rho=1000 kg/m³`, `nu=1e-6 m²/s`,
Fluidgeschwindigkeit `[0,0,0] m/s`; diese tatsächlich verwendeten Werte
werden im Ergebnis ausgegeben. Ein expliziter Laufkontext überschreibt das
Profil. Direkte Kontext-Erzeugung verlangt positive endliche Werte; ungültige
explizite Eingaben erzeugen konkrete Core-Diagnosen. Der bestehende
Simulationsstart bleibt kompatibel; Modelle ohne Rumpfrolle erhalten keine
automatisch aktivierte Hydrodynamik.

**Abnahmekriterium:** Modellimport, Laufkontext, Modellfassung,
Serialisierung und dynamische Berechnung prüfen denselben eindeutig
zugeordneten Konfigurationsstand; Standard- und expliziter Kontext sowie
Kompatibilität von Modellen ohne Rumpfrolle sind getestet.

## 12. Folgeumfang: dreidimensionale Hydrodynamik für alle festgelegten Körper

**Status: fachlich finalisiert am 28.10.2026; technische Umsetzung nicht
beauftragt.**

Dieser Abschnitt legt den fachlichen Folgeumfang und seine Anforderungen
A-C-00073 bis A-C-00078 fest. Er ändert die abgeschlossene C-0.6-Umsetzung
nicht rückwirkend. Die Entscheidungsherkunft und noch ausstehende technische
Nachweise stehen mit ihrer IDEE-Zuordnung in der
[Entwurfspunkteliste](Entwurfspunkte/Hydrodynamik.json). Die geordnete
Umsetzung und U-C-Kennungen stehen im
[Umsetzungsplan C-0.7](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md).

### 12.1 Geltungsbereich der Körper

Der Rechenpfad muss für alle nachfolgend aufgeführten physischen Körper
funktionieren:

| Modellfamilie | Hydrodynamisches Hauptobjekt | Einbezogene Teilobjekte |
|---|---|---|
| Einfache Referenzkörper | einziger Körper, im Modell derzeit als `Rumpf` geführt | Bleistift, Kegel, Kugel, Platte, Pyramide, Quader, Rolle und unregelmäßiger Körper; alle werden fachlich als Rumpfkörper behandelt |
| Optimist und 420er | Rumpf | Rumpf, Schwert und Ruderblatt |
| Folkeboot | Rumpf | Rumpf, Kiel, separates Bulb-Teilobjekt und Ruderblatt |

Besatzung, Cockpitinnenraum, Pinne/Pinnenausleger/Ruderkopf, Mast, Bäume,
Stagen und Segel gehören nicht zu diesem Hydrodynamikumfang. Für das Ruder
bleibt die bestehende starre Befestigung am Rumpf erhalten; eine unabhängige
Ruderbewegung oder Winkelsteuerung ist nicht enthalten. Der Bulb wird als
eigenes physisches Modellelement mit eigenem Mesh geführt, auch wenn er bisher
geometrisch im Kielmesh enthalten sein sollte. Die Aufteilung und
Lagebeziehung zum Kiel sind beim Modellinventar zu prüfen; ein vorhandenes
Gesamtmesh darf nicht als zwei überlappende Volumenkörper doppelt ausgewertet
werden.

Die Elementrolle (`rumpf`, `kiel` oder `ruder`) bestimmt die fachliche
Bauteilklasse, nicht allein den verwendeten Kraftansatz. Die acht einfachen
Referenzkörper erhalten die Rolle `rumpf`; ihre JSON-Dateien enthalten diese
Rolle derzeit noch nicht. Die Modellmigration ist im späteren
Umsetzungsplan abzubilden. Das Bulb bleibt ein eigenes identifizierbares
Modellelement mit eigenem Mesh; soweit der bestehende Rollenvertrag verwendet
wird, erhält es die Rolle `kiel`.

### 12.2 Gemeinsamer dreidimensionaler Zustands- und Geometrievertrag

Der Rechenpfad muss alle drei Modellachsen abdecken: X zum Bug/vorn, Y nach
Backbord und Z nach oben. Jede berechnete Einzelkraft wird als vollständiger
3D-Vektor ausgegeben; die Komponenten dürfen nicht auf eine bevorzugte
Fahrtrichtung beschränkt werden. Hydrostatik, insbesondere Gewicht und
Auftrieb, bleibt unverändert und von hydrodynamischen Kräften getrennt.
Hydrodynamischer Widerstand bei vertikaler Bewegung wird für untergetauchte
Körper einbezogen. Kurzzeitiger Wasseraufprall beim Durchtritt der freien
Oberfläche sowie Wellenbildung sind in diesem Folgeumfang ausgenommen.

Für den jeweiligen Starrkörperpunkt `A` ist die lokale Körpergeschwindigkeit
aus Translation und Rotation zu bilden:

```text
r_AS_welt        = Pose.modellvektor_nach_welt(A_modell - S_modell)
omega_welt       = Pose.modellvektor_nach_welt(omega_modell)
v_Körper(A)_welt = v_S_welt + omega_welt × r_AS_welt
u_rel(A)_welt    = v_Fluid(A)_welt - v_Körper(A)_welt
```

In diesem Folgeumfang wird die bisher zurückgestellte rotatorische
Geschwindigkeit benötigt, damit Teilobjekte und ihre Momente bei Gieren,
Stampfen, Rollen und Rotation des einfachen Körpers konsistent bewertet
werden. Eine räumlich oder zeitlich variable Strömung ist nicht Teil dieses
Umfangs; das Fluid wird als laufweit gleichförmig angenommen.

Für jede Integratorstufe werden Pose, Geschwindigkeit, Orientierung und
benetzte Meshfläche derselben Stufe verwendet. Physik wird ausschließlich
aus dem physischen `Hydrostatik.volumen`-Mesh oder aus ausdrücklich
freigegebenen Hydrodynamikdaten abgeleitet. Anzeige- und Massenmeshes sind
keine Ersatzquelle. Benetzte Oberfläche und ihr Angriffspunkt werden
poseabhängig berechnet; Meshauflösung, geschlossene Volumenanteile,
Wasserlinien-/Schnittbehandlung und bekannte geometrische Grenzen müssen
reproduzierbar geprüft und dokumentiert werden.

### 12.3 Kraftarten und Objektparameter

„Reibung in allen drei Achsen“ bezeichnet nicht drei voneinander unabhängige
skalare Widerstandsgesetze. Die Oberflächenreibung wirkt lokal tangential
zur benetzten Oberfläche und entgegen der lokalen tangentialen
Relativströmung. Für ein benetztes Flächenelement mit Einheitsnormalenvektor
`n` gilt:

```text
u_t       = u_rel - dot(u_rel, n) * n
dF_reib   = 0.5 * rho * Cf(Re, ...) * |u_t| * u_t * dS
```

Die Beiträge werden über lokale, wasserliniengeclippte Meshpanels integriert.
Die ITTC-1957-Korrelation wird für die tangentiale Reibung in jeder
Strömungsrichtung panelweise verwendet. Weil Panelgeometrie, Rauheit und
Quer-/Vertikalübertragung für die Yachtmodelle nicht extern validiert sind,
bleiben diese Ergebnisse Schätzungen. Der verwendete lokale
Reynolds-Bereich und die resultierenden Reibungsbeiwerte werden im
Hydrodynamikergebnis mitgeführt.

Für die angestrebten Bewegungswirkungen sind die Beiträge getrennt zu
behandeln:

- **Hautreibung:** tangentiale Schubspannung auf der benetzten Fläche.
- **Form-/Druckwiderstand:** Widerstand infolge Druckverteilung und Ablösung;
  ein benötigter Ansatz muss für Körperklasse, Anströmrichtung und
  Gültigkeitsbereich belegt sein.
- **Wellenwiderstand:** Wellenbildung und Wellenwiderstand sind in diesem
  Folgeumfang ausdrücklich ausgeschlossen.
- **Auftriebs-/Seitenkraft:** für Schwert/Kiel und festes Ruderblatt bei
  seitlicher Anströmung anhand einer belegten Polare oder gleichwertig
  validierten Methode. Diese Kraft ist keine Hautreibung.

Eine fallende oder sinkende Körperbewegung muss durch den hydrodynamischen
Widerstand gebremst werden können. Eine Endgeschwindigkeit ist ein
Gleichgewicht zwischen resultierenden antreibenden Kräften und Widerstand;
sie darf nicht als künstliche Geschwindigkeitsbegrenzung codiert werden.
Hydrostatischer Auftrieb bleibt separat bilanziert. Die gewünschte
Abdrifthemmung entsteht durch nachgewiesene Seitenkräfte von Kiel/Schwert und
Ruderblatt und nicht durch einen erfundenen Reibungszuschlag.

Für Rumpf- und einfache stumpfe Körper wird der Druck-/Formwiderstand
richtungsabhängig mit einer projizierten Fläche und einem dokumentierten
Widerstandsbeiwert beschrieben:

```text
F_druck = 0.5 * rho * Cd(Re, beta, ...) * A_proj * |u_rel| * u_rel
```

`A_proj` ist die zum jeweiligen Anströmvektor projizierte Fläche; `beta`
bezeichnet die Anströmrichtung relativ zum Modell. Der konkrete
`Cd`-Datensatz oder die zugelassene Methode muss die verwendete Körperklasse,
Orientierung und Reynolds-Domäne abdecken. Solange kein objektbezogener Wert
vorliegt, verwendet C-0.7 als ausdrücklich geschätzte technische Näherung
`Cd = 1.0` auf anströmseitigen Normalpanels. Der Wert wird im Ergebnis samt
Herkunft und Unsicherheit ausgewiesen und ist nicht für konkrete Rumpfformen
kalibriert.

Für Schwert/Kiel, Bulb und Ruderblatt werden Auftriebs-/Seitenkraft und
Widerstand anhand eines expliziten Kennfelds bzw. einer Polare in Abhängigkeit
von Anstellwinkel und Reynolds-Zahl bestimmt. Belegte Daten sind zu
bevorzugen; wenn sie nicht verfügbar sind, darf ein fachlich begründeter
Schätzwert verwendet werden, sofern er samt Annahmen, Herkunft,
Unsicherheit und Gültigkeitsbereich als `geschaetzt` gekennzeichnet ist.
Die Bezugsfläche,
charakteristische Länge, Vorzeichen, Koordinatenrahmen und gegebenenfalls
endliche Streckenkorrektur sind zu dokumentieren. Eine 2D-Polare darf nicht
ohne ausgewiesene Gültigkeit als vollständiges 3D-Bauteilmodell behandelt
werden.

Kraftanteile dürfen nicht doppelt gezählt werden. Ein Kennfeld weist seine
Widerstandsanteile mit `gesamt`, `profil`, `druck`, `hautreibung` und/oder
`induzierter` aus. `profil` umfasst Profil-/Druck- und Hautreibungswiderstand,
aber nicht den induzierten Widerstand; `gesamt` umfasst alle Widerstandsanteile.
Bei Profil-, Gesamt- oder Hautreibungsanteil wird kein separater
Hautreibungsbeitrag für dasselbe Anbauteil addiert. Ein 2D-Kennfeld erhält
den endlichen induzierten Anteil, sofern dieser nicht bereits als
`induzierter` oder `gesamt` enthalten ist. Hydrodynamische Einzelbeiträge
bleiben nach Kraftart und Modellelement identifizierbar.

Objektabhängige, nicht zuverlässig aus Laufzustand und physischem Mesh
ableitbare Parameter sind im bestehenden `modell.json` direkt am jeweiligen
Element unter `Hydrodynamik` zu speichern. Der Laufkontext bleibt zuständig
für gemeinsame Fluidparameter wie Dichte, Viskosität und
Fluidgeschwindigkeit. Geometriewerte, die der Core zuverlässig und
reproduzierbar aus dem physischen Mesh bestimmt, werden nicht redundant
gespeichert.

Jeder Parameterwert und jeder Kennfeld-Datensatz ist als `validiert`,
`abgeleitet` oder `geschaetzt` zu kennzeichnen. Jeder Eintrag benötigt
Einheit, Herkunft und Gültigkeits-/Unsicherheitsangabe. `validiert` verlangt
unabhängige Belege für die konkrete Anwendung; `abgeleitet` verlangt
nachvollziehbare Eingaben und Rechenweg; `geschaetzt` kennzeichnet einen
vorläufigen, nicht fachlich abgenommenen Wert. Nutzerseitig vorgegebene oder
recherchierte Schätzwerte dürfen im Simulationslauf verwendet werden, bleiben
aber als `geschaetzt` sichtbar und begründen keine fachliche Validierung.
Recherchierte Quellen sind auf ihre Übertragbarkeit zum konkreten Bootsteil,
Material, Oberflächenzustand und Strömungsbereich zu prüfen; ein Internetfund
allein macht einen Wert nicht validiert.

Eindeutig materialgebundene Stoffeigenschaften dürfen im vorhandenen
Materialkatalog
[`materialien.json`](Beispiele_Modellformat/materialien.json) abgelegt und
vom Hydrodynamik-Element referenziert werden. Das Materialkatalogdokument ist
derzeit nur ein Entwurf und kein implementierter Laufzeitvertrag.
Objekt-/bauteilspezifische Größen, Oberflächenzustände und
Hydrodynamikkoeffizienten verbleiben am Modellelement unter
`Hydrodynamik`; sie dürfen nicht als allgemeingültige Materialeigenschaft
ausgegeben werden.

Der Modellimport akzeptiert für den C-0.7-Folgeumfang folgende optionale
Erweiterung des bestehenden Hydrodynamikblocks:

```json
{
  "Hydrodynamik": {
    "rolle": "kiel",
    "modelle": {
      "hautreibung": {
        "methode": "<Methodenkennung>",
        "parameter": {},
        "kennfeld": [],
        "gueltigkeit": "<Gültigkeitsbeschreibung>",
        "quelle": "<Herkunft>"
      },
      "druckwiderstand": {
        "methode": "<Methodenkennung>",
        "parameter": {},
        "kennfeld": [],
        "gueltigkeit": "<Gültigkeitsbeschreibung>",
        "quelle": "<Herkunft>"
      },
      "seitenkraft": {
        "methode": "<Methodenkennung>",
        "parameter": {},
        "kennfeld": [],
        "widerstandsanteile": ["<explizite enthaltene Anteile>"],
        "gueltigkeit": "<Gültigkeitsbeschreibung>",
        "quelle": "<Herkunft>"
      }
    },
    "materialreferenz": "<optionale Material-ID>"
  }
}
```

Nur für das Objekt relevante Modellblöcke werden angegeben. `parameter`
enthält ausschließlich objektabhängige Eingaben, die das gewählte Modell
benötigt und die nicht verlässlich aus dem Mesh ableitbar sind. Jeder
Parameterdatensatz besitzt die Felder `wert`, `einheit`, `status`,
`quelle`, `gueltigkeit` und `unsicherheit`; `status` ist genau einer der
Werte `validiert`, `abgeleitet`, `geschaetzt`. Eine unbekannte Unsicherheit
muss ausdrücklich mit Begründung angegeben werden. `kennfeld` enthält
methodenspezifische Tabellenwerte einschließlich Koordinatenachsen und
Einheiten; jeder Datensatz führt ebenfalls Status, Quelle,
Gültigkeitsbereich und Unsicherheit. Quellen geben mindestens Herkunft und
Version bzw. eine eindeutige Referenz an. Gültigkeitsbereiche benennen
mindestens die benötigten dimensionslosen Bereiche und Anströmwinkel.
Eine Materialreferenz wird nur für eindeutig materialgebundene Werte
verwendet. Erforderliche fehlende Parameter, unbekannte Methoden oder Werte
außerhalb ihres Gültigkeitsbereichs müssen explizit diagnostiziert werden.
Im derzeit implementierten Parametervertrag ist für den Druckwiderstand
`cd` zulässig; für Seitenkraftmodelle sind `streckung`, `wirkungsgrad`,
`cd0` und `cl_max` zulässig. Diese Größen sind dimensionslos. Nicht
implementierte oder unbekannte Schlüssel und falsche Einheiten werden beim
Import abgelehnt; Hautreibung verwendet derzeit keine objektspezifischen
Eingabeparameter.
Für jeden angegebenen Modellblock sind `parameter` und `kennfeld`
erforderlich; beide dürfen leer sein. Jeder Kennfeldpunkt enthält `anstellwinkel_rad`,
`reynolds_zahl`, `cl`, `cd`, `status`, `quelle`, `gueltigkeit` und
`unsicherheit`. `widerstandsanteile` ist bei einem nichtleeren
Seitenkraftkennfeld erforderlich. Unbekannte oder doppelte Anteilkennungen
werden abgelehnt; `gesamt` darf nicht mit Teilanteilen kombiniert werden.
Die verwendeten Widerstandsanteile werden zusammen mit den wirksamen
Parametern im Ergebnisbeitrag serialisiert. Zulässige Methoden sind
`ittc_1957_mit_laminarer_naeherung`/`ittc_1957`,
`geschaetzter_normaler_formwiderstand`/`platten_normal_drag` und
`geschaetzte_endliche_streckungspolare`/`polare_2d` für die jeweils passende
Kraftart. Die Modellparameter dürfen explizit überschrieben werden; für
fehlende Werte nutzt der technische Folgeumfang nur dokumentierte,
ausdrücklich als `geschaetzt` ausgewiesene Näherungen. Effektive Parameter,
Status, Methode, Herkunft und Unsicherheit werden je Ergebnisbeitrag
serialisiert. Innerhalb des Kennfelds wird stückweise linear, bei zwei
unabhängigen Tabellenachsen bilinear interpoliert; außerhalb wird der
Beitrag als `nicht_berechnet` diagnostiziert und die Bilanz als unvollständig
ausgewiesen.

### 12.4 Sechs Umsetzungsschritte und Abnahmekriterien

| Schritt | Inhalt | Mindestabnahme |
|---|---|---|
| 1. Umfang und Achsenvertrag | Körperliste aus Abschnitt 12.1, Koordinaten, Relativgeschwindigkeit und Trennung von Widerstand, Auftrieb und Seitenkraft bestätigen | Eindeutige Vorzeichen und physikalische Bedeutung für X-, Y- und Z-Bewegungen; keine implizite Erweiterung auf ausgeschlossene Teile |
| 2. Mesh- und Oberflächeninventar | Alle einfachen Körper sowie Rümpfe, Schwert/Kiel und Ruderblätter prüfen; Benetzung und Angriffspunkte reproduzierbar ableiten | Für jedes Modellkörper-Element Herkunft, physische Meshdatei, geeignete Geometrie, Benetzungsverfahren und Unsicherheit dokumentiert |
| 3. Methoden und JSON-Daten | Pro Kraftart und Körperklasse geeignete Modelle sowie deren Gültigkeitsbereiche festlegen; objektabhängige Parameter im `Hydrodynamik`-Block definieren | Quellen, Methode, Parameterbedeutung/Einheiten, Gültigkeitsbereich und JSON-Validierung festgelegt; keine unbelegten Extrapolationen |
| 4. Isolierte Referenztests | Einfache Körper als Rumpf-Testfälle und Yachtbauteile einzeln in X/Y/Z-Anströmung prüfen | Nullfluss, Vorzeichen, Rotationstransformation, Meshschnitt, Grenzwerte, Referenzkraft, Angriffspunkt und Moment pro Modellstufe getestet |
| 5. Dynamikintegration | Gültige Einzelkräfte je Bauteil in allen erforderlichen Integratorstufen additiv in die bestehende Starrkörperdynamik einbinden | Kraft-/Momentenbilanz stimmt mit dem Integrator überein; Batch-/Live-Parität, Altverhalten und diagnostizierte Bilanzlücken sind getestet |
| 6. Bewegungs- und Fachabnahme | Freier Fall/Sinken sowie Querfahrt/Abdrift getrennt für einfache Körper, Jollen und Folkeboot bewerten | Technische Wirkung durch Bewegungs- und Bilanztests belegt; unabhängige Referenzen und quantitative Toleranzen werden genutzt, sofern verfügbar. Fehlen sie, bleiben Genauigkeitsvalidierung und deren Status ausdrücklich offen und blockieren nicht die technische Umsetzung. |

Die sechs Schritte bilden die verbindliche fachliche Reihenfolge für den
versionsbezogenen Umsetzungsplan; ihre konkreten Arbeitspakete und
Abhängigkeiten sind mit U-C-Kennungen im
[Umsetzungsplan C-0.7](../Umsetzungsplaene/Umsetzungsplan_C-0.7_Hydrodynamik.md)
festgehalten. Tests an einfachen Geometrien prüfen
Implementierung und Grenzfälle, ersetzen aber keine unabhängigen
hydrodynamischen Referenzdaten für die Yachtmodelle.

### 12.5 Schätzwerte, Gültigkeitsgrenzen und fachliche Validierung

Der technische Folgeumfang verwendet bis zur späteren objektbezogenen
Kalibrierung diese Näherungen:

| Größe | Ansatz und Gültigkeit | Status und Unsicherheit |
|---|---|---|
| Hautreibung | Lokale tangentiale Geschwindigkeit; charakteristische Panellänge `sqrt(A_panel)`; `Cf = 1.328/sqrt(Re)` bis `5e5`, lineare Überblendung bis `1e6`, danach ITTC-1957 `0.075/(log10(Re)-2)^2` bis `Re = 1e9`. Oberhalb davon wird der Reibungsbeitrag als `nicht_berechnet` diagnostiziert; kein Extrapolieren. | `geschaetzt`; Plattenkorrelation auf diskrete Panels übertragen, Oberflächenrauheit und Bauteilwechselwirkungen fehlen. |
| Rumpf/Formwiderstand | `Cd = 1.0`; richtungsabhängige Normalpanelintegration über benetzte Fläche; kein Wellenwiderstand. | `geschaetzt`; nicht für die konkrete Rumpfform kalibriert. |
| Kiel/Ruder ohne Polare | `AR = Spannweite^2/benetzte Planformfläche`; Auftriebsanstieg `2*pi*AR/(AR+2)`; Wirkungsgrad `0.8`, `Cd0 = 0.02`, `Cl_max = 1.0`; induzierter Widerstand `Cl^2/(pi*e*AR)`. Bulbähnliche Geometrie mit Dickenverhältnis über `0.35` wird nicht als Tragfläche gerechnet. | `geschaetzt`, außer geometrisch abgeleitetes `AR`; Profil, Ablösung und Rumpfinteraktionen sind nicht kalibriert. |
| Kiel/Ruder mit 2D-Polare | Bilineare Interpolation von `Cl/Cd`; induzierter Widerstand wird ergänzt, wenn Kennfeldanteile weder `induzierter` noch `gesamt` enthalten. | Datenstatus der Kennfeldpunkte; keine Extrapolation außerhalb beider Achsen. |

Die `geschaetzt`-Werte sind technische Startwerte, keine Messwerte oder
validierten Yachtkennwerte. Eine spätere objektbezogene Polare oder ein
`Hydrodynamik.modelle`-Parameterdatensatz kann die entsprechenden
Näherungen ersetzen. Herkunft, Unsicherheit und Gültigkeitsbereich werden
am Kraftbeitrag mit ausgegeben.

Fehlende externe Referenzdaten oder quantitative Toleranzen blockieren
weder die Modellierung noch die technische Umsetzung. Sie begrenzen
ausschließlich die Aussage über die quantitative Genauigkeit und den
Validierungsstatus. Der Umsetzungsplan muss daher technische
Funktionsprüfungen mit den vorhandenen bzw. geschätzten Daten ermöglichen
und fehlende externe Validierung als offene Einschränkung dokumentieren.

Der Umfang schließt untergetauchten Z-Widerstand ein, lässt Wasseraufprall
beim Oberflächendurchtritt und Wellenbildung jedoch aus. Insbesondere belegt
das Vorhandensein einer Meshdatei oder einer Bauteilrolle keine Widerstands-,
Auftriebs- oder Ablösekennwerte. Die konkreten Bulb-Meshes und ihre
nichtüberlappende Trennung vom Kiel müssen inventarisiert werden.

Geschätzte Kraftmodelle dürfen für alle ausdrücklich deklarierten
Gültigkeitsbereiche verwendet werden. Außerhalb dieser Bereiche wird
weiterhin nicht extrapoliert; der Beitrag wird als `nicht_berechnet`
diagnostiziert und die Bilanz als unvollständig ausgewiesen. Tatsächlich
fehlende Teilmodelle dürfen nicht still als Nullbeitrag gelten. Eine
technisch integrierte Stufe darf als dreidimensionale Hydrodynamik mit
geschätzten Parametern beschrieben werden, muss jedoch klar von einer
extern validierten Vorhersage und von nachgewiesener Genauigkeit
unterschieden werden.

### A-C-00073: Körperumfang und hydrodynamische Rollen des Folgeumfangs

Der allgemeine Hydrodynamikpfad umfasst die acht einfachen Referenzkörper
Bleistift, Kegel, Kugel, Platte, Pyramide, Quader, Rolle und unregelmäßiger
Körper als Rümpfe. Der Core ordnet den acht kanonischen Modell-IDs
`bleistift`, `kegel`, `kugel`, `platte`, `pyramide`, `quader`, `rolle` und
`unregelmaessig` automatisch die Rumpfrolle zu und verlangt dafür genau ein
physisches Volumenmesh. Bei Optimist und 420er umfasst er Rumpf, Schwert und
Ruderblatt. Beim Folkeboot umfasst er Rumpf, Kiel, ein als separates
Modellelement mit eigenem physischen Mesh geführtes Bulb-Teilobjekt sowie
Ruderblatt. Besatzung, Cockpitinnenraum, Pinne/Pinnenausleger/Ruderkopf und
Rigg sind ausgeschlossen. Das Ruder bleibt starr am Rumpf.

**Abnahmekriterium:** Der Modellvertrag ordnet jedes eingeschlossene
physische Element genau einer passenden Rolle zu. Das Kiel-/Bulb-Inventar
verhindert überlappende Doppelberechnung; das Bulb-Mesh und seine Lage zum
Kiel sind dokumentiert. Alle übrigen genannten Elemente erzeugen keine
hydrodynamischen Beiträge dieses Umfangs.

### A-C-00074: Drei Achsen, lokale Anströmung und benetzte Geometrie

Hydrodynamische Einzelkräfte sind vollständige Vektoren in den Modellachsen
X (Bug/vorn), Y (Backbord) und Z (oben). Die lokale Körpergeschwindigkeit
berücksichtigt Translation und Rotation am jeweiligen Berechnungspunkt.
Benetzte Flächen und Angriffspunkte werden aus dem physischen
`Hydrostatik.volumen`-Mesh für jede Integratorstufe bestimmt. Das Fluid ist
innerhalb eines Laufs räumlich und zeitlich gleichförmig. Untergetauchter
Widerstand bei vertikaler Bewegung ist enthalten; Hydrostatik bleibt
unverändert und separat. Dynamischer Wasseraufprall an der freien
Oberfläche sowie Wellenbildung und Wellenwiderstand sind ausgeschlossen.

**Abnahmekriterium:** Translation, Rotation, Pose, Frame-Transformation,
Benetzung und Angriffspunkt stimmen für X/Y/Z und jede Integratorstufe mit
unabhängig berechneten geometrisch-kinematischen Referenzen überein.

### A-C-00075: Getrennte Hautreibungs-, Druck- und Seitenkraftmodelle

Die Hautreibung wird aus lokaler tangentialer Relativströmung über die
benetzte Oberfläche integriert. Die ITTC-1957-Korrelation wird nur in ihrer
belegten Anwendungsdomäne verwendet. Rumpf- und einfache stumpfe Körper
erhalten zusätzlich einen richtungsabhängigen Druck-/Formwiderstand mit
projizierter Fläche und dokumentiertem `Cd`-Datensatz. Kiel/Schwert, Bulb und Ruderblatt verwenden für Seitenkraft und Widerstand
eine explizite Polare bzw. ein gleichwertiges Kennfeld über Anstellwinkel und
Reynolds-Zahl. Als Datenbasis sind validierte, abgeleitete oder ausdrücklich
geschätzte Werte zulässig; Status, Quelle, Annahmen und Unsicherheit müssen
pro Wert nachvollziehbar sein. Endliche Streckeneffekte sind einzubeziehen, wenn die
verwendete 2D-Polare sie nicht abdeckt und das angewandte Verfahren belegt
ist. Kraftanteile müssen ohne Doppelzählung zusammengesetzt werden.

Kennfeldwerte werden innerhalb ihres belegten Bereichs stückweise linear
und bei zwei unabhängigen Tabellenachsen bilinear interpoliert. Außerhalb
der Gültigkeit wird nicht extrapoliert; der Einzelbeitrag wird
`nicht_berechnet` und die Bilanz unvollständig ausgewiesen. Wellenwiderstand
ist nicht Teil dieser Anforderung.

**Abnahmekriterium:** Für jede Kraftart und Körperklasse sind Gleichungen,
Bezugsflächen/-längen, Vorzeichen, Kennfeldanteile, Datenquelle und
Gültigkeitsdomäne dokumentiert. Isolierte Tests belegen Kraftrichtung,
Grenzverhalten und Summen ohne Doppelzählung.

### A-C-00076: Objektparameter, Provenienz und Materialreferenzen

Nicht zuverlässig aus dem physischen Mesh oder Laufkontext ableitbare
objektabhängige Hydrodynamikparameter und Kennfelder stehen am
Modellelement unter `Hydrodynamik`. Jeder Wert und Datensatz enthält
mindestens Wert bzw. Kennfelddaten, Einheit, Status `validiert`,
`abgeleitet` oder `geschaetzt`, Herkunft, Gültigkeitsbereich sowie
Unsicherheit oder eine begründete Kennzeichnung, dass sie unbekannt ist.
Nutzerseitig vorgegebene und recherchierte Schätzwerte dürfen im
Simulationslauf verwendet werden, müssen aber als `geschaetzt` sichtbar
bleiben und erfüllen nicht die fachliche Abnahme.

Eindeutig materialgebundene Stoffeigenschaften dürfen im Materialkatalog
[`materialien.json`](Beispiele_Modellformat/materialien.json) stehen und
von einem Modellelement referenziert werden. Objekt-, Beschichtungs-,
Oberflächenzustands- und bauteilspezifische Kennwerte bleiben im
Hydrodynamikblock des Elements. Der Katalog muss vor Nutzung im Core als
Modellvertrag spezifiziert und validiert werden.

**Abnahmekriterium:** Strikter Import validiert Felder, Einheiten,
Herkunftsstatus, Quellen, Materialreferenzen und Gültigkeitsdomänen.
Fehlende, widersprüchliche oder ungültige Werte werden mit Modellelement
und konkretem Feld diagnostiziert. Kein stiller Default oder nicht
gekennzeichneter Schätzwert wird verwendet.

### A-C-00077: Isolierte Tests und unabhängige fachliche Referenzen

Die einfachen Körper dienen als wiederholbare geometrische Referenzfälle;
Rumpf, Schwert/Kiel, Bulb und Ruderblatt werden zusätzlich einzeln geprüft.
Technische Tests decken X/Y/Z-Anströmung, Rotation, Nullfluss, Benetzung,
Reynolds-Grenzen, Kennfeldinterpolation, Momentenbildung und Bilanzparität
ab. Für die Parameterwahl sind verfügbare unabhängige externe Mess- oder
Referenzdaten zu nutzen. Sind sie nicht verfügbar, wird die Umsetzung mit
physikalisch begründeten und ausdrücklich als `geschaetzt` markierten Werten
fortgesetzt; ihre Annahmen und Unsicherheiten werden dokumentiert.

**Abnahmekriterium:** Die Validierungs-Matrix ordnet jedem Modellwert seinen
Status, seine Quelle oder Herleitung, Annahmen, Gültigkeitsbereich,
Unsicherheit und Testfall zu. Technische Abnahme setzt bestandene
Komponenten- und Bewegungstests sowie nachvollziehbare Datenherkunft voraus,
aber keine externen Messdaten. Ohne unabhängige Referenzen wird die
fachliche Genauigkeit ausdrücklich als nicht extern validiert ausgewiesen;
Schätzwerte dürfen nicht als Messwerte oder als fachlich validiert
dargestellt werden. Externe Referenzdaten und quantitative Toleranzen
verbessern die spätere Validierung, sind aber kein Umsetzungsblocker.

### A-C-00078: Dynamikintegration, Endgeschwindigkeit und Abdrift

Alle gültigen hydrodynamischen Einzelbeiträge werden in jeder erforderlichen
Integratorstufe additiv in die bestehende Starrkörperdynamik eingebunden.
Fehlende oder außerhalb der Kennfelddomäne liegende Beiträge werden
diagnostiziert und nicht als Nullbeiträge behandelt. Endgeschwindigkeit
entsteht ausschließlich aus dem Kräftegleichgewicht der wirksamen Kräfte;
eine künstliche Geschwindigkeitsbegrenzung ist unzulässig. Der Kiel bzw.
das Schwert und das starre Ruderblatt hemmen Abdrift durch belegte
Seitenkraftmodelle, nicht durch Hautreibung allein. Hydrostatik und
Auftrieb bleiben unverändert separat.

**Abnahmekriterium:** Batch- und Live-Pfad verwenden dieselben
Hydrodynamikdaten und Kraftmodelle. Dynamische Referenzfälle für freies
Sinken und Querfahrt zeigen mit dem jeweils dokumentierten Parametersatz
Kräftegleichgewicht/Endgeschwindigkeit bzw. eine Abdriftwirkung; kein
Geschwindigkeitsbegrenzer trägt zum Ergebnis bei. Bei geschätzten Daten
belegen diese Tests die technische Wirkung des Modells, nicht die
quantitative Genauigkeit eines realen Boots. Der Parametersatz und dieser
Validierungsstatus werden mit dem Ergebnis ausgewiesen.

## 13. Dynamische Verstellung von Ruder und Schwert

Dieser Abschnitt ist eine fachliche Erweiterung nach dem Umfang von C-0.7.
Er definiert Bewegungsdaten im Core-Modellvertrag und die nötige
GUI-Steuerung; er ändert nicht rückwirkend die C-0.7-Anforderungen oder
deren Umsetzungsplan. Die Einstellung verändert Ruder- und
Schwertgeometrie für Darstellung und Hydrodynamik, nicht den übrigen
Starrkörperzustand. Die Bewegungsrichtungen verwenden ausschließlich das
modellfeste rechtshändige Bezugssystem aus der
[Modellbeschreibungsspezifikation](Spezifikation_Modellbeschreibung.md):
+X zum Bug, +Y nach Backbord, +Z nach oben in der aufrechten Referenzlage.

Die hier angegebenen Modellkoordinaten sind meshbasierte Startwerte in
Metern, keine vermessenen Werft- oder Lagerdaten. Sie dienen als
konkrete Ausgangskonfiguration für die späteren Modell-JSONs und dürfen
bei einer nachweisbaren Abweichung der realen Gelenklage modellbezogen
korrigiert werden. Achsen-, Winkel-, Null- und Vollausfahrkonventionen
bleiben davon unberührt.

### A-C-00079: JSON-Vertrag für bewegliche Modellelemente

Ein bewegliches Element deklariert in seiner Elementdefinition auf
derselben Ebene wie `Darstellung`, `Hydrostatik` und `Hydrodynamik` einen
`Bewegung`-Block mit genau einer Bewegungsart `drehen` oder `heben`.
Vektoren und Drehpunkte sind in Metern bzw. normierter Richtung im
modellfesten Frame angegeben. Eine Drehung verlangt eine normierte
`achse_modell` und einen `drehpunkt_modell_m`; ein Hub verlangt eine
normierte `richtung_modell`; sie zeigt in die physische Hubrichtung
(beim Optimisten +Z, zur Stellung 0 %). Der Hubweg wird, soweit möglich, beim
Modellimport aus dem vollen beweglichen Mesh und der definierten
Rumpfreferenzlinie abgeleitet und nicht als projektweite Konstante
dupliziert.

Der `Bewegung`-Block darf eine Liste `mitbewegte_elemente` führen, die
die zusätzlich zur steuernden Elementgeometrie gemeinsam zu
transformierenden Darstellungselemente anhand ihrer Modell-IDs
identifiziert. Bewegungsart und Achsen-/Pivotdaten sind
modellindividuell; die projektweiten Bedienbereiche und
Null-/Vollausfahrstellungen werden nicht im JSON wiederholt. Ein
Element ohne gültigen Bewegungsblock ist nicht verstellbar. Der Import
weist unbekannte Element-IDs, ungültige oder nicht endliche Vektoren,
einen fehlenden Drehpunkt bzw. eine fehlende Achse oder Hubrichtung
explizit mit Modell- und Elementbezug zurück.

**Abnahmekriterium:** Ein Modellimport kann gültige Dreh- und
Hubdefinitionen getrennt laden, abgeleitete Hubwege reproduzierbar
ermitteln und ungültige Definitionen diagnostizieren. Vorhandene
Modelle ohne Bewegungsblock bleiben unverändert ladbar und erhalten
keine implizite Beweglichkeit.

### A-C-00080: Modellachsen, Drehpunkte und globale Stellkonventionen

Die Achsen und Start-Drehpunkte der Referenzmodelle sind:

| Modell/Element | Bewegung | Modellfeste Achse/Richtung | Start-Drehpunkt bzw. Null-/Vollstellung |
|---|---|---|---|
| Optimist, Ruder | Drehen | +Z, (0, 0, 1) | Näherung (-1.189895, 0, 0.182972) m; aus dem vorderen oberen Rand des Ruderblatts abgeleitet |
| 420er, Ruder | Drehen | +Z, (0, 0, 1) | (-2.279293, 0, 0.410047) m; Mittelpunkt der im Ruderkopf-Mesh dargestellten Stockachse |
| Optimist, Schwert | Heben | +Z, (0, 0, 1) | Bei 100 % unveränderte volle Mesh-Ausfahrt; bei 0 % liegt die Schwertunterkante auf Höhe der unteren Rumpflinie. Hubweg aus Mesh und Rumpfgeometrie ableiten; der aktuelle Meshvergleich ergibt ungefähr 0.523 m. |
| 420er, Schwert | Drehen | +Y, (0, 1, 0) | Näherung (-0.359293, 0, 0.56) m, obere vordere Schwertecke als angenommener Lagerpunkt. Bei 100 % steht das Schwert in der Mesh-Ausgangslage; bei 0 % ist es 90° nach achtern (-X) geschwenkt. |

Die Rudereinstellung ist projektweit ein Winkel von -90° bis +90°:
negative Werte bedeuten Backbord, positive Werte Steuerbord. Die
Winkelbedeutung ist von der gerichteten Achse im JSON zu unterscheiden;
die Core-Transformation muss das Vorzeichen so abbilden, dass das
Ruderblatt bei positivem Winkel physisch nach Steuerbord ausschlägt.
Beim Drehschwert des 420ers entspricht die Skalenstellung 100 % der
unveränderten ausgefahrenen Mesh-Lage und 0 % einer 90°-Drehung nach
achtern; dazwischen wird der Drehwinkel gleichmäßig abgebildet. Beim
Hebeschwert des Optimisten wird die Stellung zwischen beiden
Endstellungen entlang der JSON-Hubrichtung abgebildet.

**Abnahmekriterium:** Referenztests zeigen je Modell korrekte
Null-/Endstellungen, Vorzeichen und Achsrichtung. Die Ruderdrehung bei
-90° und +90° liegt auf den vereinbarten Backbord-/Steuerbordseiten;
das 420er-Schwert dreht bei 0 % um seinen Pivot nach achtern, und das
Optimist-Schwert erreicht bei 0 % die Rumpfunterkante ohne Veränderung
seiner Skalierung. Die beiden genannten Näherungs-Pivots werden durch
Mesh-/Darstellungsprüfungen plausibilisiert; ihre Koordinaten bleiben
modellbezogene JSON-Daten.

### A-C-00081: Steuer-Overlay und Bauteilauswahl

Während einer dynamischen Simulation bietet die GUI für jedes im
geladenen Modell gültig deklarierte Ruder einen Winkelregler von -90°
bis +90° und für jedes gültig deklarierte bewegliche Schwert einen
Regler von 0 % bis 100 % an. Die Regler arbeiten kontinuierlich und
zeigen ihren Zahlenwert an. Sie werden nicht für Modellelemente ohne
passende Bewegungsdefinition angeboten; insbesondere erhält ein
unbewegliches Kiel-/Schwert-Element keinen Bedieneintrag.

Die GUI übermittelt Stellwerte als Eingaben an den Core. Sie berechnet
weder Achsen, Meshtransformationen, Hydrodynamikkräfte noch
Massenkennwerte selbst. Der Core ist für Validierung, Zustandsführung
und physikalische Wirkung zuständig. Darstellung und Simulationszustand
müssen dieselbe bestätigte Stellung verwenden.

**Abnahmekriterium:** Für Modelle mit und ohne jeweilige
Bewegungsdefinition erscheinen genau die dazugehörigen Regler. Die
Grenzwerte sind einschließlich -90°, +90°, 0 % und 100 % erreichbar;
angezeigter, an den Core übermittelter und gerenderter Stellwert stimmen
überein. GUI- und Core-Builds dürfen die Bewegungs- oder
Hydrodynamikberechnung nicht duplizieren.

### A-C-00082: Zeitliche Eingabe und dynamische Hydrodynamikwirkung

Eine Stellwertänderung wird am nächsten Simulationszeitschritt wirksam,
bleibt bis zur nächsten Änderung bestehen und gilt einheitlich für
alle Integratorauswertungen dieses Zeitschritts. Eine separate
Aktuatorbewegung, Stellgeschwindigkeit, Stellkraft oder
Aktuatorträgheit wird nicht simuliert; die neue Stellung ist am
Wirksamkeitszeitpunkt unmittelbar erreicht. Das Bauteil erhält im
Hydrodynamikmodell die Pose aus seiner Stellung und dem aktuellen
Starrkörperzustand. Seine benetzte Geometrie, lokale Anströmung und
Kraftangriffspunkte werden aus dieser Pose für die jeweilige
Integratorstufe bestimmt; die resultierenden Einzelkräfte und Momente
werden additiv in die bestehende Dynamik eingebracht.

Jeder Lauf mit dynamischer Verstellung speichert oder exportiert eine
reproduzierbare, zeitgeordnete Eingabefolge aus Simulationszeit,
monotoner Ereignisfolgenummer, Bauteil-ID und Stellwert zusammen mit
Modellfassung und Simulationsparametern. Bei gleichem Zeitstempel
ordnet die Core-vergebene Folgenummer die Ereignisse; für dieselbe
Bauteil-ID gilt der zuletzt angenommene Stellwert. Die GUI darf die
Simulation weiter bedienen, pausieren oder fortsetzen; die Eingabezeit ist
Simulationszeit, nicht unaufgezeichnete Wandzeit.

Die Berechnung umfasst keine Strömungsnachläufe, gegenseitige
Abschirmung, Bauteilinterferenz, dynamische Ablösung oder
bewegungsinduzierte Zusatzkräfte infolge der Betätigung. Nicht
implementierte Anteile werden nicht als validierte Physik ausgegeben.

**Abnahmekriterium:** Ein Testlauf mit mehreren Stellereignissen
verwendet ab dem jeweiligen Zeitschritt die neue Pose in allen
Integratorstufen. Wiederholung desselben gespeicherten
Eingabeprotokolls mit derselben Modellfassung und denselben
Simulationsparametern reproduziert dieselben Stellzustände und
Hydrodynamikbeiträge. Ruder- und Schwertverstellung ändern die jeweils
betroffenen hydrodynamischen Kräfte bzw. Momente, ohne andere
Kraftgruppen doppelt zu zählen.

### A-C-00083: Vollständige Darstellung und separates Simulationsmesh

Die Darstellung transformiert die vollständige bewegte Baugruppe um
deren Drehpunkt bzw. entlang ihrer Hubrichtung. Beim Optimisten sind
Ruder, Pinne, Pinnenausleger und Gummigelenk der Ruderbewegung
zuzuordnen. Beim 420er gehört der Ruderkopf zur Ruderbewegung. Die
bewegte Geometrie behält ihre Form und Abmessungen; sie wird nicht
skaliert oder durch eine vereinfachte Visualisierung ersetzt.

Die hydrodynamische Berechnung verwendet die dafür erforderlichen
physischen Meshbereiche beweglicher Bauteile in ihrer aktuellen Pose.
Sie darf auf die für die Hydrodynamik relevanten Bereiche, typischerweise
die benetzten und unter dem Rumpf liegenden Bereiche, beschränkt sein;
oberhalb des Wassers liegende Anteile dürfen nicht als benetzte Fläche
behandelt werden. Ein Simulationsmesh darf Rumpfdurchbrüche an
beweglichen Bauteilen schließen, sofern dadurch ausschließlich der
hydrodynamische Simulationskörper vervollständigt wird. Diese
Vereinfachung verändert weder die Darstellungs- noch die
Hydrostatikgeometrie. Wasserzutritt oder Fluten durch solche
Durchbrüche wird in diesem Umfang nicht simuliert. Kollisionen mit
Rigg, Großbaum und Segeln werden nicht berechnet.

**Abnahmekriterium:** Bei jeder End- und Zwischenstellung werden
vollständige sichtbare Baugruppen gemeinsam und um den vorgesehenen
Bezug bewegt. Hydrodynamiktests weisen nach, dass nur die aktuell
benetzte bewegte Simulationsgeometrie Kräfte erzeugt und ein
vereinfachtes geschlossenes Simulationsmesh weder Darstellung noch
Hydrostatik verändert.

### A-C-00084: Unveränderte Masse, Schwerpunkt, Trägheit und Hydrostatik

Die Verstellung von Ruder und Schwert ändert weder Gesamtmasse noch
Gesamtschwerpunkt noch Trägheitstensor des Boots. Die in der
Modellfassung festgelegte Massenverteilung bleibt während des
gesamten Simulationslaufs unverändert; die Masse leichter beweglicher
Bauteile wird nicht dynamisch umverteilt. Gewicht, Auftrieb,
Hydrostatikvolumen, Wasserlinie und Flutungszustand werden durch die
Verstellung ebenfalls nicht neu berechnet. Die dynamische Verstellung
wirkt ausschließlich auf die Darstellung und die hydrodynamischen
Kraft-/Momentenbeiträge der betroffenen Bauteile.

Ein Hub-Kiel wird nicht unterstützt. Das Optimist-Schwert ist als
Hubschwert zugelassen; das 420er-Schwert ist als Drehschwert
zugelassen. Diese Schwertmechanismen sind keine Freigabe für einen
Hub-Kiel oder für dynamische Gewichtsverlagerung.

**Abnahmekriterium:** Für identische Rumpfzustände und Massenparameter
bleiben Masse, Schwerpunkt, Trägheitstensor und hydrostatische Beiträge
vor, während und nach jeder Ruder-/Schwertverstellung invariant.
Modellvalidierung lehnt einen als Hub-Kiel deklarierten Mechanismus
explizit ab.

### A-C-00085: Bewegungs- und Integrationsnachweise

Die technische Abnahme umfasst JSON-Import und Fehlerdiagnosen,
Achsen-/Pivottransformationen, Bediengrenzen, End- und Zwischenlagen,
Baugruppenbewegung, reproduzierbare Stellereignisse und
Hydrodynamikkopplung. Sie prüft außerdem die unveränderten
Massen-/Hydrostatikwerte sowie die Abgrenzung von nicht simulierten
Interaktionen und Betätigungskräften. Live- und Batch-Simulation
verwenden denselben Core-Bewegungs- und Hydrodynamikpfad; die GUI
ist ausschließlich Eingabe- und Darstellungsschicht.

**Abnahmekriterium:** Automatisierte Core-Tests prüfen die
Bewegungsdaten, Transformationsmathematik, Ereignisfolge,
Hydrodynamikänderungen und invarianten Masse-/Hydrostatikdaten.
Integrationstests prüfen GUI-Regler, Core-Eingaben und Darstellung
über den gemeinsamen Vertrag. Fachliche Genauigkeit der
Hydrodynamik ist nur dann extern validiert, wenn unabhängige
Referenzdaten vorliegen; fehlende Referenzen und nicht kalibrierte
Pivots bleiben als Einschränkungen ausgewiesen und werden nicht als
vollständige Validierung deklariert.

### Herkunft und Zuordnung der finalisierten Anforderungen

Die folgenden Anforderungen stammen aus der ausdrücklichen
Nutzerfreigabe zur Finalisierung von IDEE-C-00050 am 28.10.2026.
Die Herkunftszuordnung bleibt in der
[Entwurfspunkteliste Hydrodynamik](Entwurfspunkte/Hydrodynamik.json)
erhalten:

| IDEE-ID | Anforderung | Fachlicher Gegenstand |
|---|---|---|
| IDEE-C-00050 | A-C-00079 | JSON-Bewegungsvertrag |
| IDEE-C-00050 | A-C-00080 | Modellachsen, Drehpunkte und Stellkonventionen |
| IDEE-C-00050 | A-C-00081 | Steuer-Overlay und Bauteilauswahl |
| IDEE-C-00050 | A-C-00082 | Zeitliche Eingabe und Hydrodynamikwirkung |
| IDEE-C-00050 | A-C-00083 | Darstellung und Simulationsmesh |
| IDEE-C-00050 | A-C-00084 | Masse, Schwerpunkt, Trägheit und Hydrostatik |
| IDEE-C-00050 | A-C-00085 | Bewegungs- und Integrationsnachweise |

Diese Finalisierung erteilt keinen Implementierungsauftrag und legt
keine konkrete Release-Version oder Umsetzungsschrittkennung fest.
