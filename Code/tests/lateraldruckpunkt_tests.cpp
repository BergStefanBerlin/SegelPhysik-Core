#include "segelphysik/simulation.hpp"

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
}

void json_und_kraefte_unveraendert()
{
    const auto m = boxmodell();
    const auto s = start(m, {0., 2., 0.});
    const auto json = zustand_json(m, s, "ok");
    for (const char* teil : {"\"druckpunkte\":{\"lateral\":{\"status\":\"gueltig\"", "\"position_modell_m\":",
            "\"querkraft_modell_n\":", "\"quellen\":["})
        pruefe(json.find(teil) != std::string::npos, std::string("JSON enthaelt ") + teil);
    const auto a = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get(),
        s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    const auto b = kraefte(m, s.pose, nullptr, 1000., 9.81, {}, s.hydrodynamik.get(),
        s.geschwindigkeit_welt_m_s, s.omega_modell_rad_s);
    pruefe(a.beitraege.size() == b.beitraege.size(), "Kraefte reproduzierbar");
    Modell leer;
    leer.id = "leer";
    pruefe(!lateraldruckpunkt(leer, s.pose, a).vorhanden, "Modell ohne Hydrodynamik: kein Block");
}
}  // namespace

int main()
{
    try {
        ruhe_ist_geometrisch();
        querfahrt_ist_kraftgewichtet();
        json_und_kraefte_unveraendert();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::cout << "Lateraldruckpunkttests bestanden\n";
    return 0;
}
