# Spezifikation: Referenzpunkte im Core–GUI-Vertrag

**Stand:** 13.11.2026 · **Status:** Fachlich freigegeben; Umsetzung, Tests
und Abnahme offen. Diese Spezifikation ist die maßgebliche Core-Festlegung
für die gemeinsame Übergabe von Referenz- und Kraftangriffspunkten. Die
Ausführung ist im [Plan C-0.23](../Umsetzungsplaene/Umsetzungsplan_C-0.23.md)
vorgesehen und ausdrücklich beauftragt.
Die fachlichen Definitionen der einzelnen Punkte bleiben in ihren
jeweiligen bestehenden Spezifikationen.

## A-C-00166 – Gemeinsame Punktesammlung

Die bisher getrennt bereitgestellten Punkte werden in einem erweiterbaren,
typisierten Core–GUI-Vertrag als Sammlung geführt. Array, Liste oder eine
andere geeignete konkrete Datenstruktur ist eine Implementierungsentscheidung.
Pro Eintrag müssen Art, eindeutige Identität beziehungsweise Beitrags-/
Elementzuordnung, Herkunft, Position und Verfügbarkeit erkennbar sein.
Mehrere Auftriebs- oder andere Kraftangriffspunkte bleiben unterscheidbar;
Kraftangriffspunkt und bloßer Momenten- oder Darstellungsbezug dürfen
nicht miteinander verwechselt werden.

Die Sammlung umfasst für den jeweiligen Ergebnisstand:

| Punktart | Herkunft und Grenze |
|---|---|
| Mastspitze | Definierter Mastbezug aus Modelldaten; fehlt eine ausdrückliche Spitze, wird sie aus der Mastgeometrie gemäß untenstehender Regel erstellt. |
| Gesamtschwerpunkt | Aktueller Core-Massenschwerpunkt. |
| Auftriebspunkte | Vorhandene Angriffspunkte der einzelnen Auftriebsbeiträge; bei fehlendem Auftrieb keinen erfundenen Angriffspunkt ausgeben. |
| Lateraldruckpunkt | Ausschließlich gemäß bestehendem LDP-Vertrag und dessen Status; kein geometrischer Ersatz als gültiger kraftbasierter LDP. |
| Fremdkraft-Angriffspunkt / Ersatz-SDP | Nur gemäß bestehendem Fremdkraft-/Ersatz-SDP-Vertrag; kein echter aerodynamischer SDP. |
| Weitere Kraftangriffspunkte | Vorhandene, fachlich bekannte Angriffspunkte einzelner Core-Kraftbeiträge mit stabiler Beitrags- und Elementzuordnung. |

Die vorhandenen Kraft-, Momenten-, LDP- und Ersatz-SDP-Verträge behalten
ihre Bedeutung einschließlich Status, Quellen, Einheiten und bisherigen
Darstellungsregeln. Fehlende Punkte dürfen nicht durch andere Punkte,
Nullkoordinaten oder eine unbeschriftete Näherung ersetzt werden. Die
Sammlung ist erweiterbar; ein echter SDP oder weitere künftige Punkte
werden dadurch nicht erfunden oder jetzt implementiert.

Ist keine Mastspitze ausdrücklich vorgegeben, ermittelt der Core sie aus
der Darstellungsgeometrie des Mastes in dessen neutraler Modelllage: Maßgeblich
ist der höchste Wert der modellfesten Z-Koordinate. Haben mehrere
Mast-Vertices diese maximale Höhe, ist deren Mittelpunkt der Mastspitzenpunkt
auf dieser Höhe. Anschließend wird der Punkt mit der Bootspose in die Welt
transformiert; Krängung verändert nicht die Auswahl auf dem Mast. Bei
mehreren Masten gilt weiterhin der eindeutig bezeichnete Hauptmast. Ohne
Mast, ohne verwendbare Mastgeometrie oder ohne eindeutigen Hauptmast wird
der Punkt als nicht verfügbar diagnostiziert; kein fremdes Objekt als Mast
verwenden.

**Abnahme:** Alle obigen aktuell vorhandenen Punktarten sind eindeutig
zuordenbar; Mehrfachbeiträge, inaktive Fremdkraft, fehlender Mast und nicht
bestimmbarer LDP bewahren ihren jeweiligen Status und ihre Herkunft.

## A-C-00167 – Position und Bewegung in beiden Bezugssystemen

Für jeden bestimmbaren Punkt übergibt der Core seine Position als
dreikomponentigen Vektor in Metern sowohl im modellfesten Boots- als auch
im Weltkoordinatensystem. Die Weltposition wird mit der Pose **desselben
Ergebnisstands** aus der Bootposition transformiert.

Ist die Bewegung des Punktes bestimmbar, wird die Geschwindigkeit
gegenüber der Welt als dreikomponentiger Vektor in m/s sowohl in
Weltachsen als auch in Bootsachsen übergeben. Die Version in Bootsachsen
ist dieselbe Weltbewegung, lediglich anders ausgedrückt; sie ist nicht die
Relativgeschwindigkeit zum Boot. Bewegungsrichtung und Betrag ergeben sich
aus diesem Vektor; bei Geschwindigkeit null ist die Richtung unbestimmt.

Ein starr befestigter Punkt wie eine feste Mastspitze berücksichtigt
Translation und Rotation des Boots. Auftriebs-, Druck- und
Kraftresultierendenpunkte können dagegen im Bootsframe wandern oder nur
momentane rechnerische Bezugspunkte sein. Für sie ist eine starre
Punktgeschwindigkeit nicht automatisch gültig. Eine Geschwindigkeit wird
nur mit eindeutigem fachlichem Bewegungsbezug ausgegeben; andernfalls
wird sie getrennt von einer gegebenenfalls gültigen Position als nicht
verfügbar gekennzeichnet. Die Geschwindigkeit des Materials am
momentanen Ort darf nicht als Geschwindigkeit eines wandernden
Resultierendenpunkts ausgegeben werden.

Jeder Punkt beziehungsweise jedes optionale Positions- und
Geschwindigkeitsergebnis besitzt einen expliziten Status und bei Bedarf
eine Diagnose. Unterschiedliche Nichtverfügbarkeiten werden nicht als
physikalischer Nullwert ausgegeben.

**Abnahme:** Bekannte Translation, Rotation und Pose liefern zueinander
passende Positionen und Geschwindigkeitsvektoren in beiden Frames. Ein
ruhender fester Punkt hat Geschwindigkeit null und keine Richtung; für
einen wandernden oder nicht bestimmbaren Punkt wird keine erfundene
Geschwindigkeit beziehungsweise Position übertragen.

## A-C-00168 – Ergebnisstand und bestehende Verbraucher

Sammlung, Pose, Modellidentität, Zeitpunkt, Punktstatus und zugehörige
Kräfte stammen aus demselben Ergebnisstand, auch bei Livebetrieb und
Wiedergabe. Änderungen wirken nicht rückwirkend auf ältere Ergebnisse.
Historische Ergebnisse ohne die neuen Daten bleiben lesbar und melden
fehlende Angaben als nicht verfügbar; aktuelle Werte dürfen nicht
nachträglich als historische ausgegeben werden.

Bestehende Verbraucher und Ergebnisformate bleiben während der Umstellung
kompatibel. Wo bisherige Felder für sie notwendig sind, werden sie als
konsistente kompatible Sicht desselben Ergebnisses erhalten; keine zweite,
abweichende fachliche Quelle schaffen. Konkrete öffentliche Interfaces
werden vor ihrer Implementierung einzeln im Core-Schnittstellenregister
registriert.

**Abnahme:** Aktuelle und historische Zustände, bestehende GUI-Verbraucher
und alte Ergebnisformate werden regressionsgeprüft; vorhandene Punkte und
deren Zustände stimmen in alter Sicht und neuer Sammlung überein.
