# Beispiele einer einheitlichen Modellbeschreibung

**Status: ENTWURF. Nicht finalisiert, kein implementiertes Core-Format
und keine Freigabe zur Simulation.**

Die Koordinatenanforderungen sind separat in der maßgeblichen
[Spezifikation der Modellbeschreibung](../Spezifikation_Modellbeschreibung.md)
freigegeben. Die vorliegende Beispielsammlung und die übrige
Modellstruktur bleiben Entwurf.

Die Beispiele werden schrittweise gemeinsam aufgebaut. Aktuell enthalten
Elemente eine eindeutige lesbare `id`, optional `Darstellung` und optional
`Hydrostatik`. Weitere Eigenschaften und Kraftmodelle werden erst nach
gemeinsamer Besprechung aufgenommen. Offene Themen stehen mit ihren
bisherigen Kennungen in der
[Entwurfspunkteliste](../Entwurfspunkte/Modellformat.json).

## Ablage

Jedes der elf Modelle besitzt eine eigene `modell.json`: acht einfache
Koerper (Bleistift, Kegel, Kugel, Platte, Pyramide, Quader, Rolle,
unregelmaessiger Koerper), Folkeboot, Optimist und 420er.

Alle Geometrien werden ausschliesslich als Meshdateien beschrieben.
Parametrische Formen oder eine Unterscheidung zwischen Form und Mesh
gibt es in der Modell-JSON nicht mehr.

```text
modelle/
  folkeboot/
    modell.json
    meshes/
      gui/
        rumpf.obj
      masse/
        rumpf.obj
      hydrostatik/
        rumpf.obj
```

Es gibt 33 GUI-, 23 Massen- und 20 Volumen-Meshdateien, insgesamt 76.
Die Dateien sind nach Zweck getrennte Kopien und koennen spaeter
unabhaengig ersetzt werden. Ein Mesh fuer Masse ist nicht automatisch
ein Verdraengungsmesh. Tragflaechenprofil- und Stroemungs-Meshes
werden weiterhin nicht angelegt.

Der [Materialkatalog](materialien.json) bleibt als separater Entwurf
erhalten. Die aktuellen Massenbloecke verwenden zur einfachen
Nachvollziehbarkeit die Dichten direkt aus den Bestandsdateien und
benoetigen den Katalog deshalb noch nicht.

## Gemeinsame Grundstruktur

Die Gruppen `rumpf`, `kiel`, `ruder` und `rigg` bleiben reine
Ordnungsknoten ohne eigene Masse oder Kraft. Element-IDs sind innerhalb
eines Modells eindeutig. Ein separates `name` oder `typ` am Element
gibt es nicht.

```json
{
  "id": "Rumpf",
  "Darstellung": {
    "datei": "meshes/gui/rumpf.obj"
  },
  "Hydrostatik": {
    "masse": {
      "art": "schale",
      "dichte_kg_pro_m3": 1900,
      "wanddicke_m": 0.008,
      "datei": "meshes/masse/rumpf.obj"
    },
    "volumen": {
      "datei": "meshes/hydrostatik/rumpf.obj"
    }
  }
}
```

`datei` ist der direkte relative Dateipfad, aufgeloest gegen die
Modell-JSON. `/` ist darin der portable Trenner. `quelle: obj` ist
redundant zur Dateiendung und entfaellt. Ein zentraler Geometriekatalog
oder eine Formbeschreibung wird nicht benoetigt.

## Masse und Materialien

**Jeder vorhandene Hydrostatikblock muss genau eine Massenvariante
unter `masse` enthalten.** Ein leerer Hydrostatikblock ist nicht mehr
zulaessig. Fehlende Massen sind ein unvollstaendiges Modell und duerfen
nicht stillschweigend zu null gesetzt werden.

Die wieder aufgenommenen Varianten aus dem Bestand:

| `masse.art` | Pflichtangaben | Bedeutung |
|---|---|---|
| `punktmasse` | `masse_kg`, `position_m` | Masse an einem modellfesten Punkt, ohne Massengeometrie |
| `vollkoerper` | `dichte_kg_pro_m3`, `datei` | Masse aus Materialvolumen des Massen-Meshs |
| `schale` | `dichte_kg_pro_m3`, `wanddicke_m`, `datei` | Masse aus duenner Materialschale |

Die Dichten, Wanddicken und Besatzungsmassen sind aus dem Bestand
uebernommen, nicht neu kalibriert. Eine Materialreferenz wurde dabei
ueber die Bestands-Materialdatei in ihre Dichte aufgeloest. Es werden
keine Stoffidentitaeten oder realen Bootsgewichte erfunden.

Die Schalenvariante bleibt fachlich zu praezisieren: Insbesondere sind
die alten verdickten Segelnetze keine verifizierten Materialmittelflaechen.
Bei einer spaeteren Berechnung duerfen zwei geometrische Segelseiten
nicht versehentlich doppelt als Materialflaeche gezaehlt werden.

## Geometrien und Hydrostatik

**`volumen` ist optional und beschreibt ausschliesslich die
wasserverdraengende Geometrie. Ohne diesen Block gilt exakt
0 m3 Verdraengungsvolumen und damit kein hydrostatischer Auftrieb.**
Es wird kein kuenstliches Epsilonvolumen angenommen und weder das
Darstellungs- noch das Massen-Mesh als Ersatz herangezogen.

Ein Element ohne Volumen wirkt also nur ueber seine Masse:

```json
{
  "id": "Besatzung",
  "Hydrostatik": {
    "masse": {
      "art": "punktmasse",
      "masse_kg": 150,
      "position_m": [0, 0, 0.38]
    }
  }
}
```

Die Besatzungspositionen aus den Quellen wurden ueber Rumpfreferenz plus
relativem Versatz in den Modellbezug umgerechnet: Folkeboot
`[0, 0, 0.38]` m, Optimist `[0, 0, -0.985812]` m. Die
Eigentraegheit einer Punktmasse ist null; ihr Steiner-Anteil wird bei
einer spaeteren Gesamtkoerperbildung beruecksichtigt.

Die Segel von Folkeboot und Optimist haben ebenfalls Masse, aber keinen
Volumenblock, entsprechend ihrer bisherigen Kennzeichnung als nicht
verdraengend. Insgesamt besitzen 25 Elemente Hydrostatik mit Masse,
davon 20 zusaetzlich Volumen. Die anderen fuenf sind zwei
Besatzungs-Punktmassen und drei Segel.

Pinne und Pinnenausleger bleiben auf Nutzerwunsch reine Darstellung,
ohne Hydrostatik oder Massenbeitrag. Andere Bestandselemente wie
Gummigelenk oder Spriet wurden nicht zusaetzlich als Zierde eingestuft.

**Der 420er bleibt vorerst ausschliesslich Darstellung.** Es fehlen
Massendaten; daher wurden nach Nutzerentscheidung alle seine
Hydrostatikbloecke und Volumenkopien entfernt. Er kann spaeter mit
belastbaren Massenangaben ergaenzt werden.

Die vorhandenen Bootsnetze sind nicht durch ihre Zuordnung zu
`volumen` physikalisch freigegeben. Dichtigkeit, Selbstschnitte,
Flutung und Volumenueberlappungen bleiben vor einer Simulation zu
pruefen. Insbesondere duerfen ueberlappende Volumina nicht einfach
addiert werden.

## Erzeugte Koerper-Meshes

Die acht bisherigen parametrischen Koerper wurden in geschlossene,
konsistent orientierte Dreiecksnetze umgewandelt. GUI-, Massen- und
Volumendatei sind jeweils bytegleiche, getrennte Kopien.

Die folgenden Erzeugungsannahmen sind ausdrueckliche
Entwurfsfestlegungen, keine bestaetigten Altformat-Vertraege:

| Koerper | Geometrie und Bezug |
|---|---|
| Quader / Platte | Um den Ursprung zentriert, Kanten parallel X/Y/Z, Dicke der Platte entlang Z |
| Kegel | Basiszentrum im Ursprung, Spitze entlang +Z |
| Kugel | Zentrum im Ursprung, 128 Umfangssegmente und 64 Polintervalle |
| Pyramide | Quadratische Basis zentriert in Z = 0, Spitze entlang +Z |
| Rolle | Laengsachse X, Mittelpunkt im Ursprung; Quelle sagt lediglich `liegend` |
| Bleistift | Sechskantschaft und sechseckige Pyramidenspitze entlang +Z, Basiszentrum im Ursprung |
| Unregelmaessiger Koerper | Konvexe Huelle der acht urspruenglichen Eckpunkte |

Kegel und Rolle besitzen 128 Umfangssegmente. Die Gesamtlaenge des
Bleistifts wird als `H_m` interpretiert, seine Schaftlaenge als
`H_m - spitzenlaenge_m`. Das ist eine polygonale Spitze, keine exakt
runde Kegelspitze. Herkunftsparameter werden nicht mehr in den
Modelldateien gespeichert; sie bleiben in den unveraenderten Quellen.

Fuer jedes erzeugte Netz wurden gueltige, nicht entartete Dreiecke,
zweifache Kantenbelegung, entgegengesetzter Kantenumlauf, Eulerzahl 2
und positives eingeschlossenes Volumen geprueft.
Bei allen sieben Koerpern mit analytischer Vergleichsform liegt die
relative Volumenabweichung unter 0,2 Prozent; die groesste ist die
Kugel mit rund 0,1004 Prozent. Fuer den unregelmaessigen Koerper
existiert keine unabhaengige analytische Vergleichsform.

## Einheitliche Koordinaten

Fuer alle Modelle gilt: rechtshaendig, X nach vorn/Bug, Y nach
Backbord, Z nach oben, Laengen in Metern, Massen in Kilogramm.
Ein eigener Koordinatensystemblock pro Modell ist nicht vorgesehen.

Folkeboot und Optimist verwenden als Importbasis die Achsenmatrix und
Skalierung der Quellen. Die anschliessende Koordinatenpruefung zeigte
beim Optimisten eine umgekehrte Bugrichtung: Ruder auf der positiven
X-Seite, Mast auf der negativen. Alle seine Meshdateien wurden deshalb
gemeinsam um 180 Grad um Z gedreht: `(x, y, z) -> (-x, -y, z)`.
Die Rotation ist rechtshaendig (Determinante +1), keine Spiegelung.
Ursprung, Hoehen und Bauteilabstaende bleiben erhalten. Die Besatzung
liegt auf X = Y = 0 und benoetigt daher keine Positionsaenderung.

Beim 420er wurden die acht platzierten Objekte der
Gesamtszene verwendet, Y gespiegelt und die Flaechenreihenfolge
umgekehrt. Die festen Quellurspruenge bleiben erhalten.
Bewegliche Verbindungen werden spaeter besprochen.

## Meshpruefung gegen die Koordinatenregeln

Der [Mesh-Pruefnachweis](mesh_koordinatenpruefung.json) erfasst alle
76 Dateien und ihre aktuellen Pruefsummen. 34 Dateien wurden geaendert:
die 25 Optimist-Netze durch die gemeinsame Rotation und gegebenenfalls
zusaetzliche Umlaufkorrektur, acht Folkeboot-Netze durch
Umlaufkorrektur sowie das 420-Rudernetz durch Umlaufkorrektur.

Bei geschlossenen, konsistent orientierten Netzen mit negativem
signierten Volumen wurde die Reihenfolge aller Flaecheneckpunkte
umgekehrt. Das betrifft beim Folkeboot Rumpf, Grossbaum und Fock,
beim Optimisten Gummigelenk, Pinne und Segel sowie beim 420er das Ruder,
jeweils in allen vorhandenen Zweckkopien.
Das korrigiert den nach innen gerichteten Umlauf, nicht die Form.
Offene oder inkonsistente Netze wurden nicht automatisch geschlossen.

Geprueft wurden Indizes, endliche Koordinaten, Umfang der Transformation,
erhaltene Topologie, unveraenderter Betrag geschlossener Volumina und
identische Zweckkopien. Bei allen drei Booten liegt das Ruder nach der
Korrektur hinter dem Mast in Richtung kleinerer X-Werte. Bei symmetrischen
Netzen laesst sich die Backbordseite ohne Markierungen nicht allein aus
der Form beweisen; dafuer wurden die Herkunftstransformationen verwendet.

Der 420-Grossbaum ist bereits in der Quellszene auffaellig vertikal
platziert. Das ist eine separate Bauteilpose-Frage, keine belegte
Abweichung des gesamten Modellachsensystems; sie wurde nicht
eigenmaechtig korrigiert. Andere offene Netz-/Modellprobleme bleiben
sichtbar im Pruefnachweis.

**Diese Arbeiten passen nur die Beispiel-Meshes an. Die Anforderungen
A-C-00001 bis A-C-00003 gelten dadurch weder als implementiert noch
als insgesamt abgenommen.** Weltachsen, Modellbewegung, Wasseroberflaeche
und Ursprungskonstanz bei Beladung sind durch statische Meshdateien
nicht implementiert oder nachgewiesen. Insbesondere wurden die Netze
nicht auf eine Wasserlinie Z = 0 verschoben.

## Grenzen und weitere Verwendung

Die speziellen `entwurf`-Bloecke sind vorlaeufig und entfallen spaeter.
Relevante Quellen und Modellannahmen sind vor der Finalisierung
dauerhaft zu dokumentieren. Es gibt noch keinen Loader, keine
implementierte Nullvolumenbehandlung und keine Simulationsfreigabe.

Folkeboot und Optimist bleiben unkalibrierte Naeherungen. Die
420-Geometrien aus `420_obj_modelle_2.zip` sind schematisch und
insbesondere nicht als geschlossenes Verdraengungsnetz bestaetigt.
Das Koerper-Meshing validiert keine realen Bootsgeometrien.

Der [Pruefnachweis](pruefnachweis.json) dokumentiert 76 bestandene
Struktur- und Geometriepruefungen sowie Volumina, Abweichungen und
Dateipruefsummen. Die Sammlung ist weiterhin keine finalisierte
Spezifikation und kein Umsetzungsplan.
