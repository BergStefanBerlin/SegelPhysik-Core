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

void bewegung_test()
{
    using namespace segelphysik;
    auto modell = boxmodell();
    Element ruder;
    ruder.id = "Ruder";
    ruder.bewegung = Bewegungsdefinition{};
    ruder.bewegung->art = Bewegungsart::drehen;
    ruder.bewegung->ruderwinkel = true;
    ruder.bewegung->achse_modell = {0., 0., 1.};
    ruder.hydrodynamik_rolle = HydrodynamikRolle::ruder;
    modell.elemente.push_back(ruder);
    auto matrix = bewegungstransformation(modell, "Ruder", {{"Ruder", 90.}});
    near(matrix[0], 0.);
    near(matrix[4], 1.);
    near(matrix[1], -1.);
    near(matrix[5], 0.);
    const Vektor3 rueckwaerts{-1., 0., 0.};
    near(matrix[0]*rueckwaerts[0] + matrix[1]*rueckwaerts[1], 0.);
    near(matrix[4]*rueckwaerts[0] + matrix[5]*rueckwaerts[1], -1.);

    Element schwert;
    schwert.id = "Schwert";
    schwert.bewegung = Bewegungsdefinition{};
    schwert.bewegung->art = Bewegungsart::drehen;
    schwert.bewegung->achse_modell = {0., 1., 0.};
    modell.elemente.push_back(schwert);
    matrix = bewegungstransformation(modell, "Schwert", {{"Schwert", 0.}});
    near(matrix[2], 1.);
    near(matrix[10], 0.);

    Element hubschwert;
    hubschwert.id = "Hubschwert";
    hubschwert.bewegung = Bewegungsdefinition{};
    hubschwert.bewegung->art = Bewegungsart::heben;
    hubschwert.bewegung->richtung_modell = {0., 0., 1.};
    hubschwert.bewegung->hubweg_m = .523;
    modell.elemente.push_back(hubschwert);
    matrix = bewegungstransformation(modell, "Hubschwert", {{"Hubschwert", 0.}});
    near(matrix[3], 0.);
    near(matrix[7], 0.);
    near(matrix[11], .523);

    Zustand start;
    const auto masse_start = modell.masse_kg;
    const auto schwerpunkt_start = modell.schwerpunkt_m;
    const auto traegheit_start = modell.traegheit_kg_m2;
    const auto hydrostatik_startwerte = hydrostatik(modell, start.pose);
    const auto next = dynamikschritt(modell, start, .01,
        std::vector<TestantriebEreignis>{},
        std::vector<Bewegungsereignis>{{1, "Ruder", 45.}}, 1000., 9.81);
    if (next.bewegungsstellungen.size() != 3 ||
        next.bewegungsstellungen.front().wert != 45. ||
        next.letzte_bewegungsereigniskennung != 1)
        throw std::runtime_error("Core uebernahm Stellereignis nicht im Zustand");
    if (modell.masse_kg != masse_start || modell.schwerpunkt_m != schwerpunkt_start ||
        modell.traegheit_kg_m2 != traegheit_start ||
        hydrostatik(modell, start.pose).volumen_m3 != hydrostatik_startwerte.volumen_m3)
        throw std::runtime_error("Ruder-/Schwertbewegung veraenderte Masse oder Hydrostatik");
    const auto output = zustand_json(modell, next, "laufend", {}, nullptr, nullptr,
        ErgebnisJsonFormat::Kompatibel);
    if (output.find("\"vertragsversion\":\"4\"") == std::string::npos ||
        output.find("\"elementtransformationen_modell\"") == std::string::npos ||
        output.find("\"bewegungsuebergang\"") == std::string::npos ||
        output.find("\"wert\":45") == std::string::npos)
        throw std::runtime_error("Bewegungszustand fehlt im Core-Ergebnis");
    bool abgelehnt = false;
    try {
        dynamikschritt(modell, next, .01, std::vector<TestantriebEreignis>{},
            std::vector<Bewegungsereignis>{{2, "Ruder", 91.}}, 1000., 9.81);
    } catch (const std::invalid_argument&) {
        abgelehnt = true;
    }
    if (!abgelehnt) throw std::runtime_error("Ungueltiger Ruderwinkel wurde angenommen");
}

const segelphysik::Beitrag& rumpfbeitrag(const segelphysik::Kraefte& forces)
{
    for (const auto& contribution : forces.beitraege)
        if (contribution.id == "Rumpf:hydrodynamik:reibung") return contribution;
    throw std::runtime_error("Rumpf-Reibungsbeitrag fehlt");
}

const segelphysik::Beitrag& hydrobeitrag(const segelphysik::Kraefte& forces,
    const std::string& element, const std::string& art)
{
    const auto id = element + ":hydrodynamik:" + art;
    for (const auto& contribution : forces.beitraege)
        if (contribution.id == id) return contribution;
    throw std::runtime_error("Hydrodynamikbeitrag fehlt: " + id);
}

segelphysik::Modell schwertmodell()
{
    auto model = boxmodell();
    auto& foil = model.elemente.front();
    foil.id = "Schwert";
    foil.hydrodynamik_rolle = segelphysik::HydrodynamikRolle::kiel;
    foil.volumen->punkte = {
        {-.5,-.01,-1.},{.5,-.01,-1.},{.5,.01,-1.},{-.5,.01,-1.},
        {-.5,-.01,1.},{.5,-.01,1.},{.5,.01,1.},{-.5,.01,1.}};
    return model;
}

void analytische_seitenkraft_panelbilanz()
{
    using namespace segelphysik;
    for (const auto rolle : {HydrodynamikRolle::kiel, HydrodynamikRolle::ruder}) {
        for (const bool kennfeld : {false, true}) {
            auto model = schwertmodell();
            auto& foil = model.elemente.front();
            foil.hydrodynamik_rolle = rolle;
            for (auto& point : foil.volumen->punkte) point[2] += .25;
            if (kennfeld) {
                HydrodynamikKraftmodell polar;
                polar.methode = "polare_2d";
                polar.quelle = "Analytische konstante Testpolare";
                polar.gueltigkeit = "Test";
                polar.widerstandsanteile = {"gesamt"};
                for (double alpha : {0., std::acos(-1.) / 2.})
                    for (double reynolds : {1e5, 1e7})
                        polar.kennfeld.push_back({alpha, reynolds, .4, .1,
                            "geschaetzt", "Test", "Test", "hoch"});
                foil.hydrodynamik_daten.emplace();
                foil.hydrodynamik_daten->modelle.emplace("seitenkraft", polar);
            }
            for (const double lateral_speed : {0., .5}) {
                Zustand state;
                state.pose = Pose{{0., 0., -3.}, {0., 0., 0., 1.}};
                state.geschwindigkeit_welt_m_s = {0., lateral_speed, 0.};
                state.omega_modell_rad_s = {0., 0., 6.};
                state = hydrodynamik_start(model, state, config());
                const auto forces = kraefte(model, state.pose, nullptr, 1000.,
                    9.81, {}, state.hydrodynamik.get(),
                    state.geschwindigkeit_welt_m_s, state.omega_modell_rad_s);
                const auto& contribution = hydrobeitrag(forces, "Schwert", "seitenkraft");
                const auto& actual = contribution.lateraldruckpunkt_panelbilanz;

                // Four lateral triangles, each area 1, projected area 1/2:
                // x=+/-1/6, y=+/-0.01, z=0.25+/-1/3.
                // At each x both faces have the same Fy; paired z offsets cancel.
                // Relative chord speed magnitude is 6*0.01=0.06 and normal
                // speed n=-(v_y+6*x). AR=2^2/2=2, cn=1.1+0.018*2.
                // Estimated method is fully stalled (alpha>1.45 rad):
                // Cl=cn*sin(alpha)*cos(alpha), Cd=.02+cn*sin(alpha)^2.
                const auto panel_fy = [kennfeld](double n) {
                    constexpr double chord = .06;
                    constexpr double cn = 1.136;
                    const double speed2 = chord * chord + n * n;
                    const double speed = std::sqrt(speed2);
                    if (kennfeld)
                        return 250. * (.4 * speed2 * (n < 0. ? -1. : 1.)
                            + .1 * speed * n);
                    return 250. * n * (cn * chord +
                        (.02 * speed2 + cn * n * n) / speed);
                };
                const double positive_x_fy = panel_fy(-lateral_speed - 1.);
                const double negative_x_fy = panel_fy(-lateral_speed + 1.);
                const double q = 2. * (positive_x_fy + negative_x_fy);
                const double b = 2. * (std::abs(positive_x_fy) + std::abs(negative_x_fy));
                const double x = (positive_x_fy - negative_x_fy) / 3.;
                if (!actual.vorhanden || contribution.modell_methode !=
                    (kennfeld ? "polare_2d" : "geschaetzte_endliche_streckungspolare"))
                    throw std::runtime_error("Analytische Referenz verwendet falschen Seitenkraftpfad");
                near(actual.querkraft_modell_n, q, 1e-8);
                near(actual.panelbetragssumme_n, b, 1e-8);
                near(actual.erstes_moment_x_nm, x, 1e-8);
                near(actual.erstes_moment_z_nm, .25 * q, 1e-8);
                near(contribution.vektor[1], q, 1e-8);
                if (b <= std::abs(q) ||
                    std::abs(x - q * contribution.punkt_m[0]) < 1.)
                    throw std::runtime_error("Referenz unterscheidet Betragsschwerpunktfehler nicht");
            }
        }
    }
}

void ruderseitenkraft_test()
{
    using namespace segelphysik;
    auto model = schwertmodell();
    auto& rudder = model.elemente.front();
    rudder.id = "Ruder";
    rudder.hydrodynamik_rolle = HydrodynamikRolle::ruder;
    rudder.bewegung = Bewegungsdefinition{};
    rudder.bewegung->art = Bewegungsart::drehen;
    rudder.bewegung->ruderwinkel = true;
    rudder.bewegung->achse_modell = {0., 0., 1.};
    rudder.bewegung->drehpunkt_modell_m = {-2., 0., 0.};
    rudder.volumen->punkte = {
        {-2.5,-.01,-1.},{-1.5,-.01,-1.},{-1.5,.01,-1.},{-2.5,.01,-1.},
        {-2.5,-.01,1.},{-1.5,-.01,1.},{-1.5,.01,1.},{-2.5,.01,1.}};
    Zustand state;
    state.geschwindigkeit_welt_m_s = {2., 0., 0.};
    const auto active = hydrodynamik_start(model, state, config());
    const auto rudder_force = [&](double angle) {
        const auto forces = kraefte(model, state.pose, nullptr, 1000., 9.81, {},
            active.hydrodynamik.get(), state.geschwindigkeit_welt_m_s, {},
            {{ "Ruder", angle }});
        return hydrobeitrag(forces, "Ruder", "seitenkraft");
    };
    const auto port = rudder_force(-20.);
    const auto neutral = rudder_force(0.);
    const auto starboard = rudder_force(20.);
    if (!(port.vektor[1] < 0.) || !(starboard.vektor[1] > 0.) ||
        !(port.moment_s_nm[2] > 0.) || !(starboard.moment_s_nm[2] < 0.))
        throw std::runtime_error(
            "Ruderausschlag erzeugt keine vorzeichenrichtige Seitenkraft und Gierwirkung");
    near(neutral.vektor[1], 0., 1e-8);
    near(neutral.moment_s_nm[2], 0., 1e-8);
}

void pruefen()
{
    bewegung_test();
    ruderseitenkraft_test();
    using namespace segelphysik;
    const auto m = boxmodell();
    const Pose pose{{}, {0.,0.,0.,1.}};
    Zustand start;
    start.geschwindigkeit_welt_m_s = {1.,0.,0.};
    const auto state = hydrodynamik_start(m, start, config());
    const auto forces = kraefte(m, pose, nullptr, 1000., 9.81, {},
        state.hydrodynamik.get(), state.geschwindigkeit_welt_m_s);
    const auto& friction = rumpfbeitrag(forces);
    if (friction.vektor[0] >= 0. || !std::isfinite(friction.vektor[0]))
        throw std::runtime_error("Rumpfhautreibung wirkt nicht entgegen der X-Anstroemung");
    near(friction.vektor[1], 0.);
    near(friction.vektor[2], 0.);
    const auto& pressure = hydrobeitrag(forces, "Rumpf", "druck");
    if (pressure.vektor[0] >= 0. || pressure.modell_status != "geschaetzt")
        throw std::runtime_error("Rumpf-Formwiderstand oder Schaetzstatus fehlt");
    near(forces.kraft_welt_n[0], friction.vektor[0] + pressure.vektor[0]);
    near(friction.moment_s_nm[1], friction.punkt_m[2] * friction.vektor[0]);
    if (friction.zustand != Datenzustand::gueltig ||
        friction.quelle != Quelle::hydrodynamik ||
        friction.gruppe != "hydrodynamik" ||
        friction.modell_status != "geschaetzt" ||
        friction.modell_methode != "ittc_1957_mit_laminarer_naeherung")
        throw std::runtime_error("Beitragsmetadaten falsch");
    Zustand submerged = start;
    submerged.pose = Pose{{0.,0.,-2.}, {0.,0.,0.,1.}};
    const auto submerged_context = hydrodynamik_start(m, submerged, config());
    const auto submerged_forces = kraefte(m, submerged.pose, nullptr, 1000., 9.81, {},
        submerged_context.hydrodynamik.get(), submerged.geschwindigkeit_welt_m_s);
    const double submerged_cf = .075 /
        std::pow(std::log10(std::sqrt(2.) / config().nu_m2_s) - 2., 2.);
    near(rumpfbeitrag(submerged_forces).vektor[0],
        -.5 * config().rho_kg_m3 * 16. * submerged_cf);
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
    if (rumpfbeitrag(rotated).vektor[0] >= 0.)
        throw std::runtime_error("Frame-Transformation der Reibung ist falsch");

    for (const auto& axis : {Vektor3{0.,1.,0.}, Vektor3{0.,0.,1.}}) {
        auto moving = start;
        moving.geschwindigkeit_welt_m_s = axis;
        const auto axis_context = hydrodynamik_start(m, moving, config());
        const auto axis_forces = kraefte(m, moving.pose, nullptr, 1000., 9.81, {},
            axis_context.hydrodynamik.get(), moving.geschwindigkeit_welt_m_s);
        const auto& axis_friction = rumpfbeitrag(axis_forces);
        const std::size_t component = axis[1] != 0. ? 1 : 2;
        if (axis_friction.vektor[component] >= 0.)
            throw std::runtime_error("Hautreibung wirkt nicht entgegen der Quer-/Vertikalbewegung");
        for (std::size_t i = 0; i < 3; ++i)
            if (i != component) near(axis_friction.vektor[i], 0.);
    }

    auto rotating = start;
    rotating.geschwindigkeit_welt_m_s = {};
    rotating.omega_modell_rad_s = {0.,0.,1.};
    const auto rotating_context = hydrodynamik_start(m, rotating, config());
    const auto rotating_forces = kraefte(m, rotating.pose, nullptr, 1000., 9.81, {},
        rotating_context.hydrodynamik.get(), rotating.geschwindigkeit_welt_m_s,
        rotating.omega_modell_rad_s);
    if (rumpfbeitrag(rotating_forces).moment_s_nm[2] >= 0.)
        throw std::runtime_error("Lokale Rotationsanstroemung erzeugt kein Gegenmoment");

    const auto foil = schwertmodell();
    Zustand crossflow;
    crossflow.pose = Pose{{0.,0.,-2.}, {0.,0.,0.,1.}};
    crossflow.geschwindigkeit_welt_m_s = {0.,1.,0.};
    const auto foil_context = hydrodynamik_start(foil, crossflow, config());
    const auto foil_forces = kraefte(foil, crossflow.pose, nullptr, 1000., 9.81, {},
        foil_context.hydrodynamik.get(), crossflow.geschwindigkeit_welt_m_s);
    const auto& foil_lift = hydrobeitrag(foil_forces, "Schwert", "seitenkraft");
    if (foil_lift.zustand != Datenzustand::gueltig || foil_lift.vektor[1] >= 0. ||
        !foil_lift.lateraldruckpunkt_panelbilanz.vorhanden ||
        std::abs(foil_lift.lateraldruckpunkt_panelbilanz.querkraft_modell_n -
            foil_lift.vektor[1]) > 1e-8 ||
        foil_lift.modell_status != "geschaetzt")
        throw std::runtime_error("Schwert-Seitenkraft hemmt die Abdrift nicht, verliert Paneldaten oder ist nicht als Schaetzung markiert");
    auto bewegtes_foil = foil;
    bewegtes_foil.elemente.front().bewegung = Bewegungsdefinition{};
    bewegtes_foil.elemente.front().bewegung->art = Bewegungsart::drehen;
    bewegtes_foil.elemente.front().bewegung->achse_modell = {0., 1., 0.};
    const auto ausgefahren = kraefte(bewegtes_foil, crossflow.pose, nullptr,
        1000., 9.81, {}, foil_context.hydrodynamik.get(),
        crossflow.geschwindigkeit_welt_m_s, {}, {{"Schwert", 100.}});
    const auto eingeklappt = kraefte(bewegtes_foil, crossflow.pose, nullptr,
        1000., 9.81, {}, foil_context.hydrodynamik.get(),
        crossflow.geschwindigkeit_welt_m_s, {}, {{"Schwert", 0.}});
    if (hydrobeitrag(ausgefahren, "Schwert", "seitenkraft").punkt_m ==
        hydrobeitrag(eingeklappt, "Schwert", "seitenkraft").punkt_m)
        throw std::runtime_error("Stellungsänderung hat die bewegte Hydrodynamikgeometrie nicht verändert");
    if (!hydrobeitrag(ausgefahren, "Schwert", "seitenkraft").lateraldruckpunkt_panelbilanz.vorhanden ||
        !hydrobeitrag(eingeklappt, "Schwert", "seitenkraft").lateraldruckpunkt_panelbilanz.vorhanden)
        throw std::runtime_error("Bewegte Seitenkraft verliert die Panelbilanz");
    const auto& extended_panels =
        hydrobeitrag(ausgefahren, "Schwert", "seitenkraft").lateraldruckpunkt_panelbilanz;
    const auto& retracted_panels =
        hydrobeitrag(eingeklappt, "Schwert", "seitenkraft").lateraldruckpunkt_panelbilanz;
    if (extended_panels.erstes_moment_x_nm == retracted_panels.erstes_moment_x_nm &&
        extended_panels.erstes_moment_z_nm == retracted_panels.erstes_moment_z_nm)
        throw std::runtime_error("Bewegte Seitenkraft aktualisiert ihre Panelmomente nicht");

    auto polar_foil = foil;
    HydrodynamikKraftmodell polar;
    polar.methode = "polare_2d";
    polar.gueltigkeit = "Testdomäne";
    polar.quelle = "Annahme: synthetisches Testkennfeld";
    polar.widerstandsanteile = {"profil"};
    for (double alpha : {.2, std::acos(-1.) / 2.}) {
        for (double reynolds : {1e6, 4e6}) {
            polar.kennfeld.push_back({alpha, reynolds, .4, .1, "geschaetzt",
                "Annahme: synthetisches Testkennfeld", "Testdomäne", "hoch"});
        }
    }
    polar_foil.elemente.front().hydrodynamik_daten.emplace();
    polar_foil.elemente.front().hydrodynamik_daten->modelle.emplace("seitenkraft", polar);
    auto diagonal_flow = crossflow;
    diagonal_flow.geschwindigkeit_welt_m_s = {1.,1.,0.};
    const auto polar_context = hydrodynamik_start(polar_foil, diagonal_flow, config());
    const auto polar_forces = kraefte(polar_foil, diagonal_flow.pose, nullptr, 1000., 9.81, {},
        polar_context.hydrodynamik.get(), diagonal_flow.geschwindigkeit_welt_m_s);
    const auto& polar_force = hydrobeitrag(polar_forces, "Schwert", "seitenkraft");
    const auto& included_friction = hydrobeitrag(polar_forces, "Schwert", "reibung");
    if (polar_force.zustand != Datenzustand::gueltig ||
        !polar_force.lateraldruckpunkt_panelbilanz.vorhanden ||
        std::abs(polar_force.lateraldruckpunkt_panelbilanz.querkraft_modell_n -
            polar_force.vektor[1]) > 1e-8 ||
        polar_force.modell_methode != "polare_2d" ||
        polar_force.modell_quelle != polar.quelle ||
        polar_force.modell_unsicherheit != "hoch" ||
        polar_force.modell_widerstandsanteile != polar.widerstandsanteile ||
        included_friction.zustand != Datenzustand::null)
        throw std::runtime_error("Polarenkennfeld oder dessen Provenienz wurde nicht verwendet");

    auto outside_polar = polar_foil;
    outside_polar.elemente.front().hydrodynamik_daten->modelle.at("seitenkraft").kennfeld.erase(
        outside_polar.elemente.front().hydrodynamik_daten->modelle.at("seitenkraft").kennfeld.begin() + 1);
    bool incomplete_polar_rejected = false;
    try {
        hydrodynamik_start(outside_polar, diagonal_flow, config());
    } catch (const std::invalid_argument& error) {
        incomplete_polar_rejected =
            std::string(error.what()).find("Kennfeld ist kein vollstaendiges Rechteck") != std::string::npos;
    }
    if (!incomplete_polar_rejected)
        throw std::runtime_error("Unvollstaendiges Polarenkennfeld wurde nicht diagnostiziert");

    auto out_of_domain = crossflow;
    out_of_domain.geschwindigkeit_welt_m_s = {1.,0.,0.};
    const auto domain_context = hydrodynamik_start(polar_foil, out_of_domain, config());
    const auto domain_forces = kraefte(polar_foil, out_of_domain.pose, nullptr, 1000., 9.81, {},
        domain_context.hydrodynamik.get(), out_of_domain.geschwindigkeit_welt_m_s);
    const auto& domain_force = hydrobeitrag(domain_forces, "Schwert", "seitenkraft");
    if (domain_force.zustand != Datenzustand::nicht_berechnet ||
        domain_force.fehler_id != "CORE-HYD-0001")
        throw std::runtime_error("Kennfeld ausserhalb seiner Domäne nicht diagnostiziert");

    auto sinking = boxmodell();
    sinking.masse_kg = 10000.;
    sinking.elemente.front().masse_kg = sinking.masse_kg;
    Zustand sinking_state;
    sinking_state.pose = Pose{{0.,0.,-2.}, {0.,0.,0.,1.}};
    sinking_state = hydrodynamik_start(sinking, sinking_state, config());
    double speed_after_3s = 0.;
    for (int step = 1; step <= 300; ++step) {
        sinking_state = dynamikschritt(sinking, sinking_state, .02);
        if (step == 150) speed_after_3s = -sinking_state.geschwindigkeit_welt_m_s[2];
    }
    const double sinking_speed = -sinking_state.geschwindigkeit_welt_m_s[2];
    if (!(speed_after_3s > 0.) || !(sinking_speed >= speed_after_3s) ||
        sinking_speed - speed_after_3s > .5)
        throw std::runtime_error("Freies Sinken erreicht keine stabile endliche Geschwindigkeit");

    auto drifting = crossflow;
    drifting = hydrodynamik_start(foil, drifting, config());
    const double initial_drift_speed = std::abs(drifting.geschwindigkeit_welt_m_s[1]);
    for (int step = 0; step < 50; ++step)
        drifting = dynamikschritt(foil, drifting, .02);
    if (!(std::abs(drifting.geschwindigkeit_welt_m_s[1]) < initial_drift_speed))
        throw std::runtime_error("Schwert reduziert die dynamische Abdrift nicht");

    auto outside_low = start;
    outside_low.geschwindigkeit_welt_m_s = {.49,0.,0.};
    const auto low_context = hydrodynamik_start(m, outside_low, config());
    const auto low_forces = kraefte(m, outside_low.pose, nullptr, 1000., 9.81, {},
        low_context.hydrodynamik.get(), outside_low.geschwindigkeit_welt_m_s);
    const auto& low_contribution = rumpfbeitrag(low_forces);
    if (low_contribution.zustand != Datenzustand::gueltig ||
        low_contribution.modell_status != "geschaetzt")
        throw std::runtime_error("Niedrig-Re-Schaetzung wurde nicht berechnet und markiert");
    outside_low.geschwindigkeit_welt_m_s = {.5,0.,0.};
    hydrodynamik_start(m, outside_low, config());
    auto outside_high = start;
    outside_high.geschwindigkeit_welt_m_s = {800.,0.,0.};
    const auto high_context = hydrodynamik_start(m, outside_high, config());
    const auto high_forces = kraefte(m, outside_high.pose, nullptr, 1000., 9.81, {},
        high_context.hydrodynamik.get(), outside_high.geschwindigkeit_welt_m_s);
    const auto& high_contribution = rumpfbeitrag(high_forces);
    if (high_contribution.zustand != Datenzustand::nicht_berechnet ||
        high_contribution.fehler_id != "CORE-HYD-0001")
        throw std::runtime_error("Re ueber 1e9 wurde extrapoliert statt diagnostiziert");
    auto high_balance_contributions = high_forces.beitraege;
    if (bilanz(m, high_balance_contributions, true).status != "teilsumme")
        throw std::runtime_error("Hoch-Re-Luecke wurde als vollstaendige Bilanz ausgegeben");
    outside_high.geschwindigkeit_welt_m_s = {707.,0.,0.};
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
    const auto json = zustand_json(m, moved, "laufend", {}, nullptr, nullptr,
        ErgebnisJsonFormat::Kompatibel);
    if (json.find("\"vertragsversion\":\"2\"") == std::string::npos ||
        json.find("\"quelle\":\"hydrodynamik\"") == std::string::npos ||
        json.find("\"hydrodynamik\":{\"rho_kg_m3\":1000") == std::string::npos ||
        json.find("\"modell_status\":\"geschaetzt\"") == std::string::npos ||
        json.find("\"modell_methode\":\"ittc_1957_mit_laminarer_naeherung\"") == std::string::npos ||
        json.find("\"reynolds_zahl_max\":{\"wert\":") == std::string::npos ||
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
    const std::vector<std::string> einfache_koerper{
        "bleistift", "kegel", "kugel", "platte", "pyramide", "quader", "rolle", "unregelmaessig"};
    const bool einfacher_koerper = std::find(einfache_koerper.begin(), einfache_koerper.end(),
        model.id) != einfache_koerper.end();
    if (einfacher_koerper && (model.elemente.size() != 1 ||
        model.elemente.front().hydrodynamik_rolle != HydrodynamikRolle::rumpf))
        throw std::runtime_error("Einfacher Referenzkoerper nicht als Rumpf zugeordnet: " + model.id);
    if (model.id == "optimist") {
        const auto sword = std::find_if(model.elemente.begin(), model.elemente.end(),
            [](const Element& element) { return element.id == "Schwert"; });
        if (sword == model.elemente.end() || !sword->bewegung ||
            sword->bewegung->art != Bewegungsart::heben ||
            std::abs(sword->bewegung->hubweg_m - .523) > .02)
            throw std::runtime_error("Optimist-Schwert-Hubweg weicht von der Meshreferenz ab");
    }
    if (model.id == "420") {
        const auto sword = std::find_if(model.elemente.begin(), model.elemente.end(),
            [](const Element& element) { return element.id == "Schwert"; });
        if (sword == model.elemente.end() || !sword->bewegung ||
            sword->bewegung->art != Bewegungsart::drehen ||
            sword->bewegung->achse_modell != Vektor3{0., 1., 0.})
            throw std::runtime_error("420er-Drehschwertdefinition fehlt");
    }
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
    if (model.id == "420") {
        const auto& hull_pressure = hydrobeitrag(forces, hull->id, "druck");
        if (!hull_pressure.lateraldruckpunkt_panelbilanz.vorhanden ||
            std::abs(hull_pressure.lateraldruckpunkt_panelbilanz.querkraft_modell_n -
                hull_pressure.vektor[1]) > 1e-8)
            throw std::runtime_error("420er-Rumpfdruck verliert die echte Panelbilanz");
        const auto rudder_force = [&](double angle) {
            const auto pose_forces = kraefte(model, start.pose, nullptr, 1000., 9.81, {},
                active.hydrodynamik.get(), start.geschwindigkeit_welt_m_s, {},
                {{"Ruder", angle}, {"Schwert", 100.}});
            return hydrobeitrag(pose_forces, "Ruder", "seitenkraft");
        };
        const auto port_rudder = rudder_force(-20.);
        const auto neutral_rudder = rudder_force(0.);
        const auto starboard_rudder = rudder_force(20.);
        if (!(port_rudder.vektor[1] < 0.) || !(starboard_rudder.vektor[1] > 0.) ||
            !port_rudder.lateraldruckpunkt_panelbilanz.vorhanden ||
            !starboard_rudder.lateraldruckpunkt_panelbilanz.vorhanden ||
            !(port_rudder.moment_s_nm[2] > 0.) || !(starboard_rudder.moment_s_nm[2] < 0.))
            throw std::runtime_error("420er-Ruder erzeugt bei Vorwaertsfahrt falsche Seitenkraft-/Giermomentvorzeichen");
        if (std::abs(neutral_rudder.vektor[1]) > 1e-8 ||
            std::abs(neutral_rudder.moment_s_nm[2]) > .01)
            throw std::runtime_error("420er-Ruder neutral: Fy=" +
                std::to_string(neutral_rudder.vektor[1]) + ", Mz=" +
                std::to_string(neutral_rudder.moment_s_nm[2]));
        for (const double large : {60., 85.}) {
            const auto port_large = rudder_force(-large);
            const auto starboard_large = rudder_force(large);
            if (port_large.zustand != Datenzustand::gueltig ||
                starboard_large.zustand != Datenzustand::gueltig ||
                !(port_large.vektor[1] < 0.) || !(starboard_large.vektor[1] > 0.) ||
                (large < 70. && (!(port_large.moment_s_nm[2] > 0.) ||
                 !(starboard_large.moment_s_nm[2] < 0.))))
                throw std::runtime_error("420er-Ruder verliert bei grossem Ausschlag die Seitenkraft: " +
                    std::to_string(large));
            if (!(starboard_large.vektor[0] < starboard_rudder.vektor[0]) ||
                !(port_large.vektor[0] < port_rudder.vektor[0]))
                throw std::runtime_error("420er-Ruder: Bremswirkung bei grossem Ausschlag waechst nicht");
        }
    }

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::current_path() /
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
        analytische_seitenkraft_panelbilanz();
        pruefen();
        for (int i = 1; i < argc; ++i) referenzboot(std::filesystem::u8path(argv[i]));
        std::cout << "Hydrodynamiktests bestanden\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
