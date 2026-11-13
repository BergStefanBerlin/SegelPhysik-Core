#include "segelphysik/wind.hpp"
#include "segelphysik/referenzpunkte.hpp"
#include "segelphysik/simulation.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace segelphysik;

namespace {

void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

bool near(double actual, double expected, double tolerance = 1e-10)
{
    return std::abs(actual - expected) < tolerance;
}

template<class F> void rejects(F action)
{
    try { action(); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error("Invalid wind input was accepted");
}

Pose heading_east()
{
    const double s = std::sqrt(.5);
    return {{}, {0., 0., -s, s}};
}

void test_profile()
{
    WindKonfiguration c;
    require(near(windprofil(c, 10.), 6.), "reference profile");
    require(near(windprofil(c, 1.), 4.72312443057448), "profile at 1m");
    require(near(windprofil(c, 5.), 5.61562215287239), "profile at 5m");
    require(near(windprofil(c, 20.), 6.38437784712761), "profile at 20m");
    require(near(windprofil(c, .0002), .01), "minimum wind");
    c.referenzgeschwindigkeit_m_s = 100.;
    require(near(windprofil(c, 20.), 106.406297452127), "local wind was capped");
    rejects([&] { (void)windprofil(c, 0.); });
    rejects([&] { (void)windprofil(c, .00019); });
    rejects([&] { (void)windprofil(c, std::numeric_limits<double>::quiet_NaN()); });
    rejects([&] { (void)windprofil(c, std::numeric_limits<double>::infinity()); });
    c.referenzhoehe_m = .0002;
    rejects([&] { (void)windprofil(c, 1.); });
    c.referenzhoehe_m = 10.;
    c.referenzgeschwindigkeit_m_s = 100.01;
    rejects([&] { (void)windprofil(c, 1.); });
    c.referenzgeschwindigkeit_m_s = .01;
    c.referenzhoehe_m = .01;
    require(near(windprofil(c, .01), .01), "minimum reference values");
    c.referenzgeschwindigkeit_m_s = 100.;
    c.referenzhoehe_m = 100.;
    require(near(windprofil(c, 100.), 100.), "maximum reference values");
    c.referenzhoehe_m = 100.01;
    rejects([&] { (void)windprofil(c, 1.); });
    c.referenzhoehe_m = 10.;
    c.referenzgeschwindigkeit_m_s = .009;
    rejects([&] { (void)windprofil(c, 1.); });
    c.referenzgeschwindigkeit_m_s = std::numeric_limits<double>::infinity();
    rejects([&] { (void)windprofil(c, 1.); });
    c.referenzgeschwindigkeit_m_s = 6.;
    c.herkunft_welt_grad = std::numeric_limits<double>::quiet_NaN();
    rejects([&] { (void)windprofil(c, 1.); });
}

void test_wind()
{
    Modell model;
    model.fassung = "wind-test";
    Zustand initial;
    WindKonfiguration config;
    auto state = wind_start(model, initial, config, Vektor3{0., 0., 10.}, "Mast", "");
    auto value = wind_auswertung(model, state, {});
    require(value.wahr && value.scheinbar && value.mastgeschwindigkeit_welt_m_s,
        "wind at stationary mast missing");
    require(near(value.wahr->stroemung_welt_m_s[0], -6.) &&
        near(value.wahr->stroemung_welt_m_s[1], 0.) &&
        near(*value.wahr->herkunft_welt_grad, 0.), "north wind");
    require(value.scheinbar->stroemung_welt_m_s == value.wahr->stroemung_welt_m_s,
        "stationary apparent wind");

    for (const auto& test : std::array<std::pair<double, Vektor3>, 4>{{
             {0., {-6., 0., 0.}}, {90., {0., 6., 0.}},
             {180., {6., 0., 0.}}, {270., {0., -6., 0.}}}}) {
        const auto directed = wind_aendern(state, test.first, {}, {});
        const auto v = wind_auswertung(model, directed, {});
        require(v.wahr && near(v.wahr->stroemung_welt_m_s[0], test.second[0])
            && near(v.wahr->stroemung_welt_m_s[1], test.second[1])
            && near(v.wahr->stroemung_welt_m_s[2], 0.)
            && near(*v.wahr->herkunft_welt_grad, test.first), "compass wind direction");
    }
    const auto wrapped = wind_auswertung(model, wind_aendern(state, 360., {}, {}), {});
    require(wrapped.wahr && near(*wrapped.wahr->herkunft_welt_grad, 0.) &&
        near(wrapped.wahr->stroemung_welt_m_s[0], -6.), "360-degree north wrap");
    auto threshold_config = config;
    threshold_config.referenzgeschwindigkeit_m_s = .1;
    auto threshold_state = wind_start(model, initial, threshold_config, Vektor3{0., 0., 10.}, "Mast", "");
    auto threshold = wind_auswertung(model, threshold_state, {});
    require(threshold.wahr && threshold.scheinbar &&
        !threshold.wahr->kompass_sichtbar && !threshold.wahr->pfeil_sichtbar &&
        !threshold.scheinbar->kompass_sichtbar && !threshold.scheinbar->pfeil_sichtbar,
        "true and apparent wind at the threshold must be hidden");
    threshold_state = wind_aendern(threshold_state, {}, .10001, {});
    threshold = wind_auswertung(model, threshold_state, {});
    require(threshold.wahr && threshold.scheinbar &&
        threshold.wahr->kompass_sichtbar && threshold.wahr->pfeil_sichtbar &&
        threshold.scheinbar->kompass_sichtbar && threshold.scheinbar->pfeil_sichtbar,
        "true and apparent wind above the threshold must be visible");
    auto heeled = state;
    heeled.pose = Pose{{}, {std::sin(.15), 0., 0., std::cos(.15)}};
    value = wind_auswertung(model, heeled, {});
    require(value.wahr && near(value.wahr->stroemung_welt_m_s[2], 0.)
            && value.mastspitze_welt_m && (*value.mastspitze_welt_m)[1] < 0.
            && near((*value.mastspitze_welt_m)[2], 10. * std::cos(.3)),
        "heeled mast must move with pose while true wind remains horizontal");

    state.geschwindigkeit_welt_m_s = {1., 2., 3.};
    state.omega_modell_rad_s = {0., .2, 0.};
    value = wind_auswertung(model, state, {});
    require(near((*value.mastgeschwindigkeit_welt_m_s)[0], 3.) &&
        near((*value.mastgeschwindigkeit_welt_m_s)[1], 2.) &&
        near((*value.mastgeschwindigkeit_welt_m_s)[2], 3.), "mast rotational velocity");
    require(near(value.scheinbar->stroemung_welt_m_s[0], -9.) &&
        near(value.scheinbar->stroemung_welt_m_s[1], -2.) &&
        near(value.scheinbar->stroemung_welt_m_s[2], -3.), "apparent wind subtraction");

    state.geschwindigkeit_welt_m_s = {-6., 0., -.2};
    state.omega_modell_rad_s = {};
    value = wind_auswertung(model, state, {});
    require(!value.scheinbar->herkunft_welt_grad && !value.scheinbar->kompass_sichtbar &&
        value.scheinbar->pfeil_sichtbar, "vertical apparent wind status");
    state.geschwindigkeit_welt_m_s[2] = -.1;
    value = wind_auswertung(model, state, {});
    require(!value.scheinbar->pfeil_sichtbar, "threshold must be inclusive");
    state.geschwindigkeit_welt_m_s[2] = -.10001;
    value = wind_auswertung(model, state, {});
    require(value.scheinbar->pfeil_sichtbar && !value.scheinbar->kompass_sichtbar,
        "vertical arrow threshold must be independent of horizontal compass threshold");

    const auto old = state;
    state.pose = heading_east();
    value = wind_auswertung(model, state, {});
    require(near(*value.wahr->herkunft_welt_grad, 0.), "world-fixed direction changed");
    auto changed = wind_aendern(state, {}, {}, WindModus::bootmitgefuehrt);
    require(near(*wind_auswertung(model, changed, {}).wahr->herkunft_welt_grad, 0.),
        "direction jumped on mode change");
    changed.pose = initial.pose;
    require(near(*wind_auswertung(model, changed, {}).wahr->herkunft_welt_grad, 270.),
        "boat-following direction");
    require(near(*wind_auswertung(model, old, {}).wahr->herkunft_welt_grad, 0.),
        "historic wind changed");
    changed = wind_aendern(changed, {}, {}, WindModus::weltfest);
    require(near(*wind_auswertung(model, changed, {}).wahr->herkunft_welt_grad, 270.),
        "direction jumped on return to world-fixed mode");
    changed = wind_aendern(changed, 90., 8., {});
    value = wind_auswertung(model, changed, {});
    require(near(*value.wahr->herkunft_welt_grad, 90.) &&
        near(value.wahr->stroemung_welt_m_s[1], 8.), "east wind change");
    rejects([&] { (void)wind_aendern(changed, {}, 0., {}); });
    require(near(changed.wind->konfiguration.referenzgeschwindigkeit_m_s, 8.),
        "invalid change modified history");

    auto vertical = state;
    const double s = std::sqrt(.5);
    vertical.pose = Pose{{0., 0., 10.}, {0., s, 0., s}};
    rejects([&] { (void)wind_aendern(vertical, {}, {}, WindModus::bootmitgefuehrt); });
    auto boat_fixed = wind_aendern(state, {}, {}, WindModus::bootmitgefuehrt);
    boat_fixed.pose = vertical.pose;
    require(wind_auswertung(model, boat_fixed, {}).diagnose == "Horizontaler Bootskurs nicht bestimmbar" &&
        !wind_auswertung(model, boat_fixed, {}).wahr,
        "vertical boat axis must not produce an invented horizontal heading");

    const auto missing = wind_start(model, initial, config, {}, "", "Mast fehlt");
    require(!wind_auswertung(model, missing, {}).wahr &&
        wind_auswertung(model, missing, {}).diagnose == "Mast fehlt", "missing mast status");
    require(!wind_auswertung(model, initial, {}).diagnose.empty(), "missing history status");
}

void test_points_and_json()
{
    Modell model;
    model.id = "test";
    model.fassung = "version";
    model.masse_kg = 1.;
    Element element;
    element.id = "body";
    element.masse_kg = 1.;
    model.elemente.push_back(element);
    Zustand state;
    state.pose = heading_east();
    state.geschwindigkeit_welt_m_s = {1., 0., 0.};
    WindKonfiguration config;
    state = wind_start(model, state, config, Vektor3{0., 0., 10.}, "Mast", "");
    Kraefte forces;
    forces.schwerpunkt_modell_m = {};
    Beitrag buoyancy;
    buoyancy.id = "body:auftrieb";
    buoyancy.element_id = "body";
    buoyancy.art = "kraft";
    buoyancy.quelle = Quelle::hydrostatik;
    buoyancy.punkt_m = {1., 0., -1.};
    forces.beitraege.push_back(buoyancy);
    Lateraldruckpunkt lateral;
    lateral.vorhanden = true;
    lateral.status = "nicht_bestimmbar";
    const auto points = referenzpunkte(model, state, forces, lateral);
    require(points.size() == 4 && points[0].id == "mastspitze" &&
        points[1].id == "schwerpunkt" && points[2].id == "beitrag:body:auftrieb" &&
        points[3].id == "lateraldruckpunkt", "point identities");
    require(points[0].quelle == "uebergebener_mastpunkt",
        "wind input must not invent a more specific mast provenance");
    require(points[0].position_welt_m && points[0].geschwindigkeit_modell_m_s &&
        near((*points[0].geschwindigkeit_modell_m_s)[1], 1.), "point frame conversion");
    require(points[2].position_modell_m && !points[2].geschwindigkeit_welt_m_s,
        "buoyancy position must not imply rigid velocity");
    require(!points[3].position_modell_m && points[3].fachstatus == "nicht_bestimmbar",
        "missing lateral pressure point");

    auto without_wind = state;
    without_wind.wind.reset();
    without_wind.omega_modell_rad_s = {0., .2, 0.};
    without_wind.mast->quelle = "explizite_modellvorgabe";
    const auto independent = referenzpunkte(model, without_wind, forces, lateral);
    require(independent[0].position_modell_m && independent[0].geschwindigkeit_welt_m_s &&
        near((*independent[0].geschwindigkeit_welt_m_s)[0], 1.) &&
        near((*independent[0].geschwindigkeit_welt_m_s)[1], -2.) &&
        independent[0].quelle == "explizite_modellvorgabe",
        "mast position, provenance and rotational velocity must not depend on wind");
    const auto independent_json = nlohmann::json::parse(zustand_json(model, without_wind, "ok"));
    require(!independent_json.contains("wind") &&
        independent_json.at("referenzpunkte")[0].at("positionsstatus") == "gueltig",
        "windless results must retain an explicitly supplied mast");
    const auto restarted = wind_start(model, without_wind, config, Vektor3{0., 0., 10.}, "Mast", "");
    require(restarted.mast->quelle == "explizite_modellvorgabe",
        "wind start must preserve explicit mast provenance");
    rejects([&] { (void)wind_start(model, without_wind, config, Vektor3{0., 0., 11.}, "Mast", ""); });
    auto wrong_version = without_wind;
    wrong_version.mast->modell_fassung = "other";
    rejects([&] { (void)referenzpunkte(model, wrong_version, forces, lateral); });

    const auto old = nlohmann::json::parse(zustand_json(model, state, "ok"));
    require(old.at("referenzpunkte").is_array() && old.at("wind").at("wahr").is_object(),
        "new JSON fields");
    require(old.at("referenzpunkte")[0].at("position_modell_m")[2] == 10.,
        "mast JSON position");
    auto changed = wind_aendern(state, 90., 8., {});
    const auto updated = nlohmann::json::parse(zustand_json(model, changed, "ok"));
    require(old.at("wind").at("referenzgeschwindigkeit_m_s") == 6. &&
        updated.at("wind").at("referenzgeschwindigkeit_m_s") == 8.,
        "historic JSON wind values");
    const auto legacy_state = nlohmann::json::parse(zustand_json(model, Zustand{}, "ok"));
    require(!legacy_state.contains("wind") &&
        legacy_state.at("referenzpunkte")[0].at("positionsstatus") == "nicht_verfuegbar",
        "absent historical wind must stay unavailable");
    const auto missing = wind_start(model, Zustand{}, config, {}, "", "Mast fehlt");
    const auto missing_json = nlohmann::json::parse(zustand_json(model, missing, "ok"));
    require(missing_json.at("wind").at("diagnose") == "Mast fehlt"
        && missing_json.at("wind").at("wahr").is_null()
        && missing_json.at("referenzpunkte")[0].at("position_modell_m").is_null()
        && missing_json.at("referenzpunkte")[0].at("geschwindigkeit_welt_m_s").is_null(),
        "missing mast must not create fabricated position, velocity, or wind");

    auto other_buoyancy = buoyancy;
    other_buoyancy.id = "other:auftrieb";
    other_buoyancy.element_id = "other";
    other_buoyancy.punkt_m = {2., 0., -1.};
    forces.beitraege.push_back(other_buoyancy);
    auto unknown = buoyancy;
    unknown.id = "unknown";
    unknown.punkt_bekannt = false;
    forces.beitraege.push_back(unknown);
    StationaeresGleichgewichtErgebnis stationary;
    stationary.konfiguration.angriffspunkt_modell_m = {3., 0., 1.};
    stationary.konfiguration.kraft_welt_n = {10., 0., 0.};
    const auto inactive = referenzpunkte(model, state, forces, lateral, &stationary);
    require(inactive[2].id == "beitrag:body:auftrieb" &&
        inactive[3].id == "beitrag:other:auftrieb" &&
        inactive[2].position_modell_m != inactive[3].position_modell_m,
        "separate buoyancy points must retain their identities and positions");
    require(inactive[4].id == "beitrag:unknown" && !inactive[4].position_modell_m &&
        !inactive[4].geschwindigkeit_welt_m_s,
        "unknown force application point must not become an origin");
    require(inactive.back().id == "ersatz_segel" &&
        !inactive.back().position_modell_m && inactive.back().fachstatus == "inaktiv",
        "inactive external force must not create a replacement sail point");
    stationary.konfiguration.aktiv = true;
    const auto active = referenzpunkte(model, state, forces, lateral, &stationary);
    require(active.back().position_modell_m == stationary.konfiguration.angriffspunkt_modell_m &&
        active.back().fachstatus == "gueltig" && !active.back().geschwindigkeit_welt_m_s,
        "active external force must retain its point without inventing velocity");
}

}

int main()
{
    try {
        test_profile();
        test_wind();
        test_points_and_json();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
