#include "segelphysik/simulation.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace segelphysik;

namespace {
void pruefe(bool ok, const std::string& text)
{
    if (!ok) throw std::runtime_error("Lateraldruckpunkttest: " + text);
}

Modell boxmodell()
{
    Modell m;
    m.id = "ldp-test";
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

Zustand start(const Modell& m, const Vektor3& v)
{
    Zustand s;
    s.pose = Pose{{0., 0., 0.}, {0., 0., 0., 1.}};
    s.geschwindigkeit_welt_m_s = v;
    return hydrodynamik_start(m, s, HydrodynamikKonfiguration{});
}

void ruhe_ist_geometrisch()
{
    const auto m = boxmodell();
    const auto s = start(m, {});
    const auto k = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get());
    const auto ldp = lateraldruckpunkt(m, s.pose, k);
    pruefe(ldp.vorhanden, "Block vorhanden");
    pruefe(ldp.status == "geometrisch", "Ruhe liefert geometrischen Punkt: " + ldp.status);
    pruefe(ldp.position_modell_m && std::abs((*ldp.position_modell_m)[0]) < 1e-6 &&
        std::abs((*ldp.position_modell_m)[1]) < 1e-12, "Punkt symmetrisch bei x=0, y=0");
    pruefe((*ldp.position_modell_m)[2] < 0., "Punkt unter der Wasserlinie");
    pruefe(ldp.lateralflaeche_m2 > 0., "Lateralflaeche positiv");
}

void querfahrt_ist_kraftgewichtet()
{
    const auto m = boxmodell();
    const auto s = start(m, {0., 2., 0.});
    const auto k = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get(),
        s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    const auto ldp = lateraldruckpunkt(m, s.pose, k);
    pruefe(ldp.status == "gueltig", "Querfahrt gueltig: " + ldp.status);
    pruefe(ldp.querkraft_modell_n < 0., "Querkraft entgegen der Fahrt");
    pruefe(!ldp.quellen.empty() && ldp.quellen[0] == "Rumpf:hydrodynamik:druck", "Quelle Rumpfdruck");
    const auto& pressure = *std::find_if(k.beitraege.begin(), k.beitraege.end(),
        [](const Beitrag& b) { return b.id == "Rumpf:hydrodynamik:druck"; });
    pruefe(pressure.lateraldruckpunkt_panelbilanz.vorhanden,
        "Druckpfad erhaelt Panelbilanz");
    pruefe(std::abs(pressure.lateraldruckpunkt_panelbilanz.querkraft_modell_n -
        pressure.vektor[1]) < 1e-8, "Druck-Panel-Q stimmt mit resultierender Querkraft ueberein");
    pruefe(pressure.lateraldruckpunkt_panelbilanz.panelbetragssumme_n >=
        std::abs(pressure.lateraldruckpunkt_panelbilanz.querkraft_modell_n),
        "Druck-Panelbetragssumme erhaelt Gegenkraefte");
}

Beitrag panelbeitrag(const std::string& id, double q, double betrag,
    double erstes_moment_x, double erstes_moment_z)
{
    Beitrag b;
    b.id = id + ":hydrodynamik:seitenkraft";
    b.element_id = id;
    b.art = "kraft";
    b.quelle = Quelle::hydrodynamik;
    b.zustand = Datenzustand::gueltig;
    b.vektor = {1234., q, 0.};
    b.moment_s_nm = {0., 0., 987654.};
    b.lateraldruckpunkt_panelbilanz = {true, q, betrag,
        erstes_moment_x, erstes_moment_z};
    return b;
}

void panelmomente_und_schwellen()
{
    auto m = boxmodell();
    Kraefte within;
    within.beitraege.push_back(panelbeitrag("Rumpf", 20., 180., 180., 40.));
    const Pose identity{{0., 0., 0.}, {0., 0., 0., 1.}};
    auto ldp = lateraldruckpunkt(m, identity, within);
    pruefe(ldp.status == "gueltig", "innere Gegenkraefte bleiben bestimmbar");
    pruefe(ldp.position_modell_m &&
        std::abs((*ldp.position_modell_m)[0] - 9.) < 1e-12 &&
        std::abs((*ldp.position_modell_m)[2] - 2.) < 1e-12,
        "Panelbilanz liefert x=9 m und z=2 m statt Bauteilmittelpunkt oder Giermoment");

    Element sword = m.elemente.front();
    sword.id = "Schwert";
    sword.hydrodynamik_rolle = HydrodynamikRolle::kiel;
    m.elemente.push_back(sword);
    Kraefte between;
    between.beitraege.push_back(panelbeitrag("Rumpf", 100., 100., 100., 50.));
    between.beitraege.push_back(panelbeitrag("Schwert", -80., 80., 80., 30.));
    ldp = lateraldruckpunkt(m, identity, between);
    pruefe(ldp.status == "gueltig" && ldp.position_modell_m &&
        std::abs((*ldp.position_modell_m)[0] - 9.) < 1e-12 &&
        std::abs(ldp.querkraft_modell_n - 20.) < 1e-12,
        "bauteiluebergreifende Gegenkraefte ergeben Q=20 N, X=180 Nm, x=9 m");

    const auto at_one_newton = [&](double q, double b) {
        Kraefte forces;
        forces.beitraege.push_back(panelbeitrag("Rumpf", q, b, q, 0.));
        return lateraldruckpunkt(m, identity, forces);
    };
    pruefe(at_one_newton(1., 20.).status == "gueltig",
        "1 N- und 5-Prozent-Schwelle sind inklusiv");
    pruefe(at_one_newton(1.001, 20.).status == "gueltig",
        "knapp oberhalb der Mindestkraft bleibt gueltig");
    pruefe(at_one_newton(.999, 20.).status != "gueltig",
        "knapp unterhalb der Mindestkraft ist nicht kraftbasiert gueltig");
    pruefe(at_one_newton(5., 100.).status == "gueltig",
        "exakte 5-Prozent-Schwelle ist inklusiv");
    pruefe(at_one_newton(4.999, 100.).status != "gueltig",
        "knapp unterhalb der 5-Prozent-Schwelle ist nicht kraftbasiert gueltig");
}

void rotationsdruck_hat_analytische_panelbilanz()
{
    const auto m = boxmodell();
    Zustand s;
    const double roll = .1;
    s.pose = Pose{{0., 0., -3.},
        {std::sin(roll / 2.), 0., 0., std::cos(roll / 2.)}};
    const double omega = -std::sqrt(.2);
    s.omega_modell_rad_s = {0., 0., omega};
    s = hydrodynamik_start(m, s, HydrodynamikKonfiguration{});
    const auto k = kraefte(m, s.pose, nullptr, 1000., 9.81, {},
        s.hydrodynamik.get(), s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    const auto pressure = std::find_if(k.beitraege.begin(), k.beitraege.end(),
        [](const Beitrag& b) { return b.id == "Rumpf:hydrodynamik:druck"; });
    pruefe(pressure != k.beitraege.end() &&
        pressure->lateraldruckpunkt_panelbilanz.vorhanden,
        "rotierender, geneigter Rumpf hat echte Druck-Paneldaten");
    const auto& panels = pressure->lateraldruckpunkt_panelbilanz;
    pruefe(std::abs(panels.querkraft_modell_n) < 1e-10,
        "symmetrische Gegenpanels heben Q auf");
    pruefe(std::abs(panels.panelbetragssumme_n - 400. / 9.) < 1e-9,
        "rotierende Seitenpanels liefern analytische Summe der Betraege");
    pruefe(std::abs(panels.erstes_moment_x_nm - 400. / 27.) < 1e-9,
        "rotierende Seitenpanels liefern analytisches signiertes X");
    pruefe(std::abs(panels.erstes_moment_z_nm) < 1e-9,
        "geneigte Pose transformiert Z-Moment in den Modellrahmen");
    pruefe(lateraldruckpunkt(m, s.pose, k).status != "gueltig",
        "geometrischer Ersatzpunkt wird bei Q=0 nicht als LDP gueltig");
}

void json_vertrag_und_beitragsanzahl()
{
    const auto m = boxmodell();
    const auto s = start(m, {0., 2., 0.});
    const auto json = zustand_json(m, s, "ok");
    for (const char* teil : {"\"vertragsversion\":\"6\"",
            "\"druckpunkte\":{\"lateral\":{\"auswertung_version\":2,\"status\":\"gueltig\"",
            "\"position_modell_m\":", "\"querkraft_modell_n\":", "\"quellen\":["})
        pruefe(json.find(teil) != std::string::npos, std::string("JSON enthaelt ") + teil);
    const auto document = nlohmann::json::parse(json);
    const auto& lateral = document.at("druckpunkte").at("lateral");
    pruefe(document.at("vertragsversion") == "6" &&
        lateral.at("auswertung_version").is_number_integer() &&
        lateral.at("auswertung_version") == 2 &&
        lateral.at("status") == "gueltig",
        "V6-Druckpunktvertrag ist syntaktisch gueltig und versioniert");
    const auto a = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get(),
        s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    const auto b = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get(),
        s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    pruefe(a.beitraege.size() == b.beitraege.size(), "Beitragsanzahl bei wiederholter Auswertung gleich");
    Modell leer;
    leer.id = "leer";
    pruefe(!lateraldruckpunkt(leer, s.pose, a).vorhanden, "Modell ohne Hydrodynamik: kein Block");

    StationaeresGleichgewichtErgebnis snapshot;
    snapshot.zustand = s;
    snapshot.status = "nicht_konvergiert";
    snapshot.konfiguration.aktiv = true;
    snapshot.konfiguration.modellfest = true;
    snapshot.konfiguration.kraft_welt_n = {0., 40., 0.};
    snapshot.konfiguration.angriffspunkt_modell_m = {3., 2., -1.};
    const auto snapshot_json = zustand_json(m, s, "nicht_konvergiert", {}, nullptr,
        &snapshot, ErgebnisJsonFormat::V6);
    pruefe(snapshot_json.find("\"ersatz_segel\":{\"status\":\"gueltig\",\"quelle\":\"fremdkraft\",\"position_modell_m\":[3,2,-1]}") !=
        std::string::npos, "gueltiger Ersatz-SDP stammt aus stationaerem Modellframe-Snapshot");
    const auto snapshot_document = nlohmann::json::parse(snapshot_json);
    const auto& replacement = snapshot_document.at("druckpunkte").at("ersatz_segel");
    pruefe(replacement.at("status") == "gueltig" &&
        replacement.at("quelle") == "fremdkraft" &&
        replacement.at("position_modell_m") == nlohmann::json::array({3., 2., -1.}),
        "gueltiger Ersatz-SDP hat exakten JSON-Typ, Quelle und Modellposition");
    const auto stationary_legacy = nlohmann::json::parse(zustand_json(m, s,
        "nicht_konvergiert", {}, nullptr, &snapshot, ErgebnisJsonFormat::Kompatibel));
    pruefe(stationary_legacy.at("vertragsversion") == "5" &&
        !stationary_legacy.contains("kursdrehrate_rad_s") &&
        stationary_legacy.at("druckpunkte").at("ersatz_segel").at("status") == "gueltig",
        "Kompatibel-Projektion behaelt V5-Felder und ergaenzt nur den Snapshot-Druckpunkt");

    snapshot.konfiguration.modellfest = false;
    const auto world_fixed_json = zustand_json(m, s, "nicht_konvergiert", {}, nullptr,
        &snapshot, ErgebnisJsonFormat::V6);
    pruefe(world_fixed_json.find("\"ersatz_segel\":{\"status\":\"gueltig\",\"quelle\":\"fremdkraft\",\"position_modell_m\":[3,2,-1]}") !=
        std::string::npos, "weltfeste Kraft behaelt modellfesten Ansatzpunkt");

    snapshot.konfiguration.aktiv = false;
    const auto inactive_json = zustand_json(m, s, "ok", {}, nullptr, &snapshot);
    pruefe(inactive_json.find("\"ersatz_segel\":{\"status\":\"inaktiv\",\"quelle\":\"fremdkraft\"}") !=
        std::string::npos, "inaktive Last hat Status ohne Position");
    const auto inactive = nlohmann::json::parse(inactive_json).at("druckpunkte").at("ersatz_segel");
    pruefe(inactive.at("status") == "inaktiv" &&
        !inactive.contains("position_modell_m"), "inaktiver JSON-Block hat keine Position");
    snapshot.konfiguration.aktiv = true;
    snapshot.konfiguration.kraft_welt_n = {};
    const auto zero_json = zustand_json(m, s, "ok", {}, nullptr, &snapshot);
    pruefe(zero_json.find("\"ersatz_segel\":{\"status\":\"kraft_null\",\"quelle\":\"fremdkraft\"}") !=
        std::string::npos, "aktive Nullkraft hat Status ohne Position");
    const auto zero = nlohmann::json::parse(zero_json).at("druckpunkte").at("ersatz_segel");
    pruefe(zero.at("status") == "kraft_null" &&
        !zero.contains("position_modell_m"), "Nullkraft-JSON-Block hat keine Position");
    pruefe(json.find("\"ersatz_segel\"") == std::string::npos,
        "ohne stationaeren Snapshot kein Ersatz-SDP");
    auto other_state = s;
    ++other_state.schritt;
    bool stale_snapshot_rejected = false;
    try {
        (void)zustand_json(m, other_state, "ok", {}, nullptr, &snapshot);
    } catch (const std::invalid_argument&) {
        stale_snapshot_rejected = true;
    }
    pruefe(stale_snapshot_rejected,
        "stationaerer Ansatzpunkt wird nicht auf einen anderen Zustand projiziert");

    const auto legacy_json = zustand_json(m, s, "ok", {}, nullptr, nullptr,
        ErgebnisJsonFormat::Kompatibel);
    pruefe(legacy_json.find("\"vertragsversion\":\"2\"") != std::string::npos &&
        legacy_json.find("\"auswertung_version\":2") != std::string::npos &&
        legacy_json.find("\"ersatz_segel\"") == std::string::npos,
        "Legacy-Serialisierung erhaelt Vertragsversion und additive LDP-Kennung");
    const auto legacy = nlohmann::json::parse(legacy_json);
    pruefe(legacy.at("vertragsversion") == "2" &&
        legacy.at("druckpunkte").at("lateral").at("auswertung_version") == 2 &&
        !legacy.at("druckpunkte").contains("ersatz_segel"),
        "Kompatibelprojektion bleibt parsebar und fuehrt keinen ungefragten Lastsnapshot ein");
}
}  // namespace

int main()
{
    try {
        ruhe_ist_geometrisch();
        querfahrt_ist_kraftgewichtet();
        panelmomente_und_schwellen();
        rotationsdruck_hat_analytische_panelbilanz();
        json_vertrag_und_beitragsanzahl();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::cout << "Lateraldruckpunkttests bestanden\n";
    return 0;
}
