# GUI-Übergabe: Desktop, spätere Web-Anwendung und Core-Anbindung

Stand: 26.10.2026. Auf Nutzerauftrag aus dem Core-Umfang in das
GUI-Projekt übergeben. Dieses Dokument ist ein Übergabenachweis,
keine zusätzliche aktive Core-Spezifikation oder Core-Umsetzungsanweisung.

## Zuständigkeit und Herkunft

Die zuletzt vereinbarten Desktop-/Web-Vorgaben gehören ins GUI-Projekt.
Der Core archiviert A-C-00033 mit dem Vermerk **in GUI übernommen**;
U-C-000018 wird aus seinem aktiven Umsetzungsplan entfernt.
Beide Kennungen bleiben reserviert. Im GUI-Projekt sind nach Prüfung
der dort bereits vergebenen Nummern eigene A-G-/U-G-Kennungen zu vergeben;
dieses Paket vergibt keine GUI-Kennungen und behauptet keine bereits
erfolgte Änderung im GUI-Repository.

Core-seitig bleiben A-C-00016, A-C-00017 und A-C-00028 gültig:
physikfreie Anschlussfähigkeit, Ergebnisvertrag und gepflegte
Schnittstellenbeschreibung sind weiterhin notwendige Core-Leistungen.

## Vereinbarte GUI-Richtung

- Desktop: C++ mit Qt 6 Widgets und VTK als vorbereiteter 3D-Bibliothek.
- Qt und VTK gehören ausschließlich zur GUI, nicht in die Core-Bibliothek.
- VTK bietet technische Meshdarstellung, Kamera, Auswahl, Vektorsymbole,
  Clipping und Beschriftung. Fotorealistisches Meer ist nicht automatisch
  enthalten. Grafik-Clipping darf die Core-Physik nicht verändern.
- Die Renderingbibliothek bestimmt die Bildqualität stärker als die
  Sprache. Python/PySide6 mit nativer Anbindung wäre grundsätzlich
  möglich; C++ wurde wegen der direkten Core-Anbindung gewählt.
- Qt Quick 3D ist eine Alternative für stärker produktorientierte
  Darstellung; seine GPLv3-/kommerziellen Lizenzbedingungen sind
  gesondert zu beachten. Für die gewählte Kombination sind die
  tatsächlich verwendeten Qt-/VTK-Versionen und Lizenzen vor Verteilung
  zu prüfen.

Referenzen:
[VTK-Qt-Widget](https://vtk.org/doc/nightly/html/classQVTKOpenGLNativeWidget.html),
[Qt Quick 3D](https://doc.qt.io/qt-6/qtquick3d-index.html).

## Architektur und Transport

```text
Desktop: Qt Widgets + VTK        Später: Browser + Web-Renderer
            |                                |
     lokaler GUI-Adapter                Netzwerk-Adapter
            |                                |
  direkte Bibliotheksaufrufe          Server-Anbindung
            |                                |
            +-------- fachlicher Vertrag ----+
                              |
                       C++-Core / Solver
```

Direkte Bibliotheksaufrufe im gleichen Prozess sind der theoretisch
geringste Kommunikationsaufwand. Bei getrennten Prozessen ist Shared
Memory ein schneller Kandidat. Beides ist für diese Anwendung noch
nicht vergleichend gemessen. JSON ist kein notwendiger Zwischenschritt
lokaler Aufrufe; rohe C++-Speicherlayouts sind dagegen kein portables
Netzwerkformat.

Die spätere Web-Anwendung verwendet die native Core-Rechnung auf einem
Server über einen Adapter. Eine Physik-Neuimplementierung im Browser
oder eine WebAssembly-Portierung ist dafür nicht erforderlich.
Die Qt-/VTK-Desktopoberfläche wird nicht unverändert zur Browser-GUI;
der Web-Renderer wird später gesondert gewählt.

### Aus dem Core übergebene fachliche Vorgaben

Herkunft: A-C-00033, Transportunabhängige Desktop- und Web-Anbindung.

- Befehle, Konfiguration, Modellidentität, vollständige Ergebniszustände
  und Fehlerdiagnosen unabhängig von GUI-Technik, Prozessgrenze und
  Codierung beschreiben.
- Lokaler Adapter und spätere Server-Anbindung bilden dieselben
  fachlichen Operationen ab: Modell laden/prüfen, Lauf konfigurieren,
  starten/abbrechen, Status/Ergebnisse und Fehlerkatalog abfragen.
  Dies ist eine Zielliste, keine Behauptung vorhandener Live-Befehle.
- Übertragbare Daten setzen keine Qt-/VTK-Objekte, Speicheradressen
  oder prozesslokalen Objektidentitäten voraus.
- Modellgeometrie und statische Metadaten getrennt von laufenden
  Zustandsupdates bereitstellen. Lokale Dateipfade allein reichen für
  einen Browser nicht; eine Ressourcenbereitstellung ist nötig.
- SI-Einheiten, xyzw, Bezugssysteme, Modellfassung und Fehler-IDs
  bleiben transportübergreifend gleich. Inkompatible Änderungen
  versionieren; binäre Codierungen ausdrücklich definieren.
- Darstellung und Simulation unabhängig takten. Die GUI darf den
  Solver nicht im Oberflächenthread blockierend ausführen.
- Konsistente Zustände mit geregelter Lebensdauer und eindeutiger
  Modell-/Laufzuordnung übergeben. Nicht auf gerade veränderte
  Solverobjekte zugreifen.
- Eine begrenzte Live-Übergabe darf ältere Anzeigestände durch neuere
  vollständige Zustände ersetzen. Keine Physikschritte, verbindlichen
  Aufzeichnungen, Befehlsantworten oder Fehler still verwerfen.
  Rückstau, Überlast und Verbindungsabbruch ausdrücklich behandeln.

### Übergebene Umsetzungshinweise

Herkunft: U-C-000018; kein aktiver Core-Umsetzungsschritt mehr.

1. Fachlichen GUI-Adaptervertrag aus dem vorhandenen Ergebnisvertrag
   ableiten; Zustandslebensdauer, Threads und Laufkennung konkretisieren.
2. Direkte lokale Anbindung ohne JSON-Zwang implementieren.
3. Lokalen und Serialisierungsadapter mit denselben Referenzzuständen
   und fachlichen Operationen prüfen: Zustandskonsistenz, Modell-/Lauf-
   zuordnung, Fehler, Rückstau und Lebensdauer.
4. Latenz und CPU-Aufwand einschließlich GUI-Empfang realistisch messen.
   Keine theoretische Variante ungeprüft als schnellste ausgeben.
5. Netzwerkprotokoll, Codierung, Server, Zugriffsschutz, Modell-/Laufbesitz,
   Ressourcenbegrenzung und Browser-GUI erst im gesonderten Web-Auftrag
   festlegen und implementieren.

## Tatsächlich vorhandene Core-Schnittstellen

Maßgeblich ist die
[Schnittstellenbeschreibung](Schnittstelle_Core_GUI.md).

| Oberfläche | Verfügbarer Stand |
|---|---|
| C++ | Modellimport, Pose, Hydrostatik, Gleichgewicht, einzelner Dynamikschritt, synchrone Gesamtläufe, Ergebnisserialisierung, Fehlerkatalog |
| CLI | `--help`, `--build-info`, `--fehlerdoku`, `--pruefen`, `--statisch`, `--dynamisch` |
| Aufzeichnung | Version 1, JSON-Array oder JSONL; echte statische/dynamische Quaderreferenzen liegen bei |
| Noch nicht vorhanden | Asynchrone Sitzungs-/Befehls-API, Live-Abonnement, explizite Live-Laufkennung, GUI-Adapter, Shared-Memory-Transport, Webserver |

Position ist die Lage des festen Modellursprungs in Weltkoordinaten.
Quaternion xyzw rotiert Modell nach Welt. Punkte werden rotiert und
verschoben, Vektoren nur rotiert. Geschwindigkeit ist die
Schwerpunktgeschwindigkeit im Weltframe, Winkelgeschwindigkeit liegt
im Modellframe. Momentenbezug ist der Gesamtschwerpunkt.
Die GUI wählt Pfeilmaßstäbe, nicht die Physik.

Der öffentliche Datentyp `Zustand` enthält nicht sämtliche Ergebnisse.
Zusätzliche Kräfte/Volumina werden über `hydrostatik` ausgewertet.
`zustand_json` enthält die dokumentierte zusammenhängende Ausgabe.
Es gibt noch keinen fertigen binären Live-Snapshot.

## Messstand und bekannte Grenzen

Die gemessenen 33,8 ms für JSONL bzw. 34,7 ms für JSON betreffen einen
vollständigen kleinen Simulationslauf mit 1000 Schritten einschließlich
Physik und Dateischreiben, nicht die GUI-Latenz. Die Streuungen überlappen.
Die Mikroprobe mit 1000 bereits serialisierten Zuständen misst nur
Zusammenstellung, nicht vollständige Serialisierung oder GUI-Empfang.
Die Rohdaten liegen im Paket bei; keine Echtzeitgarantie.

Die konservative AABB-Überlappungsprüfung kann getrennte Körper ablehnen.
Folkeboot und Optimist sind derzeit deshalb abgelehnt, 420 mangels
Hydrostatikmasse. Das Quaderbeispiel ist als startfähige Referenz
beigefügt. Allgemeine Genauigkeits-/Performanceabnahme bleibt offen.

## Inhalt und Verwendung der ZIP

Das Archiv erhält die relativen Verzeichnisse der Dokumente:

- diese Übersicht und den aktuellen Ergebnisvertrag;
- Fehlerkatalog, öffentliche C++-Header und Build-/CLI-Anleitung;
- statische und dynamische Referenzaufzeichnungen;
- Modellbeschreibung samt Entwurfspunkten und Beispielmodell Quader
  inklusive seiner zweckgetrennten Meshes;
- Hydrostatikspezifikation mit archivierter Herkunftskennung;
- numerische Grenzen, Zeitmessungen, Implementierungsnachweise und
  die beiden Performanceberichte.

Die Header beschreiben den Stand, sind aber kein eigenständiges SDK:
Zur direkten Anbindung ist die passende Core-Bibliothek aus demselben
Quellstand mit kompatiblem Compiler und Laufzeit zu bauen.
Die ZIP enthält keine ausführbaren Programme oder fertigen GUI-Binaries.
`MANIFEST.json` im Archiv benennt Dateien und SHA-256-Fingerprints.
