# SegelPhysik-Core

Nativer C++17-Rechenkern fuer Bootsmodelle: Modellimport und -pruefung,
Masse und Traegheit, Hydrostatik, hydrodynamische Kraefte, statisches
Gleichgewicht, Starrkoerperdynamik und stationaere Kurvenfahrt.
Bibliothek und CLI sind ohne GUI und ohne Python nutzbar.

## Projekte und Daten

- **Core:** Dieses Repository besitzt Berechnung, physische Modelldaten
  und den Core-GUI-Vertrag.
- [GUI](https://github.com/BergStefanBerlin/SegelPhysik-GUI):
  native Qt-/VTK-Oberflaeche fuer Bootsauswahl, statisches und dynamisches Modell.
- [Modelle](https://github.com/BergStefanBerlin/SegelPhysik-Modelle):
  versionierte Modellhistorie; mit der benoetigten Schemafassung abgleichen.
  Ein lokaler Modellordner ist nicht automatisch ein Git-Arbeitsbaum.

Die Repositories besitzen unabhaengige Versionsverlaeufe:
Core-Tags `C-X.Y`, GUI-Tags `G-X.Y`. Ein Core-Release veroeffentlicht
weder GUI noch Modelle automatisch.

## Einstieg

Voraussetzungen: CMake ab 3.20, C++17-Compiler; unter Windows z. B.
Visual Studio 2022 Build Tools. Beispiel vom Repository-Stamm:

```powershell
cmake -S Code -B build\standard -DBUILD_TESTING=ON
cmake --build build\standard --config Release
ctest --test-dir build\standard -C Release --output-on-failure
.\build\standard\Release\segelphysik_cli.exe --build-info
.\build\standard\Release\segelphysik_cli.exe --help
```

Tests benoetigen die passenden Modellfixtures standardmaessig neben
`CORE` im Ordner `Modelle`; mit `SEGELPHYSIK_MODELLE_DIR` konfigurierbar.
Bei Einkonfigurationsgeneratoren `-DCMAKE_BUILD_TYPE=Release` verwenden;
die EXE liegt dann direkt im Buildordner.
Aufrufe und Details: [nativer Build und CLI](Code/README.md).

## Vertraege und Dokumentation

- [Core-GUI-Schnittstelle und Ergebnisvertrag](Dokumentationen/Schnittstelle_Core_GUI.md)
- [Schnittstellenregister](Dokumentationen/Schnittstellenregister_Core.md)
- [Modellbeschreibung](Spezifikationen/Spezifikation_Modellbeschreibung.md)
- [Numerische/geometrische Grenzen](Dokumentationen/Umsetzungsnachweise_Hydrostatikkern.md)
- [Zeitmessungen](Dokumentationen/Zeitmessungen.md)

Koordinaten: X zum Bug, Y nach Backbord, Z nach oben; SI-Einheiten.
Definition, berechnete Groessen und reine Darstellung bleiben getrennt.
Ein gruenes Testergebnis ersetzt keine fachliche Modellfreigabe.

## Naechste geplante Stufe

Parametrische Segelmodellierung und Riggumbau werden vorbereitet:
Form aus Abmessungen, Woelbung, Bauchlage, Twist, Latten und Trimm;
starrer Mast/Vorstag, beweglicher Baum und nachvollziehbare Fuehrungen.
Erstes Ziel ist die Darstellung im vorhandenen Bootsmodell ohne
vorausgesetzte Windphysik. **Diese Stufe ist noch nicht implementiert.**
Wind-/Segelkraefte folgen separat. Optisches Flattern ist eine reine
GUI-Funktion; physikalische Tuchmechanik bleibt eine moegliche
spaetere Erweiterung.

Vor Modellveraenderungen ist der gesamte lokale Modelle-Ordner
zu sichern und das Archiv zu pruefen. Pro logischem Segel soll nur
eine aktuelle Definition existieren; Historie gehoert in Backup/Git,
nicht als weitere aktive Segelkopie in den Katalog.

## Lizenzstatus

Die [Lizenzdokumentation](Dokumentationen/Lizenzdokumentation.md)
beschreibt den aktuellen Pruefstand. Die vorgeschlagenen Bedingungen
sind noch nicht in Kraft; dieses README erteilt keine Nutzungserlaubnis
oder Verteilungsfreigabe. Rechte fremder Komponenten und Modelldaten
sind gesondert zu beachten.
