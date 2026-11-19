#include "segelphysik/sailforce.hpp"
#include "segelphysik/wind.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace segelphysik;

namespace {
void pruefe(bool value, const std::string& message)
{
    if (!value) throw std::runtime_error(message);
}

template<class F> void fehler(F call)
{
    try { call(); }
    catch (const std::invalid_argument& e) {
        pruefe(std::string(e.what()).find("CORE-SIM-0001:") == 0, "Fehlerkennung fehlt");
        return;
    }
    throw std::runtime_error("Ungueltige Eingabe nicht abgelehnt");
}

void sdp()
{
    const auto p = sailforce_sdp_gesamt({{100., 100., 100., 200.},
        {-80., 80., 80., -160.}});
    pruefe(p.status == SailforceSdpStatus::gueltig &&
        p.position_modell_m == std::optional<Vektor3>({9., 0., 2.}),
        "Signierte Rohsummen statt Punktmittelung erforderlich");
    pruefe(p.summen.einzelbetragssumme_n == 180., "Aufhebung verloren");
    pruefe(sailforce_sdp({1., 20., 2., 3.}).status == SailforceSdpStatus::gueltig,
        "Beide Schwellen muessen inklusiv sein");
    pruefe(!sailforce_sdp({std::nextafter(1., 0.), 1., 1., 0.}).position_modell_m,
        "Unterhalb 1 N darf kein Punkt entstehen");
    pruefe(!sailforce_sdp({1., std::nextafter(20., 21.), 1., 0.}).position_modell_m,
        "Unterhalb 5 Prozent darf kein Punkt entstehen");
    pruefe(sailforce_sdp({std::nextafter(1., 2.), 20., 1., 0.}).position_modell_m.has_value(),
        "Oberhalb beider Grenzen muss der Punkt bestimmbar sein");
    const auto small = SailforceSdpSummen{.6, .6, .6, 1.2};
    pruefe(!sailforce_sdp(small).position_modell_m &&
        sailforce_sdp_gesamt({small, small}).position_modell_m.has_value(),
        "Gesamtpunkt muss auch ohne bestimmbare Einzelpunkte moeglich sein");
    pruefe(!sailforce_sdp_gesamt({{10., 10., 10., 0.}, {-10., 10., 10., 0.}})
        .position_modell_m, "Gegenkraefte ergeben keinen Gesamtpunkt");
    pruefe(sailforce_sdp_gesamt({}).status == SailforceSdpStatus::inaktiv,
        "Keine aktive Quelle");
    pruefe(sailforce_sdp({}).status == SailforceSdpStatus::nicht_bestimmbar,
        "Aktiver Nullwind ist nicht inaktiv");
    fehler([] { sailforce_sdp({2., 1., 0., 0.}); });
    fehler([] { sailforce_sdp({0., 0., 1., 0.}); });
    fehler([] { sailforce_sdp({0., -1., 0., 0.}); });
    fehler([] { sailforce_sdp({1., 1., std::numeric_limits<double>::infinity(), 0.}); });
    fehler([] {
        const double max = std::numeric_limits<double>::max();
        sailforce_sdp_gesamt({{max, max, 0., 0.}, {max, max, 0., 0.}});
    });
}

void luft()
{
    const LuftGueltigkeitsbereich range{1., 1.4, 1.4e-5, 1.6e-5};
    pruefe_lufteigenschaften({}, range);
    pruefe_lufteigenschaften({1., 1.4e-5}, range);
    pruefe_lufteigenschaften({1.4, 1.6e-5}, range);
    fehler([&] { pruefe_lufteigenschaften({std::nextafter(1., 0.), 1.5e-5}, range); });
    fehler([&] { pruefe_lufteigenschaften({1.225, std::nextafter(1.6e-5, 1.)}, range); });
    fehler([&] { pruefe_lufteigenschaften({1.225, 0.}, range); });
    fehler([&] {
        pruefe_lufteigenschaften({std::numeric_limits<double>::quiet_NaN(), 1.5e-5}, range);
    });
    fehler([] { pruefe_lufteigenschaften({}, {1.4, 1., 1.4e-5, 1.6e-5}); });
}

void lasten()
{
    const double h = std::sqrt(.5);
    const Pose transform{{2., 3., 4.}, {0., 0., h, h}};
    const auto result = sailforce_modelllast({{1., 0., 0.}, {2., 0., 0.}, {1., 2., 3.}},
        transform, {1., 1., 1.});
    const auto close = [](const Vektor3& a, const Vektor3& b) {
        for (std::size_t i = 0; i < 3; ++i)
            if (std::abs(a[i] - b[i]) > 1e-10) return false;
        return true;
    };
    pruefe(close(result.darstellungsbezug_modell_m, {2., 4., 4.}), "Punkt falsch transformiert");
    pruefe(close(result.kraft_modell_n, {0., 2., 0.}), "Kraft wurde verschoben");
    pruefe(close(result.moment_s_modell_nm, {-8., 1., 5.}), "Eigenmoment oder S-Verschiebung fehlt");
    const auto pure = sailforce_modelllast({{}, {}, {1., 2., 3.}}, transform, {});
    pruefe(close(pure.moment_s_modell_nm, {-2., 1., 3.}) &&
        pure.kraft_modell_n == Vektor3{}, "Reines Moment verloren");
    fehler([&] {
        sailforce_modelllast({{}, {std::numeric_limits<double>::quiet_NaN(), 0., 0.}, {}},
            transform, {});
    });
}

void punktwind()
{
    Modell model;
    model.fassung = "sailforce-test";
    Zustand state;
    state.wind = std::make_shared<const WindKontext>(
        WindKontext{WindKonfiguration{}, 0., model.fassung});
    state.geschwindigkeit_welt_m_s = {1., 2., 3.};
    state.omega_modell_rad_s = {0., 0., .5};
    const auto local = wind_am_punkt(model, state, {2., 0., 10.}, {0., 0., 10.});
    pruefe(!state.mast, "Test darf keine Mastreferenz benoetigen");
    pruefe(local.geschwindigkeit_welt_m_s == Vektor3({1., 3., 3.}),
        "Punktgeschwindigkeit ohne korrekten Rotationsanteil");
    pruefe(local.wahr.stroemung_welt_m_s == Vektor3({-6., 0., 0.}) &&
        local.scheinbar.stroemung_welt_m_s == Vektor3({-7., -3., -3.}),
        "Scheinbarer Wind nicht aus bestehendem Windfeld");
    const auto with_mast = wind_start(model, state, {}, Vektor3{2., 0., 10.}, "mast", "");
    const auto mast = wind_auswertung(model, with_mast, {0., 0., 10.});
    pruefe(mast.wahr && mast.scheinbar &&
        mast.wahr->stroemung_welt_m_s == local.wahr.stroemung_welt_m_s &&
        mast.scheinbar->stroemung_welt_m_s == local.scheinbar.stroemung_welt_m_s,
        "Mast- und Segelpunktwind muessen denselben Rechenweg verwenden");
    Zustand tilted = state;
    const double h = std::sqrt(.5);
    tilted.pose = Pose{{0., 0., 10.}, {h, 0., 0., h}};
    const auto heel = wind_am_punkt(model, tilted, {0., 2., 0.}, {});
    pruefe(std::abs(heel.position_welt_m[2] - 12.) < 1e-10 &&
        std::abs(heel.geschwindigkeit_welt_m_s[0]) < 1e-10 &&
        std::abs(heel.geschwindigkeit_welt_m_s[1] - 2.) < 1e-10 &&
        std::abs(heel.scheinbar.stroemung_welt_m_s[2] + 3.) < 1e-10 &&
        std::abs(heel.wahr.stroemung_welt_m_s[0] + windprofil({}, 12.)) < 1e-10,
        "3D-Kraengung darf Windhorizontalitaet oder Punktkinematik nicht aendern");
    state.geschwindigkeit_welt_m_s = {-6., 0., 0.};
    state.omega_modell_rad_s = {};
    pruefe(wind_am_punkt(model, state, {0., 0., 10.}, {}).scheinbar.raeumlich_m_s == 0.,
        "Scheinbarer Nullwind wurde gekappt");
    fehler([&] { wind_am_punkt(model, state, {}, {}); });
    Modell foreign;
    foreign.fassung = "andere-fassung";
    fehler([&] { wind_am_punkt(foreign, state, {0., 0., 10.}, {}); });
}
}

int main()
{
    try {
        sdp();
        luft();
        lasten();
        punktwind();
        std::cout << "sailforce-Vertragsgrundlagen bestanden\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
