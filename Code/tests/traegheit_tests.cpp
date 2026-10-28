#include "segelphysik/simulation.hpp"

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace segelphysik;

namespace {
void pruefe(bool ok, const std::string& text)
{
    if (!ok) throw std::runtime_error("Traegheitstest: " + text);
}
void nahe(double ist, double soll, double tol, const std::string& text)
{
    pruefe(std::isfinite(ist) && std::abs(ist - soll) <= tol,
        text + ": " + std::to_string(ist) + " statt " + std::to_string(soll));
}

Modell boxmodell()
{
    Modell m;
    m.id = "traegheit-test";
    m.fassung = "testfassung";
    m.masse_kg = 4000.;
    m.traegheit_kg_m2 = {2000., 0., 0., 0., 2000., 0., 0., 0., 2000.};
    Element hull;
    hull.id = "Rumpf";
    hull.masse_kg = m.masse_kg;
    hull.hydrodynamik_rolle = HydrodynamikRolle::rumpf;
    hull.volumen = Mesh{
        {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
        {{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},
         {1,2,6},{1,6,5},{2,3,7},{2,7,6},{3,0,4},{3,4,7}}};
    m.elemente.push_back(std::move(hull));
    return m;
}

void ellipsoid_test()
{
    const double rho = 1000., v = 2.;
    const auto kugel = ellipsoid_zusatzmasse(1., 1., 1., v, rho);
    for (int i = 0; i < 3; ++i) nahe(kugel[i], .5 * rho * v, 1e-3 * rho * v, "Kugel Translation");
    for (int i = 3; i < 6; ++i) nahe(kugel[i], 0., 1e-9, "Kugel Rotation");
    // gestrecktes Rotationsellipsoid a/b = 2: k1 = 0,209, k2 = 0,702 (Lamb)
    const auto sph = ellipsoid_zusatzmasse(2., 1., 1., v, rho);
    nahe(sph[0] / (rho * v), .209, 3e-3, "k1 Spheroid");
    nahe(sph[1] / (rho * v), .702, 3e-3, "k2 Spheroid");
    nahe(sph[1], sph[2], 1e-6 * rho * v, "Axialsymmetrie");
    nahe(sph[3], 0., 1e-9, "Rotation um Symmetrieachse");
    pruefe(sph[4] > 0. && std::abs(sph[4] - sph[5]) < 1e-6 * rho * v, "Querrotation positiv und symmetrisch");
    const auto ell = ellipsoid_zusatzmasse(3., 2., 1., v, rho);
    for (double w : ell) pruefe(std::isfinite(w) && w > 0., "dreiachsiges Ellipsoid endlich positiv");
    bool geworfen = false;
    try { ellipsoid_zusatzmasse(0., 1., 1., v, rho); } catch (const std::invalid_argument&) { geworfen = true; }
    pruefe(geworfen, "ungueltige Halbachse");
}

Zustand startzustand(const Modell& m, const HydrodynamikKonfiguration& cfg, double z)
{
    Zustand s;
    s.pose = Pose{{0., 0., z}, {0., 0., 0., 1.}};
    return hydrodynamik_start(m, s, cfg);
}

HydrodynamischeTraegheitModell nullblock()
{
    HydrodynamischeTraegheitModell b;
    for (const char* k : {"a11", "a22", "a33", "a44", "a55", "a66"}) b.zusatzmasse[k] = 0.;
    for (const char* k : {"zeta_rollen", "zeta_stampfen", "zeta_tauchen", "gieren_linear", "gieren_quadratisch"})
        b.daempfung[k] = 0.;
    return b;
}

void integrator_test()
{
    // A = 0 und keine Zusatzdaempfung muss exakt die Gleichung ohne Zusatzmasse ergeben (Munk-Terme entfallen).
    auto m = boxmodell();
    m.hydrodynamische_traegheit = nullblock();
    HydrodynamikKonfiguration an, aus;
    aus.zusatzmasse_daempfung = false;
    auto a = startzustand(m, an, -.1), b = startzustand(m, aus, -.1);
    for (auto* s : {&a, &b}) {
        s->geschwindigkeit_welt_m_s = {1., .5, 0.};
        s->omega_modell_rad_s = {.3, .2, .1};
    }
    for (int i = 0; i < 200; ++i) {
        a = dynamikschritt(m, a, .005);
        b = dynamikschritt(m, b, .005);
    }
    for (int i = 0; i < 3; ++i) {
        nahe(a.geschwindigkeit_welt_m_s[i], b.geschwindigkeit_welt_m_s[i], 1e-9, "Geschwindigkeit A=0");
        nahe(a.omega_modell_rad_s[i], b.omega_modell_rad_s[i], 1e-9, "Winkelgeschwindigkeit A=0");
        nahe(a.pose.position_m()[i], b.pose.position_m()[i], 1e-9, "Position A=0");
    }

    // Beschleunigung a = F / (m + a33) beim Tauchen aus der Ruhe.
    auto mz = boxmodell();
    auto blk = nullblock();
    HydrodynamikKonfiguration cfg;
    auto s0 = startzustand(mz, cfg, -.1);
    mz.hydrodynamische_traegheit = blk;
    auto s_ohne = dynamikschritt(mz, startzustand(mz, cfg, -.1), 1e-4);
    blk.zusatzmasse["a33"] = 4000.;
    mz.hydrodynamische_traegheit = blk;
    auto s_mit = dynamikschritt(mz, startzustand(mz, cfg, -.1), 1e-4);
    const double v0 = s_ohne.geschwindigkeit_welt_m_s[2], v1 = s_mit.geschwindigkeit_welt_m_s[2];
    pruefe(std::abs(v0) > 1e-8, "Tauchbeschleunigung vorhanden");
    nahe(v1 / v0, .5, 1e-3, "a = F/(m+a33)");
    (void)s0;
}

void daempfung_test()
{
    auto m = boxmodell();
    auto blk = nullblock();
    blk.daempfung["zeta_tauchen"] = .3;
    m.hydrodynamische_traegheit = blk;
    HydrodynamikKonfiguration cfg;
    auto s = startzustand(m, cfg, -.3);
    double amp_frueh = 0., amp_spaet = 0.;
    // Gleichgewichtshoehe aus dem Ruhezustand gegen Ende bestimmen: Amplitude der Geschwindigkeit.
    for (int i = 0; i < 4000; ++i) {
        s = dynamikschritt(m, s, .005);
        const double v = std::abs(s.geschwindigkeit_welt_m_s[2]);
        if (i < 1000) amp_frueh = std::max(amp_frueh, v);
        if (i >= 3000) amp_spaet = std::max(amp_spaet, v);
    }
    pruefe(amp_spaet < .25 * amp_frueh, "Tauchbewegung klingt ab");
    const auto& ht = *s.hydrodynamik->traegheit;
    pruefe(ht.zeta_tauchen.status == "modell" && ht.b_tauchen_ns_m > 0., "Herkunft modell, B > 0");
    pruefe(ht.zeta_rollen.status == "modell", "Nullueberschreibung gilt");
    const auto k = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get(),
        s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    int gefunden = 0;
    for (const auto& b : k.beitraege) if (b.id.rfind("daempfung:", 0) == 0) ++gefunden;
    pruefe(gefunden == 4, "vier Daempfungsbeitraege");

    const auto json = zustand_json(m, s, "ok");
    for (const char* teil : {"\"hydrodynamische_traegheit\":{\"zusatzmasse\":{\"a11\":{\"wert\":",
            "\"effektive_masse_kg\":", "\"rueckstellung\":", "\"vertragsversion\":", "\"summen\":",
            "\"hydrodynamik\":{\"rho_kg_m3\":1000"})
        pruefe(json.find(teil) != std::string::npos, std::string("Ergebnisfeld fehlt: ") + teil);
    HydrodynamikKonfiguration aus;
    aus.zusatzmasse_daempfung = false;
    const auto json_aus = zustand_json(m, startzustand(m, aus, -.3), "ok");
    pruefe(json_aus.find("hydrodynamische_traegheit") == std::string::npos, "ohne Block bei deaktiviert");
    pruefe(json_aus.find("\"hydrodynamik\":{\"rho_kg_m3\":1000") != std::string::npos, "Altfelder bleiben");
}

void referenz_test(const std::filesystem::path& datei)
{
    const auto m = lade_modell(datei);
    HydrodynamikKonfiguration cfg;
    Zustand start;
    double zmin = 1e9, zmax = -1e9;
    for (const auto& e : m.elemente)
        if (e.hydrodynamik_rolle == HydrodynamikRolle::rumpf && e.volumen)
            for (const auto& p : e.volumen->punkte) { zmin = std::min(zmin, p[2]); zmax = std::max(zmax, p[2]); }
    if (zmin > zmax) return;
    start.pose = Pose{{0., 0., -(zmin + zmax) / 2.}, {0., 0., 0., 1.}};
    const auto gg = gleichgewicht(m, start);
    const auto s = hydrodynamik_start(m, gg.zustand, cfg);
    const auto& ht = *s.hydrodynamik->traegheit;
    for (const auto& w : ht.zusatzmasse)
        pruefe(std::isfinite(w.wert) && w.wert >= 0. && w.status == "geschaetzt", "Zusatzmasse endlich: " + m.id);
    pruefe(ht.effektive_masse_kg[0] > 0. && ht.effektive_traegheit_kg_m2[2] > 0., "effektive Werte positiv: " + m.id);
    auto stepped = s;
    for (int i = 0; i < 20; ++i) stepped = dynamikschritt(m, stepped, .005);
    pruefe(std::isfinite(stepped.geschwindigkeit_welt_m_s[2]), "Referenzlauf endlich: " + m.id);
}

void modellblock_test(const std::filesystem::path& quader)
{
    namespace fs = std::filesystem;
    const auto ziel = fs::temp_directory_path() / "segelphysik_traegheit_test";
    fs::remove_all(ziel);
    fs::copy(quader.parent_path(), ziel, fs::copy_options::recursive);
    const auto datei = ziel / quader.filename();
    std::string original;
    { std::ifstream in(datei, std::ios::binary); original.assign(std::istreambuf_iterator<char>(in), {}); }
    auto schreibe = [&](const std::string& block) {
        const auto pos = original.find('{');
        std::ofstream out(datei, std::ios::binary | std::ios::trunc);
        out << original.substr(0, pos + 1) << block << original.substr(pos + 1);
    };
    auto wirft = [&](const std::string& block) {
        schreibe(block);
        try { lade_modell(datei); } catch (const std::exception& e) {
            return std::string(e.what()).find("CORE-MOD-0002") != std::string::npos;
        }
        return false;
    };
    schreibe("\"Hydrodynamische_Traegheit\":{\"zusatzmasse\":{\"a11\":12.5},"
             "\"daempfung\":{\"zeta_rollen\":0.2}},");
    const auto m = lade_modell(datei);
    pruefe(m.hydrodynamische_traegheit && m.hydrodynamische_traegheit->zusatzmasse.at("a11") == 12.5 &&
        !m.hydrodynamische_traegheit->zusatzmasse.count("a22"), "partieller Block gelesen");
    HydrodynamikKonfiguration cfg;
    Zustand z;
    z.pose = Pose{{0., 0., 0.}, {0., 0., 0., 1.}};
    const auto s = hydrodynamik_start(m, z, cfg);
    pruefe(s.hydrodynamik->traegheit->zusatzmasse[0].status == "modell" &&
        s.hydrodynamik->traegheit->zusatzmasse[0].wert == 12.5 &&
        s.hydrodynamik->traegheit->zusatzmasse[1].status == "geschaetzt" &&
        s.hydrodynamik->traegheit->zeta_rollen.wert == .2, "Ueberschreiben und Herkunft");
    pruefe(wirft("\"Hydrodynamische_Traegheit\":{\"zusatzmasse\":{\"a11\":-1}},"), "negativer Wert");
    pruefe(wirft("\"Hydrodynamische_Traegheit\":{\"zusatzmasse\":{\"a77\":1}},"), "unbekannter Schluessel");
    pruefe(wirft("\"Hydrodynamische_Traegheit\":{\"x\":{}},"), "unbekannte Gruppe");
    pruefe(wirft("\"Hydrodynamische_Traegheit\":{\"daempfung\":{\"zeta_rollen\":\"a\"}},"), "Text statt Zahl");
    fs::remove_all(ziel);
    pruefe(!lade_modell(quader).hydrodynamische_traegheit, "Altmodell ohne Block");
}
}

int main(int argc, char** argv)
{
    try {
        ellipsoid_test();
        integrator_test();
        daempfung_test();
        for (int i = 1; i < argc; ++i) {
            const std::filesystem::path p = std::filesystem::u8path(argv[i]);
            if (p.parent_path().filename() == "quader") modellblock_test(p);
            referenz_test(p);
        }
        std::cout << "Traegheitstests bestanden\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
