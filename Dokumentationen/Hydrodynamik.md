# Hydrodynamik im CORE

Die Hydrodynamik soll schrittweise vollständig im CORE entstehen. Zunächst
soll die bestehende Bewegungsanzeige weiter Pose und Geschwindigkeit aus der
Core-Simulation verwenden; eine neue GUI-Physik oder Rudersteuerung ist nicht
vorgesehen. Das Ruder bleibt in seiner bestehenden Konfiguration relativ zum
Rumpf fest; eine unabhängige Ruderbewegung, Steuerung oder Verstellung ist
ausdrücklich nicht Teil des aktuellen Umfangs.

Als verfügbare Referenzmodelle dienen der Optimist und der 420er als Jollen
sowie das Folkeboot als Kielboot. Die Arbeit setzt keine besseren Geometrien
voraus. Die Qualität der vorhandenen physischen Meshes und die daraus
ableitbaren Werte werden modellbezogen geprüft und dokumentiert.

Der Core-Sollvertrag für hydrodynamische Kraft- und Momentenbeiträge ist im
Abschnitt 7 der
[Schnittstellenbeschreibung Core → GUI](Schnittstelle_Core_GUI.md)
dokumentiert. Er nutzt die vorhandene Vektor-/Beitragsform mit Angriffspunkt,
Frame und Moment um den Gesamtschwerpunkt. Die aktuelle GUI kann neue
Kraftbeiträge generisch anzeigen; eine speziell gestaltete Hydrodynamikgruppe
und zusätzliche Darstellungen sind für spätere GUI-Arbeit vorgesehen.

Die freigegebene fachliche Spezifikation für C-0.6 liegt in der
[Spezifikation Hydrodynamik](../Spezifikationen/Spezifikation_Hydrodynamik.md).
Sie enthält die Anforderungen A-C-00062 bis A-C-00072. Die Herkunft der
Entscheidungen und verbleibende technische Grenzen sind in der
[Entwurfspunkteliste](../Spezifikationen/Entwurfspunkte/Hydrodynamik.json)
nachvollziehbar; die vorhandene Modellrollenanforderung A-C-00061 steht in
der Modellbeschreibungsspezifikation.

Der dazugehörige
[Umsetzungsplan C-0.6](../Umsetzungsplaene/Umsetzungsplan_Hydrodynamik.md) zerlegt
die Arbeit in Daten-/Schnittstellenvertrag, ITTC-Reibung, dynamische
Integration, Rumpf-Restwiderstand, Kiel-Seitenkraft und nachgelagerte
Erweiterungen. Der Core-Implementierungsauftrag ist erteilt; technische
Umsetzung, Modellvalidierung und fachliche Abnahme werden getrennt
ausgewiesen. Es gibt keine GUI-, Commit-, Push-, Tag- oder Release-Freigabe.

Fachliche Ausgangsquellen:

- [Hydrodynamik eines Bootes](../../Ideen/hydrodynamik_boot.pdf)
- [Berechnungsvorschrift Hydrodynamik Rev. 2](../../Ideen/Berechnung_Hydrodynamik_2_2.md)

## Technischer Stand und Grenzen

Der Core importiert die Rollen `rumpf`, `kiel` und `ruder` aus dem
Modellvertrag. In den drei verfügbaren physischen Rumpfmeshes liegen die
ungefähren Gesamtoberflächen bei 6,42 m² (Optimist), 16,07 m² (420er) und
28,78 m² (Folkeboot); die X-Bounds entsprechen ca. 2,36 m, 4,20 m und
7,68 m. Diese Werte sind Meshinventar, keine benetzte Fläche oder gemessene
Wasserlinienlänge. Die Rechenstufe bestimmt benetzte Fläche und Wasserlinie
jeweils aus dem physischen Rumpfmesh und der aktuellen Pose. Die Modelle
bleiben in ihren JSON-Metadaten Entwürfe ohne Simulationsfreigabe.

Mit `Re >= 10^6` und dem Standardwasserprofil (`nu=1e-6 m²/s`) wird der
ITTC-Reibungsbeitrag bei langsamer Fahrt unterhalb der Re-Grenze nicht
berechnet. Das Boot kann weiter simuliert werden; der Reibungsbeitrag ist
`nicht_berechnet`, trägt `CORE-HYD-0001`, und die Ergebnisbilanz bleibt eine
Teilsumme. Es wird kein Reibungswert extrapoliert. Bei nominalen
Wasserlinienlängen von etwa 2,36 m, 4,20 m und
6,00 m entspräche die Untergrenze grob 0,42, 0,24 bzw. 0,17 m/s; die
tatsächliche Schwelle hängt von der poseabhängigen Mesh-Wasserlinie ab.
Damit bleibt die Hydrodynamik bei sehr kleiner positiver Geschwindigkeit
unvollständig. Nullgeschwindigkeit bleibt ein gültiger Nullbeitrag.

Ein veröffentlichter Optimist-bezogener Schleppversuch wurde bei der Recherche
identifiziert, aber seine Daten und die Übertragbarkeit auf das vorhandene
Optimist-Mesh konnten nicht verifiziert werden. Für 420er und Folkeboot wurden
keine verwendbaren modellbezogenen Schleppdaten gefunden. Auch passende
Kiel-/Ruderpolaren fehlen. Daher sind Rest-/Wellenwiderstand,
Kiel-Seitenkraft, Gesamtbewegungsabnahme und Ruderkräfte nicht implementiert
und fachlich nicht abgenommen. Es wurden dafür keine Koeffizienten erfunden.
Die Core-Tests mit den drei Modellen belegen nur den Rollenimport und die
ausführbare ITTC-Reibungsstufe innerhalb ihres Re-Bereichs.

Externe Quellen zur fachlichen Einordnung:

- [ITTC Recommended Procedure 7.5-02-02-01: Resistance Test](https://ittc.info/media/9595/75-02-02-01.pdf) und der historische [1957-ITTC-Korrelationslinien-Bericht](https://ntrl.ntis.gov/NTRL/dashboard/searchResults/titleDetail/N7112763.xhtml). Sie begründen die verwendete Reibungslinie, nicht einen Gesamtwiderstand oder die Genauigkeit dieser Modelle.
- [Remmlinger: Regression DSYHS](http://remmlinger.com/Regression%20DSYHS.pdf) und [TU Delft: DSYHS Manual](http://dsyhs.tudelft.nl/Manual%20DSYHS.pdf). Die Anwendbarkeit und Parameterdomäne für Optimist, 420er und Folkeboot sind nicht nachgewiesen; keine DSYHS-Koeffizienten wurden übernommen.
- [Andersson et al.: Design of a Foiling Optimist](https://research.chalmers.se/en/publication/505704),
  *Journal of Sailboat Technology* (2018), DOI
  [10.5957/jst.2018.06](https://doi.org/10.5957/jst.2018.06). Der
  Chalmers-Nachweis bestätigt Schleppversuche zu Rumpfhydrodynamik und
  Foilkonfiguration im Kontext eines Foiling-Optimist-Entwurfs. Der Volltext
  bzw. auswertbare Messkurven waren in der Recherche nicht zugänglich; die
  Übereinstimmung des getesteten Rumpfs mit dem vorhandenen Optimist-Modell
  und die Übertragbarkeit der Daten sind deshalb nicht belegt.
- [Nordic Folkboat International Association: Klassendaten](https://folkboat.com/?classrules) nennt 6,00 m LWL und 7,68 m LOA. Das ist eine nominelle Klassenangabe, kein Schleppversuch und keine Validierung des vorhandenen Meshes.

Der maschinenlesbare Umsetzungsstatus steht im
[Plan C-0.6](../Umsetzungsplaene/Umsetzungsplan_Hydrodynamik.md); die
öffentlichen C++-Kennungen stehen im
[Schnittstellenregister](Schnittstellenregister_Core.md).

Die erste Core-Stufe importiert die Bauteilrollen, berechnet ITTC-1957-
Rumpfreibung aus dem benetzten physischen Mesh und integriert sie in die
dynamische Kraft-/Momentenbilanz. Das ist ein Rechen- und
Integrationsnachweis, keine fachliche Bewegungsabnahme. Rumpf-Restwiderstand,
Kielpolaren und Gesamtbewegung bleiben nur dann umsetzbar/abnehmbar, wenn
belastbare Quellen und Referenzdaten vorliegen. GUI-Physik und spezielle
Hydrodynamikdarstellungen sind nicht beauftragt.
