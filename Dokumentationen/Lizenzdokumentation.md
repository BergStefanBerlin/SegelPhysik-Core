# Lizenzdokumentation SegelPhysik-Core

**Stand:** 27.10.2026 · **Geprüfter CORE-Commit:** `61e78de`  
**Ergebnis:** technische Bestandsprüfung und Bedingungenentwurf;
**keine wirksame Lizenzfestlegung oder Verteilungsfreigabe**.

## 1. Eigenständiger Umfang und Nutzerentscheidung

Diese Dokumentation gilt ausschließlich für SegelPhysik-Core und ist ein
Nachweis, keine neue Spezifikation und kein zusätzlicher Umsetzungsplan.
GUI hat eine eigenständige Prüfung, Lizenzentscheidung und Freigabe.
Modelle außerhalb dieses Repositories werden nicht mitlizenziert.

Der Nutzer hat am 27.10.2026 folgende Richtung vorgegeben:

- Private, nicht-kommerzielle Nutzung und kostenlose Weiterentwicklung.
- Schulen, Universitäten und gemeinnützige Vereine dürfen die Software auch
  bei Kursgebühren und bezahlten Forschungsprojekten nutzen; Originallink
  muss genannt werden.
- Bei Weitergabe müssen Software und weitergegebene Änderungen kostenlos
  bleiben; Quellcode, Lizenzbedingungen, Urheberhinweise und Originallink
  müssen erhalten bleiben. Dasselbe soll für nachfolgende Ableitungen gelten.
- Verkauf, betriebliche Nutzung für kommerzielle Arbeit und bezahlter
  Support, Schulungen und Auftragsentwicklung außerhalb dieser Ausnahme
  sind verboten.

Originalprojekt: https://github.com/BergStefanBerlin/SegelPhysik-Core

Die gewünschte Richtung ist dokumentiert. Der
[Bedingungenentwurf](Lizenzbedingungen_Entwurf.md) ist ausdrücklich nicht
in Kraft gesetzt und keine freie Open-Source-Lizenz. Es wird keine
Standardlizenz behauptet, die diese Kombination vollständig abdeckt.

## 2. Prüfung bestehender Lizenzmodelle

| Modell | Ergebnis |
|---|---|
| MIT | Erlaubt kommerzielle Nutzung und geschlossene Ableitungen; ungeeignet. |
| GPL-3.0-or-later | Erlaubt kommerzielle Nutzung und Verkauf; trotz Quellcodepflicht bei Weitergabe ungeeignet. |
| PolyForm Noncommercial 1.0.0 | Nicht-kommerzielle Zwecke und Ausnahmen für Bildung/gemeinnützige Organisationen passen teilweise. Kostenlose Weitergabe, vollständiger Quellcode und gleiche Bedingungen für eigene Änderungen werden aber nicht verlangt; deckt den Nutzerwunsch nicht vollständig ab. |

Der [PolyForm-Originaltext](https://polyformproject.org/licenses/noncommercial/1.0.0)
wurde geprüft, nicht verändert oder als wirksame Projektlizenz übernommen.
Der eigene Entwurf ist ein projektbezogener Vorschlag zur rechtlichen
Prüfung; daraus folgt keine geprüfte Durchsetzbarkeit.

## 3. Technischer Befund ausschließlich für CORE

Geprüft wurden [Code/CMakeLists.txt](../Code/CMakeLists.txt), die Include-
Anweisungen der C++-Quellen/Header unter `Code`, der Standard-Buildcache
und die direkten PE-Importe der vorhandenen CLI.

- Eigenständige C++17-Bibliothek `segelphysik_core` als statisches Target;
  `segelphysik_cli` wird dagegen gelinkt.
- Die Builddefinition lädt keine Qt-/VTK-Pakete und deklariert keine
  fremden C++-Paketabhängigkeiten. Die gelesenen Includes verweisen auf
  C++-Standardheader, eigene Projektdateien und den aus eigenen Ressourcen
  erzeugten Fehlerkatalog.
- Das ist keine Herkunftsprüfung sämtlicher Implementierungen und kein
  Nachweis, dass alle Beiträge ausschließlich eigene Rechte enthalten.
- Der Cache `build\standard\CMakeCache.txt` nennt Visual Studio 17 2022
  und den `Code`-Quellordner dieses Repositories.
- `dumpbin /DEPENDENTS build\standard\Release\segelphysik_cli.exe`
  (MSVC 14.44.35229.0) zeigt direkt:
  `MSVCP140.dll`, `MSVCP140_2.dll`, `VCRUNTIME140.dll`,
  `VCRUNTIME140_1.dll`, `KERNEL32.dll` sowie die UCRT-API-Sets
  für runtime, heap, math, string, stdio, filesystem und locale.
  Keine direkte Qt-/VTK-Abhängigkeit in diesem vorhandenen Binärstand.
- Das ist keine rekursive Untersuchung aller Laufzeitabhängigkeiten.
  Ein neuer Build zur Gleichheitsprüfung mit HEAD wurde nicht durchgeführt.
  Binärbefund und Quellcodebefund dürfen deshalb nicht gleichgesetzt werden.

**Weitergabe der Windows-Laufzeit:** Die vorhandene CLI verwendet Microsoft-
Laufzeitkomponenten. Diese werden nicht durch die eigene Projektlizenz
freigegeben. Vor einer Mitverteilung sind die zutreffenden Visual-Studio-
Lizenz-/Redistributable-Bedingungen zu prüfen. Kostenloses Anbieten bedeutet
nicht, dass das Paket ausschließlich aus frei lizenzierten Komponenten besteht.

## 4. Abgrenzung gegenüber GUI und Modellen

Ein späterer nicht-kommerzieller Lizenztext für CORE macht Qt/VTK weder
nicht-kommerziell noch freigegeben. Bei Einbindung von CORE in GUI sind die
GUI-spezifischen Fremdlizenzen für die Kombination gesondert zu prüfen,
insbesondere LGPLv3 § 4. Eine Freigabe hier schließt IDEE-G-00006 nicht.

Die [Buildanleitung](../Code/README.md) verwendet Modelle aus dem
Geschwisterordner `Modelle`. Deren Herkunft und Weitergaberechte sind für
ein Paket mit Modellen gesondert nachzuweisen. Das gilt auch für Inhalte in
Übergabearchiven; eine eigene Code-Lizenz erfasst diese nicht automatisch.

## 5. Abschlusskriterien

- [x] Gewünschte eigene CORE-Lizenzrichtung mit dem Nutzer klären und dokumentieren.
- [x] Standardmodelle auf die wesentlichen gewünschten Bedingungen vergleichen.
- [x] Eigene Builddefinition, Include-Arten und direkte CLI-Importe technisch prüfen.
- [x] Getrennten, ausdrücklich nicht wirksamen Bedingungenentwurf erstellen.
- [ ] Rechteinhaber und Zustimmungen aller betroffenen Beiträge bestätigen.
- [ ] Reichweite der Bildungs-/Vereinsausnahme, Dienstleistungsverbote und
  Weitergabepflichten bei Bibliothekseinbindung abschließend prüfen.
- [ ] Rechtlich geeigneten endgültigen Text einschließlich Gewährleistung,
  Haftung, Verstößen und gegebenenfalls Patentrechten prüfen und freigeben.
- [ ] Konkretes Verteilpaket und dessen Quellen/Versionen bestimmen; enthaltene
  Laufzeitkomponenten, Modelle und fremde Inhalte vollständig abgrenzen.
- [ ] Erforderliche Lizenztexte, Urheberhinweise und Quellcodebeigaben bereitstellen;
  Windows-Runtime-Verteilungsbedingungen prüfen.
- [ ] Endgültige Lizenz ausdrücklich in Kraft setzen und das konkrete Paket freigeben.

Es wurden keine bestehenden A-/U-/IDEE-Kennungen neu vergeben oder geschlossen.
Offene fachliche Core-Abnahmen bleiben von dieser Prüfung unberührt.
GitHub-Nachverfolgung:
[CORE-Issue #1](https://github.com/BergStefanBerlin/SegelPhysik-Core/issues/1).
Die dort beschriebenen Dokumentationsänderungen sind noch nicht committet
oder gepusht; das Issue ersetzt keine veröffentlichte Lizenz.

## 6. Quellen und Nachweisgrenzen

- [PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0).
- [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.html), insbesondere §§ 4–6.
- [GNU LGPLv3](https://www.gnu.org/licenses/lgpl-3.0.html), insbesondere § 4
  für eine spätere Kombination mit der GUI.
- [Microsoft: Redistribute Visual C++ Files](https://learn.microsoft.com/en-us/cpp/windows/redistributing-visual-cpp-files?view=msvc-170).
- Eigene Builddefinition, Quell-Includes, lokaler Standard-Buildcache und
  direkte `dumpbin`-Importliste des vorhandenen CLI-Binärstands am Prüftag.

Die lokalen Dateien sind veränderliche Nachweise, kein archiviertes und
gehashtes Verteilpaket. Keine vollständige Provenienzprüfung, keine
juristische Abnahme, kein neuer Build und keine neuen Laufzeittests.
Diese technische Prüfung ist keine Rechtsberatung.
