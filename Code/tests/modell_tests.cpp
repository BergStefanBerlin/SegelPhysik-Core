#include "segelphysik/modell.hpp"
#include "segelphysik/messung.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {

void pruefe(bool bedingung, const std::string& text)
{
    if (!bedingung) throw std::runtime_error(text);
}

void gleich(double ist, double soll)
{
    if (!std::isfinite(ist) || std::abs(ist - soll) > 2e-10 * std::max(1., std::abs(soll)))
        throw std::runtime_error("Zahlenabweichung: " + std::to_string(ist) + " statt " + std::to_string(soll));
}

// Fixtures live below the working directory, never in a shared system temp.
struct Dateien {
    std::filesystem::path ordner;
    Dateien()
    {
        const auto start = std::chrono::steady_clock::now().time_since_epoch().count();
        for (unsigned i = 0; i < 100; ++i) {
            auto kandidat = std::filesystem::current_path() /
                ("modell_test_" + std::to_string(start) + "_" + std::to_string(i));
            if (std::filesystem::create_directory(kandidat)) { ordner = std::move(kandidat); return; }
        }
        throw std::runtime_error("Kein eindeutiger Fixture-Ordner");
    }
    ~Dateien()
    {
        std::error_code ec;
        std::filesystem::remove_all(ordner, ec);
    }
    void schreiben(const std::string& name, const std::string& inhalt) const
    {
        std::ofstream ausgabe(ordner / std::filesystem::u8path(name), std::ios::binary);
        ausgabe << inhalt;
        if (!ausgabe) throw std::runtime_error("Fixture nicht schreibbar");
    }
    segelphysik::Modell laden(const std::string& inhalt) const
    {
        schreiben("modell.json", inhalt);
        return segelphysik::lade_modell(ordner / "modell.json");
    }
};

std::string modell(const std::string& elemente, const std::string& extra = "")
{
    return R"({"schema_version":"0.1-entwurf","id":"test",)" + extra +
        R"("gruppen":{"testgruppe":{"elemente":[)" + elemente + "]}}}";
}

std::string element(const std::string& masse, const std::string& volumen = "", const std::string& id = "e")
{
    return R"({"id":")" + id + R"(","Hydrostatik":{"masse":)" + masse +
        (volumen.empty() ? "" : R"(,"volumen":{"datei":")" + volumen + R"("})") + "}}";
}

std::string punkt(double masse = 1., const std::string& position = "[0,0,0]")
{
    return R"({"art":"punktmasse","masse_kg":)" + std::to_string(masse) +
        R"(,"position_m":)" + position + "}";
}

std::string solid(const std::string& pfad = "mesh.obj", double dichte = 1.)
{
    return R"({"art":"vollkoerper","dichte_kg_pro_m3":)" + std::to_string(dichte) +
        R"(,"datei":")" + pfad + R"("})";
}

std::string schale(const std::string& pfad = "mesh.obj")
{
    return R"({"art":"schale","dichte_kg_pro_m3":10,"wanddicke_m":0.2,"datei":")" + pfad + R"("})";
}

std::string tetra(bool umgekehrt = false, double versatz = 0.)
{
    std::ostringstream s;
    s << "v " << versatz << " 0 0\nv " << versatz+1. << " 0 0\nv " <<
        versatz << " 1 0\nv " << versatz << " 0 1\n";
    s << (umgekehrt ? "f 1 2 3\nf 1 4 2\nf 1 3 4\nf 2 4 3\n" :
                             "f 1 3 2\nf 1 2 4\nf 1 4 3\nf 2 3 4\n");
    return s.str();
}

const std::string wuerfel =
    "v -1 -1 -1\nv -1 -1 1\nv -1 1 -1\nv -1 1 1\n"
    "v 1 -1 -1\nv 1 -1 1\nv 1 1 -1\nv 1 1 1\n"
    "f 3 1 2\nf 3 2 4\nf 2 1 5\nf 2 5 6\nf 5 1 3\nf 5 3 7\n"
    "f 4 2 6\nf 4 6 8\nf 7 3 4\nf 7 4 8\nf 6 5 7\nf 6 7 8\n";

const std::string dreieck = "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";

std::string anhaengen(const std::string& erstes, const std::string& zweites, std::size_t offset)
{
    std::istringstream quelle(zweites);
    std::ostringstream ziel;
    ziel << erstes;
    std::string zeile;
    while (std::getline(quelle, zeile)) {
        if (zeile.compare(0, 2, "f ") == 0) {
            std::istringstream face(zeile.substr(2));
            std::size_t a = 0, b = 0, c = 0;
            face >> a >> b >> c;
            ziel << "f " << a+offset << ' ' << b+offset << ' ' << c+offset << '\n';
        } else ziel << zeile << '\n';
    }
    return ziel.str();
}

template <typename Funktion>
void fehler(Funktion funktion, const std::string& id, const std::string& kontext = "modell.json")
{
    try {
        funktion();
    } catch (const std::runtime_error& e) {
        const std::string text = e.what();
        pruefe(text.find(id) != std::string::npos, "Falsche Fehler-ID: " + text);
        pruefe(text.find(kontext) != std::string::npos, "Fehlender Kontext: " + text);
        return;
    }
    throw std::runtime_error("Fehler nicht erkannt: " + id);
}

void json_tests(Dateien& d)
{
    for (const auto& ungueltig : {"", "{}", "[]", R"({"schema_version":"1","id":"x","gruppen":{}})"})
        fehler([&] { d.laden(ungueltig); }, ungueltig[0] == '\0' ? "CORE-MOD-0001" : "CORE-MOD-0002");
    for (const std::string& json : {
            std::string(R"({"x":1,"x":2})"), std::string(R"({"x":1,"\u0078":2})"),
            std::string("{\"x\":01}"), std::string("{\"x\":1.}"), std::string("{\"x\":+1}"),
            std::string("{\"x\":1e}"), std::string("{\"x\":NaN}"), std::string("{\"x\":1e999}"),
            std::string("{\"x\":Infinity}"), std::string("{\"x\":true,}"),
            std::string("{\"x\":[0,]}"), std::string("{}{}"), std::string(R"({"x":"\q"})"),
            std::string(R"({"x":"\ud800"})"), std::string(R"({"x":"\udc00"})"),
            std::string(R"({"x":"\ud800\u0041"})"), std::string("{\"x\":\"\n\"}"),
            std::string("{\"x\":\"") + '\xc0' + '\x80' + "\"}",
            std::string("{\"x\":\"") + '\xed' + '\xa0' + '\x80' + "\"}"}) {
        fehler([&] { d.laden(json); }, "CORE-MOD-0001");
    }
    std::string tief(130, '[');
    tief += "0" + std::string(130, ']');
    fehler([&] { d.laden(tief); }, "CORE-MOD-0001");
    auto unicode = d.laden(modell(element(punkt(), "", R"(Gr\u00f6\u00dfe-\ud83d\ude80)"),
                                 R"("name":"a\"\\\/\b\f\n\r\t","entwurf":{"simulationsfreigabe":false},)"));
    pruefe(unicode.elemente.front().id == u8"Größe-🚀", "Unicode-Dekodierung");
    auto roh = d.laden(modell(element(punkt(), "", u8"Größe-🚀")));
    pruefe(roh.elemente.front().id == unicode.elemente.front().id, "UTF-8-Dekodierung");
    auto grossbaum = d.laden(modell(element(punkt(), "", u8"Großbaum")));
    auto escaped = d.laden(modell(element(punkt(), "", R"(Gro\u00dfbaum)")));
    pruefe(grossbaum.elemente.front().id == escaped.elemente.front().id, "Raw-UTF-8-ID Grossbaum");
    fehler([&] { d.laden(modell(element(punkt(), "", std::string("x") + '\xff'))); }, "CORE-MOD-0001");
    fehler([&] { d.laden(modell(element(punkt(), "", u8"Großbaum") + "," +
                              element(punkt(), "", R"(Gro\u00dfbaum)"))); }, "CORE-MOD-0003");
    fehler([&] { d.laden(modell("")); }, "CORE-MOD-0004", "Keine Physik");
    for (const auto& falsch : {
            R"({"id":"e"})", R"({"Hydrostatik":{"masse":{}}})",
            R"({"id":"","Darstellung":{"datei":"a.obj"}})", R"({"id":2,"Darstellung":{"datei":"a.obj"}})"}) {
        fehler([&] { d.laden(modell(falsch)); },
               std::string(falsch) == R"({"id":"e"})" ? "CORE-MOD-0002" : "CORE-MOD-0003");
    }
    fehler([&] { d.laden(modell(element(punkt()) + "," + element(punkt()))); }, "CORE-MOD-0003", "Element e");
    fehler([&] { d.laden(R"({"schema_version":"0.1-entwurf","id":"x","gruppen":{"a":{"elemente":[)" +
        element(punkt()) + R"(]},"b":{"elemente":[)" + element(punkt()) + "]}}}"); }, "CORE-MOD-0003");
    fehler([&] { d.laden(modell(element(punkt()), R"("unbekannt":1,)")); }, "CORE-MOD-0002");
    fehler([&] { d.laden(modell(R"({"id":"e","Darstellung":{"datei":"a.obj"},"position_m":[0,0,0]})")); },
           "CORE-MOD-0002");
    fehler([&] { d.laden(modell(R"({"id":"e","Hydrostatik":{}})")); }, "CORE-MOD-0004", "Element e");
    fehler([&] { d.laden(modell(R"({"id":"e","Hydrostatik":null})")); }, "CORE-MOD-0002");
    d.schreiben("mesh.obj", wuerfel);
    const std::string hydrorumpf = R"({"id":"Rumpf","Hydrostatik":{"masse":)" +
        punkt() + R"(,"volumen":{"datei":"mesh.obj"}},"Hydrodynamik":{"rolle":"rumpf"}})";
    auto rolle = d.laden(modell(hydrorumpf));
    pruefe(rolle.elemente.front().hydrodynamik_rolle == segelphysik::HydrodynamikRolle::rumpf,
        "Hydrodynamikrolle nicht importiert");
    const auto alte_fassung = rolle.fassung;
    const std::string hydrokiel = R"({"id":"Rumpf","Hydrostatik":{"masse":)" +
        punkt() + R"(,"volumen":{"datei":"mesh.obj"}},"Hydrodynamik":{"rolle":"kiel"}})";
    rolle = d.laden(modell(hydrokiel));
    pruefe(rolle.elemente.front().hydrodynamik_rolle == segelphysik::HydrodynamikRolle::kiel &&
        rolle.fassung != alte_fassung, "Hydrodynamikrolle fehlt in der Modellfassung");
    fehler([&] { d.laden(modell(R"({"id":"e","Hydrostatik":{"masse":{"art":"punktmasse","masse_kg":1,"position_m":[0,0,0]}},"Hydrodynamik":{"rolle":"rumpf"}})")); },
        "CORE-MOD-0003", "Hydrodynamik");
    fehler([&] { d.laden(modell(R"({"id":"e","Hydrostatik":{"masse":{"art":"punktmasse","masse_kg":1,"position_m":[0,0,0]},"volumen":{"datei":"mesh.obj"}},"Hydrodynamik":{"rolle":"fluegel"}})")); },
        "CORE-MOD-0003", "Hydrodynamik.rolle");
    fehler([&] { d.laden(modell(R"({"id":"e","Hydrostatik":{"masse":{"art":"punktmasse","masse_kg":1,"position_m":[0,0,0]},"volumen":{"datei":"mesh.obj"}},"Hydrodynamik":{"rolle":"rumpf","extra":true}})")); },
        "CORE-MOD-0002", "Hydrodynamik");
    fehler([&] { d.laden(modell(R"({"id":"e","Darstellung":{"datei":"/absolut.obj"}})")); }, "CORE-MOD-0002");
    fehler([&] { d.laden(modell(R"({"id":"e","Darstellung":{"datei":"a\u0000.obj"}})")); }, "CORE-MOD-0002");
    fehler([&] { segelphysik::lade_modell(d.ordner / "fehlt.json"); }, "CORE-MOD-0005", "fehlt.json");
}

void masse_tests(Dateien& d)
{
    auto m = d.laden(modell(element(punkt(2., "[1,2,3]"), "", "a") + "," +
                            element(punkt(2., "[-1,-2,-3]"), "", "b")));
    gleich(m.masse_kg, 4.);
    for (double c : m.schwerpunkt_m) gleich(c, 0.);
    const std::array<double, 9> steiner{52., -8., -12., -8., 40., -24., -12., -24., 20.};
    for (std::size_t i = 0; i < 9; ++i) gleich(m.traegheit_kg_m2[i], steiner[i]);
    pruefe(!m.elemente.front().volumen, "Keine Ersatzverdraengung");
    auto versetzt = d.laden(modell(element(punkt(2., "[101,202,303]"), "", "a") + "," +
                                    element(punkt(2., "[99,198,297]"), "", "b")));
    for (std::size_t i = 0; i < 9; ++i) gleich(versetzt.traegheit_kg_m2[i], steiner[i]);
    gleich(versetzt.schwerpunkt_m[0], 100.);
    gleich(versetzt.schwerpunkt_m[1], 200.);
    gleich(versetzt.schwerpunkt_m[2], 300.);
    for (const auto& ungueltig : {
            "null", "{}", R"({"art":"unbekannt"})", R"({"art":"punktmasse","masse_kg":0,"position_m":[0,0,0]})",
            R"({"art":"punktmasse","masse_kg":-1,"position_m":[0,0,0]})",
            R"({"art":"punktmasse","masse_kg":"2","position_m":[0,0,0]})",
            R"({"art":"punktmasse","masse_kg":2,"position_m":[0,0]})",
            R"({"art":"punktmasse","masse_kg":2,"position_m":[0,null,0]})",
            R"({"art":"punktmasse","masse_kg":2,"position_m":[0,0,0],"datei":"a.obj"})",
            R"({"art":"vollkoerper","dichte_kg_pro_m3":-1,"datei":"a.obj"})",
            R"({"art":"schale","dichte_kg_pro_m3":1,"wanddicke_m":0,"datei":"a.obj"})",
            R"({"art":"schale","dichte_kg_pro_m3":1,"datei":"a.obj"})"}) {
        fehler([&] { d.laden(modell(element(ungueltig))); }, "CORE-MOD-0004", "Element e");
    }
    const std::string bild = R"({"id":"bild","Darstellung":{"datei":"nicht-vorhanden.obj"}})";
    fehler([&] { d.laden(modell(bild)); }, "CORE-MOD-0004", "Keine Physik");
    auto zeichnung = d.laden(modell(bild + "," + element(punkt())));
    gleich(zeichnung.masse_kg, 1.);
    gleich(zeichnung.elemente.front().masse_kg, 0.);
    pruefe(!zeichnung.elemente.front().volumen, "Darstellung wird nicht physikalisch verwendet");
    d.schreiben("mesh.obj", wuerfel);
    m = d.laden(modell(element(solid("mesh.obj", 3.), "mesh.obj")));
    gleich(m.masse_kg, 24.);
    pruefe(m.elemente.front().volumen.has_value(), "Volumen fehlt");
    gleich(m.traegheit_kg_m2[0], 16.);
    gleich(m.traegheit_kg_m2[4], 16.);
    gleich(m.traegheit_kg_m2[8], 16.);
    d.schreiben("mesh.obj", tetra());
    m = d.laden(modell(element(solid("mesh.obj", 6.))));
    gleich(m.masse_kg, 1.);
    for (double c : m.schwerpunkt_m) gleich(c, .25);
    const std::array<double, 9> tetra_i{.075, .0125, .0125, .0125, .075, .0125, .0125, .0125, .075};
    for (std::size_t i = 0; i < 9; ++i) gleich(m.traegheit_kg_m2[i], tetra_i[i]);
    d.schreiben("mesh.obj", tetra(false, 1000.));
    const auto verschoben = d.laden(modell(element(solid("mesh.obj", 6.))));
    gleich(verschoben.schwerpunkt_m[0], 1000.25);
    for (std::size_t i = 0; i < 9; ++i) gleich(verschoben.traegheit_kg_m2[i], tetra_i[i]);
    d.schreiben("mesh.obj", dreieck);
    m = d.laden(modell(element(schale())));
    gleich(m.masse_kg, 1.);
    gleich(m.schwerpunkt_m[0], 1./3.);
    gleich(m.schwerpunkt_m[1], 1./3.);
    gleich(m.schwerpunkt_m[2], 0.);
    gleich(m.traegheit_kg_m2[0], 1./18.);
    gleich(m.traegheit_kg_m2[4], 1./18.);
    gleich(m.traegheit_kg_m2[8], 1./9.);
    gleich(m.traegheit_kg_m2[1], 1./36.);
    const auto einzel = m;
    d.schreiben("mesh.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nv .5 .5 0\nf 1 2 4\nf 1 4 3\n");
    m = d.laden(modell(element(schale())));
    gleich(m.masse_kg, einzel.masse_kg);
    for (std::size_t i = 0; i < 9; ++i) gleich(m.traegheit_kg_m2[i], einzel.traegheit_kg_m2[i]);
    // A tilted triangle tests all tensor entries, not only axis-aligned diagonals.
    d.schreiben("mesh.obj", "v 0 0 0\nv 1 0 1\nv 0 1 1\nf 1 2 3\n");
    m = d.laden(modell(element(schale())));
    const double wurzel3 = std::sqrt(3.);
    gleich(m.masse_kg, wurzel3);
    const std::array<double, 9> geneigt{1./9., 1./36., -1./36.,
        1./36., 1./9., -1./36., -1./36., -1./36., 1./9.};
    for (std::size_t i = 0; i < 9; ++i) gleich(m.traegheit_kg_m2[i], wurzel3 * geneigt[i]);
    // A closed shell remains a shell: cube area 24, areal density 2.
    d.schreiben("mesh.obj", wuerfel);
    m = d.laden(modell(element(schale())));
    gleich(m.masse_kg, 48.);
    gleich(m.traegheit_kg_m2[0], 160./3.);
    // Displacement geometry must not change the mass geometry's integrals.
    d.schreiben("volumen.obj", tetra(false, 10.));
    m = d.laden(modell(element(solid(), "volumen.obj")));
    gleich(m.masse_kg, 8.);
    gleich(m.schwerpunkt_m[0], 0.);
    gleich(m.elemente.front().volumen->punkte.front()[0], 10.);
    // A-C-00047: only open shells with displacement volume get a flooding rim.
    pruefe(m.elemente.front().flutrand_m.empty(), "Vollkoerper mit Flutrand");
    d.schreiben("mesh.obj", dreieck);
    m = d.laden(modell(element(schale(), "volumen.obj")));
    pruefe(m.elemente.front().flutrand_m.size() == 3, "Offene Schale ohne Flutrand");
    m = d.laden(modell(element(schale())));
    pruefe(m.elemente.front().flutrand_m.empty(), "Flutrand ohne Volumen");
    d.schreiben("mesh.obj", wuerfel);
    m = d.laden(modell(element(schale(), "volumen.obj")));
    pruefe(m.elemente.front().flutrand_m.empty(), "Geschlossene Schale mit Flutrand");
    fehler([&] { d.laden(modell(element(
        R"({"art":"vollkoerper","dichte_kg_pro_m3":1e308,"datei":"mesh.obj"})"))); }, "CORE-MOD-0004");
    fehler([&] { d.laden(modell(element(solid("fehlt.obj")))); }, "CORE-MOD-0005", "Element e");
    fehler([&] { d.laden(modell(element(punkt(1e308), "", "a") + "," +
                                      element(punkt(1e308), "", "b"))); }, "CORE-MOD-0004");
    fehler([&] { d.laden(modell(element(punkt(1., "[1e308,0,0]"), "", "a") + "," +
                                      element(punkt(1., "[-1e308,0,0]"), "", "b"))); }, "CORE-MOD-0004");
}

void mesh_tests(Dateien& d)
{
    for (const auto& mesh : {
            "", "v nan 0 0\n", "v inf 0 0\n", "v 1e999 0 0\n", "v 0 0\n",
            "v 0 0 0 1\n", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 0\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 4\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 -4\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 2\n",
            "v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1/1 2/1 3/1\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1/ 2 3\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1// 2 3\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3 1\n",
            "v 0 0 0\nv 1 0 0\nv 0 1 0\nl 1 2 3\n"}) {
        d.schreiben("mesh.obj", mesh);
        fehler([&] { d.laden(modell(element(schale()))); }, "CORE-MOD-0006", "mesh.obj");
    }
    d.schreiben("mesh.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt .5 .5\nvn 0 0 1\nf -3/-1/-1 -2/-1/-1 -1/-1/-1 # triangle\n");
    gleich(d.laden(modell(element(schale()))).masse_kg, 1.);
    d.schreiben("mesh.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
    gleich(d.laden(modell(element(schale()))).masse_kg, 1.);
    d.schreiben("mesh.obj", "v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//1 2//1 3//1\n");
    gleich(d.laden(modell(element(schale()))).masse_kg, 1.);
    d.schreiben("mesh.obj", dreieck + "f 3 2 1\n");
    fehler([&] { d.laden(modell(element(schale()))); }, "CORE-MOD-0008", "Element e");
    d.schreiben("mesh.obj", dreieck);
    fehler([&] { d.laden(modell(element(solid()))); }, "CORE-MOD-0006");
    fehler([&] { d.laden(modell(element(punkt(), "mesh.obj"))); }, "CORE-MOD-0006");
    d.schreiben("mesh.obj", tetra(true));
    fehler([&] { d.laden(modell(element(solid()))); }, "CORE-MOD-0006");
    // Aggregate volume remains positive, but the second component is inward.
    d.schreiben("mesh.obj", anhaengen(wuerfel, tetra(true, 3.), 8));
    fehler([&] { d.laden(modell(element(solid()))); }, "CORE-MOD-0006",
           "Volumenkomponente nicht nach aussen orientiert");
    d.schreiben("mesh.obj", anhaengen(wuerfel, tetra(false, 3.), 8));
    gleich(d.laden(modell(element(solid()))).masse_kg, 8.+1./6.);
    std::string inkonsistent = tetra();
    const auto face = inkonsistent.find("f 1 3 2");
    inkonsistent.replace(face, 7, "f 1 2 3");
    d.schreiben("mesh.obj", inkonsistent);
    fehler([&] { d.laden(modell(element(solid()))); }, "CORE-MOD-0006");
    d.schreiben("mesh.obj", tetra() + "v 0 0 0\n");
    fehler([&] { d.laden(modell(element(solid()))); }, "CORE-MOD-0006");
    // Duplicate material face using different vertex indices.
    d.schreiben("mesh.obj", dreieck + "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 4 5 6\n");
    fehler([&] { d.laden(modell(element(schale()))); }, "CORE-MOD-0008");
    // Non-coplanar and coplanar material intersections.
    d.schreiben("mesh.obj", dreieck +
        "v .25 .25 -1\nv .25 .25 1\nv .75 .25 0\nf 4 5 6\n");
    fehler([&] { d.laden(modell(element(schale()))); }, "CORE-MOD-0008");
    d.schreiben("mesh.obj", dreieck +
        "v .2 -.2 0\nv .2 .8 0\nv .8 .2 0\nf 4 5 6\n");
    fehler([&] { d.laden(modell(element(schale()))); }, "CORE-MOD-0008");
    // Coplanar triangles touching only in a vertex with nearly collinear opposite edges
    // (420er transom) are legal.
    d.schreiben("mesh.obj",
        "v -2.00438 0.266472 0.236609\nv -2.00438 0.277575 0.226499\nv -2.00438 0.284857 0.229548\n"
        "v -2.00438 0.277575 0.221445\nv -2.00438 0.283126 0.221445\nf 1 2 3\nf 4 5 2\n");
    d.laden(modell(element(schale())));
    d.schreiben("mesh.obj", tetra());
    d.schreiben("andere.obj", tetra(false, .5));
    fehler([&] { d.laden(modell(element(punkt(), "mesh.obj", "a") + "," +
                                      element(punkt(), "andere.obj", "b"))); }, "CORE-MOD-0007", "Element b");
    fehler([&] { d.laden(modell(element(punkt(), "mesh.obj", "a") + "," +
                                      element(punkt(), "andere.obj", "b"))); }, "CORE-MOD-0007",
           "Verdraengungsvolumina durchdringen oder enthalten sich");
    // Merely touching AABBs have no positive-volume overlap.
    d.schreiben("andere.obj", tetra(false, 1.));
    gleich(d.laden(modell(element(punkt(), "mesh.obj", "a") + "," +
                           element(punkt(), "andere.obj", "b"))).masse_kg, 2.);
    const std::string kontaktkomponente =
        "v 1 .25 0\nv 2 .25 0\nv 1 1.25 0\nv 1 .25 1\n"
        "f 1 3 2\nf 1 2 4\nf 1 4 3\nf 2 3 4\n";
    d.schreiben("mesh.obj", anhaengen(tetra(), kontaktkomponente, 4));
    gleich(d.laden(modell(element(solid()))).masse_kg, 1./3.);
    d.schreiben("mesh.obj", tetra());
    d.schreiben("andere.obj", tetra(false, 2.));
    gleich(d.laden(modell(element(punkt(), "mesh.obj", "a") + "," +
                           element(punkt(), "andere.obj", "b"))).masse_kg, 2.);
    std::filesystem::create_directory(d.ordner / "unterordner");
    d.schreiben("unterordner\\masse.obj", tetra());
    gleich(d.laden(modell(element(solid("unterordner/masse.obj", 6.)))).masse_kg, 1.);
}

void fassung_tests(Dateien& d)
{
    d.schreiben("mesh.obj", tetra());
    const auto json = modell(element(solid()));
    const auto a = d.laden(json);
    const auto b = d.laden(json);
    pruefe(!a.fassung.empty() && a.fassung == b.fassung, "Fingerprint nicht stabil");
    d.schreiben("mesh.obj", tetra(false, 1.));
    const auto c = d.laden(json);
    pruefe(a.fassung != c.fassung, "Physikmesh nicht im Fingerprint");
    pruefe(c.fassung != d.laden(json + "\n").fassung, "JSON nicht im Fingerprint");
    const auto darstellung = modell(R"({"id":"bild","Darstellung":{"datei":"bild.obj"}},)" + element(punkt()));
    const auto vorher = d.laden(darstellung);
    d.schreiben("bild.obj", "beliebiger Grafikinhalt");
    pruefe(vorher.fassung == d.laden(darstellung).fassung, "Grafikinhalt veraendert Physikfassung");
}

void volumen_ueberschneidung_tests(Dateien& d)
{
    const auto paar = [&] {
        return d.laden(modell(element(punkt(), "a.obj", "a") + "," +
                              element(punkt(), "b.obj", "b")));
    };
    const std::string diagnose = "Verdraengungsvolumina durchdringen oder enthalten sich";
    d.schreiben("a.obj", tetra());
    // A-C-00050: x+y+z <= 1 versus x+y+z >= 2: disjoint despite identical AABBs.
    d.schreiben("b.obj",
        "v 1 1 1\nv 0 1 1\nv 1 0 1\nv 1 1 0\n"
        "f 1 2 3\nf 1 4 2\nf 1 3 4\nf 2 4 3\n");
    gleich(paar().masse_kg, 2.);
    // Positive-volume intersection.
    d.schreiben("b.obj", tetra(false, .25));
    fehler(paar, "CORE-MOD-0007", diagnose);
    // Strict containment without any triangle-triangle intersection.
    d.schreiben("b.obj",
        "v .1 .1 .1\nv .2 .1 .1\nv .1 .2 .1\nv .1 .1 .2\n"
        "f 1 3 2\nf 1 2 4\nf 1 4 3\nf 2 3 4\n");
    fehler(paar, "CORE-MOD-0007", diagnose);
    // An air body in a hollow of a hull, separated by a small gap, is legal:
    // a U-shaped hull (two towers on a slab) with a box between the towers.
    const auto quader = [](double x0, double y0, double z0, double x1, double y1, double z1, int basis) {
        std::ostringstream s;
        s << "v " << x0 << ' ' << y0 << ' ' << z0 << "\nv " << x1 << ' ' << y0 << ' ' << z0
          << "\nv " << x1 << ' ' << y1 << ' ' << z0 << "\nv " << x0 << ' ' << y1 << ' ' << z0
          << "\nv " << x0 << ' ' << y0 << ' ' << z1 << "\nv " << x1 << ' ' << y0 << ' ' << z1
          << "\nv " << x1 << ' ' << y1 << ' ' << z1 << "\nv " << x0 << ' ' << y1 << ' ' << z1 << '\n';
        const int f[12][3] = {{1,3,2},{1,4,3},{5,6,7},{5,7,8},{1,2,6},{1,6,5},
                              {2,3,7},{2,7,6},{3,4,8},{3,8,7},{4,1,5},{4,5,8}};
        for (const auto& t : f) s << "f " << t[0]+basis << ' ' << t[1]+basis << ' ' << t[2]+basis << '\n';
        return s.str();
    };
    d.schreiben("a.obj", quader(0, 0, 0, 3, 1, 1, 0) + quader(0, 0, 1.5, 1, 1, 3, 8) +
                         quader(2, 0, 1.5, 3, 1, 3, 16));
    d.schreiben("b.obj", quader(1.1, .1, 1.1, 1.9, .9, 2.9, 0));
    gleich(paar().masse_kg, 2.);
    // A shared oblique face is surface contact without common volume: legal.
    d.schreiben("a.obj", tetra());
    d.schreiben("b.obj",
        "v 1 1 1\nv 1 0 0\nv 0 1 0\nv 0 0 1\n"
        "f 1 2 3\nf 1 4 2\nf 1 3 4\nf 2 4 3\n");
    gleich(paar().masse_kg, 2.);
    // A point of contact only, with AABB interiors separated.
    d.schreiben("b.obj", tetra(false, 1.));
    gleich(paar().masse_kg, 2.);
}

void sweep_tests(Dateien& d)
{
    std::ostringstream mesh;
    // Reverse spatial order exercises sorting and removal from the active set.
    for (std::size_t i = 0; i < 200; ++i) {
        const auto x = (200 - i) * 3;
        mesh << "v " << x << " 0 0\nv " << x+1 << " 0 0\nv " << x << " 1 0\n";
        const auto v = i * 3 + 1;
        mesh << "f " << v << ' ' << v+1 << ' ' << v+2 << '\n';
    }
    d.schreiben("mesh.obj", mesh.str());
    gleich(d.laden(modell(element(schale()))).masse_kg, 200.);
    // The last triangle intersects the first one after many expired boxes.
    mesh << "v 600.25 .25 -1\nv 600.25 .25 1\nv 600.75 .25 0\nf 601 602 603\n";
    d.schreiben("mesh.obj", mesh.str());
    fehler([&] { d.laden(modell(element(schale()))); }, "CORE-MOD-0008");
}

void beispiel_test(const std::filesystem::path& argument)
{
    if (argument.empty()) return;
    const auto m = segelphysik::lade_modell(argument);
    pruefe(m.id == "quader" && m.elemente.size() == 1, "Falsches Beispiel");
    gleich(m.masse_kg, 7600.);
    for (double c : m.schwerpunkt_m) gleich(c, 0.);
    gleich(m.traegheit_kg_m2[0], 7600.*2./3.);
    pruefe(m.elemente.front().volumen.has_value(), "Beispielvolumen fehlt");
}

void messung_test(Dateien& d)
{
#ifdef SEGELPHYSIK_TEST_INSTRUMENTATION
    segelphysik::testmessungen_zuruecksetzen();
    segelphysik::testmessungen_aktivieren(true);
    d.schreiben("mesh.obj", tetra());
    d.laden(modell(element(solid())));
    segelphysik::testmessungen_aktivieren(false);
    const auto werte = segelphysik::testmessungen();
    pruefe(werte.at("modellvorbereitung").anzahl == 1, "Importmessung fehlt");
    pruefe(werte.at("masseneigenschaften").anzahl == 1, "Massenmessung fehlt");
#else
    static_cast<void>(d);
#endif
}

}

int main(int argc, char** argv)
{
    try {
        Dateien dateien;
        json_tests(dateien);
        masse_tests(dateien);
        mesh_tests(dateien);
        volumen_ueberschneidung_tests(dateien);
        sweep_tests(dateien);
        fassung_tests(dateien);
        messung_test(dateien);
        std::filesystem::path beispiel;
        if (argc > 1) beispiel = std::filesystem::u8path(argv[1]);
        else {
            const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
            const auto kandidat = root.parent_path() / "Modelle" / "quader" / "modell.json";
            if (std::filesystem::exists(kandidat)) beispiel = kandidat;
        }
        beispiel_test(beispiel);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::cout << "Modellimport: alle Pruefungen bestanden.\n";
    return 0;
}
