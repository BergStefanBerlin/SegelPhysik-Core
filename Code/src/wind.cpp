#include "segelphysik/wind.hpp"
#include "referenzpunkte_intern.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace segelphysik {
namespace {

constexpr double pi = 3.14159265358979323846;
constexpr double z0 = 0.0002;

double normiere(double grad)
{
    const double result = std::fmod(grad, 360.);
    return result < 0. ? result + 360. : result;
}

void pruefe_konfiguration(const WindKonfiguration& config)
{
    if (!std::isfinite(config.herkunft_welt_grad) ||
        !std::isfinite(config.referenzgeschwindigkeit_m_s) ||
        config.referenzgeschwindigkeit_m_s < .01 || config.referenzgeschwindigkeit_m_s > 100. ||
        !std::isfinite(config.referenzhoehe_m) ||
        config.referenzhoehe_m < .01 || config.referenzhoehe_m > 100. ||
        (config.modus != WindModus::weltfest && config.modus != WindModus::bootmitgefuehrt))
        throw std::invalid_argument("CORE-SIM-0001: Ungueltige Windkonfiguration.");
}

std::optional<double> kurs(const Pose& pose)
{
    const auto v = pose.modellvektor_nach_welt({1., 0., 0.});
    if (!std::isfinite(v[0]) || !std::isfinite(v[1]) || std::hypot(v[0], v[1]) <= 1e-8)
        return {};
    return normiere(std::atan2(-v[1], v[0]) * 180. / pi);
}

double benoetigter_kurs(const Pose& pose)
{
    const auto value = kurs(pose);
    if (!value)
        throw std::invalid_argument("CORE-SIM-0001: Horizontaler Bootskurs nicht bestimmbar.");
    return *value;
}

std::optional<double> richtung(const WindKontext& context, const Pose& pose)
{
    const auto& config = context.konfiguration;
    if (config.modus == WindModus::weltfest) return normiere(config.herkunft_welt_grad);
    const auto heading = kurs(pose);
    if (!heading) return {};
    return normiere(config.herkunft_welt_grad + *heading - context.referenzkurs_grad);
}

WindVektor auswerten(const Vektor3& v)
{
    WindVektor result;
    result.stroemung_welt_m_s = v;
    result.horizontal_m_s = std::hypot(v[0], v[1]);
    result.raeumlich_m_s = std::hypot(v[0], v[1], v[2]);
    if (result.horizontal_m_s > 0.)
        result.herkunft_welt_grad = normiere(std::atan2(v[1], -v[0]) * 180. / pi);
    result.kompass_sichtbar = result.horizontal_m_s > .1;
    result.pfeil_sichtbar = result.raeumlich_m_s > .1;
    return result;
}

}

Zustand wind_start(const Modell& modell, const Zustand& zustand,
    const WindKonfiguration& konfiguration, std::optional<Vektor3> mastspitze_modell_m,
    const std::string& mast_element_id, const std::string& mast_diagnose)
{
    pruefe_konfiguration(konfiguration);
    if (mastspitze_modell_m &&
        (!std::all_of(mastspitze_modell_m->begin(), mastspitze_modell_m->end(),
             [](double value) { return std::isfinite(value); }) ||
         mast_element_id.empty()))
        throw std::invalid_argument("CORE-SIM-0001: Ungueltige Mastspitze.");
    if (!mastspitze_modell_m && mast_diagnose.empty())
        throw std::invalid_argument("CORE-SIM-0001: Diagnose fuer fehlende Mastspitze erforderlich.");
    auto context = std::make_shared<WindKontext>();
    context->konfiguration = konfiguration;
    context->konfiguration.herkunft_welt_grad = normiere(konfiguration.herkunft_welt_grad);
    context->referenzkurs_grad = konfiguration.modus == WindModus::bootmitgefuehrt
        ? benoetigter_kurs(zustand.pose) : 0.;
    context->modell_fassung = modell.fassung;
    Zustand result = zustand;
    if (result.mast) {
        detail::pruefe_mast(modell, *result.mast);
        if (result.mast->position_modell_m != mastspitze_modell_m ||
            result.mast->element_id != mast_element_id)
            throw std::invalid_argument("CORE-SIM-0001: Wind und Mastreferenz widersprechen sich.");
    } else {
        result.mast = MastReferenz{mastspitze_modell_m, mast_element_id,
            "uebergebener_mastpunkt", mast_diagnose, modell.fassung};
    }
    result.wind = std::move(context);
    return result;
}

Zustand wind_aendern(const Zustand& zustand, std::optional<double> herkunft_welt_grad,
    std::optional<double> referenzgeschwindigkeit_m_s, std::optional<WindModus> modus)
{
    if (!zustand.wind)
        throw std::invalid_argument("CORE-SIM-0001: Windkontext fehlt.");
    auto context = std::make_shared<WindKontext>(*zustand.wind);
    const auto aktuelle_richtung = richtung(*context, zustand.pose);
    if (!herkunft_welt_grad && !aktuelle_richtung)
        throw std::invalid_argument("CORE-SIM-0001: Aktuelle Windrichtung nicht bestimmbar.");
    if (herkunft_welt_grad) context->konfiguration.herkunft_welt_grad = *herkunft_welt_grad;
    else context->konfiguration.herkunft_welt_grad = *aktuelle_richtung;
    if (referenzgeschwindigkeit_m_s)
        context->konfiguration.referenzgeschwindigkeit_m_s = *referenzgeschwindigkeit_m_s;
    if (modus) context->konfiguration.modus = *modus;
    pruefe_konfiguration(context->konfiguration);
    context->konfiguration.herkunft_welt_grad = normiere(context->konfiguration.herkunft_welt_grad);
    context->referenzkurs_grad = context->konfiguration.modus == WindModus::bootmitgefuehrt
        ? benoetigter_kurs(zustand.pose) : 0.;
    Zustand result = zustand;
    result.wind = std::move(context);
    return result;
}

double windprofil(const WindKonfiguration& konfiguration, double hoehe_m)
{
    pruefe_konfiguration(konfiguration);
    if (!std::isfinite(hoehe_m) || hoehe_m < z0)
        throw std::invalid_argument("CORE-SIM-0001: Ungueltige Windhoehe.");
    return std::max(.01, konfiguration.referenzgeschwindigkeit_m_s *
        (std::log(hoehe_m) - std::log(z0)) /
        (std::log(konfiguration.referenzhoehe_m) - std::log(z0)));
}

WindAuswertung wind_auswertung(const Modell& modell, const Zustand& zustand,
    const Vektor3& schwerpunkt_modell_m)
{
    WindAuswertung result;
    result.zeit_s = zustand.zeit_s;
    result.schritt = zustand.schritt;
    if (!zustand.wind) {
        result.diagnose = "Kein Windkontext fuer diesen Ergebnisstand";
        return result;
    }
    const auto& context = *zustand.wind;
    if (context.modell_fassung != modell.fassung)
        throw std::invalid_argument("CORE-SIM-0001: Windkontext gehoert zu anderer Modellfassung.");
    pruefe_konfiguration(context.konfiguration);
    if (!zustand.mast) {
        result.diagnose = "Keine Mastdaten fuer diesen Ergebnisstand";
        return result;
    }
    detail::pruefe_mast(modell, *zustand.mast);
    if (!zustand.mast->position_modell_m) {
        result.diagnose = zustand.mast->diagnose;
        return result;
    }
    const auto point = zustand.pose.modellpunkt_nach_welt(*zustand.mast->position_modell_m);
    result.mastspitze_welt_m = point;
    const auto velocity = detail::starre_punktgeschwindigkeit(
        zustand, *zustand.mast->position_modell_m, schwerpunkt_modell_m);
    result.mastgeschwindigkeit_welt_m_s = velocity;
    if (!std::isfinite(point[2]) || point[2] < z0) {
        result.diagnose = "Mastspitze unterhalb der gueltigen Windhoehe";
        return result;
    }
    const auto direction = richtung(context, zustand.pose);
    if (!direction) {
        result.diagnose = "Horizontaler Bootskurs nicht bestimmbar";
        return result;
    }
    const double speed = windprofil(context.konfiguration, point[2]);
    const double angle = *direction * pi / 180.;
    const Vektor3 true_wind{-speed * std::cos(angle), speed * std::sin(angle), 0.};
    result.wahr = auswerten(true_wind);
    result.scheinbar = auswerten({true_wind[0] - velocity[0],
        true_wind[1] - velocity[1], true_wind[2] - velocity[2]});
    return result;
}

}
