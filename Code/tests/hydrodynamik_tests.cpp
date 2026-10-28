#include "segelphysik/simulation.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
void near(double actual, double expected, double tolerance = 1e-9)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance)
        throw std::runtime_error("Hydrodynamikabweichung: " +
            std::to_string(actual) + " statt " + std::to_string(expected));
}

segelphysik::Modell boxmodell()
{
    using namespace segelphysik;
    Modell m;
    m.id = "hydrodynamik-test";
    m.fassung = "testfassung";
    m.masse_kg = 4000.;
    m.traegheit_kg_m2 = {2000.,0.,0.,0.,2000.,0.,0.,0.,2000.};
    Element hull;
    hull.id = "Rumpf";
    hull.masse_kg = m.masse_kg;
    hull.hydrodynamik_rolle = HydrodynamikRolle::rumpf;
    hull.volumen = Mesh{
        {{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
         {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
        {{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},
         {1,2,6},{1,6,5},{2,3,7},{2,7,6},{3,0,4},{3,4,7}}};
    m.elemente.push_back(std::move(hull));
    return m;
}

segelphysik::HydrodynamikKonfiguration config()
{
    segelphysik::HydrodynamikKonfiguration result;
    result.rho_kg_m3 = 1000.;
    result.nu_m2_s = 1e-6;
    return result;
}

const segelphysik::Beitrag& rumpfbeitrag(const segelphysik::Kraefte& forces)
{
    for (const auto& contribution : forces.beitraege)
        if (contribution.id == "Rumpf:hydrodynamik:reibung") return contribution;
    throw std::runtime_error("Rumpf-Reibungsbeitrag fehlt");
}

void pruefen()
{
    using namespace segelphysik;
    const auto m = boxmodell();
    const Pose pose{{}, {0.,0.,0.,1.}};
    Zustand start;
    start.geschwindigkeit_welt_m_s = {1.,0.,0.};
    const auto state = hydrodynamik_start(m, start, config());
    const auto forces = kraefte(m, pose, nullptr, 1000., 9.81, {},
        state.hydrodynamik.get(), state.geschwindigkeit_welt_m_s);
    const double cf = .075 / std::pow(std::log10(2e6) - 2., 2.);
    const double expected = .5 * 1000. * 12. * cf;
    const auto& friction = rumpfbeitrag(forces);
    near(friction.vektor[0], -expected);
    near(friction.vektor[1], 0.);
    near(friction.vektor[2], 0.);
    near(forces.kraft_welt_n[0], -expected);
    near(friction.moment_s_nm[1], friction.punkt_m[2] * friction.vektor[0]);
    if (friction.zustand != Datenzustand::gueltig ||
        friction.quelle != Quelle::hydrodynamik ||
        friction.gruppe != "hydrodynamik")
        throw std::runtime_error("Beitragsmetadaten falsch");
    auto balanced_contributions = forces.beitraege;
    const auto balance = bilanz(m, balanced_contributions, true);
    if (balance.status != "vollstaendig")
        throw std::runtime_error("Hydrodynamikbilanz unvollstaendig");
    near(balance.gesamt.kraft_modell_n[0], forces.kraft_welt_n[0]);
    near(balance.gesamt.moment_s_nm[1], forces.moment_modell_nm[1]);

    Zustand stillstand;
    const auto stiller_start = hydrodynamik_start(m, stillstand, config());
    const auto nullkraft = kraefte(m, stillstand.pose, nullptr, 1000., 9.81, {},
        stiller_start.hydrodynamik.get(), stillstand.geschwindigkeit_welt_m_s);
    if (rumpfbeitrag(nullkraft).zustand != Datenzustand::null)
        throw std::runtime_error("Nullfluss ist kein gueltiger Nullbeitrag");

    auto current_config = config();
    current_config.fluidgeschwindigkeit_welt_m_s = {1.,0.,0.};
    const auto with_current = hydrodynamik_start(m, stillstand, current_config);
    const auto current_forces = kraefte(m, stillstand.pose, nullptr, 1000., 9.81, {},
        with_current.hydrodynamik.get(), stillstand.geschwindigkeit_welt_m_s);
    if (rumpfbeitrag(current_forces).vektor[0] <= 0.)
        throw std::runtime_error("Kraftrichtung bei Fluidstroemung falsch");

    Pose yaw90{{}, {0.,0.,std::sqrt(.5),std::sqrt(.5)}};
    Zustand sideways = start;
    sideways.geschwindigkeit_welt_m_s = {0.,1.,0.};
    const auto rotated = kraefte(m, yaw90, nullptr, 1000., 9.81, {},
        state.hydrodynamik.get(), sideways.geschwindigkeit_welt_m_s);
    near(rumpfbeitrag(rotated).vektor[0], -expected);

    auto outside_low = start;
    outside_low.geschwindigkeit_welt_m_s = {.49,0.,0.};
    const auto low_context = hydrodynamik_start(m, outside_low, config());
    const auto low_forces = kraefte(m, outside_low.pose, nullptr, 1000., 9.81, {},
        low_context.hydrodynamik.get(), outside_low.geschwindigkeit_welt_m_s);
    const auto& low_contribution = rumpfbeitrag(low_forces);
    if (low_contribution.zustand != Datenzustand::nicht_berechnet ||
        low_contribution.fehler_id != "CORE-HYD-0001")
        throw std::runtime_error("Re unter 1e6 nicht als unberechnet diagnostiziert");
    auto low_balance_contributions = low_forces.beitraege;
    if (bilanz(m, low_balance_contributions, true).status != "teilsumme")
        throw std::runtime_error("Unberechnete Niedrig-Re-Kraft wurde als Vollbilanz ausgegeben");
    outside_low.geschwindigkeit_welt_m_s = {.5,0.,0.};
    hydrodynamik_start(m, outside_low, config());
    auto outside_high = start;
    outside_high.geschwindigkeit_welt_m_s = {500.1,0.,0.};
    const auto high_context = hydrodynamik_start(m, outside_high, config());
    const auto high_forces = kraefte(m, outside_high.pose, nullptr, 1000., 9.81, {},
        high_context.hydrodynamik.get(), outside_high.geschwindigkeit_welt_m_s);
    const auto& high_contribution = rumpfbeitrag(high_forces);
    if (high_contribution.zustand != Datenzustand::nicht_berechnet ||
        high_contribution.fehler_id != "CORE-HYD-0001")
        throw std::runtime_error("Re ueber 1e9 nicht als unberechnet diagnostiziert");
    outside_high.geschwindigkeit_welt_m_s = {500.,0.,0.};
    hydrodynamik_start(m, outside_high, config());

    auto bad_config = config();
    bad_config.nu_m2_s = 0.;
    bool viscosity_rejected = false;
    try {
        hydrodynamik_start(m, start, bad_config);
    } catch (const std::invalid_argument&) {
        viscosity_rejected = true;
    }
    if (!viscosity_rejected) throw std::runtime_error("Nullviskositaet wurde akzeptiert");
    bad_config = config();
    bad_config.rho_kg_m3 = -1.;
    bool density_rejected = false;
    try {
        hydrodynamik_start(m, start, bad_config);
    } catch (const std::invalid_argument&) {
        density_rejected = true;
    }
    if (!density_rejected) throw std::runtime_error("Negative Fluiddichte wurde akzeptiert");
    bad_config = config();
    bad_config.fluidgeschwindigkeit_welt_m_s[1] = std::numeric_limits<double>::quiet_NaN();
    bool current_rejected = false;
    try {
        hydrodynamik_start(m, start, bad_config);
    } catch (const std::invalid_argument& error) {
        current_rejected = std::string(error.what()).find("Fluidgeschwindigkeit") != std::string::npos;
    }
    if (!current_rejected) throw std::runtime_error("Nicht endliche Fluidgeschwindigkeit ohne Diagnose");

    const auto moved = dynamikschritt(m, state, .01);
    if (!(moved.geschwindigkeit_welt_m_s[0] < start.geschwindigkeit_welt_m_s[0]) ||
        !(moved.pose.position_m()[0] > start.pose.position_m()[0]))
        throw std::runtime_error("Hydrodynamik wirkt nicht auf den Integrator");
    const auto json = zustand_json(m, moved, "laufend");
    if (json.find("\"vertragsversion\":\"2\"") == std::string::npos ||
        json.find("\"quelle\":\"hydrodynamik\"") == std::string::npos ||
        json.find("\"hydrodynamik\":{\"rho_kg_m3\":1000") == std::string::npos ||
        json.find("\"bilanzstatus\":\"vollstaendig\"") == std::string::npos)
        throw std::runtime_error("Hydrodynamik-Ergebnisvertrag unvollstaendig");

    auto lost_context = moved;
    lost_context.hydrodynamik.reset();
    bool context_rejected = false;
    try {
        dynamikschritt(m, lost_context, .01);
    } catch (const std::invalid_argument&) {
        context_rejected = true;
    }
    if (!context_rejected) throw std::runtime_error("Verlorener Hydrokontext nicht erkannt");
}

void referenzboot(const std::filesystem::path& path)
{
    using namespace segelphysik;
    const auto model = lade_modell(path);
    const auto hull = std::find_if(model.elemente.begin(), model.elemente.end(),
        [](const Element& element) {
            return element.hydrodynamik_rolle == HydrodynamikRolle::rumpf;
        });
    if (hull == model.elemente.end() || !hull->volumen)
        throw std::runtime_error("Referenzmodell ohne physisches Rumpfmesh: " + path.u8string());
    double min_z = hull->volumen->punkte.front()[2];
    double max_z = min_z;
    for (const auto& point : hull->volumen->punkte) {
        min_z = std::min(min_z, point[2]);
        max_z = std::max(max_z, point[2]);
    }
    Zustand start;
    start.pose = Pose{{0.,0.,-(min_z + max_z) / 2.}, {0.,0.,0.,1.}};
    start.geschwindigkeit_welt_m_s = {1.,0.,0.};
    const auto active = hydrodynamik_start(model, start, config());
    const auto forces = kraefte(model, start.pose, nullptr, 1000., 9.81, {},
        active.hydrodynamik.get(), start.geschwindigkeit_welt_m_s);
    const auto& friction = rumpfbeitrag(forces);
    if (friction.zustand != Datenzustand::gueltig ||
        !std::isfinite(friction.vektor[0]) || friction.vektor[0] >= 0.)
        throw std::runtime_error("Referenzmodell-Rumpfreibung ungueltig: " + model.id);

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::temp_directory_path() /
        ("segelphysik-hydrodynamik-" + model.id + "-" + std::to_string(nonce) + ".jsonl");
    if (std::filesystem::exists(output))
        throw std::runtime_error("Hydrodynamik-Testausgabe existiert bereits: " + output.u8string());
    struct Ausgabedatei {
        std::filesystem::path path;
        ~Ausgabedatei()
        {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } cleanup{output};
    Laufoptionen options;
    options.dynamisch = true;
    options.dauer_s = .01;
    options.dt_s = .01;
    options.start = start;
    const auto report = simulieren(path, output, options);
    if (report.status != "abgeschlossen")
        throw std::runtime_error("Dynamischer Hydrodynamiklauf fehlgeschlagen: " +
            model.id + ": " + report.diagnose);
    std::ifstream input(output, std::ios::binary);
    if (!input) throw std::runtime_error("Hydrodynamik-Ergebnis nicht lesbar: " + output.u8string());
    const std::string json{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (json.find("\"quelle\":\"hydrodynamik\"") == std::string::npos ||
        json.find("\"hydrodynamik\":{\"rho_kg_m3\":1000") == std::string::npos)
        throw std::runtime_error("Automatische Hydrodynamik fehlt im dynamischen Core-Lauf: " + model.id);
}
}

int main(int argc, char** argv)
{
    try {
        pruefen();
        for (int i = 1; i < argc; ++i) referenzboot(std::filesystem::u8path(argv[i]));
        std::cout << "Hydrodynamiktests bestanden\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
