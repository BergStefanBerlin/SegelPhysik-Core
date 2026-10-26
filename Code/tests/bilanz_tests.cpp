#include "segelphysik/simulation.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

using namespace segelphysik;

namespace {
void pruefe(bool ok, const std::string& text)
{
    if (!ok) throw std::runtime_error(text);
}

// Reference tolerance of A-C-00035: rtol 1e-10, atol 1e-9 (SI).
bool nahe(const Vektor3& a, const Vektor3& b)
{
    for (std::size_t i = 0; i < 3; ++i)
        if (std::abs(a[i] - b[i]) > 1e-9 + 1e-10 * std::max(std::abs(a[i]), std::abs(b[i]))) return false;
    return true;
}

Vektor3 summe(const Vektor3& a, const Vektor3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }

void referenzfaelle()
{
    const Vektor3 s{1., 2., 3.};
    pruefe(nahe(moment_um(s, s, {0, 0, 10}), {0, 0, 0}), "Kraft durch S erzeugt Moment");
    pruefe(nahe(moment_um({1., 2.35, 3.}, s, {0, 0, 10}), {3.5, 0, 0}), "Hebelarm 0,35 m");
    pruefe(nahe(moment_um({1.1, 2.35, 2.7}, s, {0, 0, 49050}), {17167.5, -4905., 0.}),
        "Referenzfall 49 050 N");
    // Force couple: zero resultant, moment independent of the reference point.
    const Vektor3 f{0, 0, 7}, minus_f{0, 0, -7};
    const auto m1 = summe(moment_um({0, 1, 0}, s, f), moment_um({0, -1, 0}, s, minus_f));
    const auto m2 = summe(moment_um({0, 1, 0}, {5, -4, 9}, f), moment_um({0, -1, 0}, {5, -4, 9}, minus_f));
    pruefe(nahe(m1, m2) && nahe(m1, {14, 0, 0}), "Kraeftepaar bezugsabhaengig");
    // Wrench about Q moved to P: M_P = M_Q + (Q - P) x F.
    const Vektor3 q{0.4, -0.2, 0.1}, p{-1., 0.5, 2.}, fq{3., -4., 5.}, mq{0.7, 0.1, -0.3};
    const auto direkt = moment_um({1., 1., 1.}, p, fq);
    const auto ueber_q = moment_um(q, p, fq, moment_um({1., 1., 1.}, q, fq));
    pruefe(nahe(direkt, ueber_q), "Bezugswechsel ueber Q");
    pruefe(nahe(moment_um(q, p, fq, mq), summe(mq, moment_um(q, p, fq))), "Wrench mit Eigenmoment");
    // Frame change and reference change commute (points rotated+shifted, vectors rotated).
    const Pose pose{{3., -1., 2.}, {0.1, -0.2, 0.3, std::sqrt(1. - 0.14)}};
    const Vektor3 a{0.3, 0.2, -0.5};
    const auto welt = moment_um(pose.modellpunkt_nach_welt(a), pose.modellpunkt_nach_welt(s),
                                pose.modellvektor_nach_welt(fq));
    pruefe(nahe(welt, pose.modellvektor_nach_welt(moment_um(a, s, fq))), "Frame-/Bezugswechsel");
}

void ids_und_zustaende(const Modell& m)
{
    Zustand z;
    z.pose = Pose{{0., 0., -0.5}, {0.05, 0.02, 0., std::sqrt(1. - 0.0029)}};
    const auto a = hydrostatik(m, z.pose);
    const auto n = dynamikschritt(m, z, 0.01);
    const auto b = hydrostatik(m, n.pose);
    pruefe(a.beitraege.size() == b.beitraege.size(), "Beitragszahl nicht stabil");
    std::set<std::string> ids;
    for (std::size_t i = 0; i < a.beitraege.size(); ++i) {
        pruefe(a.beitraege[i].id == b.beitraege[i].id, "ID nicht stabil");
        pruefe(ids.insert(a.beitraege[i].id).second, "ID nicht eindeutig");
        pruefe(!a.beitraege[i].gruppe.empty(), "Gruppe fehlt");
    }
    pruefe(std::string(name(a.beitraege[0].quelle)) == "schwerkraft", "Quelle Gewicht");
    pruefe(std::string(name(a.beitraege[1].quelle)) == "hydrostatik", "Quelle Auftrieb");
    pruefe(a.beitraege.back().rolle == Rolle::gesamtsumme, "Gesamtmoment nicht als Summe markiert");
    // Dry body: buoyancy is a valid zero vector, never omitted.
    const auto trocken = hydrostatik(m, Pose{{0., 0., 100.}, {0., 0., 0., 1.}});
    const auto& auftrieb = trocken.beitraege.at(1);
    pruefe(auftrieb.id == "Rumpf:auftrieb" && auftrieb.zustand == Datenzustand::null &&
        nahe(auftrieb.vektor, {0, 0, 0}), "Trockener Auftrieb nicht als null geliefert");
    auto kopie = trocken.beitraege;
    pruefe(bilanz(m, kopie).status == "vollstaendig", "null ist keine Luecke");
    // Invalid value becomes ungueltig with catalogue error ID, balance is partial.
    kopie = a.beitraege;
    kopie[0].vektor[2] = std::numeric_limits<double>::quiet_NaN();
    const auto ungueltig = bilanz(m, kopie);
    pruefe(kopie[0].zustand == Datenzustand::ungueltig && kopie[0].fehler_id == "CORE-BIL-0001",
        "Ungueltiger Wert nicht markiert");
    pruefe(ungueltig.status == "teilsumme" && ungueltig.fehlende_beitraege.size() == 1 &&
        ungueltig.fehlende_beitraege[0] == "Rumpf:gewicht", "Teilsumme ohne Luecken-ID");
    const auto json = zustand_json(m, z, "laufend");
    pruefe(json.find("\"zustand\":\"null\"") == std::string::npos, "Unerwarteter null-Zustand");
    // Forced missing contribution.
    kopie = a.beitraege;
    kopie.erase(kopie.begin() + 1);
    const auto fehlt = bilanz(m, kopie);
    pruefe(fehlt.status == "teilsumme" && fehlt.fehlende_beitraege == std::vector<std::string>{"Rumpf:auftrieb"},
        "Fehlender Beitrag nicht gemeldet");
    kopie = a.beitraege;
    kopie[1].zustand = Datenzustand::deaktiviert;
    pruefe(bilanz(m, kopie).status == "vollstaendig", "deaktiviert ist keine Luecke");
}

void summen_und_paritaet(const Modell& m)
{
    for (const auto& pose : {Pose{{0., 0., -0.9}, {0., 0., 0., 1.}},
                             Pose{{0.2, -0.1, -0.6}, {0.15, -0.08, 0.2, std::sqrt(1. - 0.0689)}}}) {
        auto k = hydrostatik(m, pose);
        const auto b = bilanz(m, k.beitraege);
        pruefe(b.status == "vollstaendig", "Referenzquader nicht vollstaendig");
        Vektor3 f{}, mom{};
        for (const auto& beitrag : k.beitraege)
            if (beitrag.rolle == Rolle::einzelbeitrag) {
                f = summe(f, beitrag.vektor);
                mom = summe(mom, beitrag.moment_s_nm);
                pruefe(nahe(beitrag.moment_s_nm, moment_um(beitrag.punkt_m, m.schwerpunkt_m, beitrag.vektor)),
                    "Beitragsmoment um S falsch");
            }
        pruefe(nahe(b.gesamt.kraft_modell_n, f) && nahe(b.gesamt.moment_s_nm, mom), "Gesamtsumme falsch");
        pruefe(b.gruppen.size() == 1 && nahe(b.gruppen[0].kraft_modell_n, f) &&
            b.gruppen[0].quellen.size() == 2, "Gruppensumme falsch");
        // Parity with the balance used by solver and integrator.
        pruefe(nahe(pose.modellvektor_nach_welt(b.gesamt.kraft_modell_n), k.kraft_welt_n),
            "Paritaet Kraft verletzt");
        pruefe(nahe(b.gesamt.moment_s_nm, k.moment_modell_nm), "Paritaet Moment verletzt");
    }
}

void gleichgewichtsangaben(const Modell& m)
{
    const auto r = gleichgewicht(m, Zustand{});
    pruefe(r.status == "konvergiert" && r.gleichgewicht, "Statik ohne Gleichgewichtsangaben");
    const auto& g = *r.gleichgewicht;
    pruefe(g.krafttoleranz_n == .001 && g.momenttoleranz_nm == .001 && g.iterationen == r.iterationen &&
        g.freiheitsgrade.size() == 3, "Kriterien unvollstaendig");
    pruefe(g.residuum_kraft_n <= g.krafttoleranz_n && g.residuum_moment_nm <= g.momenttoleranz_nm,
        "Residuen ueber Toleranz trotz Konvergenz");
    Zustand schief;
    schief.pose = Pose{{0., 0., -0.2}, {0.4, 0.3, 0., std::sqrt(1. - 0.25)}};
    const auto n = gleichgewicht(m, schief, 1e-9, 1e-9, 1);
    pruefe(n.status != "konvergiert" && n.gleichgewicht &&
        (n.gleichgewicht->residuum_kraft_n > 1e-9 || n.gleichgewicht->residuum_moment_nm > 1e-9),
        "Nicht konvergierter Fall ohne Residuum ueber Toleranz");
    const auto statisch = zustand_json(m, r.zustand, r.status, r.diagnose, &g);
    const auto dynamisch = zustand_json(m, dynamikschritt(m, r.zustand, .01), "laufend");
    for (const auto* feld : {"\"vertragsversion\":\"2\"", "\"modell_id\"", "\"modell_fassung\"", "\"status\"",
            "\"diagnose\"", "\"zeit_s\"", "\"schritt\"", "\"position_m\"", "\"orientierung_xyzw\"",
            "\"geschwindigkeit_welt_m_s\"", "\"omega_modell_rad_s\"", "\"volumen_m3\"", "\"vektoren\"",
            "\"einheit\"", "\"frame\":\"modell\"", "\"punkt_m\"", "\"vektor\"", "\"elementvolumina\"",
            "\"moment_modell_nm\"", "\"moment_bezug_modell_m\"", "\"quelle\"", "\"gruppe\"", "\"rolle\"",
            "\"punktart\"", "\"zustand\"", "\"fehler_id\"", "\"moment_s_nm\"", "\"summen\"",
            "\"bilanzstatus\":\"vollstaendig\"", "\"erwartete_beitraege\"", "\"fehlende_beitraege\":[]",
            "\"beitraege_auswertung\":\"gelieferter_zustand\"", "\"geflutete_elemente\":[]",
            "\"geflutet\":false"}) {
        pruefe(statisch.find(feld) != std::string::npos, std::string("Statik ohne Feld ") + feld);
        pruefe(dynamisch.find(feld) != std::string::npos, std::string("Dynamik ohne Feld ") + feld);
    }
    pruefe(statisch.find("\"residuum_kraft_n\"") != std::string::npos, "Statik ohne Residuum");
    pruefe(dynamisch.find("\"gleichgewicht\":null") != std::string::npos, "Dynamik mit Gleichgewichtsaussage");
}
}

int main(int argc, char* argv[])
{
    if (argc != 2) return 2;
    try {
        const auto m = lade_modell(argv[1]);
        referenzfaelle();
        ids_und_zustaende(m);
        summen_und_paritaet(m);
        gleichgewichtsangaben(m);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
