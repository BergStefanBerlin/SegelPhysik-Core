# Spezifikation Core: Stationaeres Fahrtgleichgewicht

**Status:** Abschnitte 1-7 finalisiert; Abschnitt 7 erweitert den bestehenden geradlinigen Vertrag um stationaere Dreh- und Kurvenfahrt. Nachweise im einzigen C-0.8-Versionsplan.
**Stand:** 02.11.2026.

Maessgebliche Entwurfspunkte:
[Stationaeres_Fahrtgleichgewicht.json](Entwurfspunkte/Stationaeres_Fahrtgleichgewicht.json).
GUI-Gegenstueck:
[Statisches Modell](../../GUI/Spezifikationen/Spezifikation_GUI_Statisches_Modell.md).
Zielversion: C-0.8. Verbindlicher Plan:
[Core-Versionsplan](../Umsetzungsplaene/Umsetzungsplan_Modellinspektion.md#stationaeres-fahrtgleichgewicht).
Konkrete exportierte Schnittstellen werden im ersten Planschritt festgelegt
und vor Implementierung einzeln im Core-Schnittstellenregister vergeben.

## 1. Auftrag und Abgrenzung

Gesucht ist eine gleichfoermige, geradlinige Fahrt unter einer vorgegebenen
Fremdkraft und fest vorgegebenen Ruder-/Schwertstellungen. Nicht die Kraft
oder die Stellungen werden optimiert: Gesucht werden die resultierende
Schwimmlage, Orientierung und Geschwindigkeit. Jede andere Stellung oder
Fremdkraft ist ein eigener, reproduzierbarer Rechenfall.

Dies ist eine Erweiterung gegenueber dem hydrostatischen Ruhegleichgewicht,
keine Umbenennung des bisherigen Solvers. Bestehende Hydrostatik und Dynamik
bleiben in ihrer bisherigen Bedeutung erhalten. Windphysik, Segelmodell,
Autopilot und Stellungsoptimierung gehoeren nicht dazu. Die urspruengliche
Abgrenzung stationaerer Kurvenfahrt wird durch Abschnitt 7 fuer dessen
definierte Randbedingungen ergaenzt.

## 2. Gepruefte Ausgangslage vor der ersten Umsetzung

- `Code/src/hydrostatik.cpp`, `gleichgewicht_suche`: variiert nur Z, Rollen
  und Stampfen. Residuum ist Fz, Mx, My aus `hydrostatik(...)`; X, Y und
  Gierwinkel bleiben aus dem Startzustand erhalten.
- `gleichgewicht`: setzt die Ergebnisgeschwindigkeit und Winkelgeschwindigkeit
  auf null. Ein aktiver temporaerer Testantrieb wird abgewiesen.
- `Code/src/simulation.cpp`, `simulieren`: weist Hydrodynamikkonfiguration
  und Hydrodynamikkontext im statischen Lauf ausdruecklich zurueck.
- `Code/include/segelphysik/hydrostatik.hpp`, `kraefte`, und dessen Umsetzung
  koennen bereits geschwindigkeitsabhaengige Hydrodynamik, Fluidgeschwindigkeit
  und Bewegungsstellungen gemeinsam auswerten. Das ist noch kein
  stationaerer Gleichgewichtsloeser.
- Bewegungsstellungen transformieren im bestehenden Kraftpfad
  hydrodynamische Volumennetze; der hydrostatische Grundaufruf bekommt diese
  Stellungen nicht uebergeben. Eine einheitliche Wirkung auf alle betroffenen
  physikalischen Eigenschaften ist vor Umsetzung gesondert nachzuweisen.
- Der Testantrieb ist ein spezieller horizontaler Bugzug mit
  Geschwindigkeitsbegrenzung im dynamischen Ablauf, keine allgemeine Fremdkraft.

Quellen sind die genannten lokalen Implementierungen, geprueft am 02.11.2026.
Die bestehende [Hydrodynamik-Spezifikation](Spezifikation_Hydrodynamik.md)
schliesst stationaeres Gleichgewicht aus ihrem bisherigen Umfang aus;
dieses neue Thema hebt jene historische Abgrenzung nicht rueckwirkend auf.

## 3. A-C-00114: Konstante Eingaben und gesuchte Groessen

Pro Lauf unveraenderlich: Modellfassung, Fremdkraftdefinition mit Betrag,
Richtung, Angriffspunkt und Bezugssystem, Elementstellungen, Wasserparameter,
gleichfoermige Wasserstroemung sowie Solvergrenzen und Toleranzen.
Modellfeste Komponenten bleiben im Modellrahmen konstant; ihre Weltkomponenten
koennen sich bei einer iterierten Orientierung aendern. Das ist keine
zeitabhaengige Eingabeaenderung.

Genau eine aktivierbare Fremdkraft ist konfigurierbar. Ihr Angriffspunkt
ist modellfest, ihr Kraftvektor wahlweise modell- oder weltfest. Eine
deaktivierte Fremdkraft wirkt mit null Kraft und null Moment.

Vorgeschlagener Suchraum: Z, Rollen, Stampfen, Gieren sowie die horizontalen
Geschwindigkeitskomponenten Vx/Vy im Weltrahmen. Es gelten Vz = 0 und
Winkelgeschwindigkeit = 0. Die gesuchte Orientierung ist eine physikalische
3D-Orientierung; eine numerische Parametrisierung darf keine Euler-Singularitaet
als physikalische Unloesbarkeit ausgeben.

X/Y sind bei homogenem Wasser und ortsunabhaengigen Lasten nicht aus einer
Kraftbilanz bestimmbar. Sie sind Referenzkoordinaten, keine Solverergebnisse.
Eine konstante Weltgeschwindigkeit bedeutet fortschreitende X/Y-Position,
nicht einen ortsfesten Koerper. Zwangskraefte fuer festgehaltene Freiheitsgrade
duerfen nicht stillschweigend eingefuehrt werden.

## 4. A-C-00115: Gemeinsame physikalische Bilanz

Zu loesen sind alle drei Komponenten von Summe F = 0 und Summe M_S = 0.
Einzubeziehen sind Gewicht, Auftrieb, vorhandene Hydrodynamik und Fremdkraft
mit ihrem Moment um den aktuellen Gesamtschwerpunkt S.
Ein Geschwindigkeitsbegrenzer oder ein kuenstliches Festhalten darf die
fehlende Kraft-/Momentenbilanz nicht ersetzen.

Die Elementstellungen gelten konsistent fuer alle davon betroffenen
Geometrien und Eigenschaften, insbesondere Benetzung, Auftrieb, Masseverteilung,
Schwerpunkt und hydrodynamische Angriffspunkte. Unveraenderte physikalische
Algorithmen und ein Quellmesh je Element bleiben Grundlage; keine zweite
unabhaengige Physikgeometrie einfuehren.

Relative Anstroemung ergibt sich aus gesuchter Bootsgeschwindigkeit und
vorgegebener Fluidgeschwindigkeit. Beschleunigungsabhaengige Zusatzmassen
tragen im stationaeren Endzustand keine Beschleunigungskraft bei;
bestehende stationaer wirksame Widerstandsbeitraege bleiben enthalten.
Ein numerischer Relaxationsweg waere kein physikalischer Zeitverlauf.

## 5. A-C-00116: Ergebnis, Eindeutigkeit und Fehler

Ergebnis: Pose, Welt-/Relativgeschwindigkeit einschliesslich Betrag und
Abdrift, einzelne Kraft-/Momentenbeitraege mit Herkunft und Bezugssystem,
alle sechs ungerundeten Residuen, Toleranzen, Iterationen, Modellfassung
und eingefrorene Eingaben. Vertikale Geschwindigkeit und Drehraten sind
explizit als fest null auszuweisen.

Nur wenn jede Kraft- und Momentenkomponente ihre jeweilige Toleranz erfuellt,
darf das Ergebnis als konvergiert gelten. Keine blosse Fz/Mx/My-Pruefung.
Konvergenz ist kein Nachweis dynamischer Stabilitaet oder globaler Eindeutigkeit.
Mehrere Loesungen, Rangdefizite, fehlende Richtungsvorgaben, fehlender
Widerstand, unzulaessige Stellungen, nicht endliche Werte, Flutung,
Iterationsgrenze und Nutzerabbruch sind explizit zu behandeln.

Insbesondere kann eine rein modellfeste Last in ruhigem homogenem Wasser
den absoluten Gierwinkel unbestimmt lassen. In diesem Fall bleibt der
Startkurs als ausdruecklich deklarierte Referenz erhalten, nicht als
berechneter Kurs. Dies fuehrt kein kuenstliches Moment ein; weiterhin
sind alle sechs Bilanzkomponenten zu pruefen. Nicht ausgeglichene
Giermomente bei festem Ruderausschlag koennen eine geradlinige stationaere
Loesung ausschliessen; keine automatische Kreisfahrt oder Ruderregelung.
Flutungsstatus und benoetigte Wiederholungen muessen nachvollziehbar sein.

## 6. A-C-00117: Numerische Abnahme und Regression

Verbindliche Obergrenzen: jede Kraftresiduumkomponente <= 0,1 N, jede
Momentresiduumkomponente <= 0,1 Nm; Fahrtabweichung gegen eine unabhaengige
analytische Referenz <= 0,1 m/s. Intern wird eine hoehere Genauigkeit
angestrebt, soweit Modell und Numerik sie tragen. Tatsaechliche Residuen
und verwendete engere Toleranzen sind zu berichten; keine stillschweigende
Lockerung ueber die Obergrenzen und kein unbelegtes Genauigkeitsversprechen.

- Symmetrischer Koerper ohne Fremdkraft, ruhendes Wasser: bisherige
  hydrostatische Lage erhalten; keine kuenstliche Fahrt. Freies Gieren
  als unbestimmt ausweisen.
- Kontrolliertes Widerstandsmodell mit F = c*v^2 und zentrischem Zug:
  stationaerer Betrag stimmt mit sqrt(F/c) innerhalb deklarierter
  Geschwindigkeitstoleranz ueberein; alle sechs Bilanzkomponenten pruefen.
- Exzentrischer Zug und unterschiedliche feste Ruder-/Schwertstellungen:
  tatsaechliche Core-Geometrie und Einzelbeitraege nachweisen; entweder
  vollstaendige Bilanz oder explizit kein gefundenes Gleichgewicht.
- Galilei-Vergleich bei gleichmaessig verschobener Wasser- und
  Bootsgeschwindigkeit; gleiche relative Anstroemung liefert gleiche Lasten.
- Modell-/Weltkoordinatentransformation der Fremdkraft und ihres Hebelarms
  sowie Einheiten mit unabhaengigen Referenzen pruefen.
- Ungueltige Eingaben, Rangdefizit, Abbruch, fehlende Loesung und
  unveraenderte Eingaben waehrend eines Laufs testen.

Loesungsstrategie, endliche Suchgrenzen und numerische Konditionierung sind
im Plan vor Implementierung zu konkretisieren und mit den obigen
Abnahmekriterien zu pruefen. Grenzen duerfen keine Loesung vortaeuschen;
Grenztreffer sind zu melden. Diese Liste behauptet keine bereits
ausgefuehrten neuen Physiktests.

## 7. Stationaere Dreh- und Kurvenfahrt

**Status:** Am 02.11.2026 ausdruecklich finalisiert und fuer C-0.8 zur
Umsetzung freigegeben. Der Modus ist additiv und wird nur mit
`StationaereGleichgewichtKonfiguration::kurvenfahrt=true` aktiviert;
der Standard `false` behaelt den geradlinigen Solverpfad unveraendert.
Die Abschnitte 1-6 gelten im bisherigen Umfang weiter. Die Herkunft der
Erweiterungsanforderungen ist in der
[Entwurfspunkteliste](Entwurfspunkte/Stationaeres_Fahrtgleichgewicht.json)
aufgefuehrt.

### 7.1 A-C-00118: Gesuchter Zustand und unveraenderliche Eingaben

Gesucht wird ein relatives Gleichgewicht: Die Geschwindigkeit des
Gesamtschwerpunkts relativ zum gleichfoermig bewegten Wasser ist im
bootsfesten Rahmen konstant. Damit bleiben Fahrtbetrag und Abdrift
konstant; Krängung und Stampfen bleiben konstant, ebenso die vorzeichenbehaftete
Kursdrehrate um die Weltvertikale. Geradeausfahrt mit Drehrate null,
Ruhe und gegebenenfalls reine Drehung sind gesondert unterscheidbare
Sonderfaelle, keine erzwungenen Ersatzloesungen.

Konstanter Fahrtbetrag allein genuegt nicht: Richtungsaenderung,
Beschleunigung und Momentenbilanz muessen zur gesuchten Bewegung passen.
Konstantes unausgeglichenes Moment bedeutet nicht konstante Drehrate.
Es wird kein zeitlich eingeschwungener Zustand behauptet, nur weil
aufeinanderfolgende numerische Iterationen aehnliche Werte liefern.

Unveraendert genau eine aktivierbare Kraft mit drei Komponenten und einem
gemeinsamen modellfesten Ansatzpunkt; Kraftbezug modell- oder weltfest.
Modellfassung, Wasserparameter, Stroemung und alle Ruder-/Schwertstellungen
bleiben pro Lauf fest. Keine Ruderoptimierung, kein Autopilot,
keine neue Windphysik und kein Geschwindigkeitsbegrenzer als Bilanzersatz.
Die Drehrate ist Ergebnis, nicht ein zusaetzlich optimiertes Stellglied.

### 7.2 A-C-00119: Kinematik, Bezugssysteme und physikalische Residuen

Fuer die vertikale Kurvenfahrt seien R die Rotation Modell -> Welt,
k die Weltvertikale, u die konstante bootsfeste wasserrelative
Schwerpunktgeschwindigkeit und r die konstante Kursdrehrate in rad/s.
Dann gilt bei gleichfoermiger Stroemung c:

```text
v_S,W = c + R*u
omega_W = r*k
omega_B = R^T*(r*k)
a_S,W = omega_W x (R*u)
alpha_W = 0
```

Die Geometrie, Vertikalbedingung fuer den Schwerpunkt und die
Pose-Rekonstruktion sind konsistent zu loesen; ein gerolltes Boot
hat im Allgemeinen nicht omega_B = [0,0,r]. Ursprungsgeschwindigkeit
und Schwerpunktgeschwindigkeit duerfen nicht verwechselt werden.
X/Y und bei Symmetrie der Startkurs bleiben explizite Referenzen.

Sechs Residuen sind weiterhin erforderlich, jetzt jedoch als
**Bewegungsbilanz**, nicht pauschal als Summe der aeusseren Lasten:

```text
R_F,W = Summe F_aeusserlich,W - m*a_S,W
R_M,B = Summe M_aeusserlich,S,B
        - (I_S,B*dot(omega_B) + omega_B x (I_S,B*omega_B))
```

Diese Gleichungen beschreiben den Starrkoerperanteil. Bei
beschleunigungsabhaengigen Fluid-/Zusatzmassentermen ist der vorhandene
Core-Traegheitsvertrag konsistent zu verwenden und zu dokumentieren:
Kurvenfahrt hat trotz konstantem Fahrtbetrag Beschleunigung. Die
Null-Beschleunigungsannahme aus Abschnitt 4 gilt hier nicht; Terme weder
unterschlagen noch doppelt zaehlen. Keine neue unabhaengige GUI-Physik.
Bei stationaerer Roll-/Stampflage ist dot(omega_B) null; der Kreiselterm
kann dennoch ungleich null sein.

Hydrodynamik erhaelt auch die lokale Anstroemung aus Rotation
omega_W x (p_W-S_W). Bewegte Elementgeometrie, Schwerpunkt, Traegheit,
Benetzung und Hebelarme muessen denselben eingefrorenen Stellungen folgen.
Der Plan legt Suchgroessen, endliche Grenzen, Skalierung,
Trust-Region-Levenberg-Marquardt-Schritt, Rangdiagnose und Abbruchbedingungen
fest; eine zusaetzliche Drehratenvariable bei unveraendertem
Nullkraftresiduum genuegt nicht.

### 7.3 A-C-00120: Lastinvarianz und Grenzen einer stationaeren Loesung

Eine modellfeste Kraft kann mit dem Boot mitdrehen. Eine konstante
Weltkraft ist bei r != 0 dagegen im Bootsrahmen im Allgemeinen
zeitabhaengig. Eine nur fuer eine Kursposition ausgeglichene Bilanz
darf deshalb nicht als dauerhaft stationaere Kreisfahrt gelten.
Core muss die Invarianz des gesamten Falls pruefen: zum Beispiel
ist eine vertikale Weltkraft unter Gierdrehung invariant, eine
horizontale Weltkraft im Allgemeinen nicht. Unvertraegliche Faelle
benoetigen eine explizite Diagnose; kein stiller Wechsel des Kraftframes
und keine automatische dynamische Simulation.

Kurvenradius nur bei geeignetem r != 0 aus der horizontalen
wasserrelativen Fahrt bestimmen und als Radius im mitstroemenden
Wasserrahmen kennzeichnen. Bei r = 0 keinen endlichen Radius erfinden;
bei reiner Drehung keinen Abdriftwinkel aus einem Nullvektor berechnen.
Die Weltbahn kann bei Stroemung von einem Kreis abweichen.

### 7.4 A-C-00121: Ergebnisvertrag Core -> GUI

Core liefert den expliziten Loesungstyp geradeaus/Kurvenfahrt/reine
Drehung, Modellfassung, unveraenderliche Eingaben, Bezugssysteme,
Pose-Referenz, Welt- und Relativgeschwindigkeit, vorzeichenbehaftete
Kursdrehrate in rad/s sowie omega in Modellachsen in rad/s. Die
Kursdrehrate ist nicht der Betrag von omega und bei Schraeglage
nicht einfach dessen Modell-Z-Komponente.

Auszugeben sind aeussere Kraft-/Momentensummen **getrennt** von
den sechs Bewegungsresiduen, Beschleunigung/Kreiselbeitraegen,
Toleranzen, Iterationen, Referenzstatus und Diagnose. Eine resultierende
Seitenkraft kann eine korrekte Kreisbewegung tragen; die GUI darf sie
nicht anhand des alten Summe-F=0-Vertrags als Fehler einstufen.
Konvergiert nur bei erfuellter Kinematik, invarianten Lasten und allen
sechs Bewegungsresiduen innerhalb der deklarierten Toleranzen.
Nichtkonvergenz ist kein Beweis der globalen Nichtexistenz einer Loesung.

V6 ist der Standard fuer `Laufoptionen::ergebnis_json_format` und
`zustand_json(..., ErgebnisJsonFormat)`. Mit `ErgebnisJsonFormat::Kompatibel`
bleiben V1-V5 in Feldinhalt und Bedeutung unveraendert abrufbar; V5-Nullraten
werden nicht umgedeutet. `StationaereGleichgewichtKonfiguration::kurvenfahrt`
ist ein bool mit Standard `false`; ein in alten Adaptern fehlendes JSON-Feld
bedeutet `false`. Neue C++-Schnittstellen sind vor Implementierung einzeln
im Core-Register vergeben. Die GUI zeigt und konvertiert nur Core-Werte.

V6 fuehrt top-level `kursdrehrate_rad_s` (Zahl oder `null`) und
`kursdrehrate_status` (`gueltig` oder `nicht_bestimmbar`) sowohl fuer
dynamische als auch stationaere Zustandszeilen ein. Positiv ist die
Rechtshaender-Drehrate um Welt-`+Z`, also die Ableitung von
`atan2(forward_y, forward_x)`; bei nahezu vertikaler Vorwaertsachse ist sie
nicht bestimmbar. Im `stationaer`-Objekt stehen `kurvenfahrt`,
`loesungstyp` (`geradeaus`, `kurvenfahrt`, `reine_drehung`),
`omega_modell_rad_s`, `aeussere_kraft_welt_n`,
`aeusseres_moment_modell_nm`, `residuum_kraft_bewegung_welt_n`,
`residuum_moment_bewegung_modell_nm`, `radius_m` (Zahl oder `null`) und
`radius_status` (`gueltig`, `geradeaus`, `reine_drehung`,
`nicht_bestimmbar`). Ein inkompatibler V5-Serializer lehnt Kurvenresultate
explizit ab.

### 7.5 A-C-00122: Numerische Abnahme und Regression

- Gegen unabhaengige Referenzen gelten: Kursdrehratenfehler <= 0,01 Grad/s;
  Roll- und Stampfwinkelfehler je <= 0,1 Grad; Schwerpunkt-Vz <= 0,001 m/s;
  Roll- und Stampfrate je <= 0,01 Grad/s; Fahrtbetragsfehler <= 0,1 m/s.
- Jede Komponente des Bewegungs-Kraftresiduums <= 0,1 N und des
  Bewegungs-Momentresiduums <= 0,1 Nm. Intern verwendete engere Toleranzen
  und ungerundete Messwerte sind auszugeben; die Obergrenzen werden nie
  stillschweigend gelockert.
- Geradeaus-/Ruhegrenzfall, positive und negative Drehrate, kontrollierte
  Kreisreferenz F_quer = m*v*r ohne Zusatzmasse sowie unabhaengige
  Starrkoerper-Kreiselreferenz.
- Lokale Rotationsanstroemung, Fluidzusatztraegheit, mehrere feste Stellungen,
  Frame-/Galilei-Vergleich, unbestimmter Referenzkurs und inkompatible
  weltfeste Last pruefen.
- Reine Drehung, Grenzfall r -> 0, ungueltige Werte, Flutung, Abbruch,
  Nichtkonvergenz und unveraenderte Altvertraege pruefen.
- Numerische Konvergenz belegt weder dynamische Stabilitaet noch globale
  Eindeutigkeit. Diese Eigenschaften bleiben ausserhalb der Abnahme.

## Herkunft

| Entwurfspunkt | Finale Anforderung |
|---|---|
| IDEE-C-00076 | A-C-00114 |
| IDEE-C-00077 | A-C-00115 |
| IDEE-C-00078 | A-C-00116 |
| IDEE-C-00079 | A-C-00117 |
| IDEE-C-00080 | A-C-00118 |
| IDEE-C-00081 | A-C-00119 |
| IDEE-C-00082 | A-C-00120 |
| IDEE-C-00083 | A-C-00121 |
| IDEE-C-00084 | A-C-00122 |
