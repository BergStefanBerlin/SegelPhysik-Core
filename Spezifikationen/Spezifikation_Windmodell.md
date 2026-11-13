# Spezifikation: Windmodell und Windvertrag

**Stand:** 13.11.2026 · **Status:** Fachlich freigegeben; Umsetzung, Tests
und Abnahme noch offen. Diese Datei ist die einzige Core-Spezifikation für
das Windfeld und den fachlichen Windvertrag. Die Herkunft der Anforderungen
steht in der [Entwurfspunkteliste](Entwurfspunkte/Windmodell.json); der
[Umsetzungsplan C-0.23](../Umsetzungsplaene/Umsetzungsplan_C-0.23.md)
führt die Arbeiten; die Durchführung wurde ausdrücklich beauftragt.
Das GUI-Gegenstück ist die
[GUI-Windspezifikation](../../GUI/Spezifikationen/Spezifikation_GUI_Winddarstellung.md).
Für die gemeinsame Sammlung der Mastspitze und weiterer Punkte gilt die
[Referenzpunkt-Spezifikation](Spezifikation_Referenzpunkte.md).

## Anforderungszuordnung

Die folgenden Anforderungen umfassen die bezeichneten Abschnitte und die
konkreten Prüffälle in Abschnitt 8. Alle sind zur Umsetzung offen.

| Kennung | Fachlicher Vertrag und Abnahmekriterium |
|---|---|
| A-C-00158 | Weltkompass und Strömungsrichtung gemäß Abschnitten 2–3: Himmelsrichtungen, Herkunft und horizontale Windvektoren stimmen auch bei Krängung und Stampfen. |
| A-C-00159 | Homogenes Feld und Richtungsmodi gemäß Abschnitt 4: gleiche Höhe liefert gleiche Werte; Statik und beide Dynamikmodi sowie sprungfreies Umschalten sind nachgewiesen. |
| A-C-00160 | Höhenprofil, Referenzgrenzen und Fehler gemäß Abschnitt 5: Formel, lokaler Mindestwert und ungekappte lokale Werte oberhalb 100 m/s werden geprüft. |
| A-C-00161 | Lokaler scheinbarer Wind an der Mastspitze gemäß Abschnitt 6: Translation und Rotation ergeben die korrekte Vektordifferenz im Weltframe. |
| A-C-00162 | Richtungsstatus und Anzeigeschwelle gemäß Abschnitt 6: horizontale und räumliche Beträge sowie wahre und scheinbare Windrichtung werden getrennt geprüft. |
| A-C-00163 | Zeitbezogene Windänderung und Wiedergabe gemäß Abschnitten 4 und 7: bestätigte Änderungen wirken ab ihrem Zeitpunkt; historische Zustände bleiben reproduzierbar. |
| A-C-00164 | Typisierter Core–GUI-Vertrag gemäß Abschnitt 7: Einstellungen, Profil, Vektoren, Zeit, Bezug und Diagnosen sind konsistent abrufbar; Fehler werden nicht als gültige Werte ausgegeben. |
| A-C-00165 | Mastspitzenbezug gemäß Abschnitten 6–7: eine vorhandene Spitze übernehmen oder gemäß Referenzpunkt-Spezifikation aus dem höchsten Mastpunkt bestimmen; fehlende oder mehrdeutige Mastdaten ausdrücklich melden. |

## 1. Ziel und Abgrenzung

Der Core beschreibt den wahren Wind im Simulationsraum und bestimmt daraus
den scheinbaren Wind relativ zu bewegten Bezugspunkten. Er rechnet
unabhängig von der GUI und stellt fachlich eindeutige Eingaben, Ergebnisse
und Diagnosen bereit. Die GUI holt diese Daten ab, visualisiert und bedient
sie, berechnet aber keine zweite Windphysik. Das Modell gilt für Statik
und Dynamik.

Diese Spezifikation definiert **keine Segelkraft**, keinen aerodynamischen
Lookup, keine Abschattung, keinen Nachlauf und kein Materialmodell. Aus
vorhandener Windanzeige folgt keine funktionsfähige Segelphysik. Der bisher
existierende Ersatz-SDP aus einer Fremdkraft ist nicht automatisch ein
echter aerodynamischer Segeldruckpunkt; siehe
[Lateraldruckpunkt](Spezifikation_Lateraldruckpunkt.md). Ob und welcher
SDP für eine spätere Windanzeige verfügbar ist, bleibt eine offene
Entscheidung.

## 2. Begriffe und Einheiten

| Größe | Bedeutung | Darstellung / Bezug |
|---|---|---|
| TWD | Richtung des wahren Windes | Weltkompasswinkel in Grad; Richtung, **aus der** der Wind kommt |
| TWS | Betrag der wahren Windgeschwindigkeit | m/s; `v0` ist TWS an der Referenzhöhe `h0`; lokal gilt `v(h)` |
| AWD | Richtung des scheinbaren Windes | Weltkompasswinkel der Herkunft in Grad, aus der horizontalen Projektion; bei Bedarf Umrechnung in ein Zielsystem |
| AWS | Betrag der scheinbaren Windgeschwindigkeit | m/s, mit Angabe des Bezugspunkts; horizontale Komponente im Zeigerelement, räumlicher Vektor in 3D |
| `h` | Höhe des Auswertungspunkts über der Wasseroberfläche | m, nicht Höhe über Boot oder Mastfuß |
| `h0`, `v0` | Referenzhöhe und wahre Windgeschwindigkeit dort | m bzw. m/s |
| Strömungsvektor | Geschwindigkeitsvektor der Luft | Zeigt dorthin, wohin die Luft strömt; Weltkoordinaten, m/s |

TWD und AWD bezeichnen die Wind-**Herkunft**; der Strömungsvektor zeigt
horizontal um 180° entgegengesetzt. Beispiel: 0° bedeutet Wind aus Norden
und Strömung nach Süden; 90° bedeutet Wind aus Osten und Strömung nach
Westen (270°). Die 3D-Pfeilspitzen zeigen ausdrücklich **wohin** der Wind
strömt. Werte ohne eindeutigen Zeitpunkt, Bezugspunkt, Höhe und Frame
sind nicht als vergleichbar zu behandeln.

## 3. Koordinatensystem und Windrichtung

Die Richtungsskala ist immer weltbezogen: Nord 0°, Ost 90°, Süd 180°,
West 270°, Winkel im Uhrzeigersinn und zyklisch modulo 360°. Der Core
rechnet mit Weltvektoren; eine spätere Transformation in Modell-, Segel-
oder Darstellungssysteme verändert nicht die fachliche Bedeutung.

Das bestehende modellfeste System ist rechtshändig: X zum Bug, Y nach
Backbord, Z nach oben bei aufrechtem Modell
([Modellbeschreibung](Spezifikation_Modellbeschreibung.md#a-c-00001-einheitliches-modellfestes-koordinatensystem)).
Der bestehende GUI-Kompass berechnet den Kurs aus dem horizontalen
Bootsvorwärtsvektor mit `atan2(-y, x)`, also entspricht in diesem Bezug
Welt-X Nord und Welt-Y West. Der Windvertrag muss dieselbe Zuordnung
benutzen; eine gesonderte oder stillschweigend abweichende Weltachse ist
unzulässig.

Der **wahre** Wind ist parallel zur Wasseroberfläche: Bei der bestehenden
horizontalen Wasserfläche hat sein Weltvektor die Z-Komponente null.
Bootskrängung oder Stampfen kippt diesen Vektor nicht mit. Die Ausrichtung
des Boots für den mitgeführten Modus wird aus seiner **horizontalen**
Vorwärtsrichtung abgeleitet, nicht aus einer vollständigen 3D-Rotation
des Windvektors. Für eine nicht bestimmbar horizontale Vorwärtsrichtung
ist kein Winkel zu erfinden; der Zustand wird ausdrücklich diagnostiziert.
Die numerische Toleranz ist bei der Umsetzung festzulegen und nachzuweisen.

## 4. Windfeld und zeitliches Verhalten

Zu jedem Simulationszeitpunkt liegt über dem gesamten Simulationsraum ein
Feld mit überall gleicher horizontaler Richtung. An Punkten gleicher
Höhe über Wasser ist die wahre Geschwindigkeit gleich. Sie hängt gemäß
Abschnitt 5 von der Höhe ab; das Feld hängt nicht von der horizontalen
Position ab.

| Betriebsart | Richtungsverhalten |
|---|---|
| Statik | Vor der Berechnung festgelegte Richtung wird bei Änderungen der Bootsausrichtung mitgeführt. Ihre Angabe bleibt ein Weltkompasswinkel. |
| Dynamik, bootmitgeführt | Richtung ändert sich mit der horizontalen Bootsausrichtung; die relative horizontale Richtung zum Boot bleibt erhalten. |
| Dynamik, weltfest | Richtung bleibt im Weltkoordinatensystem konstant. Die Bootsbewegung ändert sie nicht. |

Die dynamischen Verhaltensweisen sind über einen Umschalter wählbar.
Beim Umschalten während der Dynamik bleibt die aktuelle Weltrichtung
zunächst erhalten; erst danach wirkt das neue Richtungsverhalten. Richtung
und Geschwindigkeit dürfen während der Dynamik geändert werden; bestätigte
Änderungen wirken ab dem Änderungszeitpunkt, ohne frühere Ergebnisse
rückwirkend zu verändern. Die Referenzhöhe `h0` kann nur vor einem neuen
Start geändert werden, nicht während der Dynamik. Die frühere Idee eines
„Testmodus mitdrehend“ ist dieselbe bootmitgeführte Variante und kein
dritter Modus. Start und Neustart verwenden die jeweils bestätigten
Einstellungen; historische Ergebnisse werden dabei nicht nachträglich
verändert. Weitere Eingabeänderungen außerhalb des hier beschriebenen
Umfangs sind nicht Gegenstand dieser Spezifikation.

Die horizontale Geschwindigkeit darf auch bei bootmitgeführter Richtung
nicht aufgrund der Krängung eine Welt-Z-Komponente erhalten. Ausrichtung
und Geschwindigkeitsprofil sind getrennte Größen.

## 5. Höhenprofil

Mit der vorgegebenen Rauigkeitslänge `z0 = 0,0002 m` und der bestätigten
lokalen Mindestgeschwindigkeit gilt im gültigen Höhenbereich:

```text
v_roh(h) = v0 * ln(h / z0) / ln(h0 / z0)
v(h)     = max(0,01 m/s, v_roh(h))
```

`ln` ist der natürliche Logarithmus. `h`, `h0` und `z0` sind in derselben
Einheit Meter anzugeben; `v0` und `v(h)` in m/s. Bei gültigem `h0` und
`0,01 <= v0 <= 100 m/s` gilt `v(h0) = v0`. Richtungsvektor und positiver
Geschwindigkeitsbetrag ergeben den wahren Strömungsvektor auf Höhe `h`.

Die Formel ist bei `h <= 0` nicht definiert und bei `h0 = z0` singulär;
unterhalb `z0` kann sie für positives `v0` negative Werte ergeben.
Für die Referenzeingaben gelten `0,01 <= v0 <= 100 m/s` und
`0,01 <= h0 <= 100 m`; für die Auswertung gilt `h >= z0`. Die Werte
müssen endlich sein. An jeder gültigen
Auswertungshöhe werden Rohprofilwerte unter `0,01 m/s` auf `0,01 m/s`
begrenzt. Damit wird das ursprüngliche logarithmische Profil nahe der
Untergrenze ausdrücklich ergänzt. Ungültige Eingaben und Höhen erzeugen
einen Fehler, insbesondere an und unter der Wasseroberfläche; keine NaN
oder erfundenen gültigen Werte. Die Grenze `100 m/s` gilt für den
Referenzwert `v0`, nicht als Obergrenze für lokal berechnete Profilwerte:
Diese dürfen größer als `100 m/s` sein.

Die GUI begrenzt zulässige numerische Eingaben vor der Übergabe auf die
jeweilige Referenzgrenze; direkte ungültige Core-Eingaben erzeugen
weiterhin einen Fehler. Die Rundung in der Anzeige verändert weder den
bestätigten Referenzwert noch das berechnete Profil.

Scheinbarer Wind muss auch bei geringem wahrem Wind aus der
Fahrtgeschwindigkeit ermittelbar bleiben; er darf bei Vektorgleichheit
physikalisch null werden.

Beispiel mit `h0 = 10 m`, `v0 = 6 m/s`, gerundet:

| `h` | `v(h)` |
|---:|---:|
| 1 m | 4,72 m/s |
| 5 m | 5,62 m/s |
| 10 m | 6,00 m/s |
| 20 m | 6,38 m/s |

## 6. Scheinbarer Wind

Für einen Punkt `p` und denselben Zeitpunkt ist der scheinbare
**Strömungsvektor** die Luftgeschwindigkeit relativ zu diesem Punkt:

```text
v_scheinbar_welt(p) = v_wahr_welt(h(p)) - v_punkt_welt(p)
```

Das Minus gilt für Geschwindigkeitsvektoren, die jeweils die tatsächliche
Bewegungsrichtung anzeigen. Bei entgegengesetzt definierten
Herkunftsvektoren ist die Umrechnung vorher durchzuführen; Winkelsummen
ersetzen keine Vektorrechnung.

Im jetzigen Umfang wird für die Windanzeige die lokale Bewegung der
Mastspitze berücksichtigt. Die Windphysik für weitere Simulationspunkte
und bewegliche Segel wird erst bei deren späterer Spezifikation
festgelegt. Für einen starr am Boot befestigten Punkt ist die
Geschwindigkeit aus Schwerpunkttranslation und Rotation um den
Schwerpunkt zu bestimmen:

```text
v_punkt_welt = v_schwerpunkt_welt + omega_welt × (p_welt - schwerpunkt_welt)
```

Die bestehende Zustandsdarstellung führt die Winkelgeschwindigkeit im
Modellframe; vor dem Kreuzprodukt wird sie passend in den Weltframe
rotiert. Die lokale Windhöhe ist jeweils die Welt-Höhe über Wasser, nicht
die modellfeste Z-Koordinate.

AWD ist der Weltkompasswinkel der **horizontalen Projektion** des
scheinbaren Vektors. Ein durch Heben oder Rotation entstehender vertikaler
Anteil des scheinbaren Windes darf nicht als vertikaler *wahrer* Wind
missverstanden werden. Das Zeigerelement zeigt die horizontale Komponente;
die 3D-Darstellung verwendet den räumlich korrekten Vektor. Bei
verschwindender horizontaler Projektion ist AWD nicht bestimmbar; kein
willkürlicher Winkel. Ein von null verschiedener vertikaler Vektor bleibt
dabei als 3D-Wert vorhanden.

Für wahren und scheinbaren Wind gilt **jeweils separat**: Bei
Windgeschwindigkeit bis einschließlich `0,1 m/s` wird die betreffende
Richtung ausgeblendet. Für den Kompass wird der horizontale Betrag
verglichen; für den 3D-Pfeil der räumliche Betrag. Beispielsweise kann
der scheinbare Kompassmarker bei kleiner horizontaler Geschwindigkeit
ausgeblendet sein, während ein hinreichend großer vertikaler
3D-Strömungsvektor sichtbar bleibt. Die Berechnung des Vektors und seines
Betrags wird durch die Anzeigeschwelle nicht ersetzt.

Der scheinbare Wind für den Mastspitzenpfeil wird **lokal an der
Mastspitze** berechnet; ihr Bezug kommt aus der gemeinsamen
Referenzpunktsammlung. Eine separate SDP-Windanzeige folgt erst später,
wenn ein echter SDP verfügbar ist; ein fehlender echter SDP rechtfertigt
keinen stillen Ersatzpunkt.

## 7. Fachlicher Core–GUI-Vertrag

Ein typisierter Vertrag soll Einstellungen sowie zeitbezogene Vektoren,
Werte, Bezugspunkte und Status bereitstellen. Der Core rechnet unabhängig;
die GUI holt die Daten zur Darstellung ab. Die folgende Liste beschreibt
die **semantischen Daten**, keine bereits existierenden öffentlichen
C++-Typen, Funktionssignaturen oder ein neues Dateiformat. Konkrete
Exportinterfaces und gegebenenfalls Registereinträge werden erst nach
Klärung und Freigabe definiert.

| Richtung | Fachlicher Inhalt |
|---|---|
| GUI → Core | Gewählter Modus, Eingabewinkel mit expliziter Konvention, Referenzhöhe `h0` und Geschwindigkeit `v0`; Statik-/Dynamikkontext. |
| Core → GUI: Einstellungen | Bestätigte Werte und Modus; bei Fehlern nachvollziehbare Diagnose statt vorgetäuschter Übernahme. |
| Core → GUI: wahrer Wind | Ausgewerteter Welt-Strömungsvektor, Geschwindigkeit und Richtungsstatus für explizite Höhe / Position; Referenzprofil. |
| Core → GUI: scheinbarer Wind | Welt-Strömungsvektor, Betrag, Richtungsstatus, Bezugspunkt und Auswertungshöhe; die Mastspitze ist über die gemeinsame Referenzpunktsammlung eindeutig zugeordnet. |
| Core → GUI: Profil | Ausreichende Core-Daten bzw. Core-Auswertungen für das Diagramm, einschließlich Bereich, Einheiten und Referenzpunkt; kein GUI-eigener Formelvertrag. |
| Core → GUI: Konsistenz | Simulationszeit/Identität des Zustands, Pose, Werte und Bezugspunkte aus demselben Ergebnisstand, soweit für die Darstellung benötigt. |
| Fehler / Nicht verfügbar | Expliziter Status und Diagnose, insbesondere für ungültige Parameter, nicht bestimmbare Richtung, fehlenden Bezugspunkt und nicht verfügbares Profil. Keine Null als Fehlerersatz. |

Für Statik, Dynamik, Wiedergabe und Verbraucher gelten dieselben
Definitionen und Einheiten. Der Mastspitzenbezug muss Weltposition,
Zeitpunkt und benötigte Punktgeschwindigkeit eindeutig zuordnen. Die
konkrete technische Schnittstelle und ihre Verfügbarkeit werden bei der
Umsetzung nachgewiesen; Liveänderungen und Wiedergabe folgen Abschnitt 4.

## 8. Prüffälle

Diese Fälle sind verbindlich geplant, aber noch keine ausgeführten Tests:

| Fall | Zu prüfen |
|---|---|
| Referenzprofil | Bei `h = h0` ist `v = v0`; obige vier Werte innerhalb einer festzulegenden numerischen Toleranz. |
| Mindestwind | Ein gültiger Punkt mit Rohprofilwert unter `0,01 m/s` liefert `0,01 m/s`; scheinbarer Wind berücksichtigt dennoch die Punktgeschwindigkeit und kann bei Vektorgleichheit null sein. |
| Referenzgrenzen | Direkte Core-Eingaben außerhalb `0,01–100 m/s` für `v0` beziehungsweise `0,01–100 m` für `h0` erzeugen einen Fehler; ein lokaler Profilwert oberhalb `100 m/s` wird nicht gekappt. |
| Horizontale | Krängung und Stampfen kippen den wahren Windvektor nicht; gleicher Höhenwert an verschiedenen horizontalen Positionen ist gleich. |
| Mitgeführt / weltfest | Bootskursänderung ändert im einen Modus den Weltwinkel entsprechend, im anderen nicht; beim Umschalten bleibt der aktuelle Weltwinkel zunächst erhalten; jeweils Horizontalität erhalten. |
| Kompasskonvention | Nord/Ost/Süd/West, 0°/360° und Umrechnung zwischen Herkunfts- und Strömungsrichtung; ausdrücklich gewählte Winkelkonvention nachweisen. |
| Relativwind | Ruhendes Boot: scheinbarer und wahrer Strömungsvektor identisch. Bei bekannter Translation gilt die Vektordifferenz; Rotationsanteil am versetzten Punkt gesondert prüfen. |
| Höhe / Punkte | Unterschiedliche Höhen erzeugen Profilwerte; SDP und Mastspitze werden nicht stillschweigend gleichgesetzt. |
| Anzeigeschwelle | Wahrer und scheinbarer Wind werden separat geprüft: Bei `0,1 m/s` ausgeblendet, knapp darüber sichtbar. Für den Kompass gilt der horizontale, für den 3D-Pfeil der räumliche Betrag; ein vertikaler scheinbarer Vektor kann nur im 3D sichtbar sein. |
| Fehlerfälle | Nicht endliche Werte, singuläre Referenzhöhe, unzulässige Höhe, fehlender Punkt und Nullprojektion erzeugen expliziten Status, keine erfundenen Daten. |
| Snapshot | Anzeige und Auswertung verwenden zeitlich zusammengehörige Pose, Wind und Bezugspunkte; Wiedergabe reproduziert den gespeicherten Stand. |

## 9. Abgrenzung und noch ausstehende Nachweise

Konkrete Schnittstellenformen, diagnostische Texte und numerische
Implementierungstoleranzen werden bei der Umsetzung festgelegt und
nachgewiesen; sie dürfen die obigen fachlichen Verträge nicht abschwächen.
Der Bezug weiterer Simulationspunkte, beweglicher Segel und eines echten
SDP gehört zur späteren Windphysik, nicht zum vorliegenden Umfang.
Implementierung, Integration, Tests und Nutzerabnahme sind offen.
