# Spezifikation CORE: Hydrodynamik des Segelfahrzeugs

**Status: finalisierte Spezifikation für C-0.6; Core-Implementierung beauftragt, keine GUI- oder Git-Freigabe.**

**Stand:** 27.10.2026

**Entscheidungsherkunft und Restpunkte:** [Hydrodynamik](Entwurfspunkte/Hydrodynamik.json)

Diese Spezifikation beschreibt die schrittweise Erweiterung des eigenständigen
SegelPhysik-CORE um hydrodynamische Kräfte. Die fachlichen Entscheidungen sind
für die Umsetzung in C-0.6 freigegeben; ihre Herkunft und technische
Restpunkte stehen in der verlinkten Entwurfspunkteliste. Die verbindlichen
Anforderungen sind in Abschnitt 11 mit projektweiten A-C-Kennungen aufgeführt.
Der zugehörige Umsetzungsplan führt die U-C-Schritte. Die Freigabe erteilt
weder GUI- noch Commit-, Push-, Tag- oder Release-Auftrag.

## 1. Ziel und Architekturgrenze

Die hydrodynamische Physik, ihre Auswertung und die daraus entstehenden
Kraft- und Momentenbeiträge gehören vollständig in den CORE. Die Berechnung
muss headless und unabhängig von Python, Qt, GUI, Rendering und
Darstellungsdateien funktionieren. Die GUI bleibt Konsument der Core-Zustände
und berechnet oder korrigiert keine Hydrodynamik.

Für die erste Bewegungsstufe ist keine neue GUI-Eingabe, Physik oder
Rudersteuerung vorgesehen: Die GUI übergibt das Modell und verwendet Pose und
Geschwindigkeit aus den bestehenden Simulationsergebnissen. Der CORE muss
die neuen Kraftbeiträge dennoch über einen dokumentierten, stabilen Vertrag
bereitstellen, damit eine spätere GUI-Ausbaustufe diese anzeigen kann.
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

## 2. Festes Ruder und Bewegungsgrenze

Das Ruder bleibt in seiner derzeitigen Modellkonfiguration. Es gibt in diesem
Umfang keine unabhängige Ruderbewegung, kein Stellgesetz, keine
Ruderwinkelsteuerung und keine Ruderaktuatordynamik. Das Ruder ist relativ zum
Rumpf fest; bewegt oder dreht sich das Gesamtmodell, folgt es ausschließlich
dieser Starrkörperbewegung. Die spätere Einführung beweglicher Ruder ist
ausdrücklich ausgenommen und erfordert eine gesonderte fachliche Freigabe.

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
modelliert gelten.

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
