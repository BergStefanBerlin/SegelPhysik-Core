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

Die Modelle liegen seit 27.10.2026 (Nutzerauftrag) nicht mehr in diesem
Repository, sondern im gemeinsamen Ordner
[`Segelphysik/Modelle`](../../../Modelle) neben den Checkouts `CORE` und
`GUI`. Core-Tests und GUI verwenden ihn ueber die CMake-Variable
`SEGELPHYSIK_MODELLE_DIR`. Die Pfadangaben `modelle/...` in den
historischen Nachweisdateien beziehen sich auf diesen Ordner.

Jedes der elf Modelle besitzt eine eigene `modell.json`: acht einfache
Koerper (Bleistift, Kegel, Kugel, Platte, Pyramide, Quader, Rolle,
unregelmaessiger Koerper), Folkeboot, Optimist und 420er.

Alle Geometrien werden ausschliesslich als Meshdateien beschrieben.
Parametrische Formen oder eine Unterscheidung zwischen Form und Mesh
gibt es in der Modell-JSON nicht mehr.

```text
Modelle/
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
Modell-JSON. Optional traegt `Darstellung` eine Farbe `"farbe": "#rrggbb"`
(nur Anzeige, A-C-00048); ohne Angabe nutzt die GUI ihre Standardfarbe.
Ebenso optional ist `"oberflaeche": "matt" | "glaenzend" | "metall"` oder
ein Effektwert `"chrom" | "lack" | "glas" | "leuchtend" | "schillernd" |
"facettiert" | "drahtgitter" | "perlmutt"`
(nur Anzeige, A-C-00049); ohne Angabe nutzt die GUI ihr Standardmaterial.
`/` ist darin der portable Trenner. `quelle: obj` ist
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
`[0, 0, 0.38]` m, Optimist `[0, 0, 0.368188]` m (nach der Anhebung, siehe
unten). Die
Eigentraegheit einer Punktmasse ist null; ihr Steiner-Anteil wird bei
einer spaeteren Gesamtkoerperbildung beruecksichtigt.

Das Folkeboot-Segel hat Masse, aber keinen Volumenblock, entsprechend
seiner bisherigen Kennzeichnung als nicht verdraengend. Das
Optimist-Segel ist auf Nutzerwunsch reine Darstellung. Insgesamt
besitzen 29 Elemente Hydrostatik mit Masse, davon 17 zusaetzlich
Volumen. Die anderen zwoelf sind drei Besatzungs-Punktmassen, zwei
Folkeboot-Segel und die sieben Spieren (Folkeboot Mast und Grossbaum,
Optimist Mast, Grossbaum und Spriet, 420er Mast und Grossbaum).

Pinne, Pinnenausleger und Gummigelenk bleiben auf Nutzerwunsch reine
Darstellung, ohne Hydrostatik oder Massenbeitrag.

### Ueberarbeitung Optimist

Nach Nutzervorgabe wurde der Optimist so ueberarbeitet:

| Element | Hydrostatik |
|---|---|
| Rumpf | Offene Schale ohne Deck (Bordwaende, Bug, Heck, Boden), GFK 1900 kg/m3, Wand 5 mm; geschlossenes Verdraengungsvolumen bis zur Bordwandoberkante |
| Schwert, Ruder | Holz als Vollkoerper, 600 kg/m3 (angenommene Dichte), mit Volumen |
| Mast, Grossbaum, Spriet | Aluminium-Rundrohr als Schale, 2700 kg/m3, Wand 1,5 mm, ohne Verdraengung |
| Grosssegel, Pinne, Pinnenausleger, Gummigelenk | nur Darstellung (keine Masse, kein Volumen) |
| Besatzung | unveraendert (Punktmasse 50 kg) |

Geometrische Anpassungen:

- Die 110 Deckflaechen wurden aus GUI- und Massennetz des Rumpfs
  entfernt. Im Volumennetz ersetzt ein neu triangulierter, ebener
  Deckel auf der Bordwandoberkante das alte Deck; das eingeschlossene
  Volumen bleibt gleich.
- Der alte Deckel hatte an zwei fast kollinearen Bordwandecken
  (x = 0,3151 m, y = +/-0,5178 m) numerisch sich kreuzende Kanten
  (`CORE-MOD-0006/0008`, bisher von `CORE-MOD-0007` verdeckt). Diese
  beiden Punkte wurden in allen drei Rumpfnetzen um 2 mm nach aussen
  gesetzt.
- Das Schwert-Volumennetz endet an der Rumpfunterkante
  (z = -0,081812 m) mit einem ebenen Deckel. Der Teil im Schwertkasten
  verdraengt nicht zusaetzlich; die AABB-Pruefung gegen den Rumpf
  ist dadurch erfuellt. Das Massennetz bleibt das ganze Schwert.

- Auf Nutzerauftrag wurden alle Optimist-Netze (GUI, Masse, Hydrostatik;
  25 OBJ-Dateien) und die Besatzung um +1,354 m in z verschoben, damit das
  Boot schon in der Initialpose z = 0 in seiner Ruhelage schwimmt. Jede
  OBJ-Datei nennt die Verschiebung im Kopfkommentar.
- Farben (`Darstellung.farbe`, Nutzerauftrag): Rumpf beige `#E3D3B0`,
  Schwert `#B5834F` und Ruder `#A8733F` (Holz), Pinne, Pinnenausleger,
  Mast, Grossbaum und Spriet Alu `#C4C8CD`; Segel und Gummigelenk in
  GUI-Standardfarbe.
- Oberflaechen (`Darstellung.oberflaeche`, Nutzerauftrag): Rumpf,
  Schwert und Ruder `matt`, Alu-Teile `metall`.

Der statische Lauf konvergiert trocken bei z ≈ 0 (0,0002 m) mit rund 98,0 kg
Gesamtmasse (Rumpf 38,9 kg, Schwert 3,8 kg, Ruder 0,5 kg, Spieren
4,8 kg, Besatzung 50 kg), 0,098 m3 Verdraengung und rund 8 cm Tiefgang
des Rumpfbodens; der Bordrand liegt rund 0,31 m ueber der Wasserlinie.
Das bleibt eine unkalibrierte Naeherung.

**Volllaufen (A-C-00047):** Der Rumpf ist eine offene Schale mit
Verdraengungsvolumen. Taucht ein Punkt seines Bordrands unter Wasser,
laeuft er dauerhaft voll und traegt nicht mehr; Schwert und Ruder
(Vollkoerper) tragen weiter. Eine Dynamik ab der Initialpose z = 0 bleibt
aufrecht und trocken (20 s: z zwischen -0,006 und +0,006 m). Die GUI
startet einen Lauf beim Wechsel Statik → Dynamik vom angezeigten
Ruhezustand.

### Namen

Auf Nutzerauftrag (27.10.2026) heissen die Boote im Katalog kurz
`Optimist`, `Folkeboot` und `420er` (Feld `name`; vorher mit Zusaetzen
wie „Naeherungsmodell“, „Naeherung“, „Geometrieentwurf“).

### Reparatur Folkeboot

Das Folkeboot war ungueltig (`CORE-MOD-0007`):

- Mast und Grossbaum (Spieren) haben kein Volumen mehr, nur noch
  Schalenmasse; ihre Volumen-AABBs ueberschnitten sich.
- Die Hydrostatik-Netze von Kiel und Ruder ragten in die Rumpf-AABB.
  Sie wurden an der Rumpfunterkante (z = -0,47 m) eben gekappt
  (Profilschnitt, neu trianguliert). Masse- und GUI-Netze bleiben
  vollstaendig.

Der statische Lauf konvergiert bei z ≈ -0,19 m mit rund 1,66 m3
Verdraengung.

### Neuaufbau 420er

Der 420er wurde auf Nutzerauftrag (27.10.2026) aus
`Ideen/AusAltenProjekten/420er/420_zusammenbau.obj` (mm, Rumpf:
Stefgran/Thingiverse 1593594, CC BY-NC-SA, 30-fach auf 4200 mm) neu
aufgebaut. Die Teile wurden nach den Einzeldateien aus dem Zusammenbau
getrennt und auf Meter umgerechnet; die Lage im Zusammenbau bleibt
erhalten (Rumpfunterkante z = 0, Hauptspant x = 0). Nutzerentscheidungen:
Auftrieb nur aus geschlossenen Tanks, Doppelboden und Vordeck, Rumpf
etwa 80 kg, Ruder aus dem alten Entwurf, Besatzung 140 kg.

| Element | Hydrostatik |
|---|---|
| Rumpf | Schale GFK 1900 kg/m3, Wand 2,6 mm (ergibt 79,3 kg); geschlossener Koerper aus Aussenhaut, Seitentanks, Doppelboden und Vordeck als Volumen. Das Cockpit ist eine offene Mulde und gilt als geflutet; der Rumpf hat keinen Flutrand |
| Schwert | Holz-Vollkoerper 600 kg/m3 (4,4 kg); Quellnetz defekt, als geschlossenes 18-mm-Profil (konvexe Kontur) neu gebaut; Volumen unterhalb der Rumpfunterkante |
| Ruder | Holz-Vollkoerper 600 kg/m3 (1,9 kg), Blatt aus dem alten 420er-Entwurf, an den neuen Spiegel versetzt |
| Mast, Grossbaum | Alu-Schale 2700 kg/m3, Wand 1,5 mm, ohne Volumen (4,4 kg und 2,6 kg) |
| Grosssegel, Fock, Vorstag, Ruderkopf | nur Darstellung |
| Besatzung | Punktmasse 140 kg bei [-0,5, 0, 0,45] m |

Netzkorrekturen:

- Rumpf: Die Quelle war nach innen orientiert (umgedreht). An den
  Deckstufen (x ≈ 0,35 m und 0,64 m, beidseitig) und am Mastloch
  lagen 207 sich ueberlappende, koplanare Splitterdreiecke. Diese
  Bereiche wurden in Masse- und Hydrostatiknetz lokal entfernt und mit
  einer Minimalflaechen-Triangulierung neu geschlossen (Volumen
  1,1303 → 1,1301 m3). Das GUI-Netz ist die Quelle.
- Mast: uneinheitlicher Flaechenumlauf, konsistent ausgerichtet.
- Grossbaum: Quellnetz schneidet sich selbst; ersetzt durch ein
  geschlossenes Ovalrohr (24 Segmente) mit gleicher Huelle (2,40 m).

Farben wie beim Optimist: Rumpf `#E3D3B0` matt, Schwert `#B5834F`
und Ruder `#A8733F` matt, Mast, Grossbaum, Vorstag und Ruderkopf
`#C4C8CD` metall; Segel in GUI-Standardfarbe.

Der statische Lauf konvergiert trocken bei z ≈ -0,22 m (Tiefgang der
Rumpfunterkante) und rund 2,2 Grad Trimm, mit rund 233 kg Gesamtmasse
und 0,233 m3 Verdraengung. Das bleibt eine unkalibrierte Naeherung.

Die vorhandenen Bootsnetze sind nicht durch ihre Zuordnung zu
`volumen` physikalisch freigegeben. Dichtigkeit, Selbstschnitte
und Volumenueberlappungen bleiben vor einer Simulation zu
pruefen; Volllaufen behandelt der Core nur fuer offene Schalen
(A-C-00047). Insbesondere duerfen ueberlappende Volumina nicht einfach
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

Der 420er uebernimmt seit dem Neuaufbau (siehe oben) das Rumpfsystem
des Zusammenbaus (X Bug, Z oben) mit Skalierung 0,001; eine Spiegelung
ist nicht noetig.
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

Der 420-Grossbaum der alten Quellszene war auffaellig vertikal
platziert. Mit dem Neuaufbau (siehe oben) ist das ueberholt; der neue
Grossbaum liegt horizontal am Lümmelbeschlag. Andere offene
Netz-/Modellprobleme bleiben sichtbar im Pruefnachweis.

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

Folkeboot, Optimist und 420er bleiben unkalibrierte Naeherungen. Der
neue 420er-Rumpf ist ein Hobby-Netz (Thingiverse) und nicht als
massgerechte Klassengeometrie bestaetigt.
Das Koerper-Meshing validiert keine realen Bootsgeometrien.

Der [Pruefnachweis](pruefnachweis.json) dokumentiert 76 bestandene
Struktur- und Geometriepruefungen sowie Volumina, Abweichungen und
Dateipruefsummen. Die Sammlung ist weiterhin keine finalisierte
Spezifikation und kein Umsetzungsplan.

Hinweis: Nach der Optimist-Ueberarbeitung, der Folkeboot-Reparatur und
dem 420er-Neuaufbau stimmen die Pruefsummen dieser Netze in beiden
Nachweisdateien nicht mehr; die Nachweise sind fuer die drei Boote neu
zu erzeugen. Massgeblich ist bis dahin `segelphysik_cli --pruefen`.
